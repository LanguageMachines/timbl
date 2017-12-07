
ACLOCAL_AMFLAGS =-I m4 --install

SUBDIRS = src include demos docs m4

EXTRA_DIST = bootstrap.sh AUTHORS TODO NEWS timbl.pc.in README.md

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = timbl.pc

ChangeLog: NEWS
	git pull; git2cl > ChangeLog
