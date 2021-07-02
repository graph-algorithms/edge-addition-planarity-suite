SUBDIRS = c/samples

lib_LTLIBRARIES = libplanarity.la

libplanarity_la_SOURCES = \
	c/graphDFSUtils.c \
	c/graphDrawPlanar.c \
	c/graphDrawPlanar_Extensions.c \
	c/graphEmbed.c \
	c/graphExtensions.c \
	c/graphIO.c \
	c/graphIsolator.c \
	c/graphK23Search.c \
	c/graphK23Search_Extensions.c \
	c/graphK33Search.c \
	c/graphK33Search_Extensions.c \
	c/graphK4Search.c \
	c/graphK4Search_Extensions.c \
	c/graphNonplanar.c \
	c/graphOuterplanarObstruction.c \
	c/graphTests.c \
	c/graphUtils.c \
	c/listcoll.c \
	c/planarityRandomGraphs.c \
	c/planaritySpecificGraph.c \
	c/planarityUtils.c \
	c/stack.c
libplanarity_la_LDFLAGS = -no-undefined $(AM_LDFLAGS) -version-info @LT_CURRENT@:@LT_REVISION@:@LT_AGE@

pkginclude_HEADERS = \
	c/appconst.h \
	c/graph.h \
	c/graphDrawPlanar.h \
	c/graphDrawPlanar.private.h \
	c/graphExtensions.h \
	c/graphExtensions.private.h \
	c/graphFunctionTable.h \
	c/graphK23Search.h \
	c/graphK23Search.private.h \
	c/graphK33Search.h \
	c/graphK33Search.private.h \
	c/graphK4Search.h \
	c/graphK4Search.private.h \
	c/graphStructures.h \
	c/listcoll.h \
	c/planarity.h \
	c/platformTime.h \
	c/stack.h

bin_PROGRAMS = planarity
planarity_LDADD  = libplanarity.la
planarity_SOURCES = c/planarity.c c/planarityCommandLine.c

man1_MANS = planarity.1
EXTRA_DIST=$(man1_MANS)

TESTS = test-samples.sh

# The @docdir@ variable will be replaced by the ./configure script.
docdir = @docdir@
dist_doc_DATA = README.md LICENSE.TXT
