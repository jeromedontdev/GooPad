param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\dist'),
    [string]$WorkDirectory = (Join-Path $env:TEMP 'GooPad-companion-build')
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$package = Join-Path $OutputDirectory 'GooPadCompanion-Windows'
New-Item -ItemType Directory -Force $package | Out-Null
python -m PyInstaller --noconfirm --onefile --windowed --name GooPadCompanion `
    --hidden-import pystray._win32 `
    --distpath $package --workpath (Join-Path $WorkDirectory 'work') `
    --specpath $WorkDirectory (Join-Path $root 'companion\GooPadCompanion.py')
if ($LASTEXITCODE -ne 0) { throw 'Companion build failed. Install PyInstaller with python -m pip install pyinstaller.' }
Copy-Item (Join-Path $root 'companion\GooPadCompanion.py'), `
    (Join-Path $root 'companion\windows_startup.py'), `
    (Join-Path $root 'companion\requirements.txt'), `
    (Join-Path $root 'companion\Start-GooPadCompanion.cmd'), `
    (Join-Path $root 'README.md') -Destination $package
$thirdParty = Join-Path $package 'third-party'
New-Item -ItemType Directory -Force $thirdParty | Out-Null
$trayVersion = python -c "from importlib.metadata import distribution; from pathlib import Path; import shutil,sys; target=Path(sys.argv[1]); [(shutil.copyfile(d.locate_file(f),target/(name+'-'+Path(str(f)).name)) ) for name in ['Pillow','six','cryptography','cffi','pycparser'] for d in [distribution(name)] for f in d.files if Path(str(f)).name.startswith('LICENSE')]; print(distribution('pystray').version)" $thirdParty
if ($LASTEXITCODE -ne 0) { throw 'Could not package dependency licenses.' }
$trayVersion = $trayVersion.Trim()
Invoke-WebRequest "https://github.com/moses-palmer/pystray/archive/refs/tags/v$trayVersion.zip" `
    -OutFile (Join-Path $thirdParty "pystray-$trayVersion-source.zip")
Invoke-WebRequest 'https://www.gnu.org/licenses/gpl-3.0.txt' `
    -OutFile (Join-Path $thirdParty 'pystray-GPL-3.0.txt')
$archive = Join-Path $OutputDirectory 'GooPadCompanion-Windows.zip'
Compress-Archive -Path (Join-Path $package '*') -DestinationPath $archive -Force
Get-Item $archive
