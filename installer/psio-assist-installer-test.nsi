; PSIO Assist EX Installer Script - Test Version
; This version can be tested without the actual application files

!define APPNAME "PSIO Assist EX"
!define COMPANYNAME "PSIO Assist Team"
!define DESCRIPTION "A tool for managing PlayStation games for PSIO"
!define VERSIONMAJOR 1
!define VERSIONMINOR 0
!define VERSIONBUILD 0
!define HELPURL "https://github.com/gabrielBitts/psio-assist-ex"
!define UPDATEURL "https://github.com/gabrielBitts/psio-assist-ex/releases"
!define ABOUTURL "https://github.com/gabrielBitts/psio-assist-ex"
!define INSTALLSIZE 50000

RequestExecutionLevel admin
InstallDir "$PROGRAMFILES\${APPNAME}"
Name "${APPNAME}"
outFile "PSIO-Assist-EX-Installer-Test.exe"

!include LogicLib.nsh

page directory
page instfiles

!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin"
    messageBox mb_iconstop "Administrator rights required!"
    setErrorLevel 740
    quit
${EndIf}
!macroend

function .onInit
    setShellVarContext all
    !insertmacro VerifyUserIsAdmin
functionEnd

section "install"
    setOutPath $INSTDIR
    
    ; Create dummy files for testing
    fileOpen $0 "$INSTDIR\psio-assist.exe" w
    fileWrite $0 "dummy executable"
    fileClose $0
    
    fileOpen $0 "$INSTDIR\README.md" w
    fileWrite $0 "dummy readme"
    fileClose $0
    
    fileOpen $0 "$INSTDIR\LICENSE" w
    fileWrite $0 "dummy license"
    fileClose $0
    
    ; Create dummy Qt DLLs
    fileOpen $0 "$INSTDIR\Qt6Core.dll" w
    fileWrite $0 "dummy dll"
    fileClose $0
    
    ; Create dummy translations
    setOutPath "$INSTDIR\translations"
    fileOpen $0 "$INSTDIR\translations\psio-assist_en_US.qm" w
    fileWrite $0 "dummy translation"
    fileClose $0
    
    fileOpen $0 "$INSTDIR\translations\psio-assist_pt_BR.qm" w
    fileWrite $0 "dummy translation"
    fileClose $0
    
    setOutPath $INSTDIR
    
    ; Create uninstaller
    writeUninstaller "$INSTDIR\uninstall.exe"
    
    ; Create Start Menu shortcuts
    createDirectory "$SMPROGRAMS\${APPNAME}"
    createShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\psio-assist.exe" "" "$INSTDIR\psio-assist.exe"
    createShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe"
    
    ; Create Desktop shortcut
    createShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\psio-assist.exe" "" "$INSTDIR\psio-assist.exe"
    
    ; Registry information for add/remove programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "InstallLocation" "$\"$INSTDIR$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$\"$INSTDIR\psio-assist.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${COMPANYNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "HelpLink" "${HELPURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLUpdateInfo" "${UPDATEURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLInfoAbout" "${ABOUTURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "VersionMajor" ${VERSIONMAJOR}
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "VersionMinor" ${VERSIONMINOR}
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "EstimatedSize" ${INSTALLSIZE}
sectionEnd

section "uninstall"
    delete "$INSTDIR\psio-assist.exe"
    delete "$INSTDIR\*.dll"
    delete "$INSTDIR\README.md"
    delete "$INSTDIR\LICENSE"
    delete "$INSTDIR\uninstall.exe"
    
    ; Remove Qt directories
    rmDir /r "$INSTDIR\platforms"
    rmDir /r "$INSTDIR\styles"
    rmDir /r "$INSTDIR\imageformats"
    rmDir /r "$INSTDIR\translations"
    rmDir "$INSTDIR"
    
    delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
    delete "$SMPROGRAMS\${APPNAME}\Uninstall.lnk"
    rmDir "$SMPROGRAMS\${APPNAME}"
    
    delete "$DESKTOP\${APPNAME}.lnk"
    
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
sectionEnd
