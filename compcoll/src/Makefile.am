
AM_CPPFLAGS=
AM_LDFLAGS=

#noinst_PROGRAMS=lzssdran

bin_PROGRAMS=compcoll ucdet htmlescape

compcoll_SOURCES= \
    getline.c \
    editdistance.c \
    utf8utils.c \
    mymat.c \
    compcoll.c \
    $(NULL)

#compcoll_CPPFLAGS = $(AM_CPPFLAGS)
#compcoll_LDFLAGS = $(AM_LDFLAGS)
#compcoll_LDADD = #$(top_builddir)/src/libmylib.la

htmlescape_SOURCES= \
    getline.c \
    octstring.c \
    escape.c \
    $(NULL)

ucdet_SOURCES= \
    dummy.cpp \
    getline.c \
    i18n.c \
    ucdet.c \
    $(NULL)

# force the autoconf use g++ to link the c program with c++ lib
dummy.cpp:
	touch $@

ucdet_LDADD=

# add the required lib to the end of link command line
ucdet_LDADD+=$(LIBCHSETDET_CFLAGS) $(LIBCHSETDET_LIBS)

#ucdet_LDADD += /usr/lib/x86_64-linux-gnu/libicui18n.a /usr/lib/x86_64-linux-gnu/libicuuc.a /usr/lib/x86_64-linux-gnu/libicudata.a -ldl
#ucdet_LDADD +=/usr/lib/libucd.a
AM_CPPFLAGS += `pkg-config --cflags libucd`
AM_LDFLAGS += `pkg-config --libs libucd`

#ucdet_CPPFLAGS = $(LIBCHSETDET_CFLAGS) $(AM_CPPFLAGS)
#ucdet_LDFLAGS = $(LIBCHSETDET_LIBS) $(AM_LDFLAGS)






AM_LDFLAGS+=$(OPTS)
AM_CPPFLAGS+=$(OPTS)

DEFS+= \
    `getconf LFS_CFLAGS` \
    `getconf LFS64_CFLAGS` \
    -D_GNU_SOURCE \
    -D_FILE_OFFSET_BITS=64 \
    -DHAVE_MMAP64=1 \
    -DUSE_FINDHTML=1 \
    $(NULL)

AM_CPPFLAGS+= \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/include/ \
    $(NULL)

AM_LDFLAGS += \
    -L$(top_builddir)/src/ \
    `getconf LFS_LDFLAGS` \
    `getconf LFS64_LDFLAGS` \
    $(NULL)

if USE_ICONV
DEFS+= -DUSE_ICONV=1
#AM_LDFLAGS+=-L/usr/lib/x86_64-linux-gnu/
#AM_LDFLAGS += -liconv # iconv is a part of glib now, yum install glibc-common
#AM_CPPFLAGS += `pkg-config --cflags libucd`
#AM_LDFLAGS += `pkg-config --libs libucd`
AM_CPPFLAGS += $(LIBCHSETDET_CFLAGS)
AM_LDFLAGS += $(LIBCHSETDET_LIBS)

else
#AM_LDFLAGS+=-L/usr/lib/x86_64-linux-gnu/
#AM_CPPFLAGS += `pkg-config --cflags icu`
#AM_LDFLAGS+= `pkg-config --libs icu`
AM_CPPFLAGS += `pkg-config --cflags icu-i18n`
AM_LDFLAGS+= `pkg-config --libs icu-i18n`
#libseederdict_la_CPPFLAGS += $(LIBICU_CFLAGS)
#libseederdict_la_LDFLAGS+= $(LIBICU_LIBS)
endif

#AM_CPPFLAGS += `pkg-config --cflags zlib`
#AM_LDFLAGS += `pkg-config --libs zlib`
#AM_CPPFLAGS += $(ZLIB_CFLAGS)
#AM_LDFLAGS += $(ZLIB_LIBS)

#AM_LDFLAGS += -pthread


if DEBUG
# use "valgrind --tool=memcheck --leak-check=yes" to check memory leak, MemWatch will drag the program.
#DEFS+=-DMEMWATCH
DEFS+= -DDEBUG=1
AM_LDFLAGS += -lbfd
AM_CPPFLAGS+=-g -Wall

else
AM_CPPFLAGS+=-O3 -Wall
endif

#AM_CPPFLAGS+=-fpermissive


EXT_FLAGS=
@MK@GITNUMTMP=$(shell cd "$(top_srcdir)"; A=$$(git show | head -n 1 | awk '{print $$2}'); echo $${A:0:7}; cd - > /dev/null )
#@MK@SVNNUMTMP=$(shell cd "$(top_srcdir)"; LC_ALL=C svn info | grep -i Revision | awk '{print $$2}'; cd - > /dev/null )
#@MK@ifeq ($(SVNNUMTMP),)
#EXT_FLAGS+= -DSVN_VERSION='"${GITNUMTMP}"'
#@MK@else
#EXT_FLAGS+= -DSVN_VERSION='"${SVNNUMTMP}"'
#@MK@endif
@MK@ifeq ($(GITNUMTMP),)
@MK@else
EXT_FLAGS+= -DSVN_VERSION='"${GITNUMTMP}"'
@MK@endif
DEFS+=$(EXT_FLAGS)

#AM_CPPFLAGS+=$(EXT_FLAGS)
