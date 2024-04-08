

presets_in_files = 							\
	black-on-white.desktop.in						\
	dark-pastels.desktop.in						\
	green-on-black.desktop.in						\
	solarized-dark.desktop.in						\
	solarized-light.desktop.in					\
	tango.desktop.in							\
	white-on-black.desktop.in						\
	xterm.desktop.in

presetsdir = $(datadir)/xfce4/terminal/colorschemes
presets_DATA = $(presets_in_files:.desktop.in=.theme)

# FIXME: xgettext doesn't recognize .theme.in files as desktop files so we
# use .desktop.in for the translation: is there a proper solution?
%.theme: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = $(presets_in_files)

DISTCLEANFILES = $(presets_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
