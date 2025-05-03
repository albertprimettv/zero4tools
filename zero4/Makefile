IDIR := src
SRCDIR := src
ODIR := obj
LDIR :=

BLACKTDIR = ../blackt
LIBPCEDIR = ../libpce

AR=ar
CXX=g++
# Compile only
CDEFINES = -DBLACKT_ENABLE_LIBPNG
#CLIBS = -lpng
CFLAGS = -std=gnu++11 -O2 -Wall -L${LIBPCEDIR} -lpce -L${BLACKTDIR} -lblackt -lpng
CINCLUDES = -I${BLACKTDIR}/src -I${LIBPCEDIR}/src
CXXFLAGS=$(CFLAGS) $(CDEFINES) $(CINCLUDES) -I$(IDIR)

LIBDEPS := $(LIBPCEDIR)/libpce.a $(BLACKTDIR)/libblackt.a

TOOLSRCS := $(wildcard $(SRCDIR)/*.cpp)
TOOLSINDIR := $(patsubst %.cpp,%,$(TOOLSRCS))
TOOLS := $(notdir $(TOOLSINDIR))

.SECONDEXPANSION:

all: $(BLACKTDIR)/libblackt.a $(LIBPCEDIR)/libpce.a $(TOOLS)

blackt:
	$(MAKE) -C${BLACKTDIR}

libpce:
	$(MAKE) -C${LIBPCEDIR}

$(BLACKTDIR)/libblackt.a: $(BLACKTDIR)/src/**/*.cpp
	$(MAKE) -C ${BLACKTDIR} all

$(LIBPCEDIR)/libpce.a: $(LIBPCEDIR)/src/**/*.cpp
	$(MAKE) -C ${LIBPCEDIR} all

$(TOOLS): $(SRCDIR)/$$@.cpp $(LIBDEPS)
	$(MAKE) blackt
	$(MAKE) libpce
	$(CXX) $(SRCDIR)/$@.cpp $(OBJ) -o $(notdir $@) $(CXXFLAGS)

.PHONY: blackt libpce cleanme clean

cleanme:
	rm -f $(TOOLS)

clean: cleanme
#	rm -f $(LIB)
#	rm -rf $(ODIR)
	$(MAKE) -C ${BLACKTDIR} clean
	$(MAKE) -C ${LIBPCEDIR} clean
