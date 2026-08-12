; =============================================================================
; FainsGram — Windows Installer (Inno Setup)
; Packages the built app folder (incl. Qt deps from windeployqt and the bundled
; tg-ws-proxy runtime under fainsgram\wsproxy\tg-ws-proxy) into FainsGram-x64-<ver>.exe
;
; Defines passed from the CI (or `iscc` command line):
;   /DReleasePath=<folder with FainsGram.exe + Qt DLLs + fainsgram\...>
;   /DReleaseOut=<output folder for the .exe>
;   /DSourcePath=<tdesktop\Telegram\build>   (for the icon)
;   /DMyAppVersion=1.0.0
;   /DMyAppVersionFull=1.0.0
; =============================================================================

#define MyAppName "FainsGram"
#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif
#ifndef MyAppVersionFull
  #define MyAppVersionFull "1.0.0"
#endif
#ifndef MyAppExeName
  #define MyAppExeName "FainsGram.exe"
#endif
#define MyAppPublisher "FainsGram"
#define MyAppURL "https://t.me/FainsGram"
#define MyAppId "2C3D4E5F-6A7B-48C9-9D0E-1F2A3B4C5D6E"
#define CurrentYear GetDateTimeString('yyyy','','')

[Setup]
AppId={{{#MyAppId}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppCopyright={#MyAppPublisher} 2014-{#CurrentYear}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir={#ReleaseOut}
SetupIconFile={#SourcePath}\..\Resources\art\icon256.ico
UninstallDisplayName={#MyAppName}
Compression=lzma
SolidCompression=yes
DisableStartupPrompt=yes
PrivilegesRequired=lowest
VersionInfoVersion={#MyAppVersion}.0
WizardStyle=modern
OutputBaseFilename=FainsGram-x64-{#MyAppVersionFull}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; Whole build output dir (Telegram.exe renamed to FainsGram.exe + Qt DLLs + fainsgram\wsproxy\tg-ws-proxy)
Source: "{#ReleasePath}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{app}\data"
Type: filesandordirs; Name: "{app}\tdata"
Type: filesandordirs; Name: "{app}\tcache"
Type: filesandordirs; Name: "{app}\DebugLogs"
Type: dirifempty; Name: "{app}"
