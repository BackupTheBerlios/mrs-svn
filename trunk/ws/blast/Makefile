# make file for soap server

SO_EXT		= .so
TARGET		= mrsws-blast
# DEBUG should be either 1 or 0
DEBUG		= 0
DEFINES		= CPU_$(CPU) MINI_H_LIB
WARNINGS	= -Wall -Wno-multichar
LIBNAME		= mrs
LIBS		= z m bz2 gsoap++
LIBPATHS	=  ../../plugin/Lib/ ../../plugin/
INCPATHS	= ../../plugin/Lib/Includes/ ../../plugin/Lib/Posix/ ../../plugin/Lib/ ../../plugin/Sources/
SYSINCPATHS	= /usr/include
WARNINGS	= -Wall -Wno-multichar -Wno-unknown-pragmas
CFLAGS		= -fsigned-char -g -ffor-scope
CFLAGS		+= -fmessage-length=132 -fdiagnostics-show-location=every-line
LDFLAGS		= -fdata-sections -g # -ffunction-sections

include ../../make.config

LDFLAGS		+= $(shell $(PERL) -MExtUtils::Embed -e ldopts)

ifeq ($(OS),FreeBSD)
	LIBS		+= 
	INCPATHS	+= 
	SYSINCPATHS	+= /usr/local/include
	DEFINES		+= HAS_ENDIAN FREEBSD HAVE_SYS_IOCTL_H HAVE_SYS_SOCKIO_H \
					HAVE_SYS_SOCKET_H HAVE_NETINET_IN_H HAVE_SRANDOM HAVE_NET_IF_DL_H HAVE_SA_LEN
	LDFLAGS		+= -shared
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

LDFLAGS += $(addprefix -L, $(LIBPATHS)) 
LIBNAME := lib$(LIBNAME).a

Sources = \
	./Sources/mrsws-blast.cpp \
	./Sources/soapC.cpp \
	./Sources/soapServer.cpp \
	./Sources/CJob.cpp \
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

# since we're using c++ the following flag can safely be removed:
INCLUDES := $(filter-out -Wdeclaration-after-statement, $(INCLUDES))

ifneq ($(IQUOTE),0)
INCLUDES += -iquote
else
INCLUDES += -I-
endif
INCLUDES += $(foreach path, $(SYSINCPATHS) $(BOOST), $(addprefix -I, $(path)))

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

$(TARGET): $(OBJDIR) Sources/soapH.h $(OBJS)
	$(MAKE) DEBUG=$(DEBUG) -C ../../plugin $(LIBNAME)
	@ echo "Linking "$(@F)
	$(CC) -o $(TARGET) ../../plugin/$(LIBNAME) $(OBJS) $(LDFLAGS)
	@ chmod a+rx $(TARGET)
	@ echo "Done"

clean: FORCE
	rm -rf $(OBJDIR)

dist: $(TARGET)

install: dist
	$(SUDO) install mrsws-blast $(INSTALL_DIR)/mrsws-blast
	@ echo "To finish installation of the web-service you have to edit the file"
	@ echo "ws/Sources/mrsws-blast.wsdl and put it in the proper location."
#	install Sources/mrsws-blast.wsdl ../web-simple/htdocs/

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

Sources/soapC.cpp Sources/soapServer.cpp Sources/soapH.h: Sources/mrsws-blast.h
	$(SOAPCPP2) -d Sources/ Sources/mrsws-blast.h

FORCE:
