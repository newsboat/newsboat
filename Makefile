# important directories
prefix?=/usr/local
mandir?=$(prefix)/share/man
datadir?=$(prefix)/share
localedir?=$(datadir)/locale
docdir?=$(datadir)/doc/$(PACKAGE)

export CARGO_TARGET_DIR?=$(abspath target)

CPPCHECK_JOBS?=5

# By default, do not run "ignored" Rust tests -- they require some additional
# setup on the test machine.
NEWSBOAT_RUN_IGNORED_TESTS?=0

# compiler
CXX?=c++
# Compiler for building executables meant to be run on the
# host system during cross compilation
CXX_FOR_BUILD?=$(CXX)

# compiler and linker flags
DEFINES=-DLOCALEDIR='"$(localedir)"'

WARNFLAGS=-Werror -Wall -Wextra -Wunreachable-code
INCLUDES=-Iinclude -Istfl -Ifilter -I. -Irss -I$(CARGO_TARGET_DIR)/cxxbridge/
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

LIB_SRCS:=$(shell cat mk/libboat.deps)
LIB_OBJS:=$(patsubst %.cpp,%.o,$(LIB_SRCS))
LIB_OUTPUT=libboat.a

FILTERLIB_SRCS=filter/Scanner.cpp filter/Parser.cpp filter/FilterParser.cpp
FILTERLIB_OBJS:=$(patsubst %.cpp,%.o,$(FILTERLIB_SRCS))
FILTERLIB_OUTPUT=libfilter.a

NEWSBOAT=newsboat
NEWSBOAT_SRCS:=$(shell cat mk/newsboat.deps)
NEWSBOAT_OBJS:=$(patsubst %.cpp,%.o,$(NEWSBOAT_SRCS))
NEWSBOAT_LIBS=-lboat -lnewsboat -lfilter -lpthread -lrsspp -ldl

RSSPPLIB_SRCS=$(sort $(wildcard rss/*.cpp))
RSSPPLIB_OBJS=$(patsubst rss/%.cpp,rss/%.o,$(RSSPPLIB_SRCS))
RSSPPLIB_OUTPUT=librsspp.a

PODBOAT=podboat
PODBOAT_SRCS:=$(shell cat mk/podboat.deps)
PODBOAT_OBJS:=$(patsubst %.cpp,%.o,$(PODBOAT_SRCS))
PODBOAT_LIBS=-lboat -lnewsboat -lfilter -lpthread -ldl

TEST_SRCS:=$(wildcard test/*.cpp test/test_helpers/*.cpp)
TEST_OBJS:=$(patsubst %.cpp,%.o,$(TEST_SRCS))
SRC_SRCS:=$(wildcard src/*.cpp)
SRC_OBJS:=$(patsubst %.cpp,%.o,$(SRC_SRCS))

CPP_SRCS:=$(LIB_SRCS) $(FILTERLIB_SRCS) $(NEWSBOAT_SRCS) $(RSSPPLIB_SRCS) $(PODBOAT_SRCS) $(TEST_SRCS)
CPP_DEPS:=$(addprefix .deps/,$(CPP_SRCS))
# Sorting removes duplicate items, which prevents Make from spewing warnings
# about repeated items in the target that creates these directories
CPP_DEPS_SUBDIRS:=$(sort $(dir $(CPP_DEPS)))

STFL_HDRS:=$(patsubst %.stfl,%.h,$(wildcard stfl/*.stfl))

POFILES:=$(wildcard po/*.po)
MOFILES:=$(patsubst %.po,%.mo,$(POFILES))
POTFILE=po/newsboat.pot

RUST_SRCS:=Cargo.toml $(shell find rust -type f)

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

test: test/test rust-test

rust-test:
	+$(CARGO) test $(CARGO_TEST_FLAGS) --no-run

test/test: xlicense.h $(LIB_OUTPUT) $(NEWSBOATLIB_OUTPUT) $(NEWSBOAT_OBJS) $(PODBOAT_OBJS) $(FILTERLIB_OUTPUT) $(RSSPPLIB_OUTPUT) $(TEST_OBJS)
	$(CXX) $(CXXFLAGS) -o test/test $(TEST_OBJS) $(SRC_OBJS) $(NEWSBOAT_LIBS) $(LDFLAGS)

regenerate-parser:
	$(RM) filter/Scanner.cpp filter/Parser.cpp filter/Scanner.h filter/Parser.h
	cococpp -frames filter filter/filter.atg
	sed -i 's/\s\+$$//' filter/Scanner.cpp filter/Parser.cpp filter/Scanner.h filter/Parser.h

target/cxxbridge/libnewsboat-ffi/src/%.rs.h: $(NEWSBOATLIB_OUTPUT)
	@# This rule declares a dependency and doesn't need to run any
	@# commands, but we can't leave the recipe empty because GNU Make
	@# requires a recipe for pattern rules. So here you go, Make, have
	@# a comment.

$(CPP_DEPS_SUBDIRS):
	$(MKDIR) $@

%.o: %.cpp # Cancel default rule for C++ code
%.o: %.cpp | $(NEWSBOATLIB_OUTPUT) $(STFL_HDRS) $(CPP_DEPS_SUBDIRS)
	$(CXX) $(CXXFLAGS) -MD -MP -MF $(addprefix .deps/,$<) -o $@ -c $<

# This prevents Make from thinking that STFL headers are an intermediate
# dependency of C++ object files, which in turn prevents Make from removing the
# headers once the object files are compiled. That fixes the problem where
# re-running Make causes it to re-create the headers and then re-compile all
# the object files that depend on those headers.
$(STFL_HDRS):

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

clean-test:
	$(RM) test/test test/*.o test/test_helpers/*.o

clean: clean-newsboat clean-podboat clean-libboat clean-libfilter clean-doc clean-mo clean-librsspp clean-libnewsboat clean-test
	$(RM) $(STFL_HDRS) xlicense.h
	$(RM) -r .deps

profclean:
	find . -name '*.gc*' -type f -print0 | xargs -0 $(RM) --
	$(RM) app*.info

distclean: clean profclean
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

doc/xhtml/newsboat.html: doc/chapter-tagging.asciidoc
doc/xhtml/newsboat.html: doc/chapter-snownews.asciidoc
doc/xhtml/newsboat.html: doc/chapter-firststeps.asciidoc
doc/xhtml/newsboat.html: doc/chapter-cmdline.asciidoc
doc/xhtml/newsboat.html: doc/chapter-configuration.asciidoc
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
		doc/chapter-cmdline.asciidoc doc/chapter-configuration.asciidoc \
		doc/chapter-environment-variables.asciidoc \
		doc/chapter-files.asciidoc doc/man.rb
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_OPTS) --require=./doc/man.rb --backend=manpage doc/manpage-newsboat.asciidoc

doc/podboat-cfgcmds.asciidoc: doc/generate doc/podboat-cmds.dsv
	doc/generate doc/podboat-cmds.dsv 'pb-' > doc/podboat-cfgcmds.asciidoc

doc/$(PODBOAT).1: doc/manpage-podboat.asciidoc \
		doc/chapter-podcasts.asciidoc doc/chapter-podboat.asciidoc \
		doc/podboat-cfgcmds.asciidoc \
		doc/chapter-environment-variables.asciidoc \
		doc/chapter-files.asciidoc doc/man.rb
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_OPTS) --require=./doc/man.rb --backend=manpage doc/manpage-podboat.asciidoc

doc/gen-example-config: doc/gen-example-config.cpp doc/split.h
	$(CXX_FOR_BUILD) $(CXXFLAGS_FOR_BUILD) -o doc/gen-example-config doc/gen-example-config.cpp

doc/example-config: doc/gen-example-config doc/configcommands.dsv
	sed 's/+{backslash}"+/`\\"`/g' doc/configcommands.dsv | doc/gen-example-config > doc/example-config

fmt:
	astyle --project \
		*.cpp doc/*.cpp include/*.h rss/*.h rss/*.cpp src/*.cpp \
		test/*.cpp test/test_helpers/*.h test/test_helpers/*.cpp
	$(CARGO) fmt
	# We reset the locale to make the sorting reproducible.
	LC_ALL=C sort -t '|' -k 1,1 -o doc/configcommands.dsv doc/configcommands.dsv
	# Remove trailing whitespace. The purpose of the 'grep' in between is that
	# files without trailing whitespace will not be touched by 'sed'. Otherwise
	# editors might detect some external file modification and ask to reload.
	# With '-I' we guard against any potentially introduced binary (test) files
	# in the future.
	for f in $$(git ls-files | xargs grep -lI '[ 	]$$'); do \
		sed -i 's/[ \t]\+$$//' $$f; \
	done

cppcheck:
	cppcheck -j$(CPPCHECK_JOBS) --force --enable=all --suppress=unusedFunction \
		--config-exclude=3rd-party --config-exclude=$(CARGO_TARGET_DIR) --config-exclude=/usr/include \
		--suppress=*:3rd-party/* --suppress=*:$(CARGO_TARGET_DIR)/* --suppress=*:/usr/include/* \
		--inline-suppr -DDEBUG=1 -U__VERSION__ \
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
	xgettext -c/ -k_ -k_s -ktranslatable -o po/cpp.pot *.cpp src/*.cpp rss/*.cpp
	xtr rust/libnewsboat/src/lib.rs rust/regex-rs/src/lib.rs --omit-header -o po/rust.pot
	cat po/cpp.pot po/rust.pot > $(POTFILE)
	$(RM) -f po/cpp.pot po/rust.pot
	sed -i 's#Report-Msgid-Bugs-To: \\n#Report-Msgid-Bugs-To: https://github.com/newsboat/newsboat/issues\\n#' $(POTFILE)


msgmerge:
	for f in $(POFILES) ; do \
		msgmerge --backup=off -U $$f $(POTFILE) ; \
		msgattrib --no-obsolete --output-file=$$f $$f ; \
	done

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

check: test
	(cd test && ./test --order=rand --rng-seed=time)
	$(CARGO) test $(CARGO_TEST_FLAGS)
	if [ $(NEWSBOAT_RUN_IGNORED_TESTS) -eq 1 ] ; then $(CARGO) test $(CARGO_TEST_FLAGS) -- --ignored ; fi

ci-check: test
        # We want to run both C++ and Rust tests, but we also want this entire
        # command to fail if one of the test suites fails. That's why we store
        # the C++'s exit code and chain it to Rust's in the end.
	$(CARGO) test $(CARGO_TEST_FLAGS) --no-fail-fast ; \
	ret1=$$? ; \
	\
	ret2=0 ; \
	if [ $(NEWSBOAT_RUN_IGNORED_TESTS) -eq 1 ] ; \
		then $(CARGO) test $(CARGO_TEST_FLAGS) --no-fail-fast -- --ignored ; ret2=$$? ; \
		fi ; \
	\
	cd test && \
	./test --order=rand --rng-seed=time && \
	\
	[ \( $$ret1 -eq 0 \) -a \( $$ret2 -eq 0 \) ]

# miscellaneous stuff

config: config.mk

config.mk:
	@./config.sh

xlicense.h: LICENSE
	$(TEXTCONV) $< > $@

-include $(CPP_DEPS)
