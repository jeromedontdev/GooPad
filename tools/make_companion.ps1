param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\dist'),
    [string]$WorkDirectory = (Join-Path $env:TEMP 'GooPad-companion-build'),
    [string]$InnoCompiler = 'ISCC.exe'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$package = Join-Path $OutputDirectory 'GooPadCompanion-Windows'
if (Test-Path $package) { throw 'Use a fresh OutputDirectory to avoid packaging stale Companion files.' }
New-Item -ItemType Directory -Force $package | Out-Null
New-Item -ItemType Directory -Force $WorkDirectory | Out-Null
$icon = Join-Path $WorkDirectory 'GooPadCompanion.ico'
python -c "import sys; sys.path.insert(0,sys.argv[1]); from GooPadCompanion import companion_icon; companion_icon(256).save(sys.argv[2],format='ICO',sizes=[(16,16),(32,32),(48,48),(64,64),(256,256)])" (Join-Path $root 'companion') $icon
if ($LASTEXITCODE -ne 0) { throw 'Could not create the GooPad executable icon.' }
python -m PyInstaller --noconfirm --onedir --noupx --windowed --name GooPadCompanion `
    --icon $icon `
    --hidden-import pystray._win32 `
    --distpath (Join-Path $WorkDirectory 'dist') --workpath (Join-Path $WorkDirectory 'work') `
    --specpath $WorkDirectory (Join-Path $root 'companion\GooPadCompanion.py')
if ($LASTEXITCODE -ne 0) { throw 'Companion build failed. Install PyInstaller with python -m pip install pyinstaller.' }
Copy-Item (Join-Path $WorkDirectory 'dist\GooPadCompanion\*') -Destination $package -Recurse
Copy-Item (Join-Path $root 'companion\GooPadCompanion.py'), `
    (Join-Path $root 'companion\companion_update.py'), `
    (Join-Path $root 'companion\windows_startup.py'), `
    (Join-Path $root 'companion\requirements.txt'), `
    (Join-Path $root 'companion\Start-GooPadCompanion.cmd'), `
    (Join-Path $root 'README.md') -Destination $package
$thirdParty = Join-Path $package 'third-party'
New-Item -ItemType Directory -Force $thirdParty | Out-Null
$trayVersion = python -c "from importlib.metadata import distribution; from pathlib import Path; import shutil,sys; target=Path(sys.argv[1]); [(shutil.copyfile(d.locate_file(f),target/(name+'-'+Path(str(f)).name)) ) for name in ['Pillow','six','cryptography','cffi','pycparser','certifi'] for d in [distribution(name)] for f in d.files if Path(str(f)).name.startswith('LICENSE')]; print(distribution('pystray').version)" $thirdParty
if ($LASTEXITCODE -ne 0) { throw 'Could not package dependency licenses.' }
$trayVersion = $trayVersion.Trim()
Invoke-WebRequest "https://github.com/moses-palmer/pystray/archive/refs/tags/v$trayVersion.zip" `
    -OutFile (Join-Path $thirdParty "pystray-$trayVersion-source.zip")
Invoke-WebRequest 'https://www.gnu.org/licenses/gpl-3.0.txt' `
    -OutFile (Join-Path $thirdParty 'pystray-GPL-3.0.txt')
$archive = Join-Path $OutputDirectory 'GooPadCompanion-Windows.zip'
Compress-Archive -Path (Join-Path $package '*') -DestinationPath $archive -Force
$version = python -c "import sys; sys.path.insert(0,sys.argv[1]); from companion_update import VERSION; print(VERSION)" (Join-Path $root 'companion')
if ($LASTEXITCODE -ne 0) { throw 'Could not determine the Companion version.' }
$version = $version.Trim()
& $InnoCompiler "/DPackageDir=$((Resolve-Path $package).Path)" "/DOutputDir=$((Resolve-Path $OutputDirectory).Path)" "/DAppVersion=$version" (Join-Path $root 'tools\companion_installer.iss')
if ($LASTEXITCODE -ne 0) { throw 'Companion installer build failed.' }
Get-Item $archive, (Join-Path $OutputDirectory "GooPadCompanion-Setup-$version.exe")
