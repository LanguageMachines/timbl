# $Id$
# $URL$

ACLOCAL_AMFLAGS =-I m4 --install

SUBDIRS = src include demos docs init m4

EXTRA_DIST = bootstrap AUTHORS TODO NEWS timbl.pc.in timblserver.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = timbl.pc timblserver.pc

$(top_srcdir)/ChangeLog: $(top_srcdir)/NEWS
	svn update; svn2cl --include-rev
