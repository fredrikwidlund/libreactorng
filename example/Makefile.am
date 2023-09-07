ACLOCAL_AMFLAGS			= ${ACLOCAL_FLAGS} -I m4L
AM_CFLAGS			= -std=gnu2x -g -O3 -flto=auto -rdynamic -march=native -I../src -fPIC
AM_LDFLAGS			= -static
LDADD				= ../libreactor.la

bin_PROGRAMS 			= \
buffer \
encode \
validate \
shufflecat \
simpleread \
signal \
timeout \
notify \
lookup \
scan \
stream \
clients \
server \
mpmc \
queue \
flow

flow_LDADD 			= -lltdl ../libreactor.la

lib_LTLIBRARIES 		= \
module1.la \
module2.la

module1_la_LDFLAGS 		= -module -avoid-version
module1_la_LIBADD		= ../libreactor.la
module2_la_LDFLAGS 		= -module -avoid-version
module2_la_LIBADD		= ../libreactor.la

MAINTAINERCLEANFILES 		= Makefile.in
