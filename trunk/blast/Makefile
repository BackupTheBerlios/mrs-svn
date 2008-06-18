# Makefile for mrs-blast
#
# You can set local options in your make.config file
# This file will be included by this makefile
# Options to set in this make.config file are e.g.:
#
#	CC		- the c++ compiler to use, must support tr1 though (gcc 4.1 e.g.)
#	
# Now you can build debug and non debug versions, default is non-debug
# but to build a debug version you type 'make DEBUG=1' instead of simply 'make'

LIBNAME		= libmrs
VERSION		= 4.0d1
# DEBUG should be either 1 or 0
DEBUG		= 0
LIBS		= z m bz2
LIBPATHS	= ../lib/
INCPATHS	= ../lib/Lib/Includes/ ../lib/Lib/Posix/ ../lib/Lib/ ../lib/Lib/Generic/ ../lib/Sources/
SYSINCPATHS	= /usr/include
WARNINGS	= -Wall -Wno-multichar -Wno-unknown-pragmas -Wno-long-long
CFLAGS		= -fsigned-char -g -ffor-scope
CFLAGS		+= -fmessage-length=132 -fdiagnostics-show-location=every-line
LDFLAGS		= -fdata-sections -g
#OPT		+= -fstrict-aliasing
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
	./Sources/mrs-blast.cpp

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

CFLAGS	+= $(addprefix -D, $(DEFINES)) $(DBG) $(WARNINGS) $(INCLUDES)
ifeq ($(CPU), x86)
CFLAGS	+= -fPIC
endif

# add to the linker flags 
# add the -L prefix to all library paths to search
LDFLAGS += $(addprefix -L, $(LIBPATHS))
# add the -l prefix to all libs to be linked against
LDFLAGS += $(addprefix -l, $(LIBS))

all: mrs-blast

# build rules

echo_flags:
	 @ echo Compiler flags used: $(CFLAGS)
	 @ echo Include paths: $(INCLUDES)

../lib/$(LIBNAME):
	@ $(MAKE) DEBUG=$(DEBUG) -C ../lib $(LIBNAME)

clean: FORCE
	rm -rf $(OBJDIR)

dist: mrs-blast

install: mrs-blast
	$(SUDO) install -m 755 mrs-blast $(BINDIR)/mrs-blast

mrs-blast: ../lib/$(LIBNAME) Sources/mrs-blast.cpp
	$(CC) $(CFLAGS) -o $@ Sources/mrs-blast.cpp $(LDFLAGS) ../lib/$(LIBNAME)
	@ echo "Done"

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
