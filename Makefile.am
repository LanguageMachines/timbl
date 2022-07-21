
ACLOCAL_AMFLAGS =-I m4 --install

SUBDIRS = src include demos docs m4

EXTRA_DIST = bootstrap.sh AUTHORS TODO NEWS README.md timbl.pc.in codemeta.json

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = timbl.pc

ChangeLog: NEWS
	git pull; git2cl > ChangeLog

docker:
	docker build -t timbl:latest .

docker-dev:
	docker build -t timbl:dev --build-arg VERSION=development .

deps:
	./build-deps.sh
