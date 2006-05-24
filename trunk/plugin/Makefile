# Makefile for MRS. This is a somewhat complex makefile, I know.
#
# You can set local options in your make.config file
# This file will be included by this makefile
# Options to set in this make.config file are e.g.:
#
#	TARGET	- what kind of plugin to build, can be 'perl5' or 'java'
#	CC		- the c++ compiler to use, must support tr1 though (gcc 4.1 e.g.)
#	BOOST	- the location of your boost library
#	SWIG	- your local copy of swig, must be exactly version 1.3.27 
#	
# Now you can build debug and non debug versions, default is non-debug
# but to build a debug version you type 'make DEBUG=1' instead of simply 'make'

OS			= $(shell uname -s)
HOST		= $(shell uname -n)
UNAME_CPU	= $(shell uname -p)

ifeq ($(OS),Linux)
	UNAME_CPU	= $(shell uname -m)
endif

ifeq ($(UNAME_CPU),x86_64)
	CPU		= x86
endif

ifeq ($(UNAME_CPU),i686)
	CPU		= x86
endif

ifeq ($(UNAME_CPU),i586)
	CPU		= x86
endif

ifeq ($(UNAME_CPU),i386)
	CPU		= x86
endif

ifeq ($(UNAME_CPU),ppc)
	CPU		= ppc
endif

ifeq ($(UNAME_CPU),powerpc)
	CPU		= ppc
endif

ifeq ($(UNAME_CPU),mips)
	CPU		= mips
endif

ifeq ($(CPU),)
	Please make sure CPU is defined here
endif

#TARGET		= perl5 or java

SO_EXT		= .so
LIBNAME		= libmrs
VERSION		= 2.0.1
# DEBUG should be either 1 or 0
DEBUG		= 0
DEFINES		= CPU_$(CPU) MINI_H_LIB
BOOST		= /usr/include
LIBS		= z m bz2
LIBPATHS	=  ./Lib/ /usr/local/lib/
INCPATHS	= ./Lib/Includes/ ./Lib/X/ ./Lib/
SYSINCPATHS	= /usr/include ./Lib/loki $(BOOST)
WARNINGS	=  -Wall -Wno-multichar -Wno-unknown-pragmas -Wno-long-long -Wno-sign-compare
CFLAGS		= -fsigned-char -g -ffor-scope
LDFLAGS		= -fdata-sections -g # -ffunction-sections 
OPT			+= -fstrict-aliasing

CC			= c++
PERL		= perl
SWIG		= swig

include make.config

SWIG_VERSION	= $(shell $(SWIG) -version | grep Version | sed s'/SWIG Version //')
PERL_CORE		= $(shell $(PERL) -MExtUtils::Embed -e perl_inc)

ifeq ($(OS),FreeBSD)
	LIBS		+= 
	INCPATHS	+= 
	SYSINCPATHS	+= /usr/local/include
	DEFINES		+= HAS_ENDIAN FREEBSD
	LDFLAGS		+= -shared
	CTAGS		= exctags
	STRIP		= strip -d

include /etc/make.conf

ifeq ($(CPUTYPE),p4)
	OPT			+= -march=pentium4
endif

ifeq ($(CPUTYPE),i686)
	OPT			+= -march=pentium3
endif

endif

ifeq ($(OS),Linux)
	SHELLLIBS	+= dl pthread
	DEFINES		+= LINUX
	LD_SO_FLAG	= -shared
	LIBPATHS	+= /usr/local/lib64/
	LDFLAGS		+= -pthread
	LIBS		+= uuid
	STRIP		= strip -d
	OPT			+= -O3

ifeq ($(UNAME_CPU),x86_64)
	OPT			+= -m64 -march=nocona
endif

ifeq ($(UNAME_CPU),i686)
	OPT			+= -march=pentium3
endif

	JAVA_INC	= -I/opt/SUNWappserver/jdk/include/
	JAVA_INC	+= -I/opt/SUNWappserver/jdk/include/linux/

endif

ifeq ($(CPU),mips)
	DEFINES		+= IRIX
	SYSINCPATHS += /opt/MIPSpro/MIPSpro/usr/include
	SYSINCPATHS += /usr/freeware/include /home/cc/hekkel/include
	LIBPATHS	+= /usr/freeware/lib32 /home/cc/hekkel/lib
	LIBS		+= pthread
	LD_SO_FLAG	= -shared
	LDFLAGS		+= -rpath /home/cc/hekkel/lib/mabi=64 -mips4 -mabi=64
	CFLAGS		+= -mips4 -mabi=64
endif

ifeq ($(OS),Darwin)
ifeq ($(TARGET),java)
	SO_EXT		:= .jnilib
	LD_SO_FLAG	= -bundle -undefined suppress -flat_namespace
else
	SO_EXT		:= .bundle
	LD_SO_FLAG	= -dynamiclib -framework JavaVM -undefined suppress -flat_namespace
endif
	DEFINES		+= DARWIN
	LDFLAGS		+= 
	CFLAGS		+= -force_cpusubtype_ALL
	OPT			+= -O3 -mcpu=G4 -falign-loops=32
	CTAGS		= /sw/bin/ctags
	STRIP		= strip -S
	JAVA_INC	= -I/System/Library/Frameworks/JavaVM.framework/Versions/1.4.2/Headers/
endif

ifeq ($(DEBUG),0)
	CFLAGS	+= $(OPT)
	OBJDIR	= Obj.NoDebug
	DEFINES	+= NDEBUG
else
	OBJDIR	= Obj.Debug
	DEFINES	+= DEBUG
	LIBNAME	:= $(LIBNAME)d
endif

ifeq ($(PROFILE),1)
	LDFLAGS	+= -pg
	CFLAGS += -pg -g3
	OBJDIR	:= $(OBJDIR).Profile
	LIBNAME	:= $(LIBNAME)p
endif

LIBNAME	:= $(LIBNAME).a

ifeq ($(TARGET),java)
	OUTPUT		= libmrs$(SO_EXT)
else
ifeq ($(TARGET),perl5)
	OUTPUT		= MRS$(SO_EXT)
else
fout
endif
endif

LDFLAGS += $(addprefix -L, $(LIBPATHS)) 

Sources = \
	./Sources/MRSInterface.cpp \
	./Sources/CBitStream.cpp \
	./Sources/CCompress.cpp \
	./Sources/CDatabank.cpp \
	./Sources/CDbInfo.cpp \
	./Sources/CDecompress.cpp \
	./Sources/CDictionary.cpp \
	./Sources/CDocIterator.cpp \
	./Sources/CDocWeightArray.cpp \
	./Sources/CFilter.cpp \
	./Sources/CIdTable.cpp \
	./Sources/CIndex.cpp \
	./Sources/CIndexer.cpp \
	./Sources/CIterator.cpp \
	./Sources/CLexicon.cpp \
	./Sources/CQuery.cpp \
	./Sources/CRankedQuery.cpp \
	./Sources/CSemaphore.cpp \
	./Sources/CSequence.cpp \
	./Sources/CUtils.cpp \
	./Sources/CTextTable.cpp \
	./Sources/CThread.cpp \
	./Lib/Generic/HStream.cpp \
	./Lib/Generic/HError.cpp \
	./Lib/Generic/HFile.cpp \
	./Lib/Generic/HFileCache.cpp \
	./Lib/Generic/HGlobals.cpp \
	./Lib/Generic/HUrl.cpp \
	./Lib/Generic/HUtils.cpp \
	./Lib/Posix/HUnixFile.cpp \
	./Lib/Posix/HUnixUtils.cpp \
	./Lib/loki/SmallObj.cpp \
	./Sources/CBlast.cpp \
	./Sources/CBlastIndex.cpp \
	./Sources/CMatrix.cpp \
	./Sources/CNCBI.cpp \

define SOURCES
	$(Sources)
endef

define SRC_TO_OBJ
	$(addprefix $(OBJDIR)/, $(addsuffix .o, $(foreach file, $(SOURCES), $(basename $(notdir $(file))))))
endef

OBJS = $(SRC_TO_OBJ)

# create a unique list of paths to our sources
SRC_PATHS += $(sort $(foreach file, $(SOURCES), $(dir $(file))))

VPATH :=
# add the list of source paths to VPATH if not already present
VPATH += $(addprefix :, $(subst  ,:, $(filter-out $($(subst, :, ,$(VPATH))), $(SRC_PATHS))))

# add the list of source paths to INLCUDES if not already present
INCLUDES = $(foreach path, $(INCPATHS) $(SRC_PATHS), $(addprefix -I, $(path)))
ifneq ($(IQUOTE),0)
INCLUDES += -iquote
else
INCLUDES += -I-
endif
INCLUDES += $(foreach path, $(SYSINCPATHS), $(addprefix -I, $(path)))

ifeq ($(TARGET),perl5)
INCLUDES += $(PERL_CORE)
endif

ifeq ($(TARGET),java)
INCLUDES += $(JAVA_INC)
endif

# add the -L prefix to all library paths to search
LINK_PATHS = $(foreach path, $(LIBPATHS), $(addprefix -L, $(path)))

# add the -l prefix to all libs to be linked against
LINK_LIBS = $(foreach lib, $(LIBS), $(addprefix -l, $(lib)))

# add to the linker flags 
LDFLAGS += $(LINK_PATHS) $(LINK_LIBS)

CFLAGS	+= $(DBG) $(WARNINGS)
ifeq ($(CPU), x86)
CFLAGS	+= -fpic
endif

DEFS	= $(foreach define, $(DEFINES), $(addprefix -D, $(define)))
CFLAGS	+= $(DEFS)

all: $(OUTPUT)

# build rules

echo_flags:
	 @ echo Compiler flags used: $(CFLAGS)
	 @ echo Include paths: $(INCLUDES)

$(LIBNAME): $(OBJDIR) $(OBJS)
	@ echo Linking $(LIBNAME)
	@ $(LD) -r -o $@ $(OBJS)

lib: $(LIBNAME)

$(OUTPUT): echo_flags lib $(OBJDIR)/MRS_$(TARGET).o
	@ echo "Linking "$(@F)
	@ $(CC) -o $(OUTPUT) $(LD_SO_FLAG) $(LIBNAME) $(OBJDIR)/MRS_$(TARGET).o $(LDFLAGS)
	@ chmod a+rx $(OUTPUT)
	@ echo "Done"

clean: FORCE
	rm -rf $(OBJDIR)

dist: FORCE
	mkdir -p dist/lib
	$(PERL) -pi.bak -e 's/^.+VERSION.+$$/\t"VERSION"\t\t\t=> "$(VERSION)",/' dist/Makefile.PL
	cp MRS.pm $(OUTPUT) dist/lib/
	$(STRIP) dist/lib/$(OUTPUT)
	echo "MANIFEST" > dist/MANIFEST
	echo "Makefile.PL" >> dist/MANIFEST
	echo "lib/MRS.pm" >> dist/MANIFEST
	echo lib/$(OUTPUT) >> dist/MANIFEST
	rm -rf dist/MRS-$(VERSION).tar.gz
	cd dist; $(PERL) Makefile.PL; $(MAKE) dist

install: dist
	cd dist; sudo $(MAKE) install
#	install -m 755 $< /usr/local/bin/mrs

blast: echo_flags lib Sources/mrs_blast.cpp
	$(CC) $(CFLAGS) -o mrs_blast -ILib/Includes Sources/mrs_blast.cpp $(LIBNAME) $(LDFLAGS)
	@ echo "Done"

query: echo_flags lib Sources/mrs_test.cpp
	@ echo "=> "$@
	@ $(CC) $(CFLAGS) -o mrs_query -ILib/Includes Sources/mrs_test.cpp $(LIBNAME) $(LDFLAGS)
	@ echo "Done"

tags: FORCE
	$(CTAGS) $(wildcard $(addsuffix *, $(SRC_PATHS))) $(wildcard $(addsuffix *.h, $(INCPATHS)))

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)

$(OBJDIR)/%_$(TARGET).o: Sources/%_$(TARGET).cpp
	@ echo "=> "$(@F)
	$(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS) -O0

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJS:%.o=%.d)

ifeq ($(SWIG_VERSION),1.3.27)

Sources/MRS_perl5.cpp: Sources/MRSInterface.h Sources/MRS_perl5.i
	$(SWIG) $(DEFS) -c++ -perl5 -o Sources/MRS_perl5.cpp -outdir ./ Sources/MRS_perl5.i

Sources/MRS_java.cpp: Sources/MRSInterface.h Sources/MRS_java.i
	mkdir -p Java/nl/ru/cmbi/MRSPlugin
	$(SWIG) $(DEFS) -c++ -java -o Sources/MRS_java.cpp -outdir Java/nl/ru/cmbi/MRSPlugin -package nl.ru.cmbi.MRSPlugin Sources/MRS_java.i

else

Sources/MRS_perl5.cpp: Sources/MRSInterface.h Sources/MRS_perl5.i
	@ echo !!! WARNING !!! Not rebuilding required $@ since swig version is not 1.3.27

Sources/MRS_java.cpp: Sources/MRSInterface.h Sources/MRS_java.i
	@ echo !!! WARNING !!! Not rebuilding required $@ since swig version is not 1.3.27

endif

$(OBJDIR)/%.d:%.cpp
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)
	@ echo "-> "$(@F)
	@ $(SHELL) -ec '$(CC) $(DEFS) -M $(INCLUDES) $< | sed '\''s/\(.*\.o\)[ :]*/$(OBJDIR)\/\1 $(@F): /g'\'' > $@'

FORCE:

test:
	echo $(PERL)
	echo $(PERL_CORE)
