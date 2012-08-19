#!/bin/sh

FAILSTATUS=""

check_pkg() {
	pkgname=$1
	add_define=$2
	pkgconfig_args=$3
	echo -n "Checking for package ${pkgname}... "
	if pkg-config --silence-errors "${pkgname}" ; then
		echo "found"
		echo "# configuration for package ${pkgname}" >> config.mk
		result=`pkg-config --cflags $pkgconfig_args ${pkgname}`
		if [ -n "$result" ] ; then
			echo "DEFINES+=$result" >> config.mk
		fi
		if [ -n "$add_define" ] ; then
			echo "DEFINES+=${add_define}" >> config.mk
		fi
		echo "LDFLAGS+=`pkg-config --libs $pkgconfig_args ${pkgname}`" >> config.mk
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
		result=`${customconfig} --cflags`
		if [ -n "$result" ] ; then
			echo "DEFINES+=${result}" >> config.mk
		fi
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
	dlurl=`grep -i "$pkgname" README | awk '{ print $NF }'`
	echo ""
	echo "You need package ${pkgname} in order to compile this program."
	echo "Please make sure it is installed."
	echo ""
	echo "You can download ${pkgname} from here: ${dlurl}"
	FAILSTATUS="1"
}

fail_custom() {
	err=$1
	echo ""
	echo "ERROR: ${err}"
	FAILSTATUS="1"
}

all_aboard_the_fail_boat() {
	if [ "x$FAILSTATUS" != "x" ] ; then
		rm -f config.mk
		echo ""
		echo "One or more dependencies couldn't be found. Please install"
		echo "these packages and retry compilation."
		exit 1
	fi
}

check_ssl_implementation() {
	if curl-config --static-libs | egrep -- "-lssl( |^)" > /dev/null 2>&1 ; then
		check_pkg "libcrypto" || fail "libcrypto"
		echo "DEFINES+=-DHAVE_OPENSSL=1" >> config.mk
	elif curl-config --static-libs | grep -- -lgcrypt > /dev/null 2>&1 ; then
		check_pkg "gnutls" || fail "gnutls"
		check_custom "libgcrypt" "libgcrypt-config" || fail "libgcrypt"
		echo "DEFINES+=-DHAVE_GCRYPT=1" >> config.mk
	fi
}

echo "# overrides of LDFLAGS and LDFLAGS from environment" > config.mk
[ -n "$CXXFLAGS" ] && echo "CXXFLAGS+=$CXXFLAGS" >> config.mk
[ -n "$LDFLAGS" ] && echo "LDFLAGS+=$LDFLAGS" >> config.mk
echo "" >> config.mk

check_pkg "sqlite3" || fail "sqlite3"
check_pkg "libcurl" || check_custom "libcurl" "curl-config" || fail "libcurl"
check_pkg "libxml-2.0" || check_custom "libxml2" "xml2-config" || fail "libxml2"
check_pkg "stfl" || fail "stfl"
check_pkg "json" || fail "json"

if [ `uname -s` = "Darwin" ]; then
	check_custom "ncurses5.4" "ncurses5.4-config" || fail "ncurses5.4"
else 
	check_custom "ncursesw5" "ncursesw5-config" || fail "ncursesw5"
fi
check_ssl_implementation
all_aboard_the_fail_boat
