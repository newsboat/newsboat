PACKAGE=newsbeuter

# important directories
prefix?=/usr/local
mandir?=$(prefix)/share/man
datadir?=$(prefix)/share
localedir?=$(datadir)/locale
docdir=?$(datadir)/doc/$(PACKAGE)

# compiler
CXX=c++

SWIG=swig
RUBY=ruby

# compiler and linker flags
DEFINES=-D_ENABLE_NLS -DLOCALEDIR=\"$(localedir)\" -DPACKAGE=\"$(PACKAGE)\"
WARNFLAGS=-Wall -W
CXXFLAGS+=-ggdb -I./include -I./stfl -I./filter -I. -I./xmlrss $(WARNFLAGS) $(DEFINES)
CFLAGS+=-ggdb $(WARNFLAGS) $(DEFINES)
LDFLAGS+=-L.

include config.mk

SWIGFLAGS=-c++ -ruby

ifeq ($(DEBUG),1)
DEFINES+=-DDEBUG
endif

LIB_SOURCES=$(shell cat libbeuter.deps)
LIB_OBJS=$(patsubst %.cpp,%.o,$(LIB_SOURCES))
LIB_OUTPUT=libbeuter.a

FILTERLIB_SOURCES=filter/Scanner.cpp filter/Parser.cpp filter/FilterParser.cpp
FILTERLIB_OBJS=$(patsubst %.cpp,%.o,$(FILTERLIB_SOURCES))
FILTERLIB_OUTPUT=libfilter.a

NEWSBEUTER=$(PACKAGE)
NEWSBEUTER_SOURCES=$(shell cat newsbeuter.deps)
NEWSBEUTER_OBJS=$(patsubst %.cpp,%.o,$(NEWSBEUTER_SOURCES))
NEWSBEUTER_LIBS=-lbeuter -lfilter -lstfl -lncursesw -lpthread -lxmlrss

ifeq ($(FOUND_RUBY),1)
NEWSBEUTER_LIBS+=-lext -lruby1.8
CXXFLAGS+=-DHAVE_RUBY=1
endif

EXTLIB_IFILES=$(wildcard swig/*.i)
EXTLIB_SOURCES=$(patsubst swig/%.i,swig/%_wrap.cxx,$(EXTLIB_IFILES))
EXTLIB_OBJS=$(patsubst swig/%.cxx,swig/%.o,$(EXTLIB_SOURCES))
EXTLIB_OUTPUT=libext.a

XMLRSSLIB_SOURCES=$(wildcard xmlrss/*.c)
XMLRSSLIB_OBJS=$(patsubst xmlrss/%.c,xmlrss/%.o,$(XMLRSSLIB_SOURCES))
XMLRSSLIB_OUTPUT=libxmlrss.a


PODBEUTER=podbeuter
PODBEUTER_SOURCES=$(shell cat podbeuter.deps)
PODBEUTER_OBJS=$(patsubst %.cpp,%.o,$(PODBEUTER_SOURCES))
PODBEUTER_LIBS=-lbeuter -lstfl -lncursesw -lpthread

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

NB_DEPS=$(MOFILES) $(XMLRSSLIB_OUTPUT) $(LIB_OUTPUT) $(FILTERLIB_OUTPUT) $(NEWSBEUTER_OBJS)

ifeq ($(FOUND_RUBY),1)
NB_DEPS+=$(EXTLIB_OUTPUT)
endif


$(NEWSBEUTER): $(NB_DEPS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(NEWSBEUTER) $(NEWSBEUTER_OBJS) $(NEWSBEUTER_LIBS)

$(PODBEUTER): $(MOFILES) $(LIB_OUTPUT) $(PODBEUTER_OBJS)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(PODBEUTER) $(PODBEUTER_OBJS) $(PODBEUTER_LIBS)

$(LIB_OUTPUT): $(LIB_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

$(XMLRSSLIB_OUTPUT): $(XMLRSSLIB_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

$(EXTLIB_OUTPUT): $(EXTLIB_SOURCES) prepare_extlib
	$(MAKE) -C swig CFLAGS+="-I../include -I../filter -I../xmlrss" $(patsubst swig/%,%,$(EXTLIB_OBJS))
	$(RM) $@
	$(AR) qc $@ $(EXTLIB_OBJS)
	$(RANLIB) $@

prepare_extlib:
	cd swig && $(RUBY) extconf.rb

$(FILTERLIB_OUTPUT): $(FILTERLIB_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

#filter/Scanner.cpp filter/Parser.cpp: filter/filter.atg filter/Scanner.frame filter/Parser.frame
#	$(RM) filter/Scanner.cpp filter/Parser.cpp filter/Scanner.h filter/Parser.h
#	cococpp -frames filter $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<


# we need extra rules so that we don't have a clash between the libc's regex.h and Ruby's regex.h
src/interpreter.o: src/interpreter.cpp
	$(CXX) $(CXXFLAGS) $(RUBYCXXFLAGS) -o $@ -c $<

src/controller.o: src/controller.cpp
	$(CXX) $(CXXFLAGS) $(RUBYCXXFLAGS) -o $@ -c $<

src/formaction.o: src/formaction.cpp
	$(CXX) $(CXXFLAGS) $(RUBYCXXFLAGS) -o $@ -c $<



swig/%.o: swig/%.cxx
	$(CXX) $(CXXFLAGS) -o $@ -c $<

swig/%_wrap.cxx: swig/%.i
	$(SWIG) $(SWIGFLAGS) $<

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

clean-libext:
	$(RM) $(EXTLIB_OUTPUT) $(EXTLIB_OBJS) $(wildcard swig/*.cxx) swig/Makefile

clean-libxmlrss:
	$(RM) $(XMLRSSLIB_OUTPUT) $(XMLRSSLIB_OBJS)

clean-libfilter:
	$(RM) $(FILTERLIB_OUTPUT) $(FILTERLIB_OBJS) # filter/Scanner.cpp filter/Scanner.h filter/Parser.cpp filter/Parser.h

#clean-doc:
#	$(RM) -r doc/xhtml 
#	$(RM) doc/*.xml doc/*.1 doc/newsbeuter-cfgcmds.txt doc/podbeuter-cfgcmds.txt

clean: clean-newsbeuter clean-podbeuter clean-libbeuter clean-libfilter clean-libext clean-libxmlrss
	$(RM) $(STFLHDRS)

distclean: clean clean-mo test-clean
	$(RM) core *.core core.* config.mk

doc:
	$(MKDIR) doc/xhtml
	$(A2X) -f xhtml -D doc/xhtml doc/newsbeuter.txt
	doc/generate.pl doc/configcommands.dsv > doc/newsbeuter-cfgcmds.txt
	$(A2X) -f manpage -D doc doc/manpage-newsbeuter.txt
	doc/generate.pl doc/podbeuter-cmds.dsv > doc/podbeuter-cfgcmds.txt
	$(A2X) -f manpage -D doc doc/manpage-podbeuter.txt

install: install-mo
	$(MKDIR) $(prefix)/bin
	$(INSTALL) $(NEWSBEUTER) $(prefix)/bin
	$(INSTALL) $(PODBEUTER) $(prefix)/bin
	$(MKDIR) $(prefix)/share/man/man1
	$(INSTALL) doc/$(NEWSBEUTER).1 $(mandir)/man1
	$(INSTALL) doc/$(PODBEUTER).1 $(mandir)/man1
	$(MKDIR) $(docdir)
	$(INSTALL) -m 644 doc/xhtml/* $(docdir) || true
	$(MKDIR) $(docdir)/examples
	$(INSTALL) -m 644 doc/example-config $(docdir)/examples/config || true
	$(MKDIR) $(docdir)/contrib
	$(INSTALL) -m 644 contrib/bookmark-delicious.sh $(docdir)/contrib/bookmark-delicious.sh || true
	$(INSTALL) -m 644 contrib/fixwesnoth.xsl $(docdir)/contrib/fixwesnoth.xsl || true

uninstall:
	$(RM) $(prefix)/bin/$(NEWSBEUTER)
	$(RM) $(prefix)/bin/$(PODBEUTER)
	$(RM) $(mandir)/man1/$(NEWSBEUTER).1
	$(RM) $(mandir)/man1/$(PODBEUTER).1
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
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(RUBYCXXFLAGS) -o test/test src/history.o src/rss.o src/htmlrenderer.o src/cache.o src/xmlpullparser.o src/urlreader.o $(TEST_OBJS) $(NEWSBEUTER_LIBS) -lboost_unit_test_framework

test/test.o: test/test.cpp
	$(CXX) $(CXXFLAGS) $(RUBYCXXFLAGS) -o $@ -c $<

test-clean:
	$(RM) test/test test/test.o

config: config.mk

config.mk:
	@./config.sh

include mk.deps
