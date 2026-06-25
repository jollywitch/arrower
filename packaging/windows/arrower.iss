#define AppName "arrower"

#ifndef AppVersion
#define AppVersion "dev"
#endif

#ifndef SourceDir
#error SourceDir must be defined
#endif

#ifndef OutputDir
#define OutputDir "."
#endif

[Setup]
AppId={{A82C7CC8-DB54-4E8E-A585-3C0A1D6D5830}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=arrower
DefaultDirName={localappdata}\Programs\arrower
DefaultGroupName=arrower
DisableProgramGroupPage=yes
OutputDir={#OutputDir}
OutputBaseFilename=arrower-{#AppVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\arrower.exe
WizardStyle=modern

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\arrower"; Filename: "{app}\arrower.exe"
Name: "{group}\Uninstall arrower"; Filename: "{uninstallexe}"
