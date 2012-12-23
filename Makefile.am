SUBDIRS = \
	colorschemes \
	doc \
	po \
	terminal

distclean-local:
	rm -rf *.cache *~

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

desktopdir = $(datadir)/applications
desktop_in_files = xfce4-terminal.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

if HAVE_GNOME_DEFAULT_APPSDIR
xmldir = $(GNOME_DEFAULT_APPSDIR)
else
xmldir = $(datadir)/gnome-control-center/default-apps
endif
xml_in_in_files = xfce4-terminal-default-apps.xml.in.in
xml_in_files = $(xml_in_in_files:.xml.in.in=.xml.in)
%.xml.in: %.xml.in.in
	sed -e "s,\@PACKAGE_EXEC\@,$(package_exec),g" < $< > $@
xml_DATA = $(xml_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST = \
	HACKING \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(desktop_in_files) \
	$(xml_in_in_files)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(desktop_DATA) \
	$(xml_in_files) \
	$(xml_DATA)

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gen-doc

# vi:set ts=8 sw=8 noet ai nocindent:
