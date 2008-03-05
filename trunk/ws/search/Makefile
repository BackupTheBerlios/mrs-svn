# make file for soap server

TARGET		= mrsws-search
# DEBUG should be either 1 or 0
DEBUG		= 0
LIBNAME		= mrs
LIBS		= z m bz2 gsoap++
LIBPATHS	=  ../../plugin/Lib/ ../../plugin/
INCPATHS	= ../../plugin/Lib/Includes/ ../../plugin/Lib/Posix/ ../../plugin/Lib/ ../../plugin/Sources/
SYSINCPATHS	= /usr/include
WARNINGS	= -Wall -Wno-multichar -Wno-unknown-pragmas
CFLAGS		= -fsigned-char -g -ffor-scope
CFLAGS		+= -fmessage-length=132 -fdiagnostics-show-location=every-line
LDFLAGS		= -fdata-sections -g

include ../../make.config

DEFINES     += CPU_$(CPU)

ifeq ($(OS),FreeBSD)
	DEFINES		+= HAS_ENDIAN FREEBSD HAVE_SYS_IOCTL_H HAVE_SYS_SOCKIO_H \
					HAVE_SYS_SOCKET_H HAVE_NETINET_IN_H HAVE_SRANDOM \
					HAVE_NET_IF_DL_H HAVE_SA_LEN HAVE_INTTYPES_H
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
	DEFINES		+= DARWIN HAVE_SYS_IOCTL_H HAVE_SYS_SOCKIO_H \
					HAVE_SYS_SOCKET_H HAVE_NETINET_IN_H HAVE_SRANDOM HAVE_NET_IF_DL_H HAVE_SA_LEN
	LD_SO_FLAG	= -bundle -undefined suppress -flat_namespace
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

LIBNAME := lib$(LIBNAME).a

Sources = \
	./Sources/$(TARGET).cpp \
	./Sources/soapC.cpp \
	./Sources/soapServer.cpp \
	../common/Sources/WConfig.cpp \
	../common/Sources/WFormat.cpp \
	../common/Sources/WUtils.cpp \
	../common/Sources/WError.cpp

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
INCLUDES += $(shell $(PERL) -MExtUtils::Embed -e perl_inc)

CFLAGS	+= $(addprefix -D, $(DEFINES)) $(DBG) $(WARNINGS)
ifeq ($(CPU), x86)
CFLAGS	+= -fPIC
endif

# add to the linker flags 
# add the -L prefix to all library paths to search
LDFLAGS += $(addprefix -L, $(LIBPATHS))
# add the -l prefix to all libs to be linked against
LDFLAGS += $(addprefix -l, $(LIBS))
# Perl's flags
LDFLAGS		+= $(shell $(PERL) -MExtUtils::Embed -e ldopts)

all: $(TARGET)

# build rules

echo_flags:
	 @ echo Compiler flags used: $(CFLAGS)
	 @ echo Include paths: $(INCLUDES)

$(TARGET): $(OBJDIR) Sources/soapH.h $(OBJS)
	$(MAKE) DEBUG=$(DEBUG) -C ../../plugin $(LIBNAME)
	@ echo "Linking "$(@F)
	@ $(CC) -o $(TARGET) ../../plugin/$(LIBNAME) $(OBJS) $(LDFLAGS)
	@ chmod a+rx $(TARGET)
	@ echo "Done"

clean: FORCE
	rm -rf $(OBJDIR)

dist: $(TARGET)

install: dist
	$(SUDO) install $(TARGET) $(BINDIR)/$(TARGET)
	@ echo "To finish installation of the web-service you have to edit the file"
	@ echo "ws/Sources/$(TARGET).wsdl and put it in the proper location."
#	install Sources/$(TARGET).wsdl ../web-simple/htdocs/

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

Sources/soapC.cpp Sources/soapServer.cpp Sources/soapH.h: Sources/$(TARGET).h
	$(SOAPCPP2) -d Sources/ Sources/$(TARGET).h

FORCE:
