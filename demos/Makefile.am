# $Id$
# $URL: $

AM_CPPFLAGS = -I@top_srcdir@/include

if WANT_OMP
noinst_PROGRAMS = api_test1 api_test2 api_test3 api_test4 api_test5 api_test6\
	tse classify sockettestServer sockettestClient
else
noinst_PROGRAMS = api_test1 api_test2 api_test3 api_test4 api_test5 api_test6\
	tse classify
endif

LDADD = ../src/libTimbl.la

tse_SOURCES = tse.cxx

classify_SOURCES = classify.cxx

if WANT_OMP
sockettestClient_SOURCES = sockettestClient.cxx
sockettestServer_SOURCES = sockettestServer.cxx
endif

api_test1_SOURCES = api_test1.cxx

api_test2_SOURCES = api_test2.cxx

api_test3_SOURCES = api_test3.cxx

api_test4_SOURCES = api_test4.cxx

api_test5_SOURCES = api_test5.cxx

api_test6_SOURCES = api_test6.cxx

exdir = $(datadir)/doc/@PACKAGE@/examples
 
ex_DATA = dimin.script dimin.train dimin.test cross_val.test \
	small_1.train small_2.train small_3.train small_4.train small_5.train

 
EXTRA_DIST = $(ex_DATA)
