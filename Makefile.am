SUBDIRS =								\
	doc								\
	pixmaps								\
	po								\
	terminal

distclean-local:
	rm -rf *.spec *.cache *~ TerminalHelp

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

libexec_SCRIPTS =							\
	TerminalHelp

TerminalHelp: TerminalHelp.in Makefile
	rm -f TerminalHelp.gen TerminalHelp
	sed -e "s,\@datadir\@,$(datadir),g" 				\
		< $(srcdir)/TerminalHelp.in				\
		> TerminalHelp.gen
	mv TerminalHelp.gen TerminalHelp

desktopdir = $(datadir)/applications
desktop_DATA = Terminal.desktop

man_MANS = Terminal.1

uidir = $(datadir)/Terminal
ui_DATA = 								\
	Terminal-toolbars.ui						\
	Terminal.ui

EXTRA_DIST =								\
	$(desktop_DATA)							\
	$(man_MANS)							\
	$(ui_DATA)							\
	AUTHORS								\
	COPYING								\
	ChangeLog							\
	HACKING								\
	INSTALL								\
	NEWS								\
	README								\
	THANKS								\
	TODO								\
	TerminalHelp.in							\
	Terminal.spec.in						\
	Terminal.spec

ACLOCAL_AMFLAGS = -I m4
