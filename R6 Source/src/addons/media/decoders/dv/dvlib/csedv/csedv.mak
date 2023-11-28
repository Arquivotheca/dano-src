# Microsoft Developer Studio Generated NMAKE File, Based on csedv.dsp
!IF "$(CFG)" == ""
CFG=csedv - Win32 Release
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの csedv - Win32 Release を設定します。
!ENDIF 

!IF "$(CFG)" != "csedv - Win32 Release" && "$(CFG)" != "csedv - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "csedv.mak" CFG="csedv - Win32 Release"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "csedv - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "csedv - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "csedv - Win32 Release"

OUTDIR=.\obj\Release
INTDIR=.\obj\Release
# Begin Custom Macros
OutDir=.\.\obj\Release
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\csedv.dll"

!ELSE 

ALL : "$(OUTDIR)\csedv.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\csvfconv.res"
	-@erase "$(INTDIR)\dct.obj"
	-@erase "$(INTDIR)\decdv.obj"
	-@erase "$(INTDIR)\encdv.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\table.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\csedv.dll"
	-@erase "$(OUTDIR)\csedv.pdb"
	-@erase ".\lib\csedv.exp"
	-@erase ".\lib\csedv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=xicl5.exe
CPP_PROJ=/nologo /MT /W3 /GX /Zi /O2 /I "..\common\inc" /D "WIN32" /D "NDEBUG"\
 /D "_WINDOWS" /Fp"$(INTDIR)\csedv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\"\
 /FD /GM /c 
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
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\csvfconv.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\csedv.bsc" 
BSC32_SBRS= \
	
LINK32=xilink5.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\
 /pdb:"$(OUTDIR)\csedv.pdb" /debug /machine:I386 /out:"$(OUTDIR)\csedv.dll"\
 /implib:".\lib/csedv.lib" 
LINK32_OBJS= \
	"$(INTDIR)\csvfconv.res" \
	"$(INTDIR)\dct.obj" \
	"$(INTDIR)\decdv.obj" \
	"$(INTDIR)\encdv.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\table.obj"

"$(OUTDIR)\csedv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "csedv - Win32 Debug"

OUTDIR=.\obj\Debug
INTDIR=.\obj\Debug
# Begin Custom Macros
OutDir=.\.\obj\Debug
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "$(OUTDIR)\csedv.dll"

!ELSE 

ALL : "$(OUTDIR)\csedv.dll"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\csvfconv.res"
	-@erase "$(INTDIR)\dct.obj"
	-@erase "$(INTDIR)\decdv.obj"
	-@erase "$(INTDIR)\encdv.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\table.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\csedv.dll"
	-@erase "$(OUTDIR)\csedv.ilk"
	-@erase "$(OUTDIR)\csedv.pdb"
	-@erase ".\lib\csedv.exp"
	-@erase ".\lib\csedv.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=xicl5.exe
CPP_PROJ=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\common\inc" /I\
 "..\..\..\app\pview95\common\inc" /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)\csedv.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GM /c 
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
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\csvfconv.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\csedv.bsc" 
BSC32_SBRS= \
	
LINK32=xilink5.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\
 /pdb:"$(OUTDIR)\csedv.pdb" /debug /machine:I386 /out:"$(OUTDIR)\csedv.dll"\
 /implib:".\lib/csedv.lib" 
LINK32_OBJS= \
	"$(INTDIR)\csvfconv.res" \
	"$(INTDIR)\dct.obj" \
	"$(INTDIR)\decdv.obj" \
	"$(INTDIR)\encdv.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\table.obj"

"$(OUTDIR)\csedv.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "csedv - Win32 Release" || "$(CFG)" == "csedv - Win32 Debug"
SOURCE=.\src\csvfconv.rc

!IF  "$(CFG)" == "csedv - Win32 Release"


"$(INTDIR)\csvfconv.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x411 /fo"$(INTDIR)\csvfconv.res" /i "src" /d "NDEBUG" $(SOURCE)


!ELSEIF  "$(CFG)" == "csedv - Win32 Debug"


"$(INTDIR)\csvfconv.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) /l 0x411 /fo"$(INTDIR)\csvfconv.res" /i "src" /d "_DEBUG" $(SOURCE)


!ENDIF 

SOURCE=.\src\dct.c

!IF  "$(CFG)" == "csedv - Win32 Release"

DEP_CPP_DCT_C=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	

"$(INTDIR)\dct.obj" : $(SOURCE) $(DEP_CPP_DCT_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "csedv - Win32 Debug"

DEP_CPP_DCT_C=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	

"$(INTDIR)\dct.obj" : $(SOURCE) $(DEP_CPP_DCT_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\decdv.c

!IF  "$(CFG)" == "csedv - Win32 Release"

DEP_CPP_DECDV=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	".\src\table.h"\
	

"$(INTDIR)\decdv.obj" : $(SOURCE) $(DEP_CPP_DECDV) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "csedv - Win32 Debug"

DEP_CPP_DECDV=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	".\src\table.h"\
	

"$(INTDIR)\decdv.obj" : $(SOURCE) $(DEP_CPP_DECDV) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\encdv.c

!IF  "$(CFG)" == "csedv - Win32 Release"

DEP_CPP_ENCDV=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	".\src\table.h"\
	
CPP_SWITCHES=/nologo /MT /W3 /GX /Zi /O2 /I "..\common\inc" /D "WIN32" /D\
 "NDEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\csedv.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /GM /c 

"$(INTDIR)\encdv.obj" : $(SOURCE) $(DEP_CPP_ENCDV) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "csedv - Win32 Debug"

DEP_CPP_ENCDV=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	".\src\table.h"\
	
CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\common\inc" /D "WIN32" /D\
 "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\csedv.pch" /YX /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /GM /c 

"$(INTDIR)\encdv.obj" : $(SOURCE) $(DEP_CPP_ENCDV) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\src\main.c

!IF  "$(CFG)" == "csedv - Win32 Release"

DEP_CPP_MAIN_=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	".\src\table.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "csedv - Win32 Debug"

DEP_CPP_MAIN_=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	

"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\src\table.c

!IF  "$(CFG)" == "csedv - Win32 Release"

DEP_CPP_TABLE=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	".\src\table.h"\
	

"$(INTDIR)\table.obj" : $(SOURCE) $(DEP_CPP_TABLE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "csedv - Win32 Debug"

DEP_CPP_TABLE=\
	"..\common\inc\csedv.h"\
	".\src\dv.h"\
	".\src\table.h"\
	

"$(INTDIR)\table.obj" : $(SOURCE) $(DEP_CPP_TABLE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

