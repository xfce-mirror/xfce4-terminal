SUBDIRS =								\
	doc								\
	helpers								\
	icons								\
	po								\
	terminal

AUTOMAKE_OPTIONS =							\
	1.8								\
	dist-bzip2

distclean-local:
	rm -rf *.cache *~

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
desktop_in_files = Terminal.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

man_MANS = Terminal.1

uidir = $(datadir)/Terminal
ui_DATA = 								\
	Terminal-toolbars.ui						\
	Terminal.ui

EXTRA_DIST =								\
	HACKING								\
	TerminalHelp.in							\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	$(desktop_in_files)						\
	$(man_MANS)							\
	$(ui_DATA)

DISTCLEANFILES =							\
	Terminal.spec							\
	TerminalHelp							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	$(desktop_DATA)

DISTCHECK_CONFIGURE_FLAGS = --enable-xsltproc

# vi:set ts=8 sw=8 noet ai nocindent:
