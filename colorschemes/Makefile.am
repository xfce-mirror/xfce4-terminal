

presets_in_files = 							\
	black-on-white.theme.in						\
	dark-pastels.theme.in						\
	green-on-black.theme.in						\
	solarized-dark.theme.in						\
	solarized-light.theme.in					\
	tango.theme.in							\
	white-on-black.theme.in						\
	xterm.theme.in

presetsdir = $(datadir)/xfce4/terminal/colorschemes
presets_DATA = $(presets_in_files:.theme.in=.theme)
@INTLTOOL_THEME_RULE@

EXTRA_DIST = $(presets_in_files)

DISTCLEANFILES = $(presets_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
