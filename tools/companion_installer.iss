#ifndef PackageDir
  #error PackageDir must point to the packaged Companion directory.
#endif
#ifndef AppVersion
  #error AppVersion must be supplied by the packaging script.
#endif

[Setup]
AppId={{B1A919C1-1D39-44C7-B3EF-592E55A9EF5D}
AppName=GooPad Companion
AppVersion={#AppVersion}
AppPublisher=GooPad
AppPublisherURL=https://github.com/jeromedontdev/GooPad
DefaultDirName={localappdata}\GooPad\bin
DisableDirPage=yes
DefaultGroupName=GooPad
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir={#OutputDir}
OutputBaseFilename=GooPadCompanion-Setup-{#AppVersion}
UninstallDisplayIcon={app}\GooPadCompanion.exe
Compression=lzma2
SolidCompression=yes
CloseApplications=yes
RestartApplications=no

[Files]
Source: "{#PackageDir}\GooPadCompanion.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PackageDir}\third-party\*"; DestDir: "{app}\third-party"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\GooPad Companion"; Filename: "{app}\GooPadCompanion.exe"; Parameters: "--gui"

[Run]
Filename: "{app}\GooPadCompanion.exe"; Parameters: "--gui --port {param:GOOPADPORT|8787}"; Flags: nowait

[Code]
function OpenProcess(Access: Cardinal; Inherit: Boolean; ProcessId: Cardinal): THandle;
  external 'OpenProcess@kernel32.dll stdcall';
function WaitForSingleObject(Handle: THandle; Milliseconds: Cardinal): Cardinal;
  external 'WaitForSingleObject@kernel32.dll stdcall';
function CloseHandle(Handle: THandle): Boolean;
  external 'CloseHandle@kernel32.dll stdcall';

function InitializeSetup(): Boolean;
var
  ProcessId: Integer;
  ProcessHandle: THandle;
  WaitResult: Cardinal;
begin
  Result := True;
  ProcessId := StrToIntDef(ExpandConstant('{param:GOOPADPID|0}'), 0);
  if ProcessId <= 0 then exit;
  ProcessHandle := OpenProcess($00100000, False, ProcessId);
  if ProcessHandle = 0 then exit;
  WaitResult := WaitForSingleObject(ProcessHandle, 30000);
  CloseHandle(ProcessHandle);
  if WaitResult <> 0 then begin
    MsgBox('Companion did not close. Please quit it and run the installer again.', mbError, MB_OK);
    Result := False;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Command, Executable: String;
begin
  if CurUninstallStep = usUninstall then
    if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run',
                           'GooPadCompanion', Command) then begin
      Executable := ExpandConstant('{app}\GooPadCompanion.exe');
      if (CompareText(Copy(Command, 1, Length(Executable) + 2), '"' + Executable + '"') = 0) or
         (CompareText(Copy(Command, 1, Length(Executable) + 1), Executable + ' ') = 0) or
         (CompareText(Command, Executable) = 0) then
        RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'GooPadCompanion');
    end;
end;
