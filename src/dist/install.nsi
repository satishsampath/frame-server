;----------- begin script ----------------

Unicode True
!Include "WinMessages.nsh"
!include Sections.nsh
!include LogicLib.nsh
!include x64.nsh
;  !packhdr exehead.tmp 'upx.exe -9 exehead.tmp'

RequestExecutionLevel admin
Name "Debugmode FrameServer"
OutFile "fssetup.exe"
BrandingText " "
SetCompressor lzma

Page license
Page components "" "" postComponentsPage
Page directory preCoreDirPage "" postCoreDirPage
Page directory preVegasFrom18DirPage showVegasDirPage postVegasFrom18DirPage
Page directory preVegasUpto17DirPage showVegasDirPage postVegasUpto17DirPage
Page directory prePremiereDirPage showPremiereDirPage postPremiereDirPage
Page instfiles

AutoCloseWindow true

LicenseText "Please read the following license agreement and proceed"
LicenseData "license.txt"

InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "InstallDir"
DirText "Installation Directory" "Select where to install Debugmode FrameServer Core Files"

Var FsInstallDir

;--------------------------------------------
; AbortIf32BitOS
; No inputs, just checks if we are running on a 32 bit OS and if so craps out.
;--------------------------------------------
Function AbortIf32BitOS
  ${If} ${RunningX64}
  ${Else}
    MessageBox MB_OK|MB_ICONSTOP "Debugmode Frameserver can only install on 64-bit Windows."
    Abort
  ${EndIf}
FunctionEnd

;--------------------------------------------
; SetPageTextString
; input, top of stack  (e.g. HelloWorld)
; output, nothing
; Modifies R0
; Usage:
;   Push "HelloWorld"
;   Call SetPageTextString
;--------------------------------------------
Function SetPageTextString
  Exch $R2
  Push $R1
  FindWindow $R0 "#32770" "" $HWNDPARENT
  GetDlgItem $R1 $R0 1020
  SendMessage $R1 ${WM_SETTEXT} 0 "STR:$R2"
  Pop $R1
  Exch $R2
  Pop $R0
FunctionEnd

;--------------------------------------------
; AbortIfInvalidInstDir
; Checks $INSTDIR and verifies that it is a valid dir
;--------------------------------------------
Function AbortIfInvalidInstDir
  IfFileExists "$INSTDIR\*.*" noabort
  MessageBox MB_OK|MB_ICONSTOP "The specified plugin installation directory does not exist. Please verify and enter a valid plugin directory to install the plugin."
  Abort
noabort:
FunctionEnd

;--------------------------------------------
; AbortIfSectionNotSelected
; Checks $R0 and aborts if the flag SF_SELECTED is not set
;--------------------------------------------
Function AbortIfSectionNotSelected
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  Abort 1
noabort:
FunctionEnd

;----------- begin sections ----------------

ComponentText "Debugmode FrameServer comes with plug-ins for major video editing software listed below. Select which of these plug-ins you want to install."

Section "Debugmode FrameServer Core (required)" secCore
  SetShellVarContext all
  SectionIn 1 RO
  StrCpy $FsInstallDir "$0"
  SetOutPath "$0"
  File Bin\fscommon.dll
  File Bin\ImageSequence.dll
  File Bin\FsProxy.exe
  File Bin\DFsNetClient.exe
  File Bin\DFsNetServer.exe
  File Bin\dfscVegasOutV2.dll
  File Bin\dfscVegasOutV3.dll
  File dokan1.dll
  WriteUninstaller "$0\fsuninst.exe"

  ${DisableX64FSRedirection}
  SetOutPath "$SYSDIR"
  File bin\dfsc.dll
  File bin\dfscacm.dll
  SetOutPath "$WINDIR\SysWOW64"
  File bin\dfsc32.dll
  File bin\dfscacm32.dll

  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "InstallDir" "$0"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Debugmode FrameServer" "DisplayName" "Debugmode FrameServer"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\Debugmode FrameServer" "UninstallString" '"$0\fsuninst.exe"'
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "vidc.dfsc" "dfsc.dll"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "msacm.dfscacm" "dfscacm.dll"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers.Desc" "dfsc.dll" "Debugmode FSVFWC (internal use)"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers.Desc" "dfscacm.dll" "Debugmode FSACMC (internal use)"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Drivers32" "vidc.dfsc" "dfsc32.dll"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Drivers32" "msacm.dfscacm" "dfscacm32.dll"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Drivers.Desc" "dfsc32.dll" "Debugmode FSVFWC (internal use)"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Drivers.Desc" "dfscacm32.dll" "Debugmode FSACMC (internal use)"
;  WriteIniStr $SYSDIR\System.ini drivers32 vidc.dfsc dfsc.dll
;  WriteIniStr $SYSDIR\System.ini drivers32 msacm.dfscacm dfscacm.dll
;  WriteIniStr $SYSDIR\Control.ini drivers.desc dfsc.dll "Debugmode FSVFWC (internal use)"
;  WriteIniStr $SYSDIR\Control.ini drivers.desc dfscacm.dll "Debugmode FSACMC (internal use)"

  MessageBox MB_YESNO "Create a Program Group in the Start Menu?" IDNO lblAfterProgramGroup
    CreateDirectory "$SMPROGRAMS\Debugmode\FrameServer"
    CreateShortcut "$SMPROGRAMS\Debugmode\FrameServer\FrameServer Network Client.lnk" "$0\DFsNetClient.exe" 0 "$0\fscommon.dll"
    CreateShortcut "$SMPROGRAMS\Debugmode\FrameServer\Uninstall FrameServer.lnk" "$0\fsuninst.exe"
  lblAfterProgramGroup:
SectionEnd

Section "Dokan Library (required)" secDokan
  ExecWait '"$FsInstallDir\FsProxy.exe" /getdv' $4
  ${If} $4 < 400
    MessageBox MB_OK "I'll now install Dokan, a library needed by FrameServer."
    SetOutPath "$FsInstallDir"
    File Dokan_x64.msi
    ExecWait 'msiexec /package "$FsInstallDir\Dokan_x64.msi"'
  ${EndIf}
SectionEnd

Section /o "Magix Vegas Plugin - from Vegas 18" secVegasFrom18Plug
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "VegasFrom18PlugConfig Path" "$1\Frameserver.x64.fio2007-config"
  ClearErrors
  FileOpen $R0 "$1\Frameserver.x64.fio2007-config" w
  FileWrite $R0 "[FileIO Plug-Ins]"
  FileWriteByte $R0 "13"
  FileWriteByte $R0 "10"
  FileWrite $R0 "frameserver=$FsInstallDir\dfscVegasOutV3.dll"
  FileClose $R0
  IfErrors 0 funcend
    MessageBox MB_OK|MB_ICONSTOP "Unable to create file $1\Frameserver.x64.fio2007-config"
    Goto funcend
  funcend:
SectionEnd

Section /o "Magix Vegas Plugin - upto Vegas 17" secVegasUpto17Plug
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "VegasUpto17PlugConfig Path" "$2\Frameserver.x64.fio2007-config"
  ClearErrors
  FileOpen $R0 "$2\Frameserver.x64.fio2007-config" w
  FileWrite $R0 "[FileIO Plug-Ins]"
  FileWriteByte $R0 "13"
  FileWriteByte $R0 "10"
  FileWrite $R0 "frameserver=$FsInstallDir\dfscVegasOutV2.dll"
  FileClose $R0
  IfErrors 0 funcend
    MessageBox MB_OK|MB_ICONSTOP "Unable to create file $2\Frameserver.x64.fio2007-config"
    Goto funcend
  funcend:
SectionEnd

Section /o "Adobe Premiere Pro/Elements Plugin" secPremPlug
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "PremPlug Path" "$3\dfscPremiereOut.prm"
  SetOutPath "$3"
  File bin\dfscPremiereOut.prm
SectionEnd

;----------- begin functions ----------------

Function preCoreDirPage
  StrCpy $INSTDIR "$PROGRAMFILES64\Debugmode\FrameServer\"
FunctionEnd

Function preVegasFrom18DirPage
  SectionGetFlags ${secVegasFrom18Plug} $R0
  Call AbortIfSectionNotSelected
  IfFileExists "$PROGRAMFILES64\Vegas\Vegas Pro 18.0\*.*" vegas180
  IfFileExists "$PROGRAMFILES64\Vegas\Vegas Pro 19.0\*.*" vegas190
  Goto funcend
vegas180:
  StrCpy $INSTDIR "$PROGRAMFILES64\Vegas\Vegas Pro 18.0"
  Goto funcend
vegas190:
  StrCpy $INSTDIR "$PROGRAMFILES64\Vegas\Vegas Pro 19.0"
funcend:
FunctionEnd

Function preVegasUpto17DirPage
  SectionGetFlags ${secVegasUpto17Plug} $R0
  Call AbortIfSectionNotSelected
  IfFileExists "$PROGRAMFILES64\Vegas\Vegas Pro 16.0\*.*" vegas160
  IfFileExists "$PROGRAMFILES64\Vegas\Vegas Pro 17.0\*.*" vegas170
  Goto funcend
vegas160:
  StrCpy $INSTDIR "$PROGRAMFILES64\Vegas\Vegas Pro 16.0"
  Goto funcend
vegas170:
  StrCpy $INSTDIR "$PROGRAMFILES64\Vegas\Vegas Pro 17.0"
funcend:
FunctionEnd

Function prePremiereDirPage
  SectionGetFlags ${secPremPlug} $R0
  Call AbortIfSectionNotSelected
  ReadRegStr $INSTDIR HKEY_LOCAL_MACHINE "SOFTWARE\Adobe\Premiere Pro\13.1" "Plug-insDir"
  ${If} $INSTDIR == ""
    ReadRegStr $INSTDIR HKEY_LOCAL_MACHINE "SOFTWARE\Adobe\Premiere Pro\13.0" "Plug-insDir"
  ${EndIf}
  ${If} $INSTDIR == ""
    ReadRegStr $INSTDIR HKEY_LOCAL_MACHINE "SOFTWARE\Adobe\Premiere Pro\12.0" "Plug-insDir"
  ${EndIf}
  ${If} $INSTDIR == ""
    ReadRegStr $INSTDIR HKEY_LOCAL_MACHINE "SOFTWARE\Adobe\Premiere Pro\11.0" "Plug-insDir"
  ${EndIf}
  ${If} $INSTDIR == ""
    ReadRegStr $INSTDIR HKEY_LOCAL_MACHINE "SOFTWARE\Adobe\Premiere Elements\17.0" "Plug-insDir"
  ${EndIf}
  ${If} $INSTDIR == ""
    ReadRegStr $INSTDIR HKEY_LOCAL_MACHINE "SOFTWARE\Adobe\Premiere Pro\CurrentVersion" "Plug-InsDir"
  ${EndIf}
FunctionEnd

Function showVegasDirPage
  Push "Select where to install Magix Vegas Pro Plugin"
  Call SetPageTextString
FunctionEnd

Function showPremiereDirPage
  Push "Select where to install Adobe Premiere Pro Plugin"
  Call SetPageTextString
FunctionEnd

Function postComponentsPage
  SectionGetFlags ${secVegasFrom18Plug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  SectionGetFlags ${secVegasUpto17Plug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  SectionGetFlags ${secPremPlug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  MessageBox MB_YESNO|MB_ICONSTOP "You have not selected any frameserver plugin to install. $\n$\nThis means only the core frameserver will be installed and that can be used for the sole purpose of receiving data from a network frameserver. Is this what you want?" IDYES noabort
  Abort 1
noabort:
FunctionEnd

Function postCoreDirPage
  StrCpy $0 $INSTDIR
FunctionEnd

Function postVegasFrom18DirPage
  Call AbortIfInvalidInstDir
  StrCpy $1 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function postVegasUpto17DirPage
  Call AbortIfInvalidInstDir
  StrCpy $2 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function postPremiereDirPage
  Call AbortIfInvalidInstDir
  StrCpy $3 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function .onInit
  Call AbortIf32BitOS
  SetRegView 64 
FunctionEnd

Function .onInstSuccess
  DeleteRegValue HKEY_CURRENT_USER "SOFTWARE\Debugmode\FrameServer" "updateUrl"
  MessageBox MB_OK "Installation complete."
FunctionEnd
  
;----------- begin uninstall settings/section ----------------

UninstallText "This will uninstall Debugmode FrameServer from your system"

Section Uninstall
  SetShellVarContext all
  SetRegView 64 
  ReadRegStr $R1 HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "VegasPlug Path"
  ReadRegStr $R2 HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "VegasUpto17PlugConfig Path"
  ReadRegStr $R3 HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "VegasFrom18PlugConfig Path"
  ReadRegStr $R4 HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "PremPlug Path"
  UnRegDll "$R1"
  Delete "$R1"
  Delete "$R2"
  Delete "$R3"
  Delete "$R4"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Debugmode FrameServer"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "InstallDir"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "VegasPlug Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "VegasUpto17PlugConfig Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "VegasFrom18PlugConfig Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\Debugmode\FrameServer" "PremPlug Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "vidc.dfsc"
  DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers32" "msacm.dfscacm"
  DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers.Desc" "dfsc.dll"
  DeleteRegValue HKEY_LOCAL_MACHINE "Software\Microsoft\Windows NT\CurrentVersion\Drivers.Desc" "dfscacm.dll"
  DeleteRegValue HKEY_LOCAL_MACHINE "Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Drivers32" "vidc.dfsc"
  DeleteRegValue HKEY_LOCAL_MACHINE "Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Drivers32" "msacm.dfscacm"
  DeleteRegValue HKEY_LOCAL_MACHINE "Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Drivers.Desc" "dfsc32.dll"
  DeleteRegValue HKEY_LOCAL_MACHINE "Software\Wow6432Node\Microsoft\Windows NT\CurrentVersion\Drivers.Desc" "dfscacm32.dll"
;  DeleteIniStr System.ini drivers32 vidc.dfsc
;  DeleteIniStr System.ini drivers32 msacm.dfscacm
;  DeleteIniStr Control.ini drivers.desc dfsc.dll
;  DeleteIniStr Control.ini drivers.desc dfscacm.dll
  RMDir /r /REBOOTOK "$SMPROGRAMS\Debugmode\FrameServer"
  ${DisableX64FSRedirection}
  Delete /REBOOTOK "$SYSDIR\dfsc.dll"
  Delete /REBOOTOK "$SYSDIR\dfscacm.dll"
  Delete /REBOOTOK "$WINDIR\SysWOW64\dfsc32.dll"
  Delete /REBOOTOK "$WINDIR\SysWOW64\dfscacm32.dll"
  Delete "$INSTDIR\fscommon.dll"
  Delete "$INSTDIR\ImageSequence.dll"
  Delete "$INSTDIR\dokan1.dll"
  Delete "$INSTDIR\Dokan_x64.msi"
  Delete "$INSTDIR\FsProxy.exe"
  Delete "$INSTDIR\DFsNetClient.exe"
  Delete "$INSTDIR\DFsNetServer.exe"
  Delete "$INSTDIR\dfscVegasOutV2.dll"
  Delete "$INSTDIR\dfscVegasOutV3.dll"
  Delete "$INSTDIR\fsuninst.exe"
  RMDir "$INSTDIR"
  IfRebootFlag doreb noreb
doreb:
  MessageBox MB_YESNO "Some files are in use and cannot be deleted.$\nTo complete the uninstall you need to reboot the system.$\n$\nReboot now?" IDNO noreb
  Reboot
noreb:
SectionEnd
