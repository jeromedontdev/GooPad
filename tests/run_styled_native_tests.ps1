param(
    [Parameter(Mandatory = $true)][string]$ResourceDirectory,
    [string]$NodeExe = "node",
    [string]$ZigExe = "zig",
    [string]$UnicodePack = ""
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$fontDirectory = Join-Path $root "main\fonts"
$output = [System.IO.Path]::GetFullPath($ResourceDirectory)
$build = Join-Path $root "build"
New-Item -ItemType Directory -Force -Path $build, $output | Out-Null

& $NodeExe (Join-Path $root "tools\make_styled_pack.js") $fontDirectory $output
if ($LASTEXITCODE -ne 0) { throw "Styled pack generation failed ($LASTEXITCODE)." }
& $NodeExe (Join-Path $root "tests\test_styled_pack.js") $fontDirectory $output
if ($LASTEXITCODE -ne 0) { throw "Styled source parity test failed ($LASTEXITCODE)." }

$manifest = Get-Content (Join-Path $output "styled-v1.json") -Raw | ConvertFrom-Json
$packName = $manifest.file
$compilerArgs = @(
    "cc", "-std=c11",
    '-DGOOPAD_BOARD_HEADER="unicode_board.h"',
    ("-I" + (Join-Path $root "tests\styled_stubs")),
    ("-I" + (Join-Path $root "tests\unicode_stubs")),
    ("-I" + (Join-Path $root "main")),
    (Join-Path $root "tests\test_styled_cache.c"),
    (Join-Path $root "main\goopad_unicode.c")
)

function Build-And-Run([string]$PackFile, [string]$Mode, [string]$ExeName) {
    $compileArguments = @($compilerArgs[0..2]) + @('-DGOOPAD_STYLED_PATH="' + $PackFile + '"') + @($compilerArgs[3..($compilerArgs.Count - 1)]) + @("-o", (Join-Path $build $ExeName))
    & $ZigExe @compileArguments
    if ($LASTEXITCODE -ne 0) { throw "Native styled test compilation failed ($LASTEXITCODE)." }
    Push-Location $output
    try {
        & (Join-Path $build $ExeName) $Mode
        if ($LASTEXITCODE -ne 0) { throw "Native styled test '$Mode' failed ($LASTEXITCODE)." }
    }
    finally { Pop-Location }
}

$corruptName = "styled-corrupt.gsp"
try {
    Build-And-Run $packName "all" "test_styled_cache.exe"
    Build-And-Run "styled-missing.gsp" "missing" "test_styled_missing.exe"
    Copy-Item (Join-Path $output $packName) (Join-Path $output $corruptName) -Force
    Build-And-Run $corruptName "corrupt" "test_styled_corrupt.exe"
    Copy-Item (Join-Path $output $packName) (Join-Path $output $corruptName) -Force
    Build-And-Run $corruptName "metrics-corrupt" "test_styled_metrics_corrupt.exe"
}
finally {
    Remove-Item (Join-Path $output $corruptName) -Force -ErrorAction SilentlyContinue
}

if ($UnicodePack) {
    $unicodeDirectory = Join-Path $output "goopad\resources"
    New-Item -ItemType Directory -Force -Path $unicodeDirectory | Out-Null
    $unicodeFile = Join-Path $unicodeDirectory "text-371771469d5d.gfp"
    $heldUnicodeFile = Join-Path $unicodeDirectory "text-371771469d5d.held"
    Copy-Item (Join-Path $output $packName) (Join-Path $unicodeDirectory $packName) -Force
    Copy-Item $UnicodePack $unicodeFile -Force
    function Build-And-Run-Mixed([string]$PackFile, [string]$Mode, [string]$ExeName) {
        $mixedArgs = @(
            "cc", "-std=c11",
            '-DGOOPAD_BOARD_HEADER="unicode_board.h"',
            ("-I" + (Join-Path $root "tests\styled_stubs")),
            ("-I" + (Join-Path $root "tests\unicode_stubs")),
            ("-I" + (Join-Path $root "main")),
            ('-DGOOPAD_STYLED_PATH="goopad/resources/' + $PackFile + '"'),
            (Join-Path $root "tests\test_mixed_font_cache.c"),
            (Join-Path $root "main\goopad_fonts.c"),
            (Join-Path $root "main\goopad_styled.c"),
            (Join-Path $root "main\goopad_unicode.c"),
            "-o", (Join-Path $build $ExeName)
        )
        & $ZigExe @mixedArgs
        if ($LASTEXITCODE -ne 0) { throw "Mixed-cache native test compilation failed ($LASTEXITCODE)." }
        & (Join-Path $build $ExeName) $Mode
        if ($LASTEXITCODE -ne 0) { throw "Mixed-cache native test '$Mode' failed ($LASTEXITCODE)." }
    }
    function Build-And-Run-Actual-LVGL-Selector {
        $lvgl = Join-Path $root "managed_components\lvgl__lvgl"
        $selectorArgs = @(
            "cc", "-std=c11", "-O0", "-ffunction-sections", "-fdata-sections",
            "-DLV_CONF_SKIP", "-DCONFIG_LV_USE_FONT_PLACEHOLDER=1", "-DLV_USE_FONT_COMPRESSED=1",
            '-DGOOPAD_BOARD_HEADER="unicode_board.h"',
            ('-DGOOPAD_STYLED_PATH="goopad/resources/' + $packName + '"'),
            "-DLV_FONT_MONTSERRAT_8=1", "-DLV_FONT_MONTSERRAT_10=1",
            "-DLV_FONT_MONTSERRAT_12=1", "-DLV_FONT_MONTSERRAT_14=1",
            "-DLV_FONT_MONTSERRAT_16=1", "-DLV_FONT_MONTSERRAT_20=1",
            "-DLV_FONT_MONTSERRAT_28=1", "-DLV_FONT_MONTSERRAT_40=1",
            ("-I" + (Join-Path $lvgl "include\lvgl")),
            ("-I" + (Join-Path $lvgl "include")),
            ("-I" + (Join-Path $lvgl "src")),
            ("-I" + (Join-Path $root "tests\unicode_stubs")),
            ("-I" + (Join-Path $root "main")),
            (Join-Path $root "tests\test_font_selector_lvgl.c"),
            (Join-Path $root "main\goopad_fonts.c"),
            (Join-Path $root "main\goopad_styled.c"),
            (Join-Path $root "main\goopad_unicode.c"),
            (Join-Path $lvgl "src\font\lv_font.c"),
            (Join-Path $lvgl "src\font\fmt_txt\lv_font_fmt_txt.c"),
            "-Wl,--gc-sections", "-o", (Join-Path $build "test_font_selector_lvgl.exe")
        )
        foreach ($family in @("lato", "pacifico", "anton", "archivoblack", "abrilfatface",
                             "lobster", "breeserif", "cousine", "sharetechmono", "righteous",
                             "acme", "bangers", "patrickhand", "pressstart2p", "spacemono")) {
            foreach ($size in @(36, 24, 16, 12)) {
                $selectorArgs += Join-Path $fontDirectory "goopad_font_${family}_${size}.c"
            }
        }
        & $ZigExe @selectorArgs
        if ($LASTEXITCODE -ne 0) { throw "Actual-LVGL selector test compilation failed ($LASTEXITCODE)." }

        $styleFile = Join-Path $unicodeDirectory $packName
        $heldStyle = $styleFile + ".held"
        $heldUnicode = $unicodeFile + ".held"
        & (Join-Path $build "test_font_selector_lvgl.exe") "healthy"
        if ($LASTEXITCODE -ne 0) { throw "Actual-LVGL healthy-pack test failed ($LASTEXITCODE)." }
        Move-Item $styleFile $heldStyle
        try {
            & (Join-Path $build "test_font_selector_lvgl.exe") "styled-missing"
            if ($LASTEXITCODE -ne 0) { throw "Actual-LVGL styled-missing test failed ($LASTEXITCODE)." }
            Move-Item $unicodeFile $heldUnicode
            try {
                & (Join-Path $build "test_font_selector_lvgl.exe") "both-missing"
                if ($LASTEXITCODE -ne 0) { throw "Actual-LVGL both-missing test failed ($LASTEXITCODE)." }
            }
            finally { Move-Item $heldUnicode $unicodeFile -Force }
        }
        finally { Move-Item $heldStyle $styleFile -Force }

        Move-Item $unicodeFile $heldUnicode
        try {
            & (Join-Path $build "test_font_selector_lvgl.exe") "unicode-missing"
            if ($LASTEXITCODE -ne 0) { throw "Actual-LVGL Unicode-missing test failed ($LASTEXITCODE)." }
        }
        finally { Move-Item $heldUnicode $unicodeFile -Force }
    }

    Push-Location $output
    try {
        Build-And-Run-Mixed $packName "stress" "test_mixed_font_cache.exe"
        Build-And-Run-Mixed "styled-missing.gsp" "styled-missing" "test_styled_missing_mixed.exe"

        Move-Item $unicodeFile $heldUnicodeFile
        try {
            Build-And-Run-Mixed $packName "unicode-missing" "test_unicode_missing_mixed.exe"
            Build-And-Run-Mixed "styled-missing.gsp" "both-missing" "test_both_missing_mixed.exe"
        }
        finally { Move-Item $heldUnicodeFile $unicodeFile -Force }

        $mixedCorruptName = "styled-mixed-corrupt.gsp"
        Copy-Item (Join-Path $output $packName) (Join-Path $unicodeDirectory $mixedCorruptName) -Force
        try {
            Build-And-Run-Mixed $mixedCorruptName "styled-corrupt" "test_styled_corrupt_mixed.exe"
        }
        finally { Remove-Item (Join-Path $unicodeDirectory $mixedCorruptName) -Force -ErrorAction SilentlyContinue }

        Build-And-Run-Actual-LVGL-Selector
    }
    finally {
        Pop-Location
        Remove-Item (Join-Path $output "goopad") -Recurse -Force -ErrorAction SilentlyContinue
    }
}
