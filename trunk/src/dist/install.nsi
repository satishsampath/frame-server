;----------- begin script ----------------

!Include "WinMessages.nsh"
!include Sections.nsh
;  !packhdr exehead.tmp 'upx.exe -9 exehead.tmp'

Name "DebugMode FrameServer"
OutFile "fssetup.exe"
BrandingText " "
SetCompressor lzma

Page license
Page components "" "" postComponentsPage
Page directory preCoreDirPage "" postCoreDirPage
Page directory preVegasDirPage showVegasDirPage postVegasDirPage
Page directory prePremiereCS5DirPage showPremiereCS5DirPage postPremiereCS5DirPage
Page directory prePremiereDirPage showPremiereDirPage postPremiereDirPage
Page directory preMsproDirPage showMsproDirPage postMsproDirPage
Page directory preUVStudioDirPage showUVStudioDirPage postUVStudioDirPage
Page directory preEditstudioDirPage showEditstudioDirPage postEditstudioDirPage
Page directory preWaxDirPage showWaxDirPage postWaxDirPage
Page instfiles

AutoCloseWindow true

LicenseText "Please read the following license agreement and proceed"
LicenseData "license.txt"

InstallDirRegKey HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "InstallDir"
DirText "Installation Directory" "Select where to install DebugMode FrameServer Core Files"

Var FsInstallDir
Var UsePremiereV2
Var UsePremiereV2CS5

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

ComponentText "DebugMode FrameServer comes with plug-ins for major video editing software listed below. Select which of these plug-ins you want to install."

Section "DebugMode FrameServer Core (required)" secCore
  SectionIn 1 RO
  StrCpy $FsInstallDir "$0"
  SetOutPath "$0"
  File readme.txt
  File changelog.txt
  File Bin\fscommon.dll
  File Bin\ImageSequence.dll
  File Bin\DFsNetClient.exe
  File Bin\DFsNetServer.exe
  File Bin\dfscVegasV2Out.dll
  WriteUninstaller "$0\fsuninst.exe"

  SetOutPath "$SYSDIR"
  File bin\dfsc.dll
  File bin\dfscacm.dll

  SetRegView 64 
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "InstallDir" "$0"
  SetRegView 32 
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "InstallDir" "$0"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\DebugMode FrameServer" "DisplayName" "DebugMode FrameServer"
  WriteRegStr HKEY_LOCAL_MACHINE "Software\Microsoft\Windows\CurrentVersion\Uninstall\DebugMode FrameServer" "UninstallString" '"$0\fsuninst.exe"'
  WriteIniStr System.ini drivers32 vidc.dfsc dfsc.dll
  WriteIniStr System.ini drivers32 msacm.dfscacm dfscacm.dll
  WriteIniStr Control.ini drivers.desc dfsc.dll "DebugMode FSVFWC (internal use)"
  WriteIniStr Control.ini drivers.desc dfscacm.dll "DebugMode FSACMC (internal use)"

  MessageBox MB_YESNO "Create a Program Group in the Start Menu?" IDNO lblAfterProgramGroup
    CreateDirectory "$SMPROGRAMS\Debugmode\FrameServer"
    CreateShortcut "$SMPROGRAMS\Debugmode\FrameServer\FrameServer Network Client.lnk" "$0\DFsNetClient.exe" 0 "$0\fscommon.dll"
    CreateShortcut "$SMPROGRAMS\Debugmode\FrameServer\ChangeLog.lnk" "$0\changelog.txt"
    CreateShortcut "$SMPROGRAMS\Debugmode\FrameServer\Readme.lnk" "$0\readme.txt"
    CreateShortcut "$SMPROGRAMS\Debugmode\FrameServer\Uninstall FrameServer.lnk" "$0\fsuninst.exe"
  lblAfterProgramGroup:
SectionEnd

Section /o "Sony® Vegas® Plugin" secVegasPlug
  IfFileExists "$1\vegas*.exe" 0 prevegas90
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "VegasV2PlugConfig Path" "$1\Frameserver.x86.fio2007-config"
    ClearErrors
    FileOpen $R0 "$1\Frameserver.x86.fio2007-config" w
    FileWrite $R0 "[FileIO Plug-Ins]"
    FileWriteByte $R0 "13"
    FileWriteByte $R0 "10"
    FileWrite $R0 "frameserver=$FsInstallDir\dfscVegasV2Out.dll"
    FileClose $R0
    IfErrors 0 funcend
      MessageBox MB_OK|MB_ICONSTOP "Unable to create file $1\Frameserver.x86.fio2007-config"
      Goto funcend
  prevegas90:
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "VegasPlug Path" "$1\dfscVegasOut.dll"
    SetOutPath "$1"
    File bin\dfscVegasOut.dll
    RegDLL "$1\dfscVegasOut.dll"
  funcend:
SectionEnd

Section /o "Adobe® Premiere® CS5 (64-bit) Plugin" secPremCS5Plug
  IntCmp $UsePremiereV2CS5 1 0 funcend
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "PremCS5Plug Path" "$2\dfscPremiereOutCS5.prm"
    SetOutPath "$2"
    File bin\dfscPremiereOutCS5.prm
  funcend:
SectionEnd

Section /o "Adobe® Premiere®/Pro/Elements Plugin" secPremPlug
  IntCmp $UsePremiereV2 1 0 precs4
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "PremPlug Path" "$2\dfscPremiereOut.prm"
    SetOutPath "$2"
    File bin\dfscPremiereOut.prm
    Goto funcend
  precs4:
    WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "PremPlug Path" "$2\cm-dfscPremiereOut.prm"
    SetOutPath "$2"
    File bin\cm-dfscPremiereOut.prm
  funcend:
SectionEnd

Section /o "Ulead® MediaStudio® Pro Plugin" secMsproPlug
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "MsproPlug Path" "$3\dfscMSProOut.vio"
  SetOutPath "$3"
  File bin\dfscMSProOut.vio
SectionEnd

Section /o "Ulead® VideoStudio® Plugin" secUVStudioPlug
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "UVStudioPlug Path" "$4\dfscMSProOut.vio"
  SetOutPath "$4"
  File bin\dfscMSProOut.vio
SectionEnd

Section /o "Pure Motion EditStudio Plugin" secEditstudioPlug
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "EditstudioPlug Path" "$5\dfscEditStudioOut.eds_plgn"
  SetOutPath "$5"
  File bin\dfscEditStudioOut.eds_plgn
SectionEnd

Section /o "DebugMode Wax Plugin" secWaxPlug
  WriteRegStr HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "WaxPlug Path" "$6\dfscWaxOut.dll"
  SetOutPath "$6"
  File bin\dfscWaxOut.dll
SectionEnd

;----------- begin functions ----------------

Function preCoreDirPage
  StrCpy $INSTDIR "$PROGRAMFILES\DebugMode\FrameServer\"
FunctionEnd

Function preWaxDirPage
  SectionGetFlags ${secWaxPlug} $R0
  Call AbortIfSectionNotSelected
  Strcpy $INSTDIR "$PROGRAMFILES\DebugMode\Filters"
FunctionEnd

Function preVegasDirPage
  SectionGetFlags ${secVegasPlug} $R0
  Call AbortIfSectionNotSelected
  IfFileExists "$PROGRAMFILES64\Sony\Vegas Pro 10.0\*.*" vegas100x64
  IfFileExists "$PROGRAMFILES\Sony\Vegas Pro 10.0\*.*" vegas100
  IfFileExists "$PROGRAMFILES\Sony\Vegas Pro 9.0\*.*" vegas90
  StrCpy $INSTDIR "$PROGRAMFILES\Sony\Shared Plug-Ins\*.*"
  IfFileExists "$INSTDIR\*.*" funcend
  CreateDirectory "$INSTDIR"
  Goto funcend
vegas90:
  StrCpy $INSTDIR "$PROGRAMFILES\Sony\Vegas Pro 9.0"
  Goto funcend
vegas100:
  StrCpy $INSTDIR "$PROGRAMFILES\Sony\Vegas Pro 10.0"
  Goto funcend
vegas100x64:
  StrCpy $INSTDIR "$PROGRAMFILES64\Sony\Vegas Pro 10.0"
  Goto funcend
funcend:
FunctionEnd

Function prePremiereCS5DirPage
  SectionGetFlags ${secPremCS5Plug} $R0
  Call AbortIfSectionNotSelected
  ; Check if Premiere Pro CS5 or above is present.
  Push 1
  Pop $UsePremiereV2CS5
  SetRegView 64 
  ReadRegStr $INSTDIR HKEY_LOCAL_MACHINE "SOFTWARE\Adobe\Premiere Pro\CurrentVersion" "Plug-InsDir"
  IfErrors 0 funcend  ; If found, we are done.
  Push 0
  Pop $UsePremiereV2CS5
funcend:
  SetRegView 32 
FunctionEnd

Function prePremiereDirPage
  SectionGetFlags ${secPremPlug} $R0
  Call AbortIfSectionNotSelected
  ; Check if Premiere Pro CS4 or above is present.
  Push 1
  Pop $UsePremiereV2
  ReadRegStr $INSTDIR HKEY_LOCAL_MACHINE "SOFTWARE\Adobe\Premiere Pro\CurrentVersion" "Plug-InsDir"
  IfErrors 0 funcend  ; If found, we are done.
  Push 0
  Pop $UsePremiereV2
  Strcpy $INSTDIR "$PROGRAMFILES"
  IfFileExists "$PROGRAMFILES\Adobe\Premiere Elements 1.0\*.*" vElem10present
  IfFileExists "$PROGRAMFILES\Adobe\Premiere Pro 1.5\*.*" vPro15present
  IfFileExists "$PROGRAMFILES\Adobe\Premiere Pro\*.*" vPro10present
  IfFileExists "$PROGRAMFILES\Adobe\Premiere 6.5\*.*" v65present
  IfFileExists "$PROGRAMFILES\Adobe\Premiere 6.0\*.*" v60present
  Goto funcend
  v60present:
    StrCpy $INSTDIR "$PROGRAMFILES\Adobe\Premiere 6.0\Plug-ins"
    Goto funcend
  v65present:
    StrCpy $INSTDIR "$PROGRAMFILES\Adobe\Premiere 6.5\Plug-ins"
    Goto funcend
  vPro10present:
    StrCpy $INSTDIR "$PROGRAMFILES\Adobe\Premiere Pro\Plug-ins"
    Goto funcend
  vPro15present:
    StrCpy $INSTDIR "$PROGRAMFILES\Adobe\Premiere Pro 1.5\Plug-ins\en_US"
    Goto funcend
  vElem10present:
    StrCpy $INSTDIR "$PROGRAMFILES\Adobe\Premiere Elements 1.0\Plug-ins\en_US"
    Goto funcend
  funcend:
FunctionEnd

Function preMsproDirPage
  SectionGetFlags ${secMsproPlug} $R0
  Call AbortIfSectionNotSelected
  StrCpy $INSTDIR "$PROGRAMFILES\Ulead Systems\Ulead MediaStudio Pro 7.0\vio"
FunctionEnd

Function preUVStudioDirPage
  SectionGetFlags ${secUVStudioPlug} $R0
  Call AbortIfSectionNotSelected
  StrCpy $INSTDIR "$PROGRAMFILES\Ulead Systems\Ulead VideoStudio 8.0\vio"
FunctionEnd

Function preEditstudioDirPage
  SectionGetFlags ${secEditstudioPlug} $R0
  Call AbortIfSectionNotSelected
  StrCpy $INSTDIR "$PROGRAMFILES\Pure Motion\EditStudio 5\Plugins"
    IfFileExists "$INSTDIR\*.*" funcend
  StrCpy $INSTDIR "$PROGRAMFILES\Pure Motion\EditStudio 4\Plugins"
  funcend:
FunctionEnd

Function showWaxDirPage
  Push "Wax Plugin will be installed at the following location (do not change)"
  Call SetPageTextString
FunctionEnd

Function showVegasDirPage
  Push "Select where to install Sony® Vegas® Plugin"
  Call SetPageTextString
FunctionEnd

Function showPremiereCS5DirPage
  Push "Select where to install Adobe® Premiere® CS5 (64-bit) Plugin"
  Call SetPageTextString
FunctionEnd

Function showPremiereDirPage
  Push "Select where to install Adobe® Premiere®/Premiere® Pro Plugin"
  Call SetPageTextString
FunctionEnd

Function showMsproDirPage
  Push "Select where to install Ulead® MediaStudio® Pro Plugin"
  Call SetPageTextString
FunctionEnd

Function showUVStudioDirPage
  Push "Select where to install Ulead® VideoStudio® Plugin"
  Call SetPageTextString
FunctionEnd

Function showEditstudioDirPage
  Push "Select where to install Pure Motion EditStudio Plugin"
  Call SetPageTextString
FunctionEnd

Function postComponentsPage
  SectionGetFlags ${secVegasPlug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  SectionGetFlags ${secPremCS5Plug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  SectionGetFlags ${secPremPlug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  SectionGetFlags ${secMsproPlug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  SectionGetFlags ${secUVStudioPlug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  SectionGetFlags ${secEditstudioPlug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  SectionGetFlags ${secWaxPlug} $R0
  IntOp $R0 $R0 & ${SF_SELECTED}
  StrCmp $R0 "${SF_SELECTED}" noabort
  MessageBox MB_YESNO|MB_ICONSTOP "You have not selected any frameserver plugin to install. $\n$\nThis means only the core frameserver will be installed and that can be used for the sole purpose of receiving data from a network frameserver. Is this what you want?" IDYES noabort
  Abort 1
noabort:
FunctionEnd

Function postCoreDirPage
  StrCpy $0 $INSTDIR
FunctionEnd

Function postVegasDirPage
  Call AbortIfInvalidInstDir
  StrCpy $1 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function postPremiereCS5DirPage
  Call AbortIfInvalidInstDir
  StrCpy $2 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function postPremiereDirPage
  Call AbortIfInvalidInstDir
  StrCpy $3 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function postMsproDirPage
  Call AbortIfInvalidInstDir
  StrCpy $4 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function postUVStudioDirPage
  Call AbortIfInvalidInstDir
  StrCpy $5 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function postEditstudioDirPage
  Call AbortIfInvalidInstDir
  StrCpy $6 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function postWaxDirPage
  Call AbortIfInvalidInstDir
  StrCpy $7 $INSTDIR
  StrCpy $INSTDIR $0
FunctionEnd

Function .onInstSuccess
  DeleteRegValue HKEY_CURRENT_USER "SOFTWARE\DebugMode\FrameServer" "updateUrl"
  MessageBox MB_YESNO "View readme?" IDNO NoReadme
  Exec 'notepad.exe "$0\readme.txt"'
  NoReadme:
FunctionEnd
  
;----------- begin uninstall settings/section ----------------

UninstallText "This will uninstall DebugMode FrameServer from your system"

Section Uninstall
  ReadRegStr $R1 HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "VegasPlug Path"
  ReadRegStr $R2 HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "VegasV2PlugConfig Path"
  ReadRegStr $R3 HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "PremCS5Plug Path"
  ReadRegStr $R4 HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "PremPlug Path"
  ReadRegStr $R5 HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "MsproPlug Path"
  ReadRegStr $R6 HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "UVStudioPlug Path"
  ReadRegStr $R7 HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "EditstudioPlug Path"
  ReadRegStr $R8 HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "WaxPlug Path"
  UnRegDll "$R1"
  Delete "$R1"
  Delete "$R2"
  Delete "$R3"
  Delete "$R4"
  Delete "$R5"
  Delete "$R6"
  Delete "$R7"
  Delete "$R8"
  Delete "$INSTDIR\dfscVegasV2Out.dll"
  Delete "$INSTDIR\DFsNetClient.exe"
  Delete "$INSTDIR\fsuninst.exe"
  Delete /REBOOTOK "$SYSDIR\dfsc.dll"
  Delete /REBOOTOK "$SYSDIR\dfscacm.dll"
  DeleteRegKey HKEY_LOCAL_MACHINE "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\DebugMode FrameServer"
  SetRegView 64 
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "InstallDir"
  SetRegView 32 
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "InstallDir"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "VegasPlug Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "VegasV2PlugConfig Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "PremCS5Plug Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "PremPlug Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "MsproPlug Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "UVStudioPlug Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "EditstudioPlug Path"
  DeleteRegValue HKEY_LOCAL_MACHINE "SOFTWARE\DebugMode\FrameServer" "WaxPlug Path"
  DeleteIniStr System.ini, drivers32, vidc.dfsc
  DeleteIniStr System.ini, drivers32, msacm.dfscacm
  DeleteIniStr Control.ini, drivers.desc, dfsc.dll
  DeleteIniStr Control.ini, drivers.desc, dfscacm.dll
  Delete "$SMPROGRAMS\Debugmode\FrameServer\*.*"
  RMDir "$SMPROGRAMS\Debugmode\FrameServer"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\changelog.txt"
  Delete "$INSTDIR\fscommon.dll"
  Delete "$INSTDIR\ImageSequence.dll"
  Delete "$INSTDIR\DFsNetClient.exe"
  Delete "$INSTDIR\DFsNetServer.exe"
  Delete "$INSTDIR\dfscVegasV2Out.dll"
  Delete "$INSTDIR\fsuninst.exe"
  RMDir "$INSTDIR"
  IfRebootFlag doreb noreb
doreb:
  MessageBox MB_YESNO "Some files are in use and cannot be deleted.$\nTo complete the uninstall you need to reboot the system.$\n$\nReboot now?" IDNO noreb
  Reboot
noreb:
SectionEnd
