# important directories
prefix?=/usr/local
mandir?=$(prefix)/share/man
datadir?=$(prefix)/share
localedir?=$(datadir)/locale
docdir?=$(datadir)/doc/$(PACKAGE)

# compiler
CXX=c++
CC=cc

RUBY=ruby

# compiler and linker flags
DEFINES=-DLOCALEDIR=\"$(localedir)\"
WARNFLAGS=-Wall -Wextra
CXXFLAGS+=-ggdb -I/sw/include -I./include -I./stfl -I./filter -I. -I./xmlrss -I./rss $(WARNFLAGS) $(DEFINES)
CFLAGS+=-ggdb -I./xmlrss $(WARNFLAGS) $(DEFINES)
LDFLAGS+=-L. -L/sw/lib

include config.mk

LIB_SOURCES:=$(shell cat libbeuter.deps)
LIB_OBJS:=$(patsubst %.cpp,%.o,$(LIB_SOURCES))
LIB_OUTPUT=libbeuter.a

FILTERLIB_SOURCES=filter/Scanner.cpp filter/Parser.cpp filter/FilterParser.cpp
FILTERLIB_OBJS:=$(patsubst %.cpp,%.o,$(FILTERLIB_SOURCES))
FILTERLIB_OUTPUT=libfilter.a

NEWSBEUTER=newsbeuter
NEWSBEUTER_SOURCES:=$(shell cat newsbeuter.deps)
NEWSBEUTER_OBJS:=$(patsubst %.cpp,%.o,$(NEWSBEUTER_SOURCES))
NEWSBEUTER_LIBS=-lbeuter -lfilter -lstfl -lncursesw -lpthread -lxmlrss -lrsspp

XMLRSSLIB_SOURCES:=$(wildcard xmlrss/*.c)
XMLRSSLIB_OBJS:=$(patsubst xmlrss/%.c,xmlrss/%.o,$(XMLRSSLIB_SOURCES))
XMLRSSLIB_OUTPUT=libxmlrss.a

RSSPPLIB_SOURCES=$(wildcard rss/*.cpp)
RSSPPLIB_OBJS=$(patsubst rss/%.cpp,rss/%.o,$(RSSPPLIB_SOURCES))
RSSPPLIB_OUTPUT=librsspp.a


PODBEUTER=podbeuter
PODBEUTER_SOURCES:=$(shell cat podbeuter.deps)
PODBEUTER_OBJS:=$(patsubst %.cpp,%.o,$(PODBEUTER_SOURCES))
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

STFLHDRS:=$(patsubst %.stfl,%.h,$(wildcard stfl/*.stfl))
POFILES:=$(wildcard po/*.po)
MOFILES:=$(patsubst %.po,%.mo,$(POFILES))
POTFILE=po/$(PACKAGE).pot

STFLCONV=./stfl2h.pl
RM=rm -f

all: $(NEWSBEUTER) $(PODBEUTER)

NB_DEPS=$(MOFILES) $(XMLRSSLIB_OUTPUT) $(LIB_OUTPUT) $(FILTERLIB_OUTPUT) $(NEWSBEUTER_OBJS) $(RSSPPLIB_OUTPUT)

$(NEWSBEUTER): $(NB_DEPS)
	$(CXX) $(CXXFLAGS) -o $(NEWSBEUTER) $(NEWSBEUTER_OBJS) $(NEWSBEUTER_LIBS) $(LDFLAGS)

$(PODBEUTER): $(MOFILES) $(LIB_OUTPUT) $(PODBEUTER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PODBEUTER) $(PODBEUTER_OBJS) $(PODBEUTER_LIBS) $(LDFLAGS)

$(LIB_OUTPUT): $(LIB_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

$(XMLRSSLIB_OUTPUT): $(XMLRSSLIB_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

$(RSSPPLIB_OUTPUT): $(RSSPPLIB_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

$(FILTERLIB_OUTPUT): $(FILTERLIB_OBJS)
	$(RM) $@
	$(AR) qc $@ $^
	$(RANLIB) $@

filter/Scanner.cpp filter/Parser.cpp: filter/filter.atg filter/Scanner.frame filter/Parser.frame
	$(RM) filter/Scanner.cpp filter/Parser.cpp filter/Scanner.h filter/Parser.h
	cococpp -frames filter $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.h: %.stfl
	$(STFLCONV) $< > $@


clean-newsbeuter:
	$(RM) $(NEWSBEUTER) $(NEWSBEUTER_OBJS)

clean-podbeuter:
	$(RM) $(PODBEUTER) $(PODBEUTER_OBJS)

clean-libbeuter:
	$(RM) $(LIB_OUTPUT) $(LIB_OBJS)

clean-libxmlrss:
	$(RM) $(XMLRSSLIB_OUTPUT) $(XMLRSSLIB_OBJS)

clean-librsspp:
	$(RM) $(RSSPPLIB_OUTPUT) $(RSSPPLIB_OBJS)

clean-libfilter:
	$(RM) $(FILTERLIB_OUTPUT) $(FILTERLIB_OBJS) filter/Scanner.cpp filter/Scanner.h filter/Parser.cpp filter/Parser.h

clean-doc:
	$(RM) -r doc/xhtml 
	$(RM) doc/*.xml doc/*.1 doc/newsbeuter-cfgcmds.txt doc/podbeuter-cfgcmds.txt doc/newsbeuter-keycmds.txt

clean: clean-newsbeuter clean-podbeuter clean-libbeuter clean-libfilter clean-doc clean-libxmlrss clean-librsspp
	$(RM) $(STFLHDRS)

distclean: clean clean-mo test-clean
	$(RM) core *.core core.* config.mk

doc:
	$(MKDIR) doc/xhtml
	$(A2X) -f xhtml -D doc/xhtml doc/newsbeuter.txt
	doc/generate.pl doc/configcommands.dsv > doc/newsbeuter-cfgcmds.txt
	doc/generate2.pl doc/keycmds.dsv > doc/newsbeuter-keycmds.txt
	$(A2X) -f manpage -D doc doc/manpage-newsbeuter.txt
	doc/generate.pl doc/podbeuter-cmds.dsv > doc/podbeuter-cfgcmds.txt
	$(A2X) -f manpage -D doc doc/manpage-podbeuter.txt

install: install-mo
	$(MKDIR) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(NEWSBEUTER) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(PODBEUTER) $(DESTDIR)$(prefix)/bin
	$(MKDIR) $(DESTDIR)$(mandir)/man1
	$(INSTALL) doc/$(NEWSBEUTER).1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) doc/$(PODBEUTER).1 $(DESTDIR)$(mandir)/man1
	$(MKDIR) $(DESTDIR)$(docdir)
	$(INSTALL) -m 644 doc/xhtml/* $(DESTDIR)$(docdir) || true
	$(MKDIR) $(DESTDIR)$(docdir)/examples
	$(INSTALL) -m 644 doc/example-config $(DESTDIR)$(docdir)/examples/config || true

uninstall:
	$(RM) $(DESTDIR)$(prefix)/bin/$(NEWSBEUTER)
	$(RM) $(DESTDIR)$(prefix)/bin/$(PODBEUTER)
	$(RM) $(DESTDIR)$(mandir)/man1/$(NEWSBEUTER).1
	$(RM) $(DESTDIR)$(mandir)/man1/$(PODBEUTER).1
	$(RM) -r $(DESTDIR)$(docdir)

.PHONY: doc clean all test install uninstall

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
	$(MKDIR) $(DESTDIR)$(datadir)
	@for mof in $(MOFILES) ; do \
		mofile=`basename $$mof` ; \
		lang=`echo $$mofile | sed 's/\.mo$$//'`; \
		dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
		$(MKDIR) $$dir ; \
		$(INSTALL) -m 644 $$mof $$dir/$(PACKAGE).mo ; \
		echo "Installing $$mofile as $$dir/$(PACKAGE).mo" ; \
	done

test: $(LIB_OUTPUT) $(NEWSBEUTER_OBJS) test/test.o
	$(CXX) $(CXXFLAGS) -o test/test src/history.o src/rss.o src/rss_parser.o src/htmlrenderer.o src/cache.o src/tagsouppullparser.o src/urlreader.o src/regexmanager.o test/test.o $(NEWSBEUTER_LIBS) -lboost_unit_test_framework $(LDFLAGS)

test-rss: $(RSSPPLIB_OUTPUT) test/test-rss.o
	$(CXX) $(CXXFLAGS) -o test/test-rss test/test-rss.o src/utils.o $(NEWSBEUTER_LIBS) -lboost_unit_test_framework $(LDFLAGS)

test/test.o: test/test.cpp
	$(CXX) $(CXXFLAGS) $(RUBYCXXFLAGS) -o $@ -c $<

test-clean:
	$(RM) test/test test/test.o test/test-rss test/test-rss.o

config: config.mk

config.mk:
	@./config.sh

include mk.deps
