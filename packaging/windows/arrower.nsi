Unicode true
Name "arrower"

!ifndef APP_VERSION
!define APP_VERSION "dev"
!endif

!ifndef DIST_DIR
!error "DIST_DIR must be defined"
!endif

!ifndef OUTPUT_FILE
!define OUTPUT_FILE "arrower-${APP_VERSION}-windows-x64-setup.exe"
!endif

OutFile "${OUTPUT_FILE}"
InstallDir "$LOCALAPPDATA\Programs\arrower"
InstallDirRegKey HKCU "Software\arrower" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma
SetShellVarContext current

Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "${DIST_DIR}\*"

  WriteRegStr HKCU "Software\arrower" "InstallDir" "$INSTDIR"
  WriteUninstaller "$INSTDIR\uninstall.exe"

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\arrower" "DisplayName" "arrower"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\arrower" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\arrower" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\arrower" "Publisher" "arrower"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\arrower" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\arrower" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\arrower" "NoRepair" 1

  CreateDirectory "$SMPROGRAMS\arrower"
  CreateShortCut "$SMPROGRAMS\arrower\arrower.lnk" "$INSTDIR\arrower.exe"
  CreateShortCut "$SMPROGRAMS\arrower\Uninstall arrower.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\arrower\arrower.lnk"
  Delete "$SMPROGRAMS\arrower\Uninstall arrower.lnk"
  RMDir "$SMPROGRAMS\arrower"

  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "Software\arrower"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\arrower"
SectionEnd
