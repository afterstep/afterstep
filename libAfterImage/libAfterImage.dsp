# Microsoft Developer Studio Project File - Name="libAfterImage" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libAfterImage - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libAfterImage.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libAfterImage.mak" CFG="libAfterImage - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libAfterImage - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libAfterImage - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libAfterImage - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libAfterImage - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "win32\Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libAfterImage - Win32 Release"
# Name "libAfterImage - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\afterbase.c
# End Source File
# Begin Source File

SOURCE=.\ascmap.c
# End Source File
# Begin Source File

SOURCE=.\asfont.c
# End Source File
# Begin Source File

SOURCE=.\asimage.c
# End Source File
# Begin Source File

SOURCE=.\asimagexml.c
# End Source File
# Begin Source File

SOURCE=.\asvisual.c
# End Source File
# Begin Source File

SOURCE=.\blender.c
# End Source File
# Begin Source File

SOURCE=.\bmp.c
# End Source File
# Begin Source File

SOURCE=.\char2uni.c
# End Source File
# Begin Source File

SOURCE=.\export.c
# End Source File
# Begin Source File

SOURCE=.\import.c
# End Source File
# Begin Source File

SOURCE=.\pixmap.c
# End Source File
# Begin Source File

SOURCE=.\transform.c
# End Source File
# Begin Source File

SOURCE=.\ungif.c
# End Source File
# Begin Source File

SOURCE=.\xcf.c
# End Source File
# Begin Source File

SOURCE=.\ximage.c
# End Source File
# Begin Source File

SOURCE=.\xpm.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\win32\afterbase.h
# End Source File
# Begin Source File

SOURCE=.\afterimage.h
# End Source File
# Begin Source File

SOURCE=.\ascmap.h
# End Source File
# Begin Source File

SOURCE=.\asfont.h
# End Source File
# Begin Source File

SOURCE=.\asim_afterbase.h
# End Source File
# Begin Source File

SOURCE=.\asimage.h
# End Source File
# Begin Source File

SOURCE=.\asimagexml.h
# End Source File
# Begin Source File

SOURCE=.\asvisual.h
# End Source File
# Begin Source File

SOURCE=.\blender.h
# End Source File
# Begin Source File

SOURCE=.\bmp.h
# End Source File
# Begin Source File

SOURCE=.\char2uni.h
# End Source File
# Begin Source File

SOURCE=.\win32\config.h
# End Source File
# Begin Source File

SOURCE=.\export.h
# End Source File
# Begin Source File

SOURCE=.\import.h
# End Source File
# Begin Source File

SOURCE=.\pixmap.h
# End Source File
# Begin Source File

SOURCE=.\transform.h
# End Source File
# Begin Source File

SOURCE=.\ungif.h
# End Source File
# Begin Source File

SOURCE=.\xcf.h
# End Source File
# Begin Source File

SOURCE=.\ximage.h
# End Source File
# Begin Source File

SOURCE=.\xpm.h
# End Source File
# Begin Source File

SOURCE=.\xwrap.h
# End Source File
# End Group
# End Target
# End Project
