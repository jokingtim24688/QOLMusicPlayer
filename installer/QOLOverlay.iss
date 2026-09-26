; Inno Setup script for QOL Overlay.
; Built by CI:  iscc /DAppVersion=0.2.0 installer\QOLOverlay.iss   (expects build\Release\QOLOverlay.exe)
; Installs to Program Files, always creates a desktop + Start menu shortcut, and launches the app when done.

#ifndef AppVersion
  #define AppVersion "0.0.0-dev"
#endif
#ifndef ExePath
  #define ExePath "..\build\Release\QOLOverlay.exe"
#endif
#define AppName "QOL Overlay"
#define AppExe "QOLOverlay.exe"

[Setup]
AppId={{6F1C2B8E-4A7D-4C51-9E3B-2D8F0A6C7B14}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=jokingtim
AppPublisherURL=https://github.com/jokingtim24688/QOLMusicPlayer
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
DisableDirPage=auto
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=Output
OutputBaseFilename=QOLOverlay-Setup
SetupIconFile=..\res\app.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes
; The running overlay holds its exe open; the [Code] section closes it first.
CloseApplications=no

[Files]
Source: "{#ExePath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\assets\fonts\licenses\*"; DestDir: "{app}\licenses"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autoprograms}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Run]
; Checked by default on the last page, so finishing the wizard opens the overlay.
Filename: "{app}\{#AppExe}"; Description: "Launch {#AppName}"; Flags: nowait postinstall skipifsilent shellexec
; Silent auto-update (the app runs this installer with /update=1): start the new version straight away.
Filename: "{app}\{#AppExe}"; Flags: nowait shellexec; Check: IsUpdate

[UninstallRun]
Filename: "{sys}\taskkill.exe"; Parameters: "/F /IM {#AppExe}"; Flags: runhidden; RunOnceId: "StopOverlay"

[Code]
function IsUpdate: Boolean;
begin
  Result := ExpandConstant('{param:update|0}') = '1';
end;

// Close a running copy so an update can replace the exe.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Exec(ExpandConstant('{sys}\taskkill.exe'), '/F /IM {#AppExe}', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  Result := '';
end;
