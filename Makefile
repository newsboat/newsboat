
PACKAGE=newsbeuter

# important directories
prefix=/usr/local
datadir=$(prefix)/share
localedir=$(datadir)/locale
docdir=$(datadir)/doc/$(PACKAGE)

# compiler
CXX=c++

# compiler and linker flags
DEFINES=-D_ENABLE_NLS -DLOCALEDIR=\"$(localedir)\" -DPACKAGE=\"$(PACKAGE)\"
CXXFLAGS=-ggdb -I./include -I./stfl -I. -I/usr/local/include -I/sw/include -Wall $(DEFINES)
LDFLAGS=-L. -L/usr/local/lib -L/sw/lib

# libraries to link with
# LIBS=-lstfl -lmrss -lnxml -lncurses -lsqlite3 -lidn -lpthread

ifeq ($(DEBUG),1)
DEFINES+=-DDEBUG
endif

#SRC=$(wildcard *.cpp) $(wildcard src/*.cpp)
#OBJS=$(patsubst %.cpp,%.o,$(SRC))

LIB_SOURCES=$(shell cat libbeuter.deps)
LIB_OBJS=$(patsubst %.cpp,%.o,$(LIB_SOURCES))
LIB_OUTPUT=libbeuter.a

NEWSBEUTER=$(PACKAGE)
NEWSBEUTER_SOURCES=$(shell cat newsbeuter.deps)
NEWSBEUTER_OBJS=$(patsubst %.cpp,%.o,$(NEWSBEUTER_SOURCES))
NEWSBEUTER_LIBS=-lbeuter -lstfl -lmrss -lnxml -lncursesw -lsqlite3 -lpthread -lcurl


PODBEUTER=podbeuter
PODBEUTER_SOURCES=$(shell cat podbeuter.deps)
PODBEUTER_OBJS=$(patsubst %.cpp,%.o,$(PODBEUTER_SOURCES))
PODBEUTER_LIBS=-lbeuter -lstfl -lncursesw -lpthread -lcurl

ifneq ($(shell uname -s),Linux)
NEWSBEUTER_LIBS+=-liconv -lintl
PODBEUTER_LIBS+=-liconv -lintl
endif

# additional commands
MKDIR=mkdir -p
INSTALL=install
A2X=a2x
MSGFMT=msgfmt
RANLIB=ranlib
AR=ar

STFLHDRS=$(patsubst %.stfl,%.h,$(wildcard stfl/*.stfl))
POFILES=$(wildcard po/*.po)
MOFILES=$(patsubst %.po,%.mo,$(POFILES))
POTFILE=po/$(PACKAGE).pot

STFLCONV=./stfl2h.pl
RM=rm -f

all: $(NEWSBEUTER) $(PODBEUTER)

$(NEWSBEUTER): $(MOFILES) $(STFLHDRS) $(LIB_OUTPUT) $(NEWSBEUTER_OBJS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(NEWSBEUTER) $(NEWSBEUTER_OBJS) $(NEWSBEUTER_LIBS)

$(PODBEUTER): $(MOFILES) $(STFLHDRS) $(LIB_OUTPUT) $(PODBEUTER_OBJS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(PODBEUTER) $(PODBEUTER_OBJS) $(PODBEUTER_LIBS)

$(LIB_OUTPUT): $(LIB_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.h: %.stfl
	$(STFLCONV) $< > $@


testpp: src/xmlpullparser.cpp testpp.cpp
	$(CXX) -I./include -pg -g -D_TESTPP src/xmlpullparser.cpp testpp.cpp -o testpp

clean-newsbeuter:
	$(RM) $(NEWSBEUTER) $(NEWSBEUTER_OBJS)

clean-podbeuter:
	$(RM) $(PODBEUTER) $(PODBEUTER_OBJS)

clean-libbeuter:
	$(RM) $(LIB_OUTPUT) $(LIB_OBJS)

clean: clean-newsbeuter clean-podbeuter clean-libbeuter
	$(RM) $(STFLHDRS)

distclean: clean clean-mo
	$(RM) core *.core core.*

doc:
	$(MKDIR) doc/xhtml
	$(A2X) -f xhtml -d doc/xhtml doc/newsbeuter.txt

install: install-mo
	$(MKDIR) $(prefix)/bin
	$(INSTALL) $(NEWSBEUTER) $(prefix)/bin
	$(INSTALL) $(PODBEUTER) $(prefix)/bin
	$(MKDIR) $(prefix)/share/man/man1
	$(INSTALL) doc/$(NEWSBEUTER).1 $(prefix)/share/man/man1
	$(INSTALL) doc/$(PODBEUTER).1 $(prefix)/share/man/man1
	$(MKDIR) $(docdir)
	$(INSTALL) -m 644 doc/xhtml/* $(docdir) || true

uninstall:
	$(RM) $(prefix)/bin/$(NEWSBEUTER)
	$(RM) $(prefix)/bin/$(PODBEUTER)
	$(RM) $(prefix)/share/man/man1/$(NEWSBEUTER).1
	$(RM) $(prefix)/share/man/man1/$(PODBEUTER).1
	$(RM) -r $(docdir)

.PHONY: doc clean all test

# the following targets are i18n/l10n-related:

extract:
	$(RM) $(POTFILE)
	xgettext -k_ -o $(POTFILE) *.cpp src/*.cpp

msgmerge:
	for f in $(POFILES) ; do msgmerge -U $$f $(POTFILE) ; done

%.mo: %.po
	$(MSGFMT) --statistics -o $@ $<

clean-mo:
	$(RM) $(MOFILES) po/*~

install-mo:
	$(MKDIR) $(datadir)
	@for mof in $(MOFILES) ; do \
		mofile=`basename $$mof` ; \
		lang=`echo $$mofile | sed 's/\.mo$$//'`; \
		dir=$(localedir)/$$lang/LC_MESSAGES; \
		$(MKDIR) $$dir ; \
		$(INSTALL) -m 644 $$mof $$dir/$(PACKAGE).mo ; \
		echo "Installing $$mofile as $$dir/$(PACKAGE).mo" ; \
	done

TEST_OBJS=$(patsubst test/%.cpp,test/%.o,$(wildcard test/*.cpp))

test: $(LIB_OUTPUT) $(NEWSBEUTER_OBJS) $(TEST_OBJS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o test/test src/rss.o src/cache.o src/xmlpullparser.o $(TEST_OBJS) $(NEWSBEUTER_LIBS) -lboost_unit_test_framework

test-clean:
	$(RM) test/test test/test.o
