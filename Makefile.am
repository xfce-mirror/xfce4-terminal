SUBDIRS =								\
	doc								\
	icons								\
	po								\
	terminal

distclean-local:
	rm -rf *.spec *.cache *~

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

uidir = $(datadir)/Terminal
ui_DATA = Terminal.ui

EXTRA_DIST =								\
	$(desktop_DATA)							\
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
