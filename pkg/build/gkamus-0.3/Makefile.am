## Proses file ini dengan automake untuk output Makefile.in

SUBDIRS = src pixmaps data

gkamusdocdir = ${prefix}/doc/gkamus
gkamusdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO

gkamusmenudir = ${prefix}/share/applications
gkamusmenu_DATA = gkamus.desktop

EXTRA_DIST = $(gkamusdoc_DATA) \
             $(gkamusmenu_DATA)

# Copy semua file spec, hanya satu yang akan digunakan!
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done
