SUBDIRS = \
	doc \
	icons \
	po \
	terminal

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

libexec_SCRIPTS = \
	TerminalHelp

TerminalHelp: TerminalHelp.in Makefile
	rm -f TerminalHelp.gen TerminalHelp
	sed -e "s,\@docdir\@,$(docdir),g" \
		< $(srcdir)/TerminalHelp.in \
		> TerminalHelp.gen
	mv TerminalHelp.gen TerminalHelp

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

desktopdir = $(datadir)/applications
desktop_in_files = Terminal.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

if HAVE_GNOME_DEFAULT_APPSDIR
xmldir = $(GNOME_DEFAULT_APPSDIR)
else
xmldir = $(datadir)/gnome-control-center/default-apps
endif
xml_in_files = Terminal-default-apps.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

man_MANS = Terminal.1

uidir = $(datadir)/Terminal
ui_DATA = \
	Terminal-toolbars.ui \
	Terminal.glade

EXTRA_DIST = \
	HACKING \
	TerminalHelp.in \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(desktop_in_files) \
	$(xml_in_files) \
	$(man_MANS) \
	$(ui_DATA)

DISTCLEANFILES = \
	Terminal.spec \
	TerminalHelp \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(desktop_DATA) \
	$(xml_DATA)

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gen-doc

# vi:set ts=8 sw=8 noet ai nocindent:
