AUTOMAKE_OPTIONS		= subdir-objects
ACLOCAL_AMFLAGS			= ${ACLOCAL_FLAGS} -I m4
DIST_SUBDIRS                    = example

SOURCE_FILES = \
src/picohttpparser/picohttpparser.c \
src/reactor/data.c \
src/reactor/hash.c \
src/reactor/buffer.c \
src/reactor/list.c \
src/reactor/vector.c \
src/reactor/map.c \
src/reactor/mapi.c \
src/reactor/maps.c \
src/reactor/mapd.c \
src/reactor/string.c \
src/reactor/value.c \
src/reactor/encode.c \
src/reactor/decode.c \
src/reactor/pool.c \
src/reactor/reactor.c \
src/reactor/signal.c \
src/reactor/timeout.c \
src/reactor/notify.c \
src/reactor/network.c \
src/reactor/stream.c \
src/reactor/http.c \
src/reactor/server.c \
src/reactor/queue.c \
src/reactor/flow.c

HEADER_FILES = \
src/picohttpparser/picohttpparser.h \
src/reactor/data.h \
src/reactor/hash.h \
src/reactor/buffer.h \
src/reactor/list.h \
src/reactor/vector.h \
src/reactor/map.h \
src/reactor/mapi.h \
src/reactor/maps.h \
src/reactor/mapd.h \
src/reactor/string.h \
src/reactor/value.h \
src/reactor/encode.h \
src/reactor/decode.h \
src/reactor/pool.h \
src/reactor/reactor.h \
src/reactor/signal.h \
src/reactor/timeout.h \
src/reactor/notify.h \
src/reactor/network.h \
src/reactor/stream.h \
src/reactor/http.h \
src/reactor/server.h \
src/reactor/queue.h \
src/reactor/flow.h

lib_LTLIBRARIES 		= libreactor.la
libreactor_la_SOURCES 		= $(SOURCE_FILES) $(HEADER_FILES)
libreactor_la_CFLAGS		= -std=gnu2x -g -O3 -flto=auto -rdynamic -march=native -I$(srcdir)/src -fPIC
libreactor_la_LDFLAGS		= -static

headerfilesdir                  = $(includedir)/reactor
headerfiles_HEADERS             = $(HEADER_FILES)

mainheaderdir 			= $(includedir)
mainheader_HEADERS 		= src/reactor.h

pkgconfigdir 			= $(libdir)/pkgconfig
pkgconfig_DATA 			= libreactor.pc

MAINTAINERCLEANFILES 		= aclocal.m4 config.h.in configure Makefile.in libreactor-?.?.?.tar.gz

CLEANFILES 			= {.,test/,test/conf,src/reactor/,src/picohttpparser}/*.{gcno,gcda,gcov}

maintainer-clean-local:
	rm -rf autotools m4 libreactor-?.?.?

indent:
	clang-format -i src/reactor/*.c test/*.c

### unit testing ###

AM_CFLAGS 			= -std=gnu2x -g -O0 -rdynamic -fPIC -ftest-coverage -fprofile-arcs -I$(srcdir)/src -DGCOV_BUILD -DUNIT_TESTING
LDADD 				= -L. libreactor_test.a -lcmocka

check_LIBRARIES 		= libreactor_test.a
libreactor_test_a_CFLAGS        = $(AM_CFLAGS)
libreactor_test_a_SOURCES 	= $(SOURCE_FILES) $(HEADER_FILES)

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
test/encode \
test/pool \
test/reactor \
test/signal \
test/timeout \
test/notify \
test/network_resolve \
test/network \
test/stream \
test/http \
test/server \
test/queue \
test/flow \
test/flow_async

test_flow_LDADD			= $(LDADD) -lltdl
test_flow_async_LDADD		= $(LDADD) -lltdl

check_LTLIBRARIES 		= \
test/conf/module1.la \
test/conf/module2.la

test_conf_module1_la_LDFLAGS 	= -rpath /foo -module -avoid-version 
test_conf_module1_la_LIBADD  	=  -L. -lreactor
test_conf_module2_la_LDFLAGS 	= -rpath /foo -module -avoid-version
test_conf_module2_la_LIBADD  	=  -L. -lreactor

dist_noinst_SCRIPTS             = test/valgrind.sh test/coverage.sh

TESTS 				= $(check_PROGRAMS) test/valgrind.sh test/coverage.sh
