CXX=c++
CXXFLAGS=-ggdb -I./include -I./stfl -I. -I/usr/local/include -I/sw/include -Wall -pedantic
LDFLAGS=-L/usr/local/lib -L/sw/lib
LIBS=-lstfl -lmrss -lnxml -lncurses -lsqlite3 -lidn -lpthread
OUTPUT=newsbeuter
SRC=$(wildcard *.cpp) $(wildcard src/*.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRC))
prefix=/usr/local
docdir=$(prefix)/share/doc/$(OUTPUT)
MKDIR=mkdir -p
INSTALL=install
A2X=a2x

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
	$(RM) $(OUTPUT) $(OBJS) $(STFLHDRS) core *.core core.*
	$(RM) -rf doc/xhtml doc/*.xml

distclean: clean
	$(RM) Makefile.deps

doc:
	$(MKDIR) doc/xhtml
	$(A2X) -f xhtml -d doc/xhtml doc/newsbeuter.txt

install:
	$(MKDIR) $(prefix)/bin
	$(INSTALL) $(OUTPUT) $(prefix)/bin
	$(MKDIR) $(prefix)/share/man/man1
	$(INSTALL) doc/$(OUTPUT).1 $(prefix)/share/man/man1
	$(MKDIR) $(docdir)
	$(INSTALL) -m 644 doc/xhtml/* $(docdir)

uninstall:
	$(RM) $(prefix)/bin/$(OUTPUT)
	$(RM) $(prefix)/share/man/man1/$(OUTPUT).1

Makefile.deps: $(SRC)
	$(CXX) $(CXXFLAGS) -MM -MG $(SRC) > Makefile.deps

.PHONY: doc clean all

include Makefile.deps
