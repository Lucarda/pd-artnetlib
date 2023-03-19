# library name
lib.name = artnetlib


artnetlib.class.sources = \
    artnetfromarray.c \
    artnetsend.c \
    artnetudp.c \
    artnetroute.c \
    artnettoarray.c \
    artnetsetup.c \

datafiles = \
    README.md \
    CHANGELOG.txt \
    LICENSE.txt \
    $(wildcard *.pd) \

define forWindows
    ldlibs += -lws2_32 -liphlpapi
endef

# include Makefile.pdlibbuilder
PDLIBBUILDER_DIR=./pd-lib-builder
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder