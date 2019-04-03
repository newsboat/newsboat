# important directories
prefix?=/usr/local
mandir?=$(prefix)/share/man
datadir?=$(prefix)/share
localedir?=$(datadir)/locale
docdir?=$(datadir)/doc/$(PACKAGE)

CPPCHECK_JOBS?=5

# compiler
CXX?=c++
# Compiler for building executables meant to be run on the
# host system during cross compilation
CXX_FOR_BUILD?=$(CXX)

# compiler and linker flags
DEFINES=-DLOCALEDIR=\"$(localedir)\"

ifneq ($(wildcard .git/.),)
GIT_HASH:=$(shell git describe --abbrev=4 --dirty --always --tags)
DEFINES+=-DGIT_HASH=\"$(GIT_HASH)\"
endif

WARNFLAGS=-Werror -Wall -Wextra -Wunreachable-code
INCLUDES=-Iinclude -Istfl -Ifilter -I. -Irss
BARE_CXXFLAGS=-std=c++11 -O2 -ggdb $(INCLUDES)
LDFLAGS+=-L.

PACKAGE=newsboat

ifeq (, $(filter $(MAKECMDGOALS),distclean run-i18nspector))
include config.mk
endif

ifeq ($(PROFILE),1)
BARE_CXXFLAGS+=-O0 -fprofile-arcs -ftest-coverage
LDFLAGS+=-fprofile-arcs -ftest-coverage
endif

CXXFLAGS:=$(BARE_CXXFLAGS) $(WARNFLAGS) $(DEFINES) $(CXXFLAGS)
CXXFLAGS_FOR_BUILD?=$(CXXFLAGS)

LIB_SOURCES:=$(shell cat mk/libboat.deps)
LIB_OBJS:=$(patsubst %.cpp,%.o,$(LIB_SOURCES))
LIB_OUTPUT=libboat.a

FILTERLIB_SOURCES=filter/Scanner.cpp filter/Parser.cpp filter/FilterParser.cpp
FILTERLIB_OBJS:=$(patsubst %.cpp,%.o,$(FILTERLIB_SOURCES))
FILTERLIB_OUTPUT=libfilter.a

NEWSBOAT=newsboat
NEWSBOAT_SOURCES:=$(shell cat mk/newsboat.deps)
NEWSBOAT_OBJS:=$(patsubst %.cpp,%.o,$(NEWSBOAT_SOURCES))
NEWSBOAT_LIBS=-lboat -lfilter -lpthread -lrsspp

RSSPPLIB_SOURCES=$(sort $(wildcard rss/*.cpp))
RSSPPLIB_OBJS=$(patsubst rss/%.cpp,rss/%.o,$(RSSPPLIB_SOURCES))
RSSPPLIB_OUTPUT=librsspp.a

CARGO_FLAGS+=--verbose
ifeq ($(PROFILE),1)
ifdef CARGO_BUILD_TARGET
NEWSBOATLIB_OUTPUT=target/$(CARGO_BUILD_TARGET)/debug/libnewsboat.a
LDFLAGS+=-L./target/$(CARGO_BUILD_TARGET)/debug
else
NEWSBOATLIB_OUTPUT=target/debug/libnewsboat.a
LDFLAGS+=-L./target/debug
endif
else
ifdef CARGO_BUILD_TARGET
NEWSBOATLIB_OUTPUT=target/$(CARGO_BUILD_TARGET)/release/libnewsboat.a
LDFLAGS+=-L./target/$(CARGO_BUILD_TARGET)/release
else
NEWSBOATLIB_OUTPUT=target/release/libnewsboat.a
LDFLAGS+=-L./target/release
endif
CARGO_FLAGS+=--release
endif
LDFLAGS+=-lnewsboat -lpthread -ldl

PODBOAT=podboat
PODBOAT_SOURCES:=$(shell cat mk/podboat.deps)
PODBOAT_OBJS:=$(patsubst %.cpp,%.o,$(PODBOAT_SOURCES))
PODBOAT_LIBS=-lboat -lpthread

ifeq (, $(filter Linux GNU GNU/%, $(shell uname -s)))
NEWSBOAT_LIBS+=-liconv -lintl
PODBOAT_LIBS+=-liconv -lintl
endif

# additional commands
MKDIR=mkdir -p
INSTALL=install
A2X=a2x
MSGFMT=msgfmt
RANLIB?=ranlib
AR?=ar
CHMOD=chmod
CARGO=cargo

STFLHDRS:=$(patsubst %.stfl,%.h,$(wildcard stfl/*.stfl))
POFILES:=$(wildcard po/*.po)
MOFILES:=$(patsubst %.po,%.mo,$(POFILES))
POTFILE=po/newsboat.pot
RUST_SRCS:=Cargo.toml $(shell find rust -type f)

TEXTCONV=./txt2h
RM=rm -f

all: doc $(NEWSBOAT) $(PODBOAT) mo-files

NB_DEPS=xlicense.h $(LIB_OUTPUT) $(FILTERLIB_OUTPUT) $(NEWSBOAT_OBJS) $(RSSPPLIB_OUTPUT) $(NEWSBOATLIB_OUTPUT)

$(NEWSBOATLIB_OUTPUT): $(RUST_SRCS)
	$(CARGO) build $(CARGO_FLAGS)

$(NEWSBOAT): $(NB_DEPS)
	$(CXX) $(CXXFLAGS) -o $(NEWSBOAT) $(NEWSBOAT_OBJS) $(NEWSBOAT_LIBS) $(LDFLAGS)

$(PODBOAT): $(LIB_OUTPUT) $(NEWSBOATLIB_OUTPUT) $(PODBOAT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(PODBOAT) $(PODBOAT_OBJS) $(PODBOAT_LIBS) $(LDFLAGS)

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

clean-newsboat:
	$(RM) $(NEWSBOAT) $(NEWSBOAT_OBJS)

clean-podboat:
	$(RM) $(PODBOAT) $(PODBOAT_OBJS)

clean-libboat:
	$(RM) $(LIB_OUTPUT) $(LIB_OBJS)

clean-librsspp:
	$(RM) $(RSSPPLIB_OUTPUT) $(RSSPPLIB_OBJS)

clean-libfilter:
	$(RM) $(FILTERLIB_OUTPUT) $(FILTERLIB_OBJS)

clean-libnewsboat:
	cargo clean

clean-doc:
	$(RM) -r doc/xhtml 
	$(RM) doc/*.xml doc/*.1 doc/newsboat-cfgcmds.txt doc/podboat-cfgcmds.txt \
		doc/newsboat-keycmds.txt doc/configcommands-linked.dsv \
		doc/podboat-cmds-linked.dsv doc/keycmds-linked.dsv \
		doc/gen-example-config doc/example-config doc/generate doc/generate2

clean: clean-newsboat clean-podboat clean-libboat clean-libfilter clean-doc clean-librsspp clean-libnewsboat
	$(RM) $(STFLHDRS) xlicense.h

distclean: clean clean-mo test-clean profclean
	$(RM) core *.core core.* config.mk

doc: doc/newsboat.1 doc/podboat.1 doc/xhtml/newsboat.html doc/xhtml/faq.html

doc/xhtml/newsboat.html: doc/newsboat.txt doc/chapter-firststeps.txt doc/configcommands-linked.dsv \
		doc/keycmds-linked.dsv doc/chapter-tagging.txt doc/chapter-snownews.txt \
		doc/chapter-cmdline.txt doc/chapter-podcasts.txt doc/podboat-cmds-linked.dsv \
		doc/chapter-password.txt doc/chapter-environment-variables.txt
	$(MKDIR) doc/xhtml
	$(A2X) -f xhtml -D doc/xhtml doc/newsboat.txt
	$(CHMOD) u+w doc/xhtml/docbook-xsl.css
	echo "/* AsciiDoc's new tables have <p> inside <td> which wastes vertical space, so fix it */" >> doc/xhtml/docbook-xsl.css
	echo "td > p { margin: 0; }" >> doc/xhtml/docbook-xsl.css
	echo "td > p + p { margin-top: 0.5em; }" >> doc/xhtml/docbook-xsl.css
	echo "td > pre { margin: 0; white-space: pre-wrap; }" >> doc/xhtml/docbook-xsl.css

doc/xhtml/faq.html: doc/faq.txt
	$(MKDIR) doc/xhtml
	$(A2X) -f xhtml -D doc/xhtml doc/faq.txt
	$(CHMOD) u+w doc/xhtml/docbook-xsl.css
	echo "/* AsciiDoc's new tables have <p> inside <td> which wastes vertical space, so fix it */" >> doc/xhtml/docbook-xsl.css
	echo "td > p { margin: 0; }" >> doc/xhtml/docbook-xsl.css
	echo "td > p + p { margin-top: 0.5em; }" >> doc/xhtml/docbook-xsl.css
	echo "td > pre { margin: 0; white-space: pre-wrap; }" >> doc/xhtml/docbook-xsl.css

doc/generate: doc/generate.cpp doc/split.h
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) -o doc/generate doc/generate.cpp

doc/newsboat-cfgcmds.txt: doc/generate doc/configcommands.dsv
	doc/generate doc/configcommands.dsv > doc/newsboat-cfgcmds.txt

doc/generate2: doc/generate2.cpp
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) -o doc/generate2 doc/generate2.cpp

doc/newsboat-keycmds.txt: doc/generate2 doc/keycmds.dsv
	doc/generate2 doc/keycmds.dsv > doc/newsboat-keycmds.txt

doc/newsboat.1: doc/manpage-newsboat.txt doc/chapter-firststeps.txt doc/newsboat-cfgcmds.txt \
		doc/newsboat-keycmds.txt doc/chapter-tagging.txt doc/chapter-snownews.txt \
		doc/chapter-cmdline.txt doc/chapter-environment-variables.txt
	$(A2X) -f manpage doc/manpage-newsboat.txt

doc/podboat-cfgcmds.txt: doc/generate doc/podboat-cmds.dsv
	doc/generate doc/podboat-cmds.dsv 'pb-' > doc/podboat-cfgcmds.txt

doc/podboat.1: doc/manpage-podboat.txt doc/chapter-podcasts.txt doc/podboat-cfgcmds.txt \
		doc/chapter-environment-variables.txt
	$(A2X) -f manpage doc/manpage-podboat.txt

doc/gen-example-config: doc/gen-example-config.cpp doc/split.h
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) -o doc/gen-example-config doc/gen-example-config.cpp

doc/example-config: doc/gen-example-config doc/configcommands.dsv
	sed 's/+{backslash}"+/`\\"`/g' doc/configcommands.dsv | doc/gen-example-config > doc/example-config

# add hyperlinks for every configuration command
doc/configcommands-linked.dsv: doc/configcommands.dsv
	sed -E 's/^([^|]+)/[[\1]]<<\1,`\1`>>/' doc/configcommands.dsv > doc/configcommands-linked.dsv

# add hyperlinks for every configuration command
doc/podboat-cmds-linked.dsv: doc/podboat-cmds.dsv
	sed -E 's/^([^|]+)/[[\1]]<<\1,`\1`>>/' doc/podboat-cmds.dsv > doc/podboat-cmds-linked.dsv

doc/keycmds-linked.dsv: doc/keycmds.dsv
	sed -E 's/^([^|]+)/[[\1]]<<\1,`\1`>>/' doc/keycmds.dsv > doc/keycmds-linked.dsv

fmt:
	clang-format --style=file -i *.cpp doc/*.cpp include/*.h rss/*.h rss/*.cpp src/*.cpp test/*.h test/*.cpp

cppcheck:
	cppcheck -j$(CPPCHECK_JOBS) --force --enable=all --suppress=unusedFunction \
		-DDEBUG=1 \
		$(INCLUDES) $(DEFINES) \
		include filter newsboat.cpp podboat.cpp rss src stfl \
		test/*.cpp test/*.h \
		2>cppcheck.log
	@echo "Done! See cppcheck.log for details."

install-newsboat: $(NEWSBOAT) doc/$(NEWSBOAT).1
	$(MKDIR) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(NEWSBOAT) $(DESTDIR)$(prefix)/bin
	$(MKDIR) $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 doc/$(NEWSBOAT).1 $(DESTDIR)$(mandir)/man1 || true

install-podboat: $(PODBOAT) doc/$(PODBOAT).1
	$(MKDIR) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(PODBOAT) $(DESTDIR)$(prefix)/bin
	$(MKDIR) $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 doc/$(PODBOAT).1 $(DESTDIR)$(mandir)/man1 || true

install-docs: doc
	$(MKDIR) $(DESTDIR)$(docdir)
	$(INSTALL) -m 644 doc/xhtml/* $(DESTDIR)$(docdir) || true
	$(INSTALL) -m 644 CHANGELOG.md $(DESTDIR)$(docdir) || true
	$(MKDIR) $(DESTDIR)$(docdir)/contrib
	$(INSTALL) -m 755 contrib/*.sh $(DESTDIR)$(docdir)/contrib || true
	$(INSTALL) -m 755 contrib/*.rb $(DESTDIR)$(docdir)/contrib || true
	$(INSTALL) -m 755 contrib/*.pl $(DESTDIR)$(docdir)/contrib || true
	$(MKDIR) $(DESTDIR)$(docdir)/contrib/colorschemes
	$(INSTALL) -m 644 contrib/colorschemes/* $(DESTDIR)$(docdir)/contrib/colorschemes || true
	$(MKDIR) $(DESTDIR)$(docdir)/contrib/getpocket.com
	$(INSTALL) -m 755 contrib/getpocket.com/*.sh $(DESTDIR)$(docdir)/contrib/getpocket.com || true
	$(INSTALL) -m 644 contrib/getpocket.com/*.md $(DESTDIR)$(docdir)/contrib/getpocket.com || true

install-examples: doc/example-config
	$(MKDIR) $(DESTDIR)$(docdir)/examples
	$(INSTALL) -m 644 doc/example-config $(DESTDIR)$(docdir)/examples/config || true
	$(INSTALL) -m 755 doc/example-bookmark-plugin.sh $(DESTDIR)$(docdir)/examples/example-bookmark-plugin.sh || true


install: install-newsboat install-podboat install-docs install-examples install-mo

uninstall: uninstall-mo
	$(RM) $(DESTDIR)$(prefix)/bin/$(NEWSBOAT)
	$(RM) $(DESTDIR)$(prefix)/bin/$(PODBOAT)
	$(RM) $(DESTDIR)$(mandir)/man1/$(NEWSBOAT).1
	$(RM) $(DESTDIR)$(mandir)/man1/$(PODBOAT).1
	$(RM) -rf $(DESTDIR)$(docdir)
	$(RM) -r $(DESTDIR)$(docdir)

.PHONY: doc clean distclean all test test-rss extract install uninstall regenerate-parser clean-newsboat \
	clean-podboat clean-libboat clean-librsspp clean-libfilter clean-doc install-mo msgmerge clean-mo \
	test-clean config cppcheck

# the following targets are i18n/l10n-related:

run-i18nspector: $(POFILES)
	i18nspector $^

mo-files: $(MOFILES)

extract:
	$(RM) $(POTFILE)
	xgettext -c/ -k_ -k_s -o $(POTFILE) *.cpp src/*.cpp rss/*.cpp
	sed -i 's#Report-Msgid-Bugs-To: \\n#Report-Msgid-Bugs-To: https://github.com/newsboat/newsboat/issues\\n#' $(POTFILE)


msgmerge:
	for f in $(POFILES) ; do msgmerge -U $$f $(POTFILE) ; done

%.mo: %.po
	$(MSGFMT) --check --statistics -o $@ $<

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

TEST_SRCS:=$(wildcard test/*.cpp)
TEST_OBJS:=$(patsubst %.cpp,%.o,$(TEST_SRCS))
test/test: xlicense.h $(LIB_OUTPUT) $(NEWSBOATLIB_OUTPUT) $(NEWSBOAT_OBJS) $(PODBOAT_OBJS) $(FILTERLIB_OUTPUT) $(RSSPPLIB_OUTPUT) $(TEST_OBJS) test/test-helpers.h
	$(CXX) $(CXXFLAGS) -o test/test $(TEST_OBJS) src/*.o $(NEWSBOAT_LIBS) $(LDFLAGS)

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

ALL_SRCS:=$(wildcard filter/*.cpp rss/*.cpp src/*.cpp test/*.cpp)
ALL_HDRS:=$(wildcard filter/*.h rss/*.h test/*.h 3rd-party/*.hpp) $(STFLHDRS) xlicense.h
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
