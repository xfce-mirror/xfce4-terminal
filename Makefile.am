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

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

package_exec=`echo $(PACKAGE) | sed '$(program_transform_name)'`

desktopdir = $(datadir)/applications
desktop_in_in_files = Terminal.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
%.desktop.in: %.desktop.in.in
	sed -e "s,\@PACKAGE_EXEC\@,$(package_exec),g" < $< > $@
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

if HAVE_GNOME_DEFAULT_APPSDIR
xmldir = $(GNOME_DEFAULT_APPSDIR)
else
xmldir = $(datadir)/gnome-control-center/default-apps
endif
xml_in_in_files = Terminal-default-apps.xml.in.in
xml_in_files = $(xml_in_in_files:.xml.in.in=.xml.in)
%.xml.in: %.xml.in.in
	sed -e "s,\@PACKAGE_EXEC\@,$(package_exec),g" < $< > $@
xml_DATA = $(xml_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

uidir = $(datadir)/Terminal
ui_DATA = \
	Terminal-toolbars.ui \
	Terminal.glade

EXTRA_DIST = \
	HACKING \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(desktop_in_in_files) \
	$(xml_in_in_files) \
	$(ui_DATA)

DISTCLEANFILES = \
	Terminal.spec \
	intltool-extract \
	intltool-merge \
	intltool-update \
	$(desktop_in_files) \
	$(desktop_DATA) \
	$(xml_in_files) \
	$(xml_DATA)

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gen-doc

# vi:set ts=8 sw=8 noet ai nocindent:
