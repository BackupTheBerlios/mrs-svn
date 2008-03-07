# Makefile for MRS. This is a somewhat complex makefile, I know.
#
# You can set local options in your make.config file
# This file will be included by this makefile
# Options to set in this make.config file are e.g.:
#
#	CC		- the c++ compiler to use, must support tr1 though (gcc 4.1 e.g.)
#	SWIG	- your local copy of swig, must be exactly version 1.3.27 
#	
# Now you can build debug and non debug versions, default is non-debug
# but to build a debug version you type 'make DEBUG=1' instead of simply 'make'

TARGET		?= perl5
SO_EXT		= .so
LIBNAME		= libmrs
VERSION		= 4.0d1
# DEBUG should be either 1 or 0
DEBUG		= 0
LIBS		= z m bz2
LIBPATHS	= ./Lib/
INCPATHS	= ./Lib/Includes/ ./Lib/X/ ./Lib/
SYSINCPATHS	= /usr/include
WARNINGS	= -Wall -Wno-multichar -Wno-unknown-pragmas -Wno-long-long
CFLAGS		= -fsigned-char -g -ffor-scope
CFLAGS		+= -fmessage-length=132 -fdiagnostics-show-location=every-line
LDFLAGS		= -fdata-sections -g
#OPT		+= -fstrict-aliasing
OPT			+= -O3

CC			= c++		# some reasonable defaults
PERL		= perl
SWIG		= swig

include ../make.config

ifneq ($(SWIG),)
SWIG_VERSION	= $(shell $(SWIG) -version | grep Version | sed s'/SWIG Version //')
else
SWIG_VERSION	= not_installed
endif

DEFINES     += CPU_$(CPU)

ifeq ($(OS),FreeBSD)
	DEFINES		+= HAS_ENDIAN FREEBSD HAVE_SYS_IOCTL_H HAVE_SYS_SOCKIO_H \
					HAVE_SYS_SOCKET_H HAVE_NETINET_IN_H HAVE_SRANDOM \
					HAVE_NET_IF_DL_H HAVE_SA_LEN HAVE_INTTYPES_H
	LD_SO_FLAG	+= -shared
	LDFLAGS		+= -pthread
	STRIP		= strip -d
endif

ifeq ($(OS),Linux)
	DEFINES		+= LINUX HAVE_SYS_IOCTL_H \
					HAVE_SYS_SOCKET_H HAVE_NETINET_IN_H HAVE_SRANDOM HAVE_SA_LEN
	LD_SO_FLAG	= -shared
	LDFLAGS		+= -pthread
	STRIP		= strip -d
endif

ifeq ($(OS),Darwin)
	SO_EXT		:= .bundle
	LD_SO_FLAG	= -bundle -undefined suppress -flat_namespace
	DEFINES		+= DARWIN HAVE_SYS_IOCTL_H HAVE_SYS_SOCKIO_H \
					HAVE_SYS_SOCKET_H HAVE_NETINET_IN_H HAVE_SRANDOM HAVE_NET_IF_DL_H HAVE_SA_LEN
	LDFLAGS		+= 
	STRIP		= strip -S
endif

ifeq ($(NO_BLAST),1)
DEFINES			+= NO_BLAST
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

ifeq ($(TARGET),perl5)
OUTPUT ?= MRS$(SO_EXT)
endif

ifeq ($(TARGET),java)
OUTPUT ?= libmrs$(SO_EXT)
endif

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
	./Sources/CSequence.cpp \
	./Sources/CTokenizer.cpp \
	./Sources/CUtils.cpp \
	./Sources/CTextTable.cpp \
	./Sources/CThread.cpp \
	./Sources/CXMLDocument.cpp \
	./Lib/Generic/HStream.cpp \
	./Lib/Generic/HError.cpp \
	./Lib/Generic/HFile.cpp \
	./Lib/Generic/HGlobals.cpp \
	./Lib/Generic/HUrl.cpp \
	./Lib/Generic/HUtils.cpp \
	./Lib/Posix/HUnixFile.cpp \
	./Lib/Posix/HUnixUtils.cpp \
	./Lib/Posix/HMutex.cpp \
	./Lib/uuid/clear.c \
	./Lib/uuid/compare.c \
	./Lib/uuid/copy.c \
	./Lib/uuid/gen_uuid.c \
	./Lib/uuid/isnull.c \
	./Lib/uuid/pack.c \
	./Lib/uuid/parse.c \
	./Lib/uuid/unpack.c \
	./Lib/uuid/unparse.c \
	./Lib/uuid/uuid_time.c

ifneq ($(NO_BLAST),1)
Sources += \
	./Sources/CBlast.cpp \
	./Sources/CBlastIndex.cpp \
	./Sources/CMatrix.cpp \
	./Sources/CNCBI.cpp
endif

OBJS = $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(notdir $(Sources)))))

# create a unique list of paths to our sources
SRC_PATHS += $(sort $(dir $(Sources)))

VPATH :=
# add the list of source paths to VPATH if not already present
VPATH += $(addprefix :, $(subst  ,:, $(filter-out $($(subst, :, ,$(VPATH))), $(SRC_PATHS))))

# add the list of source paths to INLCUDES if not already present
INCLUDES += $(addprefix -I, $(SYSINCPATHS))
ifeq ($(IQUOTE),1)
INCLUDES += $(addprefix -iquote, $(INCPATHS) $(SRC_PATHS))
else
INCLUDES += -I- $(addprefix -I, $(INCPATHS) $(SRC_PATHS))
endif
# Perl's defines and includes

ifeq ($(TARGET),perl5)
INCLUDES += $(shell $(PERL) -MExtUtils::Embed -e perl_inc)
endif

ifeq ($(TARGET),java)
JAVA_INC ?= -I/usr/lib/jvm/java-1.5.0-sun-1.5.0.13/include/ -I/usr/lib/jvm/java-1.5.0-sun-1.5.0.13/include/linux/
INCLUDES += $(JAVA_INC)
endif

CFLAGS	+= $(addprefix -D, $(DEFINES)) $(DBG) $(WARNINGS)
ifeq ($(CPU), x86)
CFLAGS	+= -fPIC
endif

# add to the linker flags 
# add the -L prefix to all library paths to search
LDFLAGS += $(addprefix -L, $(LIBPATHS))
# add the -l prefix to all libs to be linked against
LDFLAGS += $(addprefix -l, $(LIBS))

all: $(OUTPUT) mrs_blast mrs_query mrs_merge dist

# build rules

echo_flags:
	 @ echo Compiler flags used: $(CFLAGS)
	 @ echo Include paths: $(INCLUDES)

$(LIBNAME): $(OBJDIR) $(OBJS)
	@ echo Linking $(LIBNAME)
	@ $(LD) -r -o $@ $(OBJS)

lib: $(LIBNAME)

doc: FORCE
	doxygen doc/doxygen.config

ifeq ($(TARGET),perl5)
$(OUTPUT): $(LIBNAME) $(OBJDIR)/MRS_perl5.o
	@ echo "Linking "$(@F)
	@ $(CC) -o $(OUTPUT) $(LD_SO_FLAG) $(LIBNAME) $(OBJDIR)/MRS_perl5.o $(LDFLAGS)
	@ chmod a+rx $(OUTPUT)
	@ echo "Done"
endif

ifeq ($(TARGET),java)
$(OUTPUT): $(LIBNAME) $(OBJDIR)/MRS_java.o
	@ echo "Linking "$(@F)
	@ $(CC) -o $(OUTPUT) $(LD_SO_FLAG) $(LIBNAME) $(OBJDIR)/MRS_java.o $(LDFLAGS)
	@ chmod a+rx $(OUTPUT)
	@ echo "Done"
endif

clean: FORCE
	rm -rf $(OBJDIR)

dist: dist/MRS-$(VERSION).tar.gz
	echo dist/MRS-$(VERSION).tar.gz

dist/MRS-$(VERSION).tar.gz: $(OUTPUT) MRS.pm
	mkdir -p dist/lib dist/bin
	$(PERL) -pi.bak -e 's/^.+VERSION.+$$/\t"VERSION"\t\t\t=> "$(VERSION)",/' dist/Makefile.PL
	cp MRS.pm $(OUTPUT) dist/lib/
	cp mrs.pl dist/bin
	$(STRIP) dist/lib/$(OUTPUT)
	echo "MANIFEST" > dist/MANIFEST
	echo "Makefile.PL" >> dist/MANIFEST
	echo "lib/MRS.pm" >> dist/MANIFEST
	echo "bin/mrs.pl" >> dist/MANIFEST
	echo lib/$(OUTPUT) >> dist/MANIFEST
	rm -rf dist/MRS-$(VERSION).tar.gz
	cd dist; $(PERL) Makefile.PL INSTALLSCRIPT=$(INSTALL_DIR); $(MAKE) dist

install: $(OUTPUT)
	$(MAKE) dist
	cd dist; $(SUDO) $(MAKE) install
	$(SUDO) install -m 755 mrs_blast $(BINDIR)/mrs_blast
	$(SUDO) install -m 755 mrs_query $(BINDIR)/mrs_query
	$(SUDO) install -m 755 mrs_query $(BINDIR)/mrs_merge
	$(SUDO) install -m 755 mrs.pl $(BINDIR)/mrs.pl

mrs_blast: $(LIBNAME) Sources/mrs_blast.cpp
	$(CC) $(CFLAGS) -o $@ -ILib/Includes Sources/mrs_blast.cpp $(LIBNAME) $(LDFLAGS)
	@ echo "Done"

mrs_merge: $(LIBNAME) Sources/mrs_merge.cpp
	$(CC) $(CFLAGS) -o $@ -ILib/Includes Sources/mrs_merge.cpp $(LIBNAME) $(LDFLAGS)
	@ echo "Done"

mrs_query: $(LIBNAME) Sources/mrs_query.cpp
	@ echo "=> "$@
	@ $(CC) $(CFLAGS) -o $@ -ILib/Includes Sources/mrs_query.cpp $(LIBNAME) $(LDFLAGS)
	@ echo "Done"

mrs_test: $(LIBNAME) Sources/mrs_test.cpp
	$(CC) Sources/mrs_test.cpp -o $@ $(INCLUDES) $(CFLAGS) $(LIBNAME) $(LDFLAGS)
	@ echo "Done"

mrs_test-2: $(LIBNAME) Sources/mrs_test-2.cpp
	$(CC) Sources/mrs_test-2.cpp -o $@ $(INCLUDES) $(CFLAGS) $(LIBNAME) $(LDFLAGS)
	@ echo "Done"

tags: FORCE
	$(CTAGS) $(wildcard $(addsuffix *, $(SRC_PATHS))) $(wildcard $(addsuffix *.h, $(INCPATHS)))

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)

# Swig's code should not be optimized... really
$(OBJDIR)/%_perl5.o: Sources/%_perl5.cpp
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS) -O0

$(OBJDIR)/%_java.o: Sources/%_java.cpp
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS) -O0

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

$(OBJDIR)/%.o: %.c
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJS:%.o=%.d)

${OBJS:.o=.d}:

#ifeq ($(SWIG_VERSION),1.3.27)

Sources/MRS_perl5.cpp: Sources/MRSInterface.h Sources/MRS_perl5.i
	$(SWIG) $(DEFS) -c++ -perl5 -o Sources/MRS_perl5.cpp -outdir ./ Sources/MRS_perl5.i

Sources/MRS_java.cpp: Sources/MRSInterface.h Sources/MRS_java.i
	mkdir -p Java/nl/ru/cmbi/MRSPlugin
	$(SWIG) $(DEFS) -c++ -java -o Sources/MRS_java.cpp -outdir Java/nl/ru/cmbi/MRSPlugin -package nl.ru.cmbi.MRSPlugin Sources/MRS_java.i

#else
#
#Sources/MRS_perl5.cpp: Sources/MRSInterface.h Sources/MRS_perl5.i
#	@ echo !!! WARNING !!! Not rebuilding required $@ since swig version is not 1.3.27
#
#endif

FORCE:
