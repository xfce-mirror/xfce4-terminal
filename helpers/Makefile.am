DESKTOP_IN_FILES =							\
	firefox.desktop.in						\
	konqueror.desktop.in						\
	lynx.desktop.in							\
	mozilla-browser.desktop.in					\
	mozilla-mailer.desktop.in					\
	mutt.desktop.in							\
	opera-browser.desktop.in					\
	opera-mailer.desktop.in						\
	thunderbird.desktop.in


DESKTOP_FILES =								\
	$(DESKTOP_IN_FILES:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@


helpersdir = $(datadir)/Terminal
helpers_DATA = helpers.ini


helpers.ini: stamp-helpers.ini
	@true
stamp-helpers.ini: $(DESKTOP_FILES) Makefile
	(for file in $(DESKTOP_FILES); do				\
		section=`echo $$file | sed -e 's|\.desktop$$||'` &&	\
		echo "[$$section]" &&					\
		grep '^Name' $$file &&					\
		sed -ne 's/^X-Terminal-\(.*\)/\1/p' $$file &&		\
	 	echo;							\
		done) > xgen-hi						\
	&& (cmp -s xgen-hi helpers.ini || cp xgen-hi helpers.ini)	\
	&& rm -f xgen-hi						\
	&& echo timestamp > $(@F)

CLEANFILES =								\
	xgen-hi

DISTCLEANFILES =							\
	stamp-helpers.ini						\
	$(DESKTOP_FILES)						\
	$(helpers_DATA)

EXTRA_DIST =								\
	$(DESKTOP_IN_FILES)
