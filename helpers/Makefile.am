desktopdir = $(pkgdatadir)/apps
desktop_in_files =							\
	balsa.desktop.in						\
	epiphany.desktop.in						\
	evolution.desktop.in						\
	exo-open-browser.desktop.in					\
	exo-open-mailer.desktop.in					\
	firefox.desktop.in						\
	galeon.desktop.in						\
	kmail.desktop.in						\
	konqueror.desktop.in						\
	lynx.desktop.in							\
	mozilla-browser.desktop.in					\
	mozilla-mailer.desktop.in					\
	mutt.desktop.in							\
	opera-browser.desktop.in					\
	opera-mailer.desktop.in						\
	sensible-browser.desktop.in					\
	sylpheed-claws.desktop.in					\
	thunderbird.desktop.in
desktop_DATA =								\
	$(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES =							\
	$(desktop_DATA)

EXTRA_DIST =								\
	$(desktop_in_files)
