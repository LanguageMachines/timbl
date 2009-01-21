# $Id$
# $URL: $

ACLOCAL_AMFLAGS =-I m4

SUBDIRS = src include demos docs init

EXTRA_DIST = bootstrap AUTHORS TODO NEWS timbl.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = timbl.pc

docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = ChangeLog

$(top_srcdir)/ChangeLog: $(top_srcdir)/NEWS
	svn update; svn2cl --include-rev
