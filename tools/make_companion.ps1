param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\dist'),
    [string]$WorkDirectory = (Join-Path $env:TEMP 'GooPad-companion-build')
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$package = Join-Path $OutputDirectory 'GooPadCompanion-Windows'
New-Item -ItemType Directory -Force $package | Out-Null
python -m PyInstaller --noconfirm --onefile --windowed --name GooPadCompanion `
    --distpath $package --workpath (Join-Path $WorkDirectory 'work') `
    --specpath $WorkDirectory (Join-Path $root 'companion\GooPadCompanion.py')
if ($LASTEXITCODE -ne 0) { throw 'Companion build failed. Install PyInstaller with python -m pip install pyinstaller.' }
Copy-Item (Join-Path $root 'companion\GooPadCompanion.py'), `
    (Join-Path $root 'companion\windows_startup.py'), `
    (Join-Path $root 'companion\Start-GooPadCompanion.cmd'), `
    (Join-Path $root 'README.md') -Destination $package
$archive = Join-Path $OutputDirectory 'GooPadCompanion-Windows.zip'
Compress-Archive -Path (Join-Path $package '*') -DestinationPath $archive -Force
Get-Item $archive
