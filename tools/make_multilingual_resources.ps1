param(
    [Parameter(Mandatory)][string]$ConverterPath,
    [Parameter(Mandatory)][string]$CacheDir,
    [Parameter(Mandatory)][string]$OutputDir,
    [string]$NodePath = 'node',
    [string]$PythonPath = 'python'
)
$ErrorActionPreference = 'Stop'
$ConverterPath = (Resolve-Path -LiteralPath $ConverterPath).Path
$package = Get-Content -Raw (Join-Path (Split-Path $ConverterPath) 'package.json') | ConvertFrom-Json
if ($package.name -ne 'lv_font_conv' -or $package.version -ne '1.5.3') {
    throw 'Use lv_font_conv 1.5.3 for reproducible resource generation.'
}
New-Item -ItemType Directory -Force -Path $CacheDir, $OutputDir | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\main\fonts\licenses\lato.txt') `
    -Destination (Join-Path $OutputDir 'lato-OFL.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\third_party\fontawesome\NOTICE.txt') `
    -Destination (Join-Path $OutputDir 'FontAwesome-NOTICE.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'multilingual-font-sources.json') `
    -Destination (Join-Path $OutputDir 'font-sources.json')
$sources = Get-Content -Raw (Join-Path $PSScriptRoot 'multilingual-font-sources.json') | ConvertFrom-Json
foreach ($entry in $sources.PSObject.Properties) {
    $target = Join-Path $CacheDir $entry.Value.file
    if (-not (Test-Path -LiteralPath $target)) {
        Invoke-WebRequest -Uri $entry.Value.url -OutFile "$target.download"
        Move-Item -LiteralPath "$target.download" -Destination $target
    }
    Invoke-WebRequest -Uri $entry.Value.license -OutFile (Join-Path $OutputDir "$($entry.Name)-OFL.txt")
}
& $NodePath (Join-Path $PSScriptRoot 'make_multilingual_pack.js') $ConverterPath $CacheDir $OutputDir $PythonPath
if ($LASTEXITCODE -ne 0) { throw 'Multilingual resource generation failed.' }
$font = Join-Path $PSScriptRoot '..\main\fonts\sources\Lato-Regular.ttf'
foreach ($size in @(8, 10, 12, 14, 16, 20, 24, 28, 36, 40)) {
    $output = Join-Path $PSScriptRoot "..\main\fonts\goopad_font_essential_$size.c"
    & $NodePath $ConverterPath --font $font --range '0xA0-0x17F,0x2010-0x2026,0x20AC' --size $size --bpp 1 `
        --format lvgl --lv-include lvgl.h --no-kerning --lv-font-name "goopad_font_essential_$size" -o $output
    if ($LASTEXITCODE -ne 0) { throw "Essential font generation failed at $size px." }
    $text = Get-Content -Raw -LiteralPath $output
    $text = $text -replace '(?m)^ \* Opts:.*$', ' * Lato essential offline fallback; see sources/Lato-Regular.ttf and licenses/lato.txt.'
    [IO.File]::WriteAllText($output, $text.TrimEnd() + "`n", [Text.UTF8Encoding]::new($false))
}
