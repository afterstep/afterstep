# Microsoft Developer Studio Generated NMAKE File, Based on libAfterImage.dsp
!IF "$(CFG)" == ""
CFG=libAfterImage - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libAfterImage - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libAfterImage - Win32 Release" && "$(CFG)" != "libAfterImage - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "libAfterImage - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : ".\libAfterImage.lib"


CLEAN :
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\afterbase.obj"
	-@erase "$(INTDIR)\ascmap.obj"
	-@erase "$(INTDIR)\asfont.obj"
	-@erase "$(INTDIR)\asimage.obj"
	-@erase "$(INTDIR)\asimagexml.obj"
	-@erase "$(INTDIR)\asvisual.obj"
	-@erase "$(INTDIR)\blender.obj"
	-@erase "$(INTDIR)\bmp.obj"
	-@erase "$(INTDIR)\char2uni.obj"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\export.obj"
	-@erase "$(INTDIR)\gzio.obj"
	-@erase "$(INTDIR)\import.obj"
	-@erase "$(INTDIR)\infback.obj"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\jcapimin.obj"
	-@erase "$(INTDIR)\jcapistd.obj"
	-@erase "$(INTDIR)\jccoefct.obj"
	-@erase "$(INTDIR)\jccolor.obj"
	-@erase "$(INTDIR)\jcdctmgr.obj"
	-@erase "$(INTDIR)\jchuff.obj"
	-@erase "$(INTDIR)\jcinit.obj"
	-@erase "$(INTDIR)\jcmainct.obj"
	-@erase "$(INTDIR)\jcmarker.obj"
	-@erase "$(INTDIR)\jcmaster.obj"
	-@erase "$(INTDIR)\jcomapi.obj"
	-@erase "$(INTDIR)\jcparam.obj"
	-@erase "$(INTDIR)\jcphuff.obj"
	-@erase "$(INTDIR)\jcprepct.obj"
	-@erase "$(INTDIR)\jcsample.obj"
	-@erase "$(INTDIR)\jctrans.obj"
	-@erase "$(INTDIR)\jdapimin.obj"
	-@erase "$(INTDIR)\jdapistd.obj"
	-@erase "$(INTDIR)\jdatadst.obj"
	-@erase "$(INTDIR)\jdatasrc.obj"
	-@erase "$(INTDIR)\jdcoefct.obj"
	-@erase "$(INTDIR)\jdcolor.obj"
	-@erase "$(INTDIR)\jddctmgr.obj"
	-@erase "$(INTDIR)\jdhuff.obj"
	-@erase "$(INTDIR)\jdinput.obj"
	-@erase "$(INTDIR)\jdmainct.obj"
	-@erase "$(INTDIR)\jdmarker.obj"
	-@erase "$(INTDIR)\jdmaster.obj"
	-@erase "$(INTDIR)\jdmerge.obj"
	-@erase "$(INTDIR)\jdphuff.obj"
	-@erase "$(INTDIR)\jdpostct.obj"
	-@erase "$(INTDIR)\jdsample.obj"
	-@erase "$(INTDIR)\jdtrans.obj"
	-@erase "$(INTDIR)\jerror.obj"
	-@erase "$(INTDIR)\jfdctflt.obj"
	-@erase "$(INTDIR)\jfdctfst.obj"
	-@erase "$(INTDIR)\jfdctint.obj"
	-@erase "$(INTDIR)\jidctflt.obj"
	-@erase "$(INTDIR)\jidctfst.obj"
	-@erase "$(INTDIR)\jidctint.obj"
	-@erase "$(INTDIR)\jidctred.obj"
	-@erase "$(INTDIR)\jmemmgr.obj"
	-@erase "$(INTDIR)\jmemnobs.obj"
	-@erase "$(INTDIR)\jquant1.obj"
	-@erase "$(INTDIR)\jquant2.obj"
	-@erase "$(INTDIR)\jutils.obj"
	-@erase "$(INTDIR)\pixmap.obj"
	-@erase "$(INTDIR)\png.obj"
	-@erase "$(INTDIR)\pngerror.obj"
	-@erase "$(INTDIR)\pnggccrd.obj"
	-@erase "$(INTDIR)\pngget.obj"
	-@erase "$(INTDIR)\pngmem.obj"
	-@erase "$(INTDIR)\pngpread.obj"
	-@erase "$(INTDIR)\pngread.obj"
	-@erase "$(INTDIR)\pngrio.obj"
	-@erase "$(INTDIR)\pngrtran.obj"
	-@erase "$(INTDIR)\pngrutil.obj"
	-@erase "$(INTDIR)\pngset.obj"
	-@erase "$(INTDIR)\pngtrans.obj"
	-@erase "$(INTDIR)\pngvcrd.obj"
	-@erase "$(INTDIR)\pngwio.obj"
	-@erase "$(INTDIR)\pngwrite.obj"
	-@erase "$(INTDIR)\pngwtran.obj"
	-@erase "$(INTDIR)\pngwutil.obj"
	-@erase "$(INTDIR)\transform.obj"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\uncompr.obj"
	-@erase "$(INTDIR)\ungif.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\xcf.obj"
	-@erase "$(INTDIR)\ximage.obj"
	-@erase "$(INTDIR)\xpm.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase ".\libAfterImage.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\libAfterImage.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libAfterImage.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"libAfterImage.lib" 
LIB32_OBJS= \
	"$(INTDIR)\png.obj" \
	"$(INTDIR)\pngerror.obj" \
	"$(INTDIR)\pnggccrd.obj" \
	"$(INTDIR)\pngget.obj" \
	"$(INTDIR)\pngmem.obj" \
	"$(INTDIR)\pngpread.obj" \
	"$(INTDIR)\pngread.obj" \
	"$(INTDIR)\pngrio.obj" \
	"$(INTDIR)\pngrtran.obj" \
	"$(INTDIR)\pngrutil.obj" \
	"$(INTDIR)\pngset.obj" \
	"$(INTDIR)\pngtrans.obj" \
	"$(INTDIR)\pngvcrd.obj" \
	"$(INTDIR)\pngwio.obj" \
	"$(INTDIR)\pngwrite.obj" \
	"$(INTDIR)\pngwtran.obj" \
	"$(INTDIR)\pngwutil.obj" \
	"$(INTDIR)\jcapimin.obj" \
	"$(INTDIR)\jcapistd.obj" \
	"$(INTDIR)\jccoefct.obj" \
	"$(INTDIR)\jccolor.obj" \
	"$(INTDIR)\jcdctmgr.obj" \
	"$(INTDIR)\jchuff.obj" \
	"$(INTDIR)\jcinit.obj" \
	"$(INTDIR)\jcmainct.obj" \
	"$(INTDIR)\jcmarker.obj" \
	"$(INTDIR)\jcmaster.obj" \
	"$(INTDIR)\jcomapi.obj" \
	"$(INTDIR)\jcparam.obj" \
	"$(INTDIR)\jcphuff.obj" \
	"$(INTDIR)\jcprepct.obj" \
	"$(INTDIR)\jcsample.obj" \
	"$(INTDIR)\jctrans.obj" \
	"$(INTDIR)\jdapimin.obj" \
	"$(INTDIR)\jdapistd.obj" \
	"$(INTDIR)\jdatadst.obj" \
	"$(INTDIR)\jdatasrc.obj" \
	"$(INTDIR)\jdcoefct.obj" \
	"$(INTDIR)\jdcolor.obj" \
	"$(INTDIR)\jddctmgr.obj" \
	"$(INTDIR)\jdhuff.obj" \
	"$(INTDIR)\jdinput.obj" \
	"$(INTDIR)\jdmainct.obj" \
	"$(INTDIR)\jdmarker.obj" \
	"$(INTDIR)\jdmaster.obj" \
	"$(INTDIR)\jdmerge.obj" \
	"$(INTDIR)\jdphuff.obj" \
	"$(INTDIR)\jdpostct.obj" \
	"$(INTDIR)\jdsample.obj" \
	"$(INTDIR)\jdtrans.obj" \
	"$(INTDIR)\jerror.obj" \
	"$(INTDIR)\jfdctflt.obj" \
	"$(INTDIR)\jfdctfst.obj" \
	"$(INTDIR)\jfdctint.obj" \
	"$(INTDIR)\jidctflt.obj" \
	"$(INTDIR)\jidctfst.obj" \
	"$(INTDIR)\jidctint.obj" \
	"$(INTDIR)\jidctred.obj" \
	"$(INTDIR)\jmemmgr.obj" \
	"$(INTDIR)\jmemnobs.obj" \
	"$(INTDIR)\jquant1.obj" \
	"$(INTDIR)\jquant2.obj" \
	"$(INTDIR)\jutils.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\gzio.obj" \
	"$(INTDIR)\infback.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\uncompr.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(INTDIR)\afterbase.obj" \
	"$(INTDIR)\ascmap.obj" \
	"$(INTDIR)\asfont.obj" \
	"$(INTDIR)\asimage.obj" \
	"$(INTDIR)\asimagexml.obj" \
	"$(INTDIR)\asvisual.obj" \
	"$(INTDIR)\blender.obj" \
	"$(INTDIR)\bmp.obj" \
	"$(INTDIR)\char2uni.obj" \
	"$(INTDIR)\export.obj" \
	"$(INTDIR)\import.obj" \
	"$(INTDIR)\pixmap.obj" \
	"$(INTDIR)\transform.obj" \
	"$(INTDIR)\ungif.obj" \
	"$(INTDIR)\xcf.obj" \
	"$(INTDIR)\ximage.obj" \
	"$(INTDIR)\xpm.obj"

".\libAfterImage.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libAfterImage - Win32 Debug"

OUTDIR=.
INTDIR=.\win32\Debug
# Begin Custom Macros
OutDir=.
# End Custom Macros

ALL : "$(OUTDIR)\libAfterImage.lib"


CLEAN :
	-@erase "$(INTDIR)\adler32.obj"
	-@erase "$(INTDIR)\afterbase.obj"
	-@erase "$(INTDIR)\ascmap.obj"
	-@erase "$(INTDIR)\asfont.obj"
	-@erase "$(INTDIR)\asimage.obj"
	-@erase "$(INTDIR)\asimagexml.obj"
	-@erase "$(INTDIR)\asvisual.obj"
	-@erase "$(INTDIR)\blender.obj"
	-@erase "$(INTDIR)\bmp.obj"
	-@erase "$(INTDIR)\char2uni.obj"
	-@erase "$(INTDIR)\compress.obj"
	-@erase "$(INTDIR)\crc32.obj"
	-@erase "$(INTDIR)\deflate.obj"
	-@erase "$(INTDIR)\export.obj"
	-@erase "$(INTDIR)\gzio.obj"
	-@erase "$(INTDIR)\import.obj"
	-@erase "$(INTDIR)\infback.obj"
	-@erase "$(INTDIR)\inffast.obj"
	-@erase "$(INTDIR)\inflate.obj"
	-@erase "$(INTDIR)\inftrees.obj"
	-@erase "$(INTDIR)\jcapimin.obj"
	-@erase "$(INTDIR)\jcapistd.obj"
	-@erase "$(INTDIR)\jccoefct.obj"
	-@erase "$(INTDIR)\jccolor.obj"
	-@erase "$(INTDIR)\jcdctmgr.obj"
	-@erase "$(INTDIR)\jchuff.obj"
	-@erase "$(INTDIR)\jcinit.obj"
	-@erase "$(INTDIR)\jcmainct.obj"
	-@erase "$(INTDIR)\jcmarker.obj"
	-@erase "$(INTDIR)\jcmaster.obj"
	-@erase "$(INTDIR)\jcomapi.obj"
	-@erase "$(INTDIR)\jcparam.obj"
	-@erase "$(INTDIR)\jcphuff.obj"
	-@erase "$(INTDIR)\jcprepct.obj"
	-@erase "$(INTDIR)\jcsample.obj"
	-@erase "$(INTDIR)\jctrans.obj"
	-@erase "$(INTDIR)\jdapimin.obj"
	-@erase "$(INTDIR)\jdapistd.obj"
	-@erase "$(INTDIR)\jdatadst.obj"
	-@erase "$(INTDIR)\jdatasrc.obj"
	-@erase "$(INTDIR)\jdcoefct.obj"
	-@erase "$(INTDIR)\jdcolor.obj"
	-@erase "$(INTDIR)\jddctmgr.obj"
	-@erase "$(INTDIR)\jdhuff.obj"
	-@erase "$(INTDIR)\jdinput.obj"
	-@erase "$(INTDIR)\jdmainct.obj"
	-@erase "$(INTDIR)\jdmarker.obj"
	-@erase "$(INTDIR)\jdmaster.obj"
	-@erase "$(INTDIR)\jdmerge.obj"
	-@erase "$(INTDIR)\jdphuff.obj"
	-@erase "$(INTDIR)\jdpostct.obj"
	-@erase "$(INTDIR)\jdsample.obj"
	-@erase "$(INTDIR)\jdtrans.obj"
	-@erase "$(INTDIR)\jerror.obj"
	-@erase "$(INTDIR)\jfdctflt.obj"
	-@erase "$(INTDIR)\jfdctfst.obj"
	-@erase "$(INTDIR)\jfdctint.obj"
	-@erase "$(INTDIR)\jidctflt.obj"
	-@erase "$(INTDIR)\jidctfst.obj"
	-@erase "$(INTDIR)\jidctint.obj"
	-@erase "$(INTDIR)\jidctred.obj"
	-@erase "$(INTDIR)\jmemmgr.obj"
	-@erase "$(INTDIR)\jmemnobs.obj"
	-@erase "$(INTDIR)\jquant1.obj"
	-@erase "$(INTDIR)\jquant2.obj"
	-@erase "$(INTDIR)\jutils.obj"
	-@erase "$(INTDIR)\pixmap.obj"
	-@erase "$(INTDIR)\png.obj"
	-@erase "$(INTDIR)\pngerror.obj"
	-@erase "$(INTDIR)\pnggccrd.obj"
	-@erase "$(INTDIR)\pngget.obj"
	-@erase "$(INTDIR)\pngmem.obj"
	-@erase "$(INTDIR)\pngpread.obj"
	-@erase "$(INTDIR)\pngread.obj"
	-@erase "$(INTDIR)\pngrio.obj"
	-@erase "$(INTDIR)\pngrtran.obj"
	-@erase "$(INTDIR)\pngrutil.obj"
	-@erase "$(INTDIR)\pngset.obj"
	-@erase "$(INTDIR)\pngtrans.obj"
	-@erase "$(INTDIR)\pngvcrd.obj"
	-@erase "$(INTDIR)\pngwio.obj"
	-@erase "$(INTDIR)\pngwrite.obj"
	-@erase "$(INTDIR)\pngwtran.obj"
	-@erase "$(INTDIR)\pngwutil.obj"
	-@erase "$(INTDIR)\transform.obj"
	-@erase "$(INTDIR)\trees.obj"
	-@erase "$(INTDIR)\uncompr.obj"
	-@erase "$(INTDIR)\ungif.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\xcf.obj"
	-@erase "$(INTDIR)\ximage.obj"
	-@erase "$(INTDIR)\xpm.obj"
	-@erase "$(INTDIR)\zutil.obj"
	-@erase "$(OUTDIR)\libAfterImage.lib"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libAfterImage.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libAfterImage.lib" 
LIB32_OBJS= \
	"$(INTDIR)\png.obj" \
	"$(INTDIR)\pngerror.obj" \
	"$(INTDIR)\pnggccrd.obj" \
	"$(INTDIR)\pngget.obj" \
	"$(INTDIR)\pngmem.obj" \
	"$(INTDIR)\pngpread.obj" \
	"$(INTDIR)\pngread.obj" \
	"$(INTDIR)\pngrio.obj" \
	"$(INTDIR)\pngrtran.obj" \
	"$(INTDIR)\pngrutil.obj" \
	"$(INTDIR)\pngset.obj" \
	"$(INTDIR)\pngtrans.obj" \
	"$(INTDIR)\pngvcrd.obj" \
	"$(INTDIR)\pngwio.obj" \
	"$(INTDIR)\pngwrite.obj" \
	"$(INTDIR)\pngwtran.obj" \
	"$(INTDIR)\pngwutil.obj" \
	"$(INTDIR)\jcapimin.obj" \
	"$(INTDIR)\jcapistd.obj" \
	"$(INTDIR)\jccoefct.obj" \
	"$(INTDIR)\jccolor.obj" \
	"$(INTDIR)\jcdctmgr.obj" \
	"$(INTDIR)\jchuff.obj" \
	"$(INTDIR)\jcinit.obj" \
	"$(INTDIR)\jcmainct.obj" \
	"$(INTDIR)\jcmarker.obj" \
	"$(INTDIR)\jcmaster.obj" \
	"$(INTDIR)\jcomapi.obj" \
	"$(INTDIR)\jcparam.obj" \
	"$(INTDIR)\jcphuff.obj" \
	"$(INTDIR)\jcprepct.obj" \
	"$(INTDIR)\jcsample.obj" \
	"$(INTDIR)\jctrans.obj" \
	"$(INTDIR)\jdapimin.obj" \
	"$(INTDIR)\jdapistd.obj" \
	"$(INTDIR)\jdatadst.obj" \
	"$(INTDIR)\jdatasrc.obj" \
	"$(INTDIR)\jdcoefct.obj" \
	"$(INTDIR)\jdcolor.obj" \
	"$(INTDIR)\jddctmgr.obj" \
	"$(INTDIR)\jdhuff.obj" \
	"$(INTDIR)\jdinput.obj" \
	"$(INTDIR)\jdmainct.obj" \
	"$(INTDIR)\jdmarker.obj" \
	"$(INTDIR)\jdmaster.obj" \
	"$(INTDIR)\jdmerge.obj" \
	"$(INTDIR)\jdphuff.obj" \
	"$(INTDIR)\jdpostct.obj" \
	"$(INTDIR)\jdsample.obj" \
	"$(INTDIR)\jdtrans.obj" \
	"$(INTDIR)\jerror.obj" \
	"$(INTDIR)\jfdctflt.obj" \
	"$(INTDIR)\jfdctfst.obj" \
	"$(INTDIR)\jfdctint.obj" \
	"$(INTDIR)\jidctflt.obj" \
	"$(INTDIR)\jidctfst.obj" \
	"$(INTDIR)\jidctint.obj" \
	"$(INTDIR)\jidctred.obj" \
	"$(INTDIR)\jmemmgr.obj" \
	"$(INTDIR)\jmemnobs.obj" \
	"$(INTDIR)\jquant1.obj" \
	"$(INTDIR)\jquant2.obj" \
	"$(INTDIR)\jutils.obj" \
	"$(INTDIR)\adler32.obj" \
	"$(INTDIR)\compress.obj" \
	"$(INTDIR)\crc32.obj" \
	"$(INTDIR)\deflate.obj" \
	"$(INTDIR)\gzio.obj" \
	"$(INTDIR)\infback.obj" \
	"$(INTDIR)\inffast.obj" \
	"$(INTDIR)\inflate.obj" \
	"$(INTDIR)\inftrees.obj" \
	"$(INTDIR)\trees.obj" \
	"$(INTDIR)\uncompr.obj" \
	"$(INTDIR)\zutil.obj" \
	"$(INTDIR)\afterbase.obj" \
	"$(INTDIR)\ascmap.obj" \
	"$(INTDIR)\asfont.obj" \
	"$(INTDIR)\asimage.obj" \
	"$(INTDIR)\asimagexml.obj" \
	"$(INTDIR)\asvisual.obj" \
	"$(INTDIR)\blender.obj" \
	"$(INTDIR)\bmp.obj" \
	"$(INTDIR)\char2uni.obj" \
	"$(INTDIR)\export.obj" \
	"$(INTDIR)\import.obj" \
	"$(INTDIR)\pixmap.obj" \
	"$(INTDIR)\transform.obj" \
	"$(INTDIR)\ungif.obj" \
	"$(INTDIR)\xcf.obj" \
	"$(INTDIR)\ximage.obj" \
	"$(INTDIR)\xpm.obj"

"$(OUTDIR)\libAfterImage.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libAfterImage.dep")
!INCLUDE "libAfterImage.dep"
!ELSE 
!MESSAGE Warning: cannot find "libAfterImage.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libAfterImage - Win32 Release" || "$(CFG)" == "libAfterImage - Win32 Debug"
SOURCE=.\libpng\png.c

"$(INTDIR)\png.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngerror.c

"$(INTDIR)\pngerror.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pnggccrd.c

"$(INTDIR)\pnggccrd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngget.c

"$(INTDIR)\pngget.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngmem.c

"$(INTDIR)\pngmem.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngpread.c

"$(INTDIR)\pngpread.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngread.c

"$(INTDIR)\pngread.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngrio.c

"$(INTDIR)\pngrio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngrtran.c

"$(INTDIR)\pngrtran.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngrutil.c

"$(INTDIR)\pngrutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngset.c

"$(INTDIR)\pngset.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngtrans.c

"$(INTDIR)\pngtrans.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngvcrd.c

"$(INTDIR)\pngvcrd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngwio.c

"$(INTDIR)\pngwio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngwrite.c

"$(INTDIR)\pngwrite.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngwtran.c

"$(INTDIR)\pngwtran.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libpng\pngwutil.c

"$(INTDIR)\pngwutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcapimin.c

"$(INTDIR)\jcapimin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcapistd.c

"$(INTDIR)\jcapistd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jccoefct.c

"$(INTDIR)\jccoefct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jccolor.c

"$(INTDIR)\jccolor.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcdctmgr.c

"$(INTDIR)\jcdctmgr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jchuff.c

"$(INTDIR)\jchuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcinit.c

"$(INTDIR)\jcinit.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcmainct.c

"$(INTDIR)\jcmainct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcmarker.c

"$(INTDIR)\jcmarker.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcmaster.c

"$(INTDIR)\jcmaster.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcomapi.c

"$(INTDIR)\jcomapi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcparam.c

"$(INTDIR)\jcparam.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcphuff.c

"$(INTDIR)\jcphuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcprepct.c

"$(INTDIR)\jcprepct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jcsample.c

"$(INTDIR)\jcsample.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jctrans.c

"$(INTDIR)\jctrans.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdapimin.c

"$(INTDIR)\jdapimin.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdapistd.c

"$(INTDIR)\jdapistd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdatadst.c

"$(INTDIR)\jdatadst.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdatasrc.c

"$(INTDIR)\jdatasrc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdcoefct.c

"$(INTDIR)\jdcoefct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdcolor.c

"$(INTDIR)\jdcolor.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jddctmgr.c

"$(INTDIR)\jddctmgr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdhuff.c

"$(INTDIR)\jdhuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdinput.c

"$(INTDIR)\jdinput.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdmainct.c

"$(INTDIR)\jdmainct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdmarker.c

"$(INTDIR)\jdmarker.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdmaster.c

"$(INTDIR)\jdmaster.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdmerge.c

"$(INTDIR)\jdmerge.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdphuff.c

"$(INTDIR)\jdphuff.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdpostct.c

"$(INTDIR)\jdpostct.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdsample.c

"$(INTDIR)\jdsample.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jdtrans.c

"$(INTDIR)\jdtrans.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jerror.c

"$(INTDIR)\jerror.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jfdctflt.c

"$(INTDIR)\jfdctflt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jfdctfst.c

"$(INTDIR)\jfdctfst.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jfdctint.c

"$(INTDIR)\jfdctint.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jidctflt.c

"$(INTDIR)\jidctflt.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jidctfst.c

"$(INTDIR)\jidctfst.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jidctint.c

"$(INTDIR)\jidctint.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jidctred.c

"$(INTDIR)\jidctred.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jmemmgr.c

"$(INTDIR)\jmemmgr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jmemnobs.c

"$(INTDIR)\jmemnobs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jquant1.c

"$(INTDIR)\jquant1.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jquant2.c

"$(INTDIR)\jquant2.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\libjpeg\jutils.c

"$(INTDIR)\jutils.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\adler32.c

"$(INTDIR)\adler32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\compress.c

"$(INTDIR)\compress.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\crc32.c

"$(INTDIR)\crc32.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\deflate.c

"$(INTDIR)\deflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\gzio.c

"$(INTDIR)\gzio.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\infback.c

"$(INTDIR)\infback.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\inffast.c

"$(INTDIR)\inffast.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\inflate.c

"$(INTDIR)\inflate.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\inftrees.c

"$(INTDIR)\inftrees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\trees.c

"$(INTDIR)\trees.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\uncompr.c

"$(INTDIR)\uncompr.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\zlib\zutil.c

"$(INTDIR)\zutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\afterbase.c

"$(INTDIR)\afterbase.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ascmap.c

"$(INTDIR)\ascmap.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\asfont.c

"$(INTDIR)\asfont.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\asimage.c

"$(INTDIR)\asimage.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\asimagexml.c

"$(INTDIR)\asimagexml.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\asvisual.c

"$(INTDIR)\asvisual.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\blender.c

"$(INTDIR)\blender.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\bmp.c

"$(INTDIR)\bmp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\char2uni.c

"$(INTDIR)\char2uni.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\export.c

"$(INTDIR)\export.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\import.c

"$(INTDIR)\import.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\pixmap.c

"$(INTDIR)\pixmap.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\transform.c

"$(INTDIR)\transform.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ungif.c

"$(INTDIR)\ungif.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xcf.c

"$(INTDIR)\xcf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ximage.c

"$(INTDIR)\ximage.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xpm.c

"$(INTDIR)\xpm.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

