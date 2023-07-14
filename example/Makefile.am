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
notify

MAINTAINERCLEANFILES 		= Makefile.in
