# important directories
prefix?=/usr/local
mandir?=$(prefix)/share/man
datadir?=$(prefix)/share
localedir?=$(datadir)/locale
docdir?=$(datadir)/doc/$(PACKAGE)

# Here's a problem:
# 1. our "Rust 1.44, GCC 10, Ubuntu 20.04" build fails: cxx-0.5 crate can't
#    find the target directory, thus doesn't symlink cxxbridge headers into the
#    expected location, thus C++ compilation fails.
# 2. cxx-0.5 can take a hint from CARGO_TARGET_DIR, but it wants the path to be
#    absolute.
# 3. Cargo docs say that CARGO_TARGET_DIR is relative to current working
#    directory, but in reality, Cargo works fine with an absolute path too.
# 4. We actually need a relative path for a different purpose: to tell GCC
#    where to find cxxbridge headers. Unfortunately, if we use an absolute path
#    there, GCC will also print out absolute paths when printing out dependency
#    info (see `depslist` target). So we have to supply GCC a relative path.
#
# So here's what we do:
#
# 1. if someone set CARGO_TARGET_DIR env var, we **hope** they followed Cargo
#    docs' advice and used a relative path. If the var is not set, we use
#    a default of "target" (which is Cargo's default, so nothing changes from
#    the user's point of view).
#
#    We remember this path in a variable `relative_cargo_target_dir`, which we
#    pass to GCC.
# 2. we override CARGO_TARGET_DIR, turning it into an absolute path.
#
#    That fixes cxx-0.5.
CARGO_TARGET_DIR?=target
relative_cargo_target_dir:=$(CARGO_TARGET_DIR)
export CARGO_TARGET_DIR:=$(abspath $(CARGO_TARGET_DIR))

CPPCHECK_JOBS?=5

# compiler
CXX?=c++
# Compiler for building executables meant to be run on the
# host system during cross compilation
CXX_FOR_BUILD?=$(CXX)

# compiler and linker flags
DEFINES=-DLOCALEDIR='"$(localedir)"'

WARNFLAGS=-Werror -Wall -Wextra -Wunreachable-code
INCLUDES=-Iinclude -Istfl -Ifilter -I. -Irss -I$(relative_cargo_target_dir)/cxxbridge/libnewsboat-ffi/src/
BARE_CXXFLAGS=-std=c++11 -O2 -ggdb $(INCLUDES)
LDFLAGS+=-L.

# Constants
PACKAGE=newsboat
## Do not include libnewsboat-ffi into the test executable, as it doesn't link
## and can't contain any tests anyway.
CARGO_TEST_FLAGS=--workspace --exclude libnewsboat-ffi

ifeq (, $(filter $(MAKECMDGOALS),distclean run-i18nspector fmt))
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
NEWSBOAT_LIBS=-lboat -lnewsboat -lfilter -lpthread -lrsspp -ldl

RSSPPLIB_SOURCES=$(sort $(wildcard rss/*.cpp))
RSSPPLIB_OBJS=$(patsubst rss/%.cpp,rss/%.o,$(RSSPPLIB_SOURCES))
RSSPPLIB_OUTPUT=librsspp.a

CARGO_BUILD_FLAGS+=--verbose

ifeq ($(PROFILE),1)
BUILD_TYPE=debug
else
BUILD_TYPE=release
CARGO_BUILD_FLAGS+=--release
endif

ifdef CARGO_BUILD_TARGET
NEWSBOATLIB_OUTPUT=$(CARGO_TARGET_DIR)/$(CARGO_BUILD_TARGET)/$(BUILD_TYPE)/libnewsboat.a
LDFLAGS+=-L$(CARGO_TARGET_DIR)/$(CARGO_BUILD_TARGET)/$(BUILD_TYPE)
else
NEWSBOATLIB_OUTPUT=$(CARGO_TARGET_DIR)/$(BUILD_TYPE)/libnewsboat.a
LDFLAGS+=-L$(CARGO_TARGET_DIR)/$(BUILD_TYPE)
endif

PODBOAT=podboat
PODBOAT_SOURCES:=$(shell cat mk/podboat.deps)
PODBOAT_OBJS:=$(patsubst %.cpp,%.o,$(PODBOAT_SOURCES))
PODBOAT_LIBS=-lboat -lnewsboat -lfilter -lpthread -ldl

ifeq (, $(filter Linux GNU GNU/%, $(shell uname -s)))
NEWSBOAT_LIBS+=-liconv -lintl
PODBOAT_LIBS+=-liconv -lintl
endif

# additional commands
MKDIR=mkdir -p
INSTALL=install
ASCIIDOCTOR=asciidoctor
MSGFMT=msgfmt
RANLIB?=ranlib
AR?=ar
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

$(NEWSBOATLIB_OUTPUT): $(RUST_SRCS) Cargo.lock
	+$(CARGO) build --package libnewsboat-ffi $(CARGO_BUILD_FLAGS)

$(NEWSBOAT): $(NB_DEPS)
	$(CXX) $(CXXFLAGS) -o $(NEWSBOAT) $(NEWSBOAT_OBJS) $(NEWSBOAT_LIBS) $(LDFLAGS)

$(PODBOAT): $(LIB_OUTPUT) $(NEWSBOATLIB_OUTPUT) $(PODBOAT_OBJS) $(FILTERLIB_OUTPUT)
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

target/cxxbridge/libnewsboat-ffi/src/%.rs.h: $(NEWSBOATLIB_OUTPUT)
	@# This rule declares a dependency and doesn't need to run any
	@# commands, but we can't leave the recipe empty because GNU Make
	@# requires a recipe for pattern rules. So here you go, Make, have
	@# a comment.

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
	$(CARGO) clean

clean-doc:
	$(RM) -r doc/xhtml 
	$(RM) doc/*.xml doc/*.1 doc/*-linked.asciidoc doc/newsboat-cfgcmds.asciidoc \
		doc/podboat-cfgcmds.asciidoc doc/newsboat-keycmds.asciidoc \
		doc/example-config doc/generate doc/generate2 \
		doc/gen-example-config

clean: clean-newsboat clean-podboat clean-libboat clean-libfilter clean-doc clean-librsspp clean-libnewsboat
	$(RM) $(STFLHDRS) xlicense.h

distclean: clean clean-mo clean-test profclean
	$(RM) core *.core core.* config.mk

doc: doc/$(NEWSBOAT).1 doc/$(PODBOAT).1 doc/xhtml/newsboat.html doc/xhtml/faq.html doc/example-config

doc/xhtml:
	$(MKDIR) doc/xhtml

doc/configcommands-linked.asciidoc: doc/configcommands.dsv
	sed 's/||/\t/g' doc/configcommands.dsv | awk -f doc/createConfigurationCommandsListView.awk > doc/configcommands-linked.asciidoc

doc/availableoperations-linked.asciidoc: doc/keycmds.dsv
	sed 's/||/\t/g' doc/keycmds.dsv | awk -f doc/createAvailableOperationsListView.awk > doc/availableoperations-linked.asciidoc

doc/podboat-cmds-linked.asciidoc: doc/podboat-cmds.dsv
	sed 's/||/\t/g' doc/podboat-cmds.dsv | awk -f doc/createPodboatConfigurationCommandsListView.awk > doc/podboat-cmds-linked.asciidoc

doc/cmdline-commands-linked.asciidoc: doc/cmdline-commands.dsv
	sed 's/||/\t/g' doc/cmdline-commands.dsv | awk -f doc/createAvailableCommandlineCommandsListView.awk > doc/cmdline-commands-linked.asciidoc

doc/xhtml/newsboat.html: doc/chapter-firststeps.asciidoc
doc/xhtml/newsboat.html: doc/chapter-tagging.asciidoc
doc/xhtml/newsboat.html: doc/chapter-snownews.asciidoc
doc/xhtml/newsboat.html: doc/chapter-cmdline.asciidoc
doc/xhtml/newsboat.html: doc/chapter-podcasts.asciidoc
doc/xhtml/newsboat.html: doc/chapter-podboat.asciidoc
doc/xhtml/newsboat.html: doc/chapter-files.asciidoc
doc/xhtml/newsboat.html: doc/chapter-password.asciidoc
doc/xhtml/newsboat.html: doc/chapter-environment-variables.asciidoc
doc/xhtml/newsboat.html: doc/configcommands-linked.asciidoc
doc/xhtml/newsboat.html: doc/availableoperations-linked.asciidoc
doc/xhtml/newsboat.html: doc/podboat-cmds-linked.asciidoc
doc/xhtml/newsboat.html: doc/cmdline-commands-linked.asciidoc

doc/xhtml/%.html: doc/%.asciidoc | doc/xhtml
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_OPTS) --backend=html5 -a webfonts! --destination-dir=doc/xhtml $<

doc/generate: doc/generate.cpp doc/split.h
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) -o doc/generate doc/generate.cpp

doc/newsboat-cfgcmds.asciidoc: doc/generate doc/configcommands.dsv
	doc/generate doc/configcommands.dsv > doc/newsboat-cfgcmds.asciidoc

doc/generate2: doc/generate2.cpp
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) -o doc/generate2 doc/generate2.cpp

doc/newsboat-keycmds.asciidoc: doc/generate2 doc/keycmds.dsv
	doc/generate2 doc/keycmds.dsv > doc/newsboat-keycmds.asciidoc

doc/$(NEWSBOAT).1: doc/manpage-newsboat.asciidoc doc/chapter-firststeps.asciidoc \
		doc/newsboat-cfgcmds.asciidoc doc/newsboat-keycmds.asciidoc \
		doc/chapter-tagging.asciidoc doc/chapter-snownews.asciidoc \
		doc/chapter-cmdline.asciidoc \
		doc/chapter-environment-variables.asciidoc \
		doc/chapter-files.asciidoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_OPTS) --backend=manpage doc/manpage-newsboat.asciidoc

doc/podboat-cfgcmds.asciidoc: doc/generate doc/podboat-cmds.dsv
	doc/generate doc/podboat-cmds.dsv 'pb-' > doc/podboat-cfgcmds.asciidoc

doc/$(PODBOAT).1: doc/manpage-podboat.asciidoc \
		doc/chapter-podcasts.asciidoc doc/chapter-podboat.asciidoc \
		doc/podboat-cfgcmds.asciidoc \
		doc/chapter-environment-variables.asciidoc \
		doc/chapter-files.asciidoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_OPTS) --backend=manpage doc/manpage-podboat.asciidoc

doc/gen-example-config: doc/gen-example-config.cpp doc/split.h
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) -o doc/gen-example-config doc/gen-example-config.cpp

doc/example-config: doc/gen-example-config doc/configcommands.dsv
	sed 's/+{backslash}"+/`\\"`/g' doc/configcommands.dsv | doc/gen-example-config > doc/example-config

fmt:
	astyle --project \
		*.cpp doc/*.cpp include/*.h rss/*.h rss/*.cpp src/*.cpp \
		test/*.cpp test/test-helpers/*.h test/test-helpers/*.cpp
	$(CARGO) fmt

cppcheck:
	cppcheck -j$(CPPCHECK_JOBS) --force --enable=all --suppress=unusedFunction \
		--config-exclude=3rd-party --config-exclude=$(relative_cargo_target_dir) --config-exclude=/usr/include \
		--suppress=*:3rd-party/* --suppress=*:$(relative_cargo_target_dir)/* --suppress=*:/usr/include/* \
		-DDEBUG=1 \
		$(INCLUDES) $(DEFINES) \
		include newsboat.cpp podboat.cpp rss src stfl test \
		2>cppcheck.log
	@echo "Done! See cppcheck.log for details."

install-newsboat: $(NEWSBOAT)
	$(MKDIR) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(NEWSBOAT) $(DESTDIR)$(prefix)/bin

install-podboat: $(PODBOAT)
	$(MKDIR) $(DESTDIR)$(prefix)/bin
	$(INSTALL) $(PODBOAT) $(DESTDIR)$(prefix)/bin

install-docs: doc
	$(MKDIR) $(DESTDIR)$(docdir)
	$(INSTALL) -m 644 doc/xhtml/* $(DESTDIR)$(docdir)
	$(INSTALL) -m 644 CHANGELOG.md $(DESTDIR)$(docdir)
	$(MKDIR) $(DESTDIR)$(docdir)/contrib
	$(INSTALL) -m 644 contrib/README.md $(DESTDIR)$(docdir)/contrib
	$(INSTALL) -m 755 contrib/*.sh $(DESTDIR)$(docdir)/contrib
	$(INSTALL) -m 755 contrib/*.rb $(DESTDIR)$(docdir)/contrib
	$(INSTALL) -m 755 contrib/*.pl $(DESTDIR)$(docdir)/contrib
	$(INSTALL) -m 755 contrib/*.py $(DESTDIR)$(docdir)/contrib
	$(MKDIR) $(DESTDIR)$(docdir)/contrib/colorschemes
	$(INSTALL) -m 644 contrib/colorschemes/* $(DESTDIR)$(docdir)/contrib/colorschemes
	$(MKDIR) $(DESTDIR)$(docdir)/contrib/getpocket.com
	$(INSTALL) -m 755 contrib/getpocket.com/*.sh $(DESTDIR)$(docdir)/contrib/getpocket.com
	$(INSTALL) -m 644 contrib/getpocket.com/*.md $(DESTDIR)$(docdir)/contrib/getpocket.com
	$(MKDIR) $(DESTDIR)$(docdir)/contrib/image-preview
	$(INSTALL) -m 755 contrib/image-preview/vifmimg $(DESTDIR)$(docdir)/contrib/image-preview
	$(INSTALL) -m 755 contrib/image-preview/nbrun $(DESTDIR)$(docdir)/contrib/image-preview
	$(INSTALL) -m 755 contrib/image-preview/nbparser $(DESTDIR)$(docdir)/contrib/image-preview
	$(INSTALL) -m 644 contrib/image-preview/README.org $(DESTDIR)$(docdir)/contrib/image-preview
	$(MKDIR) $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 doc/$(NEWSBOAT).1 $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 644 doc/$(PODBOAT).1 $(DESTDIR)$(mandir)/man1

install-examples: doc/example-config
	$(MKDIR) $(DESTDIR)$(docdir)/examples
	$(INSTALL) -m 644 doc/example-config $(DESTDIR)$(docdir)/examples/config
	$(INSTALL) -m 755 doc/examples/example-bookmark-plugin.sh $(DESTDIR)$(docdir)/examples/example-bookmark-plugin.sh
	$(INSTALL) -m 755 doc/examples/example-exec-script.py $(DESTDIR)$(docdir)/examples/example-exec-script.py

install-icon:
	$(MKDIR) $(DESTDIR)$(datadir)/icons/hicolor/scalable/apps
	$(INSTALL) -m 644 logo.svg $(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/newsboat.svg

install: install-newsboat install-podboat install-docs install-examples install-mo install-icon

uninstall: uninstall-mo
	$(RM) $(DESTDIR)$(prefix)/bin/$(NEWSBOAT)
	$(RM) $(DESTDIR)$(prefix)/bin/$(PODBOAT)
	$(RM) $(DESTDIR)$(mandir)/man1/$(NEWSBOAT).1
	$(RM) $(DESTDIR)$(mandir)/man1/$(PODBOAT).1
	$(RM) -rf $(DESTDIR)$(docdir)
	$(RM) -r $(DESTDIR)$(docdir)
	$(RM) $(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/newsboat.svg

.PHONY: doc clean distclean all test extract install uninstall regenerate-parser clean-newsboat \
	clean-podboat clean-libboat clean-librsspp clean-libfilter clean-doc install-mo msgmerge clean-mo \
	clean-test config cppcheck

# the following targets are i18n/l10n-related:

run-i18nspector: $(POFILES)
	i18nspector $^

mo-files: $(MOFILES)

extract:
	$(RM) $(POTFILE)
	xgettext -c/ -k_ -k_s -o po/cpp.pot *.cpp src/*.cpp rss/*.cpp
	xtr rust/libnewsboat/src/lib.rs rust/regex-rs/src/lib.rs --omit-header -o po/rust.pot
	cat po/cpp.pot po/rust.pot > $(POTFILE)
	$(RM) -f po/cpp.pot po/rust.pot
	sed -i 's#Report-Msgid-Bugs-To: \\n#Report-Msgid-Bugs-To: https://github.com/newsboat/newsboat/issues\\n#' $(POTFILE)


msgmerge:
	for f in $(POFILES) ; do msgmerge --backup=off -U $$f $(POTFILE) ; done

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

test: test/test rust-test

rust-test:
	+$(CARGO) test $(CARGO_TEST_FLAGS) --no-run

TEST_SRCS:=$(wildcard test/*.cpp test/test-helpers/*.cpp)
TEST_OBJS:=$(patsubst %.cpp,%.o,$(TEST_SRCS))
test/test: xlicense.h $(LIB_OUTPUT) $(NEWSBOATLIB_OUTPUT) $(NEWSBOAT_OBJS) $(PODBOAT_OBJS) $(FILTERLIB_OUTPUT) $(RSSPPLIB_OUTPUT) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o test/test $(TEST_OBJS) src/*.o $(NEWSBOAT_LIBS) $(LDFLAGS)

clean-test:
	$(RM) test/test test/*.o test/test-helpers/*.o

profclean:
	find . -name '*.gc*' -type f -print0 | xargs -0 $(RM) --
	$(RM) app*.info

check: test
	(cd test && ./test --order=rand --rng-seed=time)
	+$(CARGO) test $(CARGO_TEST_FLAGS)

ci-check: test
        # We want to run both C++ and Rust tests, but we also want this entire
        # command to fail if one of the test suites fails. That's why we store
        # the C++'s exit code and chain it to Rust's in the end.
	$(CARGO) test $(CARGO_TEST_FLAGS) --no-fail-fast ; ret=$$? ; cd test && ./test --order=rand --rng-seed=time && exit $$ret

# miscellaneous stuff

config: config.mk

config.mk:
	@./config.sh

xlicense.h: LICENSE
	$(TEXTCONV) $< > $@

ALL_SRCS:=$(shell ls -1 *.cpp filter/*.cpp rss/*.cpp src/*.cpp test/*.cpp test/test-helpers/*.cpp)
ALL_HDRS:=$(wildcard filter/*.h rss/*.h test/test-helpers/*.h 3rd-party/*.hpp) $(STFLHDRS) xlicense.h
# This depends on NEWSBOATLIB_OUTPUT because it produces cxxbridge headers, and
# we need to record those headers in the deps file.
depslist: $(NEWSBOATLIB_OUTPUT) $(ALL_SRCS) $(ALL_HDRS)
	> mk/mk.deps
	for file in $(ALL_SRCS) ; do \
		target=`echo $$file | sed 's/cpp$$/o/'`; \
		$(CXX) $(BARE_CXXFLAGS) -MM -MG -MQ $$target $$file >> mk/mk.deps ; \
		echo $$file ; \
	done;

include mk/mk.deps
