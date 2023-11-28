# Microsoft Developer Studio Generated NMAKE File, Based on csccdvc.dsp
!IF "$(CFG)" == ""
CFG=csccdvc - Win32 Release
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの csccdvc - Win32 Release を設定します。
!ENDIF 

!IF "$(CFG)" != "csccdvc - Win32 Release" && "$(CFG)" !=\
 "csccdvc - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "csccdvc.mak" CFG="csccdvc - Win32 Release"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "csccdvc - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "csccdvc - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "csccdvc - Win32 Release"

OUTDIR=.\obj\Release
INTDIR=.\obj\Release
# Begin Custom Macros
OutDir=.\.\obj\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\csccdvc.dll"

!ELSE 

ALL : "$(OUTDIR)\csccdvc.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cputype.obj"
	-@erase "$(INTDIR)\csccdvc.res"
	-@erase "$(INTDIR)\d1RGBQ.obj"
	-@erase "$(INTDIR)\d1RGBT.obj"
	-@erase "$(INTDIR)\d1YUY2.obj"
	-@erase "$(INTDIR)\decoder.obj"
	-@erase "$(INTDIR)\drvproc.obj"
	-@erase "$(INTDIR)\e1RGBQ.obj"
	-@erase "$(INTDIR)\e1RGBT.obj"
	-@erase "$(INTDIR)\e1YUY2.obj"
	-@erase "$(INTDIR)\encoder.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\csccdvc.dll"
	-@erase "$(OUTDIR)\csccdvc.exp"
	-@erase "$(OUTDIR)\csccdvc.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=xicl5.exe
CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".\res" /I "..\common\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\csccdvc.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /GM /c 
CPP_OBJS=.\obj\Release/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\csccdvc.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\csccdvc.bsc" 
BSC32_SBRS= \
	
LINK32=xilink5.exe
LINK32_FLAGS=..\csedv\lib\csedv.lib version.lib winmm.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo\
 /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\csccdvc.pdb"\
 /machine:I386 /def:".\res\csccdvc.def" /out:"$(OUTDIR)\csccdvc.dll"\
 /implib:"$(OUTDIR)\csccdvc.lib" 
DEF_FILE= \
	".\res\csccdvc.def"
LINK32_OBJS= \
	"$(INTDIR)\buffcopy.obj" \
	"$(INTDIR)\cputype.obj" \
	"$(INTDIR)\csccdvc.res" \
	"$(INTDIR)\d1RGBQ.obj" \
	"$(INTDIR)\d1RGBT.obj" \
	"$(INTDIR)\d1YUY2.obj" \
	"$(INTDIR)\decoder.obj" \
	"$(INTDIR)\drvproc.obj" \
	"$(INTDIR)\e1RGBQ.obj" \
	"$(INTDIR)\e1RGBT.obj" \
	"$(INTDIR)\e1YUY2.obj" \
	"$(INTDIR)\encoder.obj"

"$(OUTDIR)\csccdvc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "csccdvc - Win32 Debug"

OUTDIR=.\obj\Debug
INTDIR=.\obj\Debug
# Begin Custom Macros
OutDir=.\.\obj\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\csccdvc.dll"

!ELSE 

ALL : "$(OUTDIR)\csccdvc.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\cputype.obj"
	-@erase "$(INTDIR)\csccdvc.res"
	-@erase "$(INTDIR)\d1RGBQ.obj"
	-@erase "$(INTDIR)\d1RGBT.obj"
	-@erase "$(INTDIR)\d1YUY2.obj"
	-@erase "$(INTDIR)\decoder.obj"
	-@erase "$(INTDIR)\drvproc.obj"
	-@erase "$(INTDIR)\e1RGBQ.obj"
	-@erase "$(INTDIR)\e1RGBT.obj"
	-@erase "$(INTDIR)\e1YUY2.obj"
	-@erase "$(INTDIR)\encoder.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\csccdvc.dll"
	-@erase "$(OUTDIR)\csccdvc.exp"
	-@erase "$(OUTDIR)\csccdvc.ilk"
	-@erase "$(OUTDIR)\csccdvc.lib"
	-@erase "$(OUTDIR)\csccdvc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=xicl5.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I ".\res" /I "..\common\inc" /D\
 "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\csccdvc.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 
CPP_OBJS=.\obj\Debug/
CPP_SBRS=.

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /win32 
RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\csccdvc.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\csccdvc.bsc" 
BSC32_SBRS= \
	
LINK32=xilink5.exe
LINK32_FLAGS=..\csedv\lib\csedv.lib version.lib winmm.lib kernel32.lib\
 user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib\
 ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo\
 /subsystem:windows /dll /incremental:yes /pdb:"$(OUTDIR)\csccdvc.pdb" /debug\
 /machine:I386 /def:".\res\csccdvc.def" /out:"$(OUTDIR)\csccdvc.dll"\
 /implib:"$(OUTDIR)\csccdvc.lib" 
DEF_FILE= \
	".\res\csccdvc.def"
LINK32_OBJS= \
	"$(INTDIR)\buffcopy.obj" \
	"$(INTDIR)\cputype.obj" \
	"$(INTDIR)\csccdvc.res" \
	"$(INTDIR)\d1RGBQ.obj" \
	"$(INTDIR)\d1RGBT.obj" \
	"$(INTDIR)\d1YUY2.obj" \
	"$(INTDIR)\decoder.obj" \
	"$(INTDIR)\drvproc.obj" \
	"$(INTDIR)\e1RGBQ.obj" \
	"$(INTDIR)\e1RGBT.obj" \
	"$(INTDIR)\e1YUY2.obj" \
	"$(INTDIR)\encoder.obj"

"$(OUTDIR)\csccdvc.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "csccdvc - Win32 Release" || "$(CFG)" ==\
 "csccdvc - Win32 Debug"
SOURCE=..\common\src\cputype.c
DEP_CPP_CPUTY=\
	"..\common\inc\cputype.h"\
	

"$(INTDIR)\cputype.obj" : $(SOURCE) $(DEP_CPP_CPUTY) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\common\src\d1RGBQ.c
DEP_CPP_D1RGB=\
	"..\common\inc\csedv.h"\
	"..\common\inc\putimage.h"\
	"..\common\inc\yuvtorgb.h"\
	

"$(INTDIR)\d1RGBQ.obj" : $(SOURCE) $(DEP_CPP_D1RGB) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\common\src\d1RGBT.c
DEP_CPP_D1RGBT=\
	"..\common\inc\csedv.h"\
	"..\common\inc\putimage.h"\
	"..\common\inc\yuvtorgb.h"\
	

"$(INTDIR)\d1RGBT.obj" : $(SOURCE) $(DEP_CPP_D1RGBT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\common\src\d1YUY2.c
DEP_CPP_D1YUY=\
	"..\common\inc\csedv.h"\
	

"$(INTDIR)\d1YUY2.obj" : $(SOURCE) $(DEP_CPP_D1YUY) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\decoder.c

!IF  "$(CFG)" == "csccdvc - Win32 Release"

DEP_CPP_DECOD=\
	"..\common\inc\csedv.h"\
	"..\common\inc\putimage.h"\
	".\src\codec.h"\
	

"$(INTDIR)\decoder.obj" : $(SOURCE) $(DEP_CPP_DECOD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "csccdvc - Win32 Debug"

DEP_CPP_DECOD=\
	"..\common\inc\csedv.h"\
	"..\common\inc\putimage.h"\
	".\src\codec.h"\
	

"$(INTDIR)\decoder.obj" : $(SOURCE) $(DEP_CPP_DECOD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\drvproc.c

!IF  "$(CFG)" == "csccdvc - Win32 Release"

DEP_CPP_DRVPR=\
	"..\common\inc\cputype.h"\
	"..\common\inc\csedv.h"\
	".\src\codec.h"\
	

"$(INTDIR)\drvproc.obj" : $(SOURCE) $(DEP_CPP_DRVPR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "csccdvc - Win32 Debug"

DEP_CPP_DRVPR=\
	"..\common\inc\cputype.h"\
	"..\common\inc\csedv.h"\
	".\src\codec.h"\
	

"$(INTDIR)\drvproc.obj" : $(SOURCE) $(DEP_CPP_DRVPR) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\common\src\e1RGBQ.c
DEP_CPP_E1RGB=\
	"..\common\inc\csedv.h"\
	"..\common\inc\rgbtoyuv.h"\
	

"$(INTDIR)\e1RGBQ.obj" : $(SOURCE) $(DEP_CPP_E1RGB) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\common\src\e1RGBT.c
DEP_CPP_E1RGBT=\
	"..\common\inc\csedv.h"\
	"..\common\inc\rgbtoyuv.h"\
	

"$(INTDIR)\e1RGBT.obj" : $(SOURCE) $(DEP_CPP_E1RGBT) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\common\src\e1YUY2.c
DEP_CPP_E1YUY=\
	"..\common\inc\csedv.h"\
	

"$(INTDIR)\e1YUY2.obj" : $(SOURCE) $(DEP_CPP_E1YUY) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\src\encoder.c

!IF  "$(CFG)" == "csccdvc - Win32 Release"

DEP_CPP_ENCOD=\
	"..\common\inc\csedv.h"\
	"..\common\inc\getimage.h"\
	".\src\codec.h"\
	

"$(INTDIR)\encoder.obj" : $(SOURCE) $(DEP_CPP_ENCOD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "csccdvc - Win32 Debug"

DEP_CPP_ENCOD=\
	"..\common\inc\csedv.h"\
	"..\common\inc\getimage.h"\
	".\src\codec.h"\
	

"$(INTDIR)\encoder.obj" : $(SOURCE) $(DEP_CPP_ENCOD) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\res\csccdvc.rc

!IF  "$(CFG)" == "csccdvc - Win32 Release"


"$(INTDIR)\csccdvc.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x411 /fo"$(INTDIR)\csccdvc.res" /i "res" /i ".\res" /d "NDEBUG"\
 $(SOURCE)


!ELSEIF  "$(CFG)" == "csccdvc - Win32 Debug"


"$(INTDIR)\csccdvc.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x411 /fo"$(INTDIR)\csccdvc.res" /i "res" /i ".\res" /d "_DEBUG"\
 $(SOURCE)


!ENDIF 

SOURCE=.\src\buffcopy.asm

!IF  "$(CFG)" == "csccdvc - Win32 Release"

TargetDir=.\obj\Release
InputPath=.\src\buffcopy.asm
InputName=buffcopy

"$(TargetDir)\$(InputName).obj"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	e:\tools\masm611\bin\ml -coff -c -Fo$(TargetDir)\$(InputName).obj $(InputPath)

!ELSEIF  "$(CFG)" == "csccdvc - Win32 Debug"

TargetDir=.\obj\Debug
InputPath=.\src\buffcopy.asm
InputName=buffcopy

"$(TargetDir)\$(InputName).obj"	 : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	e:\tools\masm611\bin\ml -coff -c -Fo$(TargetDir)\$(InputName).obj $(InputPath)

!ENDIF 


!ENDIF 

