SUBDIRS =								\
	icons								\
	po								\
	terminal

distclean-local:
	rm -rf *.spec *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

desktopdir = $(datadir)/applications
desktop_DATA = Terminal.desktop

EXTRA_DIST =								\
	$(desktop_DATA)							\
	AUTHORS								\
	COPYING								\
	ChangeLog							\
	HACKING								\
	INSTALL								\
	NEWS								\
	README								\
	TODO								\
	Terminal.spec.in						\
	Terminal.spec

ACLOCAL_AMFLAGS = -I m4
