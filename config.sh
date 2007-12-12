#!/bin/sh

require_pkg() {
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
		echo ""
		echo "You need package ${pkgname} in order to compile this program."
		echo "Please make sure it is installed."
		exit 1
	fi
	return 0
}

optional_pkg() {
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
		return 0
	else
		return 1
	fi
}

echo -n "" > config.mk

require_pkg "nxml"
require_pkg "mrss"
require_pkg "sqlite3"
require_pkg "libcurl"
optional_pkg "lua5.1" "-DEMBED_LUA=1" || optional_pkg "lua" "-DEMBED_LUA=1" || echo "Warning: newsbeuter will be compiled without Lua support."
