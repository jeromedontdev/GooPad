$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$fixture = Join-Path ([System.IO.Path]::GetTempPath()) ("goopad-package-test-" + [guid]::NewGuid())
$originalLocation = Get-Location
$capture = [pscustomobject]@{ Calls = 0; Arguments = @() }

function esptool {
    $capture.Calls++
    $capture.Arguments = @($args)
    $outputIndex = [Array]::IndexOf($args, "-o")
    if ($outputIndex -lt 0) { throw "Missing merge output" }
    [System.IO.File]::WriteAllBytes($args[$outputIndex + 1], [byte[]](1, 2, 3))
    $global:LASTEXITCODE = 0
}

function Expect-Failure([scriptblock]$action, [string]$message) {
    $before = $capture.Calls
    try { & $action } catch {
        if ($_.Exception.Message -notlike "*$message*") { throw }
        if ($capture.Calls -ne $before) { throw "Invalid build reached esptool" }
        return
    }
    throw "Expected packaging to fail: $message"
}

try {
    $workspace = Join-Path $fixture "workspace"
    $build = Join-Path $fixture "external-build"
    New-Item -ItemType Directory -Force -Path (Join-Path $workspace "tools"),
        (Join-Path $build "bootloader"), (Join-Path $build "partition_table"),
        (Join-Path $workspace "main\fonts") | Out-Null
    Copy-Item (Join-Path $root "tools\make_release.ps1") (Join-Path $workspace "tools\make_release.ps1")
    Copy-Item (Join-Path $root "main\fonts\NOTICE.txt") (Join-Path $workspace "main\fonts\NOTICE.txt")
    Set-Content (Join-Path $workspace "CMakeLists.txt") 'set(PROJECT_VER "99.8.7")'
    Set-Content (Join-Path $workspace "sdkconfig") 'CONFIG_ESPTOOLPY_FLASHMODE="qio"'
    foreach ($name in @("bootloader\bootloader.bin", "partition_table\partition-table.bin",
                        "ota_data_initial.bin", "GooPad.bin")) {
        [System.IO.File]::WriteAllBytes((Join-Path $build $name), [byte[]](4, 5, 6, 7))
    }
    $description = Join-Path $build "project_description.json"
    $settings = Join-Path $build "flasher_args.json"
    Set-Content $description '{"project_name":"GooPad","project_version":"99.8.7"}'
    Set-Content $settings '{"flash_settings":{"flash_mode":"dio","flash_freq":"80m","flash_size":"16MB"}}'
    $packager = Join-Path $workspace "tools\make_release.ps1"
    & $packager -SkipBuild -BuildDir $build
    foreach ($pair in @(@("--flash-mode", "dio"), @("--flash-freq", "80m"), @("--flash-size", "16MB"))) {
        $index = [Array]::IndexOf($capture.Arguments, $pair[0])
        if ($index -lt 0 -or $capture.Arguments[$index + 1] -ne $pair[1]) {
            throw "Packaging did not use the selected build's $($pair[0])"
        }
    }
    $ota = Join-Path $workspace "dist\GooPad-ota-99.8.7.bin"
    if ((Get-FileHash $ota).Hash -ne (Get-FileHash (Join-Path $build "GooPad.bin")).Hash) {
        throw "OTA package changed application bytes"
    }
    if ((Get-FileHash (Join-Path $workspace "dist\GooPad-99.8.7-NOTICE.txt")).Hash -ne
        (Get-FileHash (Join-Path $workspace "main\fonts\NOTICE.txt")).Hash) {
        throw "Firmware packaging omitted or changed dependency notices"
    }
    Set-Content $description '{"project_name":"GooPad","project_version":"99.8.6"}'
    Expect-Failure { & $packager -SkipBuild -BuildDir $build } "does not match"
    Set-Content $description '{"project_name":"GooPad","project_version":"99.8.7"}'
    Set-Content $settings '{"flash_settings":{"flash_mode":"dio","flash_freq":"80m"}}'
    Expect-Failure { & $packager -SkipBuild -BuildDir $build } "Missing flash_size"
    Remove-Item -LiteralPath $settings
    Expect-Failure { & $packager -SkipBuild -BuildDir $build } "does not exist"
    Write-Host "Release packaging: external settings, OTA identity and invalid metadata checks passed."
}
finally {
    Set-Location $originalLocation
    Remove-Item -LiteralPath $fixture -Recurse -Force
}
