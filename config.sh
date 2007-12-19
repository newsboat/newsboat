#!/bin/sh

check_pkg() {
	pkgname=$1
	add_define=$2
	echo -n "Checking for package ${pkgname}... "
	if pkg-config --silence-errors "${pkgname}" ; then
		echo "found"
		echo "# configuration for package ${pkgname}" >> config.mk
		echo "DEFINES+=`pkg-config --cflags ${pkgname}`" >> config.mk
		if [ -n "$add_define" ] ; then
			echo "DEFINES+=${add_define}" >> config.mk
		fi
		echo "LDFLAGS+=`pkg-config --libs ${pkgname}`" >> config.mk
		echo "" >> config.mk
	else
		echo "not found"
		return 1
	fi
	return 0
}

check_custom() {
  pkgname=$1
  customconfig=$2
  add_define=$3
	echo -n "Checking for package ${pkgname} using ${customconfig}... "
	if ${customconfig} --cflags > /dev/null 2>&1 ; then
		echo "found"
		echo "# configuration for package ${pkgname}" >> config.mk
		echo "DEFINES+=`${customconfig} --cflags`" >> config.mk
		if [ -n "$add_define" ] ; then
			echo "DEFINES+=${add_define}" >> config.mk
		fi
		echo "LDFLAGS+=`${customconfig} --libs`" >> config.mk
		echo "" >> config.mk
	else
		echo "not found"
		return 1
	fi
	return 0
}

find_rubyflags() {
	echo -n "Checking for ruby... "
	if ruby --version > /dev/null 2>&1 ; then
		archdir=`ruby -e 'require "mkmf" ; print Config::CONFIG["archdir"]'`
		if [ "$?" -eq "0" ] ; then
			echo "# ruby configuration" >> config.mk
			echo "CXXFLAGS+=-I${archdir}" >> config.mk
			echo "" >> config.mk
			echo "found"
			return 0
		else
			echo "found, but calling mkmf failed"
			return 1
		fi
	else
		echo "not found"
		return 1
	fi
}

fail() {
    pkgname=$1
		echo ""
		echo "You need package ${pkgname} in order to compile this program."
		echo "Please make sure it is installed."
		exit 1
}

fail_custom() {
	err=$1
	echo ""
	echo "${pkgname}"
	exit 1
}

echo -n "" > config.mk

check_pkg "nxml" || fail "nxml"
check_pkg "mrss" || fail "mrss"
check_pkg "sqlite3" || fail "sqlite3"
check_pkg "libcurl" || check_custom "libcurl" "curl-config" || fail "libcurl"
find_rubyflags || fail_custom "Ruby couldn't be found."
#check_pkg "lua5.1" "-DEMBED_LUA=1" || check_pkg "lua" "-DEMBED_LUA=1" || echo "Warning: newsbeuter will be compiled without Lua support."
