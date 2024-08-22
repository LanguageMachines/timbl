AM_CPPFLAGS = -I@top_srcdir@/include
AM_CXXFLAGS = -std=c++17 -W -Wall -O3 -g -pedantic

bin_PROGRAMS = timbl

check_PROGRAMS = simpletest
TESTS = $(check_PROGRAMS)
TESTS_ENVIRONMENT = topsrcdir=$(top_srcdir)
simpletest_SOURCES = simpletest.cxx
CLEANFILES = dimin.out

LDADD = libtimbl.la

timbl_SOURCES = Timbl.cxx
lib_LTLIBRARIES = libtimbl.la
libtimbl_la_LDFLAGS= -version-info 7:0:0

libtimbl_la_SOURCES = Common.cxx \
	GetOptClass.cxx IBtree.cxx IBprocs.cxx \
	Targets.cxx Features.cxx Instance.cxx \
	MBLClass.cxx MsgClass.cxx \
	StringOps.cxx TimblAPI.cxx Choppers.cxx\
	TimblExperiment.cxx IGExperiment.cxx Metrics.cxx Testers.cxx \
	TRIBLExperiments.cxx LOOExperiment.cxx CVExperiment.cxx \
	Types.cxx neighborSet.cxx Statistics.cxx BestArray.cxx
