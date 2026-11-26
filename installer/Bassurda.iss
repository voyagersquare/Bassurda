; Bassurda Windows installer (Inno Setup 6)
; Built by CI:  ISCC /DAppVersion=0.2.0 /DVST3Dir=<folder containing Bassurda.vst3> installer\Bassurda.iss

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef VST3Dir
  #define VST3Dir "..\build\Bassurda_artefacts\Release\VST3"
#endif

[Setup]
AppId={{6C1B2E5A-8F3D-4B7E-9A21-5D0B3C7E4F19}
AppName=Bassurda
AppVersion={#AppVersion}
AppVerName=Bassurda {#AppVersion}
AppPublisher=VoyagerSquare
AppPublisherURL=https://github.com/voyagersquare/Bassurda
AppSupportURL=https://github.com/voyagersquare/Bassurda/issues
DefaultDirName={autopf}\Bassurda
DisableDirPage=yes
DisableProgramGroupPage=yes
UninstallDisplayName=Bassurda VST3
UninstallFilesDir={autopf}\Bassurda
LicenseFile=..\LICENSE
OutputDir=Output
OutputBaseFilename=Bassurda-Windows-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UsedUserAreasWarning=no
CloseApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
WelcomeLabel2=This will install the Bassurda bass synth (VST3, 64-bit) on your computer.%n%nThe plugin goes to C:\Program Files\Common Files\VST3. Your own presets and samples live in Documents\Bassurda.%n%nPlease close your DAW before continuing.

[Files]
; the plugin bundle
Source: "{#VST3Dir}\Bassurda.vst3\*"; DestDir: "{commoncf64}\VST3\Bassurda.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
; extra presets and samples shipped with the installer (never overwrite the user's files)
Source: "Content\Presets\*.bassurda"; DestDir: "{userdocs}\Bassurda\Presets"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist
Source: "Content\Samples\*.wav";  DestDir: "{userdocs}\Bassurda\Samples"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist
Source: "Content\Samples\*.aif";  DestDir: "{userdocs}\Bassurda\Samples"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist
Source: "Content\Samples\*.aiff"; DestDir: "{userdocs}\Bassurda\Samples"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist
Source: "Content\Samples\*.flac"; DestDir: "{userdocs}\Bassurda\Samples"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist
Source: "Content\Samples\*.ogg";  DestDir: "{userdocs}\Bassurda\Samples"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist
Source: "Content\Samples\*.mp3";  DestDir: "{userdocs}\Bassurda\Samples"; Flags: onlyifdoesntexist uninsneveruninstall skipifsourcedoesntexist

[Dirs]
Name: "{userdocs}\Bassurda\Presets"; Flags: uninsneveruninstall
Name: "{userdocs}\Bassurda\Samples"; Flags: uninsneveruninstall

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Bassurda.vst3"
