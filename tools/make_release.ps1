# Builds a merged first-flash image and an application-only OTA image.
#
# The normal `idf.py flash` writes four separate binaries at four offsets.
# A merged image is easier to hand out: one file, one offset, and it works
# with any flashing tool (esptool, the Espressif Flash Download Tool, or a
# web flasher).
#
# Usage:
#   .\tools\make_release.ps1
#   .\tools\make_release.ps1 -Port COM4      # also flash it
#   .\tools\make_release.ps1 -SkipBuild -BuildDir C:\Temp\GooPad-idf-build

param(
    [string]$Port = "",
    [string]$BuildDir = "build",
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
    idf.py -B $BuildDir build
    if ($LASTEXITCODE -ne 0) { throw "Build failed" }
}

$build = if ([System.IO.Path]::IsPathRooted($BuildDir)) { $BuildDir } else { Join-Path $root $BuildDir }
$bootloader = Join-Path $build "bootloader\bootloader.bin"
$partitions = Join-Path $build "partition_table\partition-table.bin"
$otaData = Join-Path $build "ota_data_initial.bin"
$app = Join-Path $build "GooPad.bin"
$notices = Join-Path $root "main\fonts\NOTICE.txt"

foreach ($f in @($bootloader, $partitions, $otaData, $app, $notices)) {
    if (-not (Test-Path $f)) { throw "Missing build artifact: $f" }
}

$description = Get-Content (Join-Path $build "project_description.json") -Raw | ConvertFrom-Json
if ($description.project_name -ne "GooPad" -or $description.project_version -ne $version) {
    throw "Selected build does not match GooPad version $version"
}
$settings = (Get-Content (Join-Path $build "flasher_args.json") -Raw | ConvertFrom-Json).flash_settings
foreach ($key in @("flash_mode", "flash_freq", "flash_size")) {
    if (-not $settings.$key) { throw "Missing $key in the selected build's flasher_args.json" }
}

$dist = Join-Path $root "dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null
$out = Join-Path $dist "GooPad-$version-merged.bin"
$otaOut = Join-Path $dist "GooPad-ota-$version.bin"

Write-Host "Merging..."
# Flash parameters must match what the build actually produced, otherwise
# the ROM loads the image with the wrong mode and the board boot-loops.
# Use the selected build's resolved settings, not the workspace sdkconfig.
$flashMode = $settings.flash_mode
$flashFreq = $settings.flash_freq
$flashSize = $settings.flash_size
Write-Host "Flash params: mode=$flashMode freq=$flashFreq size=$flashSize"

# Offsets come from partitions.csv: bootloader 0x0, partition table 0x8000,
# OTA metadata 0x10000, and the first OTA app slot 0x20000.
esptool --chip esp32s3 merge-bin `
    --flash-mode $flashMode --flash-freq $flashFreq --flash-size $flashSize `
    -o $out `
    0x0     $bootloader `
    0x8000  $partitions `
    0x10000 $otaData `
    0x20000 $app

if ($LASTEXITCODE -ne 0) { throw "merge-bin failed" }

Copy-Item -Force $app $otaOut
Copy-Item -Force $notices (Join-Path $dist "GooPad-$version-NOTICE.txt")
$size = (Get-Item $out).Length
Write-Host ""
Write-Host "Created $out ($([math]::Round($size/1KB)) KB)"
Write-Host "Created $otaOut ($([math]::Round((Get-Item $otaOut).Length/1KB)) KB; application-only OTA image)"
Write-Host ""
Write-Host "Flash with:"
Write-Host "  esptool --chip esp32s3 -p <PORT> -b 460800 write-flash 0x0 `"$out`""

if ($Port) {
    Write-Host ""
    Write-Host "Flashing to $Port..."
    esptool --chip esp32s3 -p $Port -b 460800 write-flash 0x0 $out
}
