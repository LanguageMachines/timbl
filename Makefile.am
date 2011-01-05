# $Id$
# $URL$

ACLOCAL_AMFLAGS =-I m4 --install

SUBDIRS = src include demos docs m4

EXTRA_DIST = bootstrap AUTHORS TODO NEWS timbl.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = timbl.pc

ChangeLog: NEWS
	svn update; svn2cl --include-rev
