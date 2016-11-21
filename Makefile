# important directories
prefix?=/usr/local
mandir?=$(prefix)/share/man
datadir?=$(prefix)/share
localedir?=$(datadir)/locale
docdir?=$(datadir)/doc/$(PACKAGE)

CPPCHECK_JOBS?=5

# compiler
CXX?=c++

# compiler and linker flags
DEFINES=-DLOCALEDIR=\"$(localedir)\"

ifneq ($(wildcard .git/.),)
GIT_HASH=$(shell git describe --abbrev=4 --dirty --always --tags)
DEFINES+=-DGIT_HASH=\"$(GIT_HASH)\"
endif

WARNFLAGS=-Werror -Wall -Wextra -Wunreachable-code
INCLUDES=-Iinclude -Istfl -Ifilter -I. -Irss
BARE_CXXFLAGS=-std=c++11 -ggdb $(INCLUDES)
CXXFLAGS+=$(BARE_CXXFLAGS) $(WARNFLAGS) $(DEFINES)
LDFLAGS+=-L. -fprofile-arcs -ftest-coverage

PACKAGE=newsbeuter

ifneq (distclean, $(MAKECMDGOALS))
include config.mk
endif

ifeq ($(PROFILE),1)
CXXFLAGS+=-fprofile-arcs -ftest-coverage
endif

LIB_SOURCES:=$(shell cat mk/libbeuter.deps)
LIB_OBJS:=$(patsubst %.cpp,%.o,$(LIB_SOURCES))
LIB_OUTPUT=libbeuter.a

FILTERLIB_SOURCES=filter/Scanner.cpp filter/Parser.cpp filter/FilterParser.cpp
FILTERLIB_OBJS:=$(patsubst %.cpp,%.o,$(FILTERLIB_SOURCES))
FILTERLIB_OUTPUT=libfilter.a

NEWSBEUTER=newsbeuter
NEWSBEUTER_SOURCES:=$(shell cat mk/newsbeuter.deps)
NEWSBEUTER_OBJS:=$(patsubst %.cpp,%.o,$(NEWSBEUTER_SOURCES))
NEWSBEUTER_LIBS=-lbeuter -lfilter -lpthread -lrsspp

RSSPPLIB_SOURCES=$(wildcard rss/*.cpp)
RSSPPLIB_OBJS=$(patsubst rss/%.cpp,rss/%.o,$(RSSPPLIB_SOURCES))
RSSPPLIB_OUTPUT=librsspp.a


PODBEUTER=podbeuter
PODBEUTER_SOURCES:=$(shell cat mk/podbeuter.deps)
PODBEUTER_OBJS:=$(patsubst %.cpp,%.o,$(PODBEUTER_SOURCES))
PODBEUTER_LIBS=-lbeuter -lpthread

ifeq (, $(filter Linux GNU GNU/%, $(shell uname -s)))
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
POTFILE=po/newsbeuter.pot

TEXTCONV=./txt2h.pl
RM=rm -f

all: $(NEWSBEUTER) $(PODBEUTER) mo-files

NB_DEPS=$(LIB_OUTPUT) $(FILTERLIB_OUTPUT) $(NEWSBEUTER_OBJS) $(RSSPPLIB_OUTPUT)

$(NEWSBEUTER): $(NB_DEPS)
	$(CXX) $(CXXFLAGS) -o $(NEWSBEUTER) $(NEWSBEUTER_OBJS) $(NEWSBEUTER_LIBS) $(LDFLAGS)

$(PODBEUTER): $(LIB_OUTPUT) $(PODBEUTER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PODBEUTER) $(PODBEUTER_OBJS) $(PODBEUTER_LIBS) $(LDFLAGS)

$(LIB_OUTPUT): $(LIB_OBJS)
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

regenerate-parser:
	$(RM) filter/Scanner.cpp filter/Parser.cpp filter/Scanner.h filter/Parser.h
	cococpp -frames filter filter/filter.atg

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

%.h: %.stfl
	$(TEXTCONV) $< .stfl > $@

clean-newsbeuter:
	$(RM) $(NEWSBEUTER) $(NEWSBEUTER_OBJS)

clean-podbeuter:
	$(RM) $(PODBEUTER) $(PODBEUTER_OBJS)

clean-libbeuter:
	$(RM) $(LIB_OUTPUT) $(LIB_OBJS)

clean-librsspp:
	$(RM) $(RSSPPLIB_OUTPUT) $(RSSPPLIB_OBJS)

clean-libfilter:
	$(RM) $(FILTERLIB_OUTPUT) $(FILTERLIB_OBJS)

clean-doc:
	$(RM) -r doc/xhtml 
	$(RM) doc/*.xml doc/*.1 doc/newsbeuter-cfgcmds.txt doc/podbeuter-cfgcmds.txt doc/newsbeuter-keycmds.txt

clean: clean-newsbeuter clean-podbeuter clean-libbeuter clean-libfilter clean-doc clean-librsspp
	$(RM) $(STFLHDRS) xlicense.h

distclean: clean clean-mo test-clean profclean
	$(RM) core *.core core.* config.mk

doc: doc/xhtml/newsbeuter.html doc/xhtml/faq.html doc/newsbeuter.1 doc/podbeuter.1

doc/xhtml/newsbeuter.html: doc/newsbeuter.txt
	$(MKDIR) doc/xhtml
	$(A2X) -f xhtml -D doc/xhtml doc/newsbeuter.txt

doc/xhtml/faq.html: doc/faq.txt
	$(MKDIR) doc/xhtml
	$(A2X) -f xhtml -D doc/xhtml doc/faq.txt

doc/newsbeuter-cfgcmds.txt: doc/generate.pl doc/configcommands.dsv
	doc/generate.pl doc/configcommands.dsv > doc/newsbeuter-cfgcmds.txt

doc/newsbeuter-keycmds.txt: doc/generate2.pl doc/keycmds.dsv
	doc/generate2.pl doc/keycmds.dsv > doc/newsbeuter-keycmds.txt

doc/newsbeuter.1: doc/manpage-newsbeuter.txt doc/newsbeuter-cfgcmds.txt doc/newsbeuter-keycmds.txt
	$(A2X) -f manpage doc/manpage-newsbeuter.txt

doc/podbeuter-cfgcmds.txt: doc/generate.pl doc/podbeuter-cmds.dsv
	doc/generate.pl doc/podbeuter-cmds.dsv > doc/podbeuter-cfgcmds.txt

doc/podbeuter.1: doc/manpage-podbeuter.txt doc/podbeuter-cfgcmds.txt
	$(A2X) -f manpage doc/manpage-podbeuter.txt

fmt:
	astyle --suffix=none --style=java --indent=tab --indent-classes *.cpp include/*.h src/*.cpp rss/*.{cpp,h} test/*.cpp

cppcheck:
	cppcheck -j$(CPPCHECK_JOBS) --force --enable=all --suppress=unusedFunction \
		-DDEBUG=1 \
		$(INCLUDES) $(DEFINES) \
		include filter newsbeuter.cpp podbeuter.cpp rss src stfl \
		test/*.cpp test/*.h \
		2>cppcheck.log
	@echo "Done! See cppcheck.log for details."

install-newsbeuter: $(NEWSBEUTER)
	$(MKDIR) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(NEWSBEUTER) $(DESTDIR)$(prefix)/bin
	$(MKDIR) $(DESTDIR)$(mandir)/man1
	$(INSTALL) doc/$(NEWSBEUTER).1 $(DESTDIR)$(mandir)/man1 || true

install-podbeuter: $(PODBEUTER)
	$(MKDIR) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(PODBEUTER) $(DESTDIR)$(prefix)/bin
	$(MKDIR) $(DESTDIR)$(mandir)/man1
	$(INSTALL) doc/$(PODBEUTER).1 $(DESTDIR)$(mandir)/man1 || true

install-docs:
	$(MKDIR) $(DESTDIR)$(docdir)
	$(INSTALL) -m 644 doc/xhtml/* $(DESTDIR)$(docdir) || true

install-examples:
	$(MKDIR) $(DESTDIR)$(docdir)/examples
	$(INSTALL) -m 644 doc/example-config $(DESTDIR)$(docdir)/examples/config || true

install: install-newsbeuter install-podbeuter install-docs install-examples install-mo

uninstall: uninstall-mo
	$(RM) $(DESTDIR)$(prefix)/bin/$(NEWSBEUTER)
	$(RM) $(DESTDIR)$(prefix)/bin/$(PODBEUTER)
	$(RM) $(DESTDIR)$(mandir)/man1/$(NEWSBEUTER).1
	$(RM) $(DESTDIR)$(mandir)/man1/$(PODBEUTER).1
	$(RM) -rf $(DESTDIR)$(docdir)
	$(RM) -r $(DESTDIR)$(docdir)

.PHONY: doc clean distclean all test test-rss extract install uninstall regenerate-parser clean-newsbeuter \
	clean-podbeuter clean-libbeuter clean-librsspp clean-libfilter clean-doc install-mo msgmerge clean-mo \
	test-clean config cppcheck

# the following targets are i18n/l10n-related:

mo-files: $(MOFILES)

extract:
	$(RM) $(POTFILE)
	xgettext -c/ -k_ -k_s -o $(POTFILE) *.cpp src/*.cpp rss/*.cpp

msgmerge:
	for f in $(POFILES) ; do msgmerge -U $$f $(POTFILE) ; done

%.mo: %.po
	$(MSGFMT) --statistics -o $@ $<

clean-mo:
	$(RM) $(MOFILES) po/*~

install-mo: mo-files
	$(MKDIR) $(DESTDIR)$(datadir)
	@for mof in $(MOFILES) ; do \
		mofile=`basename $$mof` ; \
		lang=`echo $$mofile | sed 's/\.mo$$//'`; \
		dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
		$(MKDIR) $$dir ; \
		$(INSTALL) -m 644 $$mof $$dir/$(PACKAGE).mo ; \
		echo "Installing $$mofile as $$dir/$(PACKAGE).mo" ; \
	done

uninstall-mo:
	@for mof in $(MOFILES) ; do \
		mofile=`basename $$mof` ; \
		lang=`echo $$mofile | sed 's/\.mo$$//'`; \
		dir=$(DESTDIR)$(localedir)/$$lang/LC_MESSAGES; \
		$(RM) -f $$dir/$(PACKAGE).mo ; \
		echo "Uninstalling $$dir/$(PACKAGE).mo" ; \
	done

# tests and coverage reports

test: test/test

TEST_SRCS:=$(shell ls test/*.cpp)
TEST_OBJS:=$(patsubst %.cpp,%.o,$(TEST_SRCS))
test/test: $(LIB_OUTPUT) $(NEWSBEUTER_OBJS) $(FILTERLIB_OUTPUT) $(RSSPPLIB_OUTPUT) $(TEST_OBJS) test/test-helpers.h
	$(CXX) $(CXXFLAGS) -o test/test $(TEST_OBJS) src/*.o $(NEWSBEUTER_LIBS) $(LDFLAGS)

test-clean:
	$(RM) test/test test/*.o

profclean:
	find . -name '*.gc*' -type f -print0 | xargs -0 $(RM) --
	$(RM) app*.info

# miscellaneous stuff

config: config.mk

config.mk:
	@./config.sh

xlicense.h: LICENSE
	$(TEXTCONV) $< > $@

ALL_SRCS:=$(shell ls filter/*.cpp rss/*.cpp src/*.cpp test/*.cpp)
ALL_HDRS:=$(shell ls filter/*.h rss/*.h test/*.h test/*.hpp) $(STFLHDRS) xlicense.h
depslist: $(ALL_SRCS) $(ALL_HDRS)
	> mk/mk.deps
	for dir in filter rss src test ; do \
		for file in $$dir/*.cpp ; do \
			target=`echo $$file | sed 's/cpp$$/o/'`; \
			$(CXX) $(BARE_CXXFLAGS) -MM -MG -MQ $$target $$file >> mk/mk.deps ; \
			echo $$file ; \
		done; \
	done

include mk/mk.deps
