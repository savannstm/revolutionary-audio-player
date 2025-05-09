#define ApplicationName "RAP"
#define ApplicationPublisher "savannstm"
#define ApplicationURL "https://github.com/savannstm/revolutionary-audio-player"
#define ExecutableName "rap.exe"
#define LicenseFile "LICENSE.md"
#define BuildDir "build\target\bin\Release"

[Setup]
AppId={{}}
AppName={#ApplicationName}
AppVersion=0.2.0
AppPublisher={#ApplicationPublisher}
AppPublisherURL={#ApplicationURL}
AppSupportURL={#ApplicationURL}
AppUpdatesURL={#ApplicationURL}
DefaultDirName={pf}\{#ApplicationName}
DefaultGroupName={#ApplicationName}
LicenseFile={#LicenseFile}
OutputDir=dist
OutputBaseFilename=rap-installer
Compression=lzma2/ultra64
SolidCompression=yes

[Files]
Source: "{#BuildDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#ApplicationName}"; Filename: "{app}\{#ExecutableName}"
Name: "{group}\Uninstall {#ApplicationName}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#ExecutableName}"; Description: "Launch {#ApplicationName}"; Flags: nowait postinstall skipifsilent