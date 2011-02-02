# $Id$
# $URL$

AM_CPPFLAGS = -I@top_srcdir@/include

bin_PROGRAMS = Timbl

LDADD = libTimbl.la

Timbl_SOURCES = Timbl.cxx
lib_LTLIBRARIES = libTimbl.la
libTimbl_la_LDFLAGS= -version-info 3:0:0

libTimbl_la_SOURCES = CommandLine.cxx Common.cxx XMLtools.cxx\
	GetOptClass.cxx IBtree.cxx IBprocs.cxx Instance.cxx \
	MBLClass.cxx MsgClass.cxx LogBuffer.cxx LogStream.cxx \
	StringOps.cxx TimblAPI.cxx Choppers.cxx\
	TimblExperiment.cxx IGExperiment.cxx Metrics.cxx Testers.cxx \
	TRIBLExperiments.cxx LOOExperiment.cxx CVExperiment.cxx \
	Tree.cxx Types.cxx neighborSet.cxx Statistics.cxx BestArray.cxx
