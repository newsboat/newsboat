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

fail() {
    pkgname=$1
		rm -f config.mk
		echo ""
		echo "You need package ${pkgname} in order to compile this program."
		echo "Please make sure it is installed."
		exit 1
}

fail_custom() {
	err=$1
	echo ""
	echo "ERROR: ${err}"
	exit 1
}

echo "" > config.mk

check_pkg "sqlite3" || fail "sqlite3"
check_pkg "libcurl" || check_custom "libcurl" "curl-config" || fail "libcurl"
check_pkg "libxml-2.0" || check_custom "libxml2" "xml2-config" || fail "libxml2"
