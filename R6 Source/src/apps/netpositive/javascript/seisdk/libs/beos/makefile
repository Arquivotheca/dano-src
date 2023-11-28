ifeq ($(EVERYTHING),1)
SRC = everything.c
else
SRC =  ../../../srccore/atexit.c ../../../srccore/brktest.c ../../../srccore/call.c ../../../srccore/code.c ../../../srccore/codeprt2.c ../../../srccore/define.c ../../../srccore/expressn.c ../../../srccore/extlib.c ../../../srccore/function.c ../../../srccore/interprt.c ../../../srccore/jsedll.c ../../../srccore/jseengin.c ../../../srccore/jselib.c ../../../srccore/library.c ../../../srccore/loclfunc.c ../../../srccore/mempool.c ../../../srccore/operator.c ../../../srccore/secode.c ../../../srccore/security.c ../../../srccore/source.c ../../../srccore/stack.c ../../../srccore/statemnt.c ../../../srccore/textcore.c ../../../srccore/token.c ../../../srccore/util.c ../../../srccore/var.c ../../../srccore/varread.c ../../../srccore/varutil.c ../../../srccore/varwrite.c ../../../srcmisc/ctype.c ../../../srcmisc/dbgprntf.c ../../../srcmisc/dirparts.c ../../../srcmisc/dlfcn.c ../../../srcmisc/globldat.c ../../../srcmisc/inet.c ../../../srcmisc/jsemem.c ../../../srcmisc/macfunc.c ../../../srcmisc/unixfunc.c ../../../srcmisc/utilhuge.c ../../../srcmisc/utilstr.c
endif

OBJ =  atexit.o brktest.o call.o code.o codeprt2.o define.o expressn.o extlib.o function.o interprt.o jsedll.o jseengin.o jselib.o library.o loclfunc.o mempool.o operator.o secode.o security.o source.o stack.o statemnt.o textcore.o token.o util.o var.o varread.o varutil.o varwrite.o ctype.o dbgprntf.o dirparts.o dlfcn.o globldat.o inet.o jsemem.o macfunc.o unixfunc.o utilhuge.o utilstr.o

ROOT = /usr/src/se403
#CXX             = g++
#CC              = gcc
#LINKER          = g++
#EXT_LINKER      = g++
# seb 98.11.12
C_FLAGS         = -D__JSE_BEOS__ -D__JSE_UNIX__
LINK_FLAGS      = 
# seb 98.11.12
EXT_LINK_FLAGS  = -nostart
DL_FLAGS        = -ldl
EXT_CFLAGS      = -fpic -DSTDC_HEADERS
CGI_BIN         = /home/httpd/cgi-bin
HTML_PATH       = /home/httpd/html
INSTALL_DIR     = /usr/local/bin
# seb 98.11.12
SYSTEM          = beos
DEFFILE         = unix/deflin.doc


# ScriptEase Core makefile

# seb 98.11.12 Removed optimization information from here
CFLAGS   = $(C_FLAGS) \
           -DJSE_FLOATING_POINT -DNDEBUG -D__JSE_LIB__ -DJSETOOLKIT_CORE \
           -I../../../seclib -I../../../seclib/pro -I../../../seclib/unix \
           -I../../../srccore -I../../../srcmisc -I../../../incjse -I../../../srclib/ecma -I../../../srcdbg
CXXFLAGS = $(CFLAGS)
VPATH    = ../../../srccore:../../../srcmisc:../../../srcdbg

# seb 98.11.12 NetPositive additions
CFLAGS	+= -DJSE_C_EXTENSIONS=0 -DJSE_INCLUDE=0 -DJSE_CONDITIONAL_COMPILE=0 \
		   -DJSE_CREATEFUNCTIONTEXTVARIABLE=0 -DJSE_FAST_MEMPOOL=1 -DJSETOOLKIT_APP_INCL_ECMABUILTINOBJECTS \
		   -DJSETOOLKIT_APP_INCL_ECMADATE -DJSETOOLKIT_APP_INCL_ECMABUFFER -DJSETOOLKIT_APP_INCL_ECMAMATH \
		   -DJSE_BROWSEROBJECTS -DJSE_ECMA_ALL -DJSE_ECMA_ARRAY=1 -DJSE_ECMA_BOOLEAN=1 -DJSE_ECMA_BUFFER=1 \
		   -DJSE_ECMA_DATE=1 -DJSE_ECMA_ESCAPE=1 -DJSE_ECMA_EVAL=1 -DJSE_ECMA_FUNCTION=1 -DJSE_ECMA_ISFINITE=1 \
		   -DJSE_ECMA_ISNAN=1 -DJSE_ECMA_MATH=1 -DJSE_ECMA_NUMBER=1 -DJSE_ECMA_OBJECT=1 -DJSE_ECMA_PARSEINT=1 \
		   -DJSE_ECMA_PARSEFLOAT=1 -DJSE_ECMA_STRING=1 -DJSE_ECMA_UNESCAPE=1 -DJSE_ECMA_ANY \
		   -I../../../srclib
ifeq ($(EVERYTHING),1)
else
SRC		+= ../../../srclib/ecma/ecmamisc.c ../../../srclib/ecma/mathobj.c \
           ../../../srclib/ecma/sebuffer.c ../../../srclib/ecma/sedate.c ../../../srclib/ecma/seecma.c \
           ../../../srclib/ecma/seobject.c ../../../srcmisc/seobjfun.c ../../../srclib/common/seliball.c
endif
OBJ		+= ecmamisc.o mathobj.o sebuffer.o sedate.o seecma.o seobject.o seobjfun.o seliball.o
VPATH	+= :../../../brwsrlib:../../../srclib/ecma:../../../srclib/common/

ifeq ($(DEBUG_MENU), 1)
CFLAGS	+= -O0 -gdwarf-2
else
ifeq ($(COMPILER), gcc)
	CFLAGS		+= -fomit-frame-pointer -Os
else
	CFLAGS		+= -O3
endif
endif


# seb 98.11.12 -- Not needed for NetPositive
#all: libserte40.a libserte40.so
all: libserte40.a

setup:  clean
	echo "SRC = " ../../../srccore/*.c ../../../srcmisc/*.c > tmp
	echo "" > /tmp/tmp2
	cat tmp | sed -e s,SRC,OBJ,1 -e s,\\.c,\\.o,g -e s,\\.\\./\\.\\./\\.\\./[a-z]*/,,g >> /tmp/tmp2
	echo "" >> /tmp/tmp2
	cat tmp /tmp/tmp2 makefile.bas > makefile.new
	cp makefile.new makefile
	rm -f tmp /tmp/tmp2

depend:
	cp makefile.new makefile
	echo "#Depend information automatically generated" >> makefile
	g++ $(CXXFLAGS) -c -E -MM $(SRC) >> makefile

clean:
# seb 98.11.12 - Removed makefile.new from clean target
	rm -f *.o *.a *.so tmp tmp2

libserte40.a: $(OBJ)
	rm -f libserte40.a
	ar rc libserte40.a $(OBJ)
ifneq ( $(SYSTEM),irix )
	ranlib libserte40.a
endif

# seb 98.11.12 -- Not needed for NetPositive
#libserte40.so: $(OBJ)
#	$(EXT_LINKER) $(EXT_LINK_FLAGS) $(OBJ) -o libserte40.so

#install: libserte40.so libserte40.a
#	cp libserte40.so /lib/libserte.so.4.0
#	rm /lib/libserte.so
#	ln -s /lib/libserte.so.4.0 /lib/libserte.so
#	ln -s /lib/libserte.so.4.0 /lib/libserte.so.4
#	cp libserte40.a /usr/lib/libserte.a.4.0
#	rm /usr/lib/libserte.a
#	ln -s /usr/lib/libserte.a.4.0 /usr/lib/libserte.a
#	ln -s /usr/lib/libserte.a.4.0 /usr/lib/libserte.a.4
#Depend information automatically generated
atexit.o: ../../../srccore/atexit.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
brktest.o: ../../../srccore/brktest.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
call.o: ../../../srccore/call.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
code.o: ../../../srccore/code.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
codeprt2.o: ../../../srccore/codeprt2.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
define.o: ../../../srccore/define.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
expressn.o: ../../../srccore/expressn.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
extlib.o: ../../../srccore/extlib.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
function.o: ../../../srccore/function.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
interprt.o: ../../../srccore/interprt.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
jsedll.o: ../../../srccore/jsedll.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
jseengin.o: ../../../srccore/jseengin.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
jselib.o: ../../../srccore/jselib.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
library.o: ../../../srccore/library.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
loclfunc.o: ../../../srccore/loclfunc.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
mempool.o: ../../../srccore/mempool.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
operator.o: ../../../srccore/operator.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
secode.o: ../../../srccore/secode.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
security.o: ../../../srccore/security.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
source.o: ../../../srccore/source.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
stack.o: ../../../srccore/stack.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
statemnt.o: ../../../srccore/statemnt.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
textcore.o: ../../../srccore/textcore.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
token.o: ../../../srccore/token.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
util.o: ../../../srccore/util.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
var.o: ../../../srccore/var.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
varread.o: ../../../srccore/varread.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
varutil.o: ../../../srccore/varutil.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
varwrite.o: ../../../srccore/varwrite.c ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srccore/jseopt.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
ctype.o: ../../../srcmisc/ctype.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
dbgprntf.o: ../../../srcmisc/dbgprntf.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h ../../../srcmisc/dbgprntf.h
dirparts.o: ../../../srcmisc/dirparts.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
dlfcn.o: ../../../srcmisc/dlfcn.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
globldat.o: ../../../srcmisc/globldat.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
inet.o: ../../../srcmisc/inet.c ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/jsemem.h ../../../srcmisc/dbgprntf.h \
 ../../../srcmisc/unixfunc.h ../../../srcmisc/sesyshdr.h \
 ../../../srcmisc/aixdlfcn.h ../../../srcmisc/inet.h \
 ../../../srccore/jseopt.h ../../../srccore/srccore.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srcmisc/utilhuge.h ../../../srcmisc/globldat.h \
 ../../../srccore/atexit.h ../../../srccore/call.h \
 ../../../srccore/secode.h ../../../incjse/selink.h \
 ../../../srccore/function.h ../../../srccore/jseengin.h \
 ../../../srccore/code.h ../../../srccore/define.h \
 ../../../srccore/extlib.h ../../../srccore/library.h \
 ../../../srccore/security.h ../../../srccore/source.h \
 ../../../srccore/token.h ../../../srccore/var.h \
 ../../../srccore/stack.h ../../../srccore/loclfunc.h \
 ../../../srccore/operator.h
jsemem.o: ../../../srcmisc/jsemem.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h ../../../srcmisc/dbgprntf.h
macfunc.o: ../../../srcmisc/macfunc.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
unixfunc.o: ../../../srcmisc/unixfunc.c ../../../srcmisc/unixfunc.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/jsemem.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilhuge.h ../../../srcmisc/globldat.h \
 ../../../srccore/atexit.h ../../../srccore/call.h \
 ../../../srccore/secode.h ../../../incjse/selink.h \
 ../../../srccore/function.h ../../../srccore/jseengin.h \
 ../../../srccore/code.h ../../../srccore/define.h \
 ../../../srccore/extlib.h ../../../srccore/library.h \
 ../../../srccore/security.h ../../../srccore/source.h \
 ../../../srccore/token.h ../../../srccore/var.h \
 ../../../srccore/stack.h ../../../srccore/loclfunc.h \
 ../../../srccore/operator.h
utilhuge.o: ../../../srcmisc/utilhuge.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
utilstr.o: ../../../srcmisc/utilstr.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
ecmamisc.o: ../../../srclib/ecma/ecmamisc.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
mathobj.o: ../../../srclib/ecma/mathobj.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
sebuffer.o: ../../../srclib/ecma/sebuffer.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
sedate.o: ../../../srclib/ecma/sedate.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
seecma.o: ../../../srclib/ecma/seecma.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
seobject.o: ../../../srclib/ecma/seobject.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h ../../../srclib/ecma/seecma.h
seobjfun.o: ../../../srcmisc/seobjfun.c ../../../srccore/jseopt.h \
 ../../../srccore/srccore.h ../../../incjse/jsetypes.h \
 ../../../incjse/jsedef.h ../../../incjse/jselib.h \
 ../../../srcmisc/seuni.h ../../../srcmisc/dirparts.h \
 ../../../srccore/textcore.h ../../../srcmisc/jsemem.h \
 ../../../srccore/mempool.h ../../../srccore/util.h \
 ../../../srcmisc/utilstr.h ../../../srcmisc/utilhuge.h \
 ../../../srcmisc/globldat.h ../../../srccore/atexit.h \
 ../../../srccore/call.h ../../../srccore/secode.h \
 ../../../incjse/selink.h ../../../srccore/function.h \
 ../../../srccore/jseengin.h ../../../srccore/code.h \
 ../../../srccore/define.h ../../../srccore/extlib.h \
 ../../../srccore/library.h ../../../srccore/security.h \
 ../../../srccore/source.h ../../../srccore/token.h \
 ../../../srccore/var.h ../../../srccore/stack.h \
 ../../../srccore/loclfunc.h ../../../srccore/operator.h \
 ../../../srcmisc/unixfunc.h
seliball.o: ../../../srclib/common/seliball.c \
 ../../../srccore/jseopt.h ../../../srccore/srccore.h \
 ../../../incjse/jsetypes.h ../../../incjse/jsedef.h \
 ../../../incjse/jselib.h ../../../srcmisc/seuni.h \
 ../../../srcmisc/dirparts.h ../../../srccore/textcore.h \
 ../../../srcmisc/jsemem.h ../../../srccore/mempool.h \
 ../../../srccore/util.h ../../../srcmisc/utilstr.h \
 ../../../srcmisc/utilhuge.h ../../../srcmisc/globldat.h \
 ../../../srccore/atexit.h ../../../srccore/call.h \
 ../../../srccore/secode.h ../../../incjse/selink.h \
 ../../../srccore/function.h ../../../srccore/jseengin.h \
 ../../../srccore/code.h ../../../srccore/define.h \
 ../../../srccore/extlib.h ../../../srccore/library.h \
 ../../../srccore/security.h ../../../srccore/source.h \
 ../../../srccore/token.h ../../../srccore/var.h \
 ../../../srccore/stack.h ../../../srccore/loclfunc.h \
 ../../../srccore/operator.h ../../../srcmisc/unixfunc.h
