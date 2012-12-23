

presets_in_files = 							\
	black-on-white.scheme.in					\
	dark-pastels.scheme.in						\
	green-on-black.scheme.in					\
	solarized-dark.scheme.in					\
	tango.scheme.in							\
	white-on-black.scheme.in					\
	xterm.scheme.in

presetsdir = $(datadir)/xfce4/terminal/colorschemes
presets_DATA = $(presets_in_files:.scheme.in=.scheme)

%.scheme: %.scheme.in
	$(AM_V_GEN) $(MKDIR_P) $(dir $@); LC_ALL=C $(INTLTOOL_MERGE) -d -u -c \
	$(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@

EXTRA_DIST = $(presets_in_files)

DISTCLEANFILES = $(presets_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
