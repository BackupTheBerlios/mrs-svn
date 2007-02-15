# make file for soap server

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

SO_EXT		= .so
TARGET		= mrsws
# DEBUG should be either 1 or 0
DEBUG		= 0
DEFINES		= CPU_$(CPU) MINI_H_LIB
WARNINGS	= -Wall -Wno-multichar
RDEFINES	= -DMACOS=0 -DUNIX=1
LIBNAME		= mrs
LIBS		= z expat m bz2 gsoap++ xml2 $(addsuffix $(BOOST_LIB_SUFFIX), boost_regex boost_filesystem)
LIBPATHS	=  ../../plugin/Lib/ /usr/local/lib/ ../../plugin/
INCPATHS	= ../../plugin/Lib/Includes/ ../../plugin/Lib/X/ ../../plugin/Lib/ ../../plugin/Sources/
SYSINCPATHS	= /usr/include
WARNINGS	=  -Wall -Wno-multichar -Wno-unknown-pragmas -Wno-long-long -Wno-sign-compare
CFLAGS		= -fsigned-char -g -ffor-scope
LDFLAGS		= -fdata-sections -g # -ffunction-sections
CC			= c++
OPT			+= -fstrict-aliasing
SOAPCPP2	= soapcpp2
PERL		= /usr/bin/perl

include ../make.config

LDFLAGS		+= $(shell $(PERL) -MExtUtils::Embed -e ldopts)

ifeq ($(OS),FreeBSD)
	LIBS		+= 
	INCPATHS	+= 
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
	LDFLAGS		+= -pthread
	STRIP		= strip -d
	OPT			+= -O3

ifeq ($(UNAME_CPU),x86_64)
	OPT			+= -m64 -march=nocona
endif

ifeq ($(UNAME_CPU),i686)
	OPT			+= -march=pentium3
endif

endif

ifeq ($(OS),Darwin)
	DEFINES		+= DARWIN
	LDFLAGS		+= 
	CFLAGS		+= -force_cpusubtype_ALL
	OPT			+= -O3 -falign-loops=32
	LDFLAGS		+= /System/Library/Perl/5.8.6/darwin-thread-multi-2level/auto/DynaLoader/DynaLoader.a
	
ifeq ($(CPU),ppc)
		OPT		+= -mcpu=G4
endif
	
	CTAGS		= /sw/bin/ctags
	STRIP		= strip -S
	JAVA_INC	= -I/System/Library/Frameworks/JavaVM.framework/Versions/1.4.2/Headers/
	SOAPCPP2	:= /opt/local/bin/soapcpp2
endif

ifeq ($(DEBUG),0)
	CFLAGS	+= $(OPT)
	OBJDIR	= Obj.NoDebug
	DEFINES	+= NDEBUG
else
	OBJDIR	= Obj.Debug
	DEFINES	+= DEBUG
#	LIBS	+= efence
	LIBNAME	:= $(LIBNAME)d
endif

ifeq ($(PROFILE),1)
	LDFLAGS	+= -pg
	CFLAGS += -pg -g3
	OBJDIR	:= $(OBJDIR).Profile
	LIBNAME	:= $(LIBNAME)p
endif

LDFLAGS += $(addprefix -L, $(LIBPATHS)) 
LIBNAME := lib$(LIBNAME).a

Sources = \
	./Sources/mrsws.cpp \
	./Sources/soapC.cpp \
	./Sources/soapServer.cpp \
	../common/Sources/WConfig.cpp \
	../common/Sources/WFormat.cpp \
	../common/Sources/WUtils.cpp \
	../common/Sources/WError.cpp

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
INCLUDES += $(shell $(PERL) -MExtUtils::Embed -e ccopts)

ifneq ($(IQUOTE),0)
INCLUDES += -iquote
else
INCLUDES += -I-
endif
INCLUDES += $(foreach path, $(SYSINCPATHS), $(addprefix -I, $(path)))

# add the -L prefix to all library paths to search
LINK_PATHS = $(foreach path, $(LIBPATHS), $(addprefix -L, $(path)))

# add the -l prefix to all libs to be linked against
LINK_LIBS = $(foreach lib, $(LIBS), $(addprefix -l, $(lib)))

# add to the linker flags 
LDFLAGS += $(LINK_PATHS) $(LINK_LIBS)

CFLAGS	+= $(DBG) $(WARNINGS)
ifeq ($(CPU), x86)
CFLAGS	+= -fPIC
endif

DEFS	= $(foreach define, $(DEFINES), $(addprefix -D, $(define)))
CFLAGS	+= $(DEFS)

all: $(TARGET)

# build rules

echo_flags:
	 @ echo Compiler flags used: $(CFLAGS)
	 @ echo Include paths: $(INCLUDES)

$(TARGET): echo_flags $(OBJS)
	$(MAKE) DEBUG=$(DEBUG) -C ../../plugin $(LIBNAME)
	@ echo "Linking "$(@F)
	$(CC) -o $(TARGET) ../../plugin/$(LIBNAME) $(OBJS) $(LDFLAGS)
	@ chmod a+rx $(TARGET)
	@ echo "Done"

clean: FORCE
	rm -rf $(OBJDIR)

dist: $(TARGET)

install: dist
	$(SUDO) install mrsws $(INSTALL_DIR)/mrsws
	@ echo "To finish installation of the web-service you have to edit the file"
	@ echo "ws/Sources/mrsws.wsdl and put it in the proper location."
#	install Sources/mrsws.wsdl ../web-simple/htdocs/

tags: FORCE
	$(CTAGS) $(wildcard $(addsuffix *, $(SRC_PATHS))) $(wildcard $(addsuffix *.h, $(INCPATHS)))

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS)

$(OBJDIR)/%.o: %.c
	@ echo "=> "$(@F)
	@ $(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJS:%.o=%.d)

soapC.cpp soapServer.cpp: Sources/mrsws.h
	$(SOAPCPP2) -d Sources/ Sources/mrsws.h

$(OBJDIR)/%.d:%.cpp
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)
	@ echo "-> "$(@F)
	@ $(SHELL) -ec '$(CC) $(DEFS) -M $(INCLUDES) $< | sed '\''s/\(.*\.o\)[ :]*/$(OBJDIR)\/\1 $(@F): /g'\'' > $@'

FORCE:
