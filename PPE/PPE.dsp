# Microsoft Developer Studio Project File - Name="ProcessPartsEditor" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=ProcessPartsEditor - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ProcessPartsEditor.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ProcessPartsEditor.mak" CFG="ProcessPartsEditor - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ProcessPartsEditor - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "ProcessPartsEditor - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ProcessPartsEditor - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\Ent" /I "..\AlgFun" /I "..\EntList" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 f_ent.lib f_alg_fun.lib f_ent_list.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\Ent\Release" /libpath:"..\AlgFun\Release" /libpath:"..\EntList\Release"

!ELSEIF  "$(CFG)" == "ProcessPartsEditor - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\Ent" /I "..\AlgFun" /I "..\EntList" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x804 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 f_ent.lib f_alg_fun.lib f_ent_list.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\Ent\Release" /libpath:"..\AlgFun\Release" /libpath:"..\EntList\Release"

!ENDIF 

# Begin Target

# Name "ProcessPartsEditor - Win32 Release"
# Name "ProcessPartsEditor - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\Buffer.cpp
# End Source File
# Begin Source File

SOURCE=.\DxfFile.cpp
# End Source File
# Begin Source File

SOURCE=.\folder_dialog.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\NcSetDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\NewPartFileDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessPartsEditor.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessPartsEditor.rc
# End Source File
# Begin Source File

SOURCE=.\ProcessPartsEditorDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\ProcessPartsEditorView.cpp
# End Source File
# Begin Source File

SOURCE=.\SideBaffleHighDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Buffer.h
# End Source File
# Begin Source File

SOURCE=.\DxfFile.h
# End Source File
# Begin Source File

SOURCE=..\AlgFun\f_alg_fun.h
# End Source File
# Begin Source File

SOURCE=..\Ent\f_ent.h
# End Source File
# Begin Source File

SOURCE=..\EntList\f_ent_list.h
# End Source File
# Begin Source File

SOURCE=.\folder_dialog.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\NcSetDlg.h
# End Source File
# Begin Source File

SOURCE=.\NewPartFileDlg.h
# End Source File
# Begin Source File

SOURCE=.\ProcessPartsEditor.h
# End Source File
# Begin Source File

SOURCE=.\ProcessPartsEditorDoc.h
# End Source File
# Begin Source File

SOURCE=.\ProcessPartsEditorView.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SideBaffleHighDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\PlateNC.ico
# End Source File
# Begin Source File

SOURCE=.\res\PlateNCDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\ProcessPartsEditor.ico
# End Source File
# Begin Source File

SOURCE=.\res\ProcessPartsEditor.rc2
# End Source File
# Begin Source File

SOURCE=.\res\ProcessPartsEditorDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
