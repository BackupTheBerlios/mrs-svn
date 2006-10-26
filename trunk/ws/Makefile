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
VERSION		= 1.2.0
TARGET		= mrsws
# DEBUG should be either 1 or 0
DEBUG		= 0
DEFINES		= CPU_$(CPU) MINI_H_LIB
WARNINGS	= -Wall -Wno-multichar
BOOST		= ../boost_1_33_0/
RDEFINES	= -DMACOS=0 -DUNIX=1
LIBS		= z expat m bz2 gsoap++
LIBPATHS	=  ../plugin/Lib/ /usr/local/lib/ ../plugin/
INCPATHS	= ../plugin/Lib/Includes/ ../plugin/Lib/X/ ../plugin/Lib/ ../plugin/Sources/
SYSINCPATHS	= /usr/include ../plugin/Lib/loki $(BOOST)
WARNINGS	=  -Wall -Wno-multichar -Wno-unknown-pragmas -Wno-long-long -Wno-sign-compare
CFLAGS		= -fsigned-char -g -ffor-scope
LDFLAGS		= -fdata-sections -g # -ffunction-sections 
CC			= c++
OPT			+= -fstrict-aliasing
SOAPCPP2	= soapcpp2

include make.config

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
	DEFINES		+= DARWIN
	LDFLAGS		+= 
	SYSINCPATHS	+= /opt/local/include
	LIBPATHS	+= /opt/local/lib
	CFLAGS		+= -force_cpusubtype_ALL
	OPT			+= -O3 -mcpu=G4 -falign-loops=32
#	WARNINGS	+= -Wno-long-double
	CTAGS		= /sw/bin/ctags
	STRIP		= strip -S
	JAVA_INC	= -I/System/Library/Frameworks/JavaVM.framework/Versions/1.4.2/Headers/
	SOAPCPP2	:= /opt/local/bin/soapcpp2
endif

ifeq ($(DEBUG),0)
	CFLAGS	+= $(OPT)
	OBJDIR	= Obj.NoDebug
	DEFINES	+= NDEBUG
	LIBS	+= mrs
else
#	DBG		+= -g
	OBJDIR	= Obj.Debug
	DEFINES	+= DEBUG
#	LIBS	+= efence
	LIBS	+= mrsd
endif

ifeq ($(PROFILE),1)
	LDFLAGS	+= -pg
	CFLAGS += -pg -g3
	OBJDIR	:= $(OBJDIR).Profile
	LIBNAME	:= $(LIBNAME)p
endif

LDFLAGS += $(addprefix -L, $(LIBPATHS)) 

Sources = \
	./Sources/mrsws.cpp \
	./Sources/WConfig.cpp \
	./Sources/soapC.cpp \
	./Sources/soapServer.cpp

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

$(TARGET): echo_flags Sources/soapC.cpp $(OBJS)
	@ echo "Linking "$(@F)
	$(CC) -o $(TARGET) $(LDFLAGS) $(OBJS)
	@ chmod a+rx $(TARGET)
	@ echo "Done"

clean: FORCE
	rm -rf $(OBJDIR)

dist: $(TARGET)

install: dist
	install mrsws ../web-simple/cgi-bin/
	install Sources/mrsws.wsdl ../web-simple/htdocs/

tags: FORCE
	$(CTAGS) $(wildcard $(addsuffix *, $(SRC_PATHS))) $(wildcard $(addsuffix *.h, $(INCPATHS)))

$(OBJDIR):
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)

$(OBJDIR)/%.o: %.cpp
	@ echo "=> "$(@F)
	@ $(CC) -c $< -o $@ $(INCLUDES) $(CFLAGS)

include $(OBJS:%.o=%.d)

Sources/soapC.cpp: Sources/mrsws.h
	$(SOAPCPP2) -Igsoap-2.7/soapcpp2/import -d Sources/ Sources/mrsws.h

$(OBJDIR)/%.d:%.cpp
	@ test -d $(OBJDIR) || mkdir $(OBJDIR)
	@ echo "-> "$(@F)
	@ $(SHELL) -ec '$(CC) $(DEFS) -M $(INCLUDES) $< | sed '\''s/\(.*\.o\)[ :]*/$(OBJDIR)\/\1 $(@F): /g'\'' > $@'

FORCE:
