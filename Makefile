CXX=c++
CXXFLAGS=-g -I./include -I./stfl -I. -I/usr/local/include -I/sw/include -Wall
LDFLAGS=-L/usr/local/lib -L/sw/lib
LIBS=-lstfl -lmrss -lnxml -lncurses -lsqlite3 -lidn
OUTPUT=noos
SRC=$(wildcard *.cpp) $(wildcard src/*.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRC))
prefix=/usr/local
MKDIR=mkdir -p
INSTALL=install

STFLHDRS=$(patsubst %.stfl,%.h,$(wildcard stfl/*.stfl))

STFLCONV=./stfl2h.pl
RM=rm -f

all: $(OUTPUT)

$(OUTPUT): $(STFLHDRS) $(OBJS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(OUTPUT) $(OBJS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.h: %.stfl
	$(STFLCONV) $< > $@

testpp: src/xmlpullparser.cpp testpp.cpp
	$(CXX) -I./include -pg -g -D_TESTPP src/xmlpullparser.cpp testpp.cpp -o testpp

clean:
	$(RM) $(OUTPUT) $(OBJS) $(STFLHDRS) core *.core

distclean: clean
	$(RM) Makefile.deps

install:
	$(MKDIR) $(prefix)/bin
	$(INSTALL) $(OUTPUT) $(prefix)/bin

Makefile.deps: $(SRC)
	$(CXX) $(CXXFLAGS) -MM -MG $(SRC) > Makefile.deps

include Makefile.deps
