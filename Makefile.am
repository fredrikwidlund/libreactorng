AUTOMAKE_OPTIONS		= subdir-objects
ACLOCAL_AMFLAGS			= ${ACLOCAL_FLAGS} -I m4
DIST_SUBDIRS                    = example

SOURCE_FILES = \
src/value/data.c \
src/value/hash.c \
src/value/buffer.c \
src/value/list.c \
src/value/vector.c \
src/value/map.c \
src/value/mapi.c \
src/value/maps.c \
src/value/mapd.c \
src/value/string.c \
src/value/value.c \
src/value/encode.c \
src/value/decode.c

HEADER_FILES = \
src/value/data.h \
src/value/hash.h \
src/value/buffer.h \
src/value/list.h \
src/value/vector.h \
src/value/map.h \
src/value/mapi.h \
src/value/maps.h \
src/value/mapd.h \
src/value/string.h \
src/value/value.h \
src/value/encode.h \
src/value/decode.h

lib_LTLIBRARIES 		= libvalue.la
libvalue_la_SOURCES 		= $(SOURCE_FILES) $(HEADER_FILES)
libvalue_la_CFLAGS		= -std=gnu2x -g -O3 -flto=auto -march=native -I$(srcdir)/src -fPIC
libvalue_la_LDFLAGS		= -static

headerfilesdir                  = $(includedir)/value
headerfiles_HEADERS             = $(HEADER_FILES)

mainheaderdir 			= $(includedir)
mainheader_HEADERS 		= src/value.h

pkgconfigdir 			= $(libdir)/pkgconfig
pkgconfig_DATA 			= libvalue.pc

MAINTAINERCLEANFILES 		= aclocal.m4 config.h.in configure Makefile.in libvalue-?.?.?.tar.gz

CLEANFILES 			= {.,test/,src/value/,src/picohttpparser}/*.{gcno,gcda,gcov}

maintainer-clean-local:
	rm -rf autotools m4 libvalue-?.?.?

indent:
	clang-format -i src/value/*.c test/*.c

### unit testing ###

AM_CFLAGS 			= -std=gnu2x -O0 -g -ftest-coverage -fprofile-arcs -I$(srcdir)/src -DGCOV_BUILD -DUNIT_TESTING
LDADD 				= -L. libvalue_test.a -lcmocka

check_LIBRARIES 		= libvalue_test.a
libvalue_test_a_CFLAGS 	        = $(AM_CFLAGS)
libvalue_test_a_SOURCES 	= $(SOURCE_FILES) $(HEADER_FILES)

check_PROGRAMS 			= \
test/include \
test/data \
test/hash \
test/buffer \
test/list \
test/vector \
test/map \
test/mapi \
test/maps \
test/mapd \
test/string \
test/value \
test/encode

dist_noinst_SCRIPTS             = test/valgrind.sh test/coverage.sh

TESTS 				= $(check_PROGRAMS) test/valgrind.sh test/coverage.sh
