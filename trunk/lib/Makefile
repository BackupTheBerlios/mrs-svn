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

LIBNAME		= libmrs
VERSION		= 4.0d2
# DEBUG should be either 1 or 0
DEBUG		= 0
LIBPATHS	= ./Lib/
INCPATHS	= ./Lib/Includes/ ./Lib/X/ ./Lib/
SYSINCPATHS	= /usr/include
WARNINGS	= -Wall -Wno-multichar -Wno-unknown-pragmas -Wno-long-long
CFLAGS		= -fsigned-char -g -ffor-scope
CFLAGS		+= -fmessage-length=132 -fdiagnostics-show-location=every-line
LDFLAGS		= -fdata-sections -g
OPT			+= -O2

include ../make.config

CC			?= c++		# some reasonable defaults

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

Sources = \
	./Sources/CBitStream.cpp \
	./Sources/CCompress.cpp \
	./Sources/CDatabank.cpp \
	./Sources/CDbInfo.cpp \
	./Sources/CDecompress.cpp \
	./Sources/CDictionary.cpp \
	./Sources/CDocIterator.cpp \
	./Sources/CDocument.cpp \
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
	./Sources/MRS.cpp \
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

CFLAGS	+= $(addprefix -D, $(DEFINES)) $(DBG) $(WARNINGS)
ifeq ($(CPU), x86)
CFLAGS	+= -fPIC
endif

# add to the linker flags 
# add the -L prefix to all library paths to search
LDFLAGS += $(addprefix -L, $(LIBPATHS))
# add the -l prefix to all libs to be linked against
LDFLAGS += $(addprefix -l, $(LIBS))

all: lib

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

clean: FORCE
	rm -rf $(OBJDIR)

install:

tags: FORCE
	$(CTAGS) $(wildcard $(addsuffix *, $(SRC_PATHS))) $(wildcard $(addsuffix *.h, $(INCPATHS)))

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

$(OBJDIR)/%.o: %.c
	@ echo "=> "$(@F)
	@ $(CC) -MD -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJS:%.o=%.d)

${OBJS:.o=.d}:

FORCE:
