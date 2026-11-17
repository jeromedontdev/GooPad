# Builds a single merged image that can be flashed to offset 0x0.
#
# The normal `idf.py flash` writes four separate binaries at four offsets.
# A merged image is easier to hand out: one file, one offset, and it works
# with any flashing tool (esptool, the Espressif Flash Download Tool, or a
# web flasher).
#
# Usage:
#   .\tools\make_release.ps1
#   .\tools\make_release.ps1 -Port COM4      # also flash it
#   .\tools\make_release.ps1 -SkipBuild      # reuse the existing build

param(
    [string]$Port = "",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# Pull the version out of the top-level CMakeLists so there is one source
# of truth for it.
$verLine = Select-String -Path "CMakeLists.txt" -Pattern 'set\(PROJECT_VER "([^"]+)"\)'
if (-not $verLine) {
    throw "Could not find PROJECT_VER in CMakeLists.txt"
}
$version = $verLine.Matches[0].Groups[1].Value
Write-Host "Version: $version"

if (-not $SkipBuild) {
    Write-Host "Building..."
    idf.py build
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
}

$build = Join-Path $root "build"
$bootloader = Join-Path $build "bootloader\bootloader.bin"
$partitions = Join-Path $build "partition_table\partition-table.bin"
$app = Join-Path $build "esp32s3_4848s040_hwtest.bin"

foreach ($f in @($bootloader, $partitions, $app)) {
    if (-not (Test-Path $f)) { throw "Missing build artifact: $f" }
}

$dist = Join-Path $root "dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null
$out = Join-Path $dist "esp32s3_4848s040_hwtest-$version.bin"

Write-Host "Merging..."
# Flash parameters must match what the build actually produced, otherwise
# the ROM loads the image with the wrong mode and the board boot-loops.
# Read them from sdkconfig rather than hardcoding: CONFIG_ESPTOOLPY_FLASHMODE
# is "dio" on this board even though FLASHMODE_QIO is also set, because the
# bootloader overrides it.
function Get-SdkValue([string]$key, [string]$default) {
    $line = Select-String -Path "sdkconfig" -Pattern "^$key=`"([^`"]+)`"" | Select-Object -First 1
    if ($line) { return $line.Matches[0].Groups[1].Value }
    return $default
}
$flashMode = Get-SdkValue "CONFIG_ESPTOOLPY_FLASHMODE" "dio"
$flashFreq = Get-SdkValue "CONFIG_ESPTOOLPY_FLASHFREQ" "80m"
$flashSize = Get-SdkValue "CONFIG_ESPTOOLPY_FLASHSIZE" "16MB"
Write-Host "Flash params: mode=$flashMode freq=$flashFreq size=$flashSize"

# Offsets come from partitions.csv: bootloader at 0x0, partition table at
# 0x8000, factory app at 0x10000.
esptool --chip esp32s3 merge-bin `
    --flash-mode $flashMode --flash-freq $flashFreq --flash-size $flashSize `
    -o $out `
    0x0     $bootloader `
    0x8000  $partitions `
    0x10000 $app

if ($LASTEXITCODE -ne 0) { throw "merge-bin failed" }

$size = (Get-Item $out).Length
Write-Host ""
Write-Host "Created $out ($([math]::Round($size/1KB)) KB)"
Write-Host ""
Write-Host "Flash with:"
Write-Host "  esptool --chip esp32s3 -p <PORT> -b 460800 write-flash 0x0 `"$out`""

if ($Port) {
    Write-Host ""
    Write-Host "Flashing to $Port..."
    esptool --chip esp32s3 -p $Port -b 460800 write-flash 0x0 $out
}
