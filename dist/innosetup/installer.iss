#include "config.iss"

[Setup]
AppCopyright=Copyright (C) LibrePCB Developers
AppId=librepcb
AppName=LibrePCB
AppPublisher=LibrePCB Developers
AppPublisherURL=https://librepcb.org
AppSupportURL=https://librepcb.org/help
AppUpdatesURL=https://librepcb.org/download
AppVerName=LibrePCB {#LIBREPCB_APP_VERSION}
AppVersion={#LIBREPCB_APP_VERSION}
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
ChangesAssociations=yes
Compression=lzma
DefaultDirName={autopf}\LibrePCB
DisableProgramGroupPage=yes
DisableWelcomePage=no
MinVersion=10.0
OutputBaseFilename=librepcb-installer
OutputDir=.
PrivilegesRequiredOverridesAllowed=commandline
ShowLanguageDialog=no
SolidCompression=yes
UninstallDisplayIcon={app}\bin\librepcb.exe
UsePreviousGroup=no
UsePreviousLanguage=no
VersionInfoDescription=LibrePCB Installer
VersionInfoTextVersion={#LIBREPCB_APP_VERSION}
WizardImageFile=watermark.bmp
WizardSmallImageFile=logo.bmp
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "armenian"; MessagesFile: "compiler:Languages\Armenian.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "bulgarian"; MessagesFile: "compiler:Languages\Bulgarian.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"
Name: "icelandic"; MessagesFile: "compiler:Languages\Icelandic.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "slovak"; MessagesFile: "compiler:Languages\Slovak.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[Files]
Source: "files\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
; *.lpp file extension
Root: HKA; Subkey: "Software\Classes\.lpp\OpenWithProgids"; ValueType: string; ValueName: "LibrePCB.lpp"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\LibrePCB.lpp"; ValueType: string; ValueName: ""; ValueData: "LibrePCB Project"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\LibrePCB.lpp\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\librepcb.exe,0"
Root: HKA; Subkey: "Software\Classes\LibrePCB.lpp\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\librepcb.exe"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\librepcb.exe\SupportedTypes"; ValueType: string; ValueName: ".lpp"; ValueData: ""
; *.lppz file extension
Root: HKA; Subkey: "Software\Classes\.lppz\OpenWithProgids"; ValueType: string; ValueName: "LibrePCB.lppz"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\LibrePCB.lppz"; ValueType: string; ValueName: ""; ValueData: "LibrePCB Project Archive"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\LibrePCB.lppz\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\librepcb.exe,0"
Root: HKA; Subkey: "Software\Classes\LibrePCB.lppz\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\librepcb.exe"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\librepcb.exe\SupportedTypes"; ValueType: string; ValueName: ".lppz"; ValueData: ""

[Icons]
Name: "{autoprograms}\LibrePCB"; Filename: "{app}\bin\librepcb.exe"

[Run]
Filename: "{app}\bin\librepcb.exe"; Description: "{cm:LaunchProgram,LibrePCB}"; Flags: nowait postinstall skipifsilent

[Code]
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  uninstallCmd: String;
  success: Boolean;
  exitCode: Integer;
begin
  // Uninstall LibrePCB <= 1.0.0 which was installed by Qt Installer Framework.
  uninstallCmd := ExpandConstant('{app}\\librepcb-maintenance.exe');
  if not FileExists(uninstallCmd) then
    uninstallCmd := 'C:\\Program Files (x86)\\LibrePCB\\librepcb-maintenance.exe';
  if (not WizardSilent()) and FileExists(uninstallCmd) then
  begin
    if SuppressibleMsgBox('An older version of LibrePCB is currently installed. Please follow the uninstaller to remove it before installing the new version.', mbConfirmation, MB_OKCANCEL, IDOK) = IDOK then
    begin
      success := Exec(uninstallCmd, '', '', SW_SHOW, ewWaitUntilTerminated, exitCode);
      if (not success) or (exitCode <> 0) then
        Result := 'Failed to uninstall older version, please uninstall manually.';
    end
    else
      Result := 'Aborted by user.';
  end;

  // Uninstall previous version for a clean upgrade (removing old files).
  // Ignore the exit code as a failed uninstallation is not *that* critical.
  success := RegQueryStringValue(HKA, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\librepcb_is1', 'QuietUninstallString', uninstallCmd);
  if (success) and (Length(uninstallCmd) > 0) then
  begin
    success := Exec('>', uninstallCmd, '', SW_SHOW, ewWaitUntilTerminated, exitCode);
  end;
end;
