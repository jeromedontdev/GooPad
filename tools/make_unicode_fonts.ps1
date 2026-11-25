param(
    [Parameter(Mandatory)][string]$ConverterPath,
    [string]$NodePath = 'node',
    [string]$FontPath = (Join-Path $PSScriptRoot '..\main\fonts\sources\Lato-Regular.ttf'),
    [string]$OutputDir = (Join-Path $PSScriptRoot '..\main\fonts')
)

$ErrorActionPreference = 'Stop'
$FontPath = (Resolve-Path -LiteralPath $FontPath).Path
$ConverterPath = (Resolve-Path -LiteralPath $ConverterPath).Path
$package = Get-Content -Raw (Join-Path (Split-Path $ConverterPath) 'package.json') | ConvertFrom-Json
if ($package.name -ne 'lv_font_conv' -or $package.version -ne '1.5.3') {
    throw 'Use lv_font_conv 1.5.3 for reproducible Unicode fallback generation.'
}
if (-not (Test-Path -LiteralPath $OutputDir -PathType Container)) {
    throw 'OutputDir must be an existing directory.'
}
foreach ($size in @(8, 10, 12, 14, 16, 20, 24, 28, 36, 40)) {
    $output = Join-Path $OutputDir "goopad_font_unicode_$size.c"
    & $NodePath $ConverterPath --font $FontPath --range '0x80-0xFFFF' --size $size --bpp 1 `
        --format lvgl --lv-include lvgl.h --no-kerning --lv-font-name "goopad_font_unicode_$size" -o $output
    if ($LASTEXITCODE -ne 0) { throw "Unicode font generation failed at size $size." }
    $text = Get-Content -Raw -LiteralPath $output
    $text = $text -replace '(?m)^ \* Opts:.*$', ' * Generated from sources/Lato-Regular.ttf with lv_font_conv 1.5.3; see licenses/lato.txt.'
    $text = $text.TrimEnd() + "`n"
    [IO.File]::WriteAllText($output, $text, [Text.UTF8Encoding]::new($false))
}
