stock_icons =								\
	stock-icon-advanced48.png					\
	stock-icon-appearance48.png					\
	stock-icon-closetab16.png					\
	stock-icon-closetab24.png					\
	stock-icon-closewindow16.png					\
	stock-icon-closewindow24.png					\
	stock-icon-colors48.png						\
	stock-icon-compose48.png					\
	stock-icon-fullscreen24.png					\
	stock-icon-general48.png					\
	stock-icon-newtab16.png						\
	stock-icon-newtab24.png						\
	stock-icon-newwindow16.png					\
	stock-icon-newwindow24.png					\
	stock-icon-reportbug16.png					\
	stock-icon-reportbug24.png					\
	stock-icon-shortcuts48.png					\
	stock-icon-showborders24.png					\
	stock-icon-showmenu24.png

INCLUDES =								\
	-DDATADIR=\"$(datadir)\"					\
	-DEXO_API_SUBJECT_TO_CHANGE					\
	-DEXO_DISABLE_DEPRECATED					\
	-DG_LOG_DOMAIN=\"Terminal\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
	-DSN_API_NOT_YET_FROZEN						\
	-DTERMINAL_HELP_BIN=\"$(libexecdir)/TerminalHelp\"		\
	-I$(top_srcdir)

bin_PROGRAMS = 								\
	Terminal

Terminal_build_sources =						\
	terminal-enum-types.c						\
	terminal-enum-types.h

Terminal_headers =							\
	terminal-accel-map.h						\
	terminal-app.h							\
	terminal-image-loader.h						\
	terminal-options.h						\
	terminal-preferences.h						\
	terminal-preferences-dialog.h					\
	terminal-shortcut-editor.h					\
	terminal-tab-header.h						\
	terminal-toolbars-model.h					\
	terminal-toolbars-view.h					\
	terminal-widget.h						\
	terminal-window.h

Terminal_SOURCES =							\
	$(Terminal_build_sources)					\
	$(Terminal_headers)						\
	main.c								\
	stock-icons.h							\
	terminal-accel-map.c						\
	terminal-app.c							\
	terminal-image-loader.c						\
	terminal-options.c						\
	terminal-preferences.c						\
	terminal-preferences-dialog.c					\
	terminal-shortcut-editor.c					\
	terminal-tab-header.c						\
	terminal-toolbars-model.c					\
	terminal-toolbars-view.c					\
	terminal-widget.c						\
	terminal-window.c

Terminal_CFLAGS =							\
	@EXO_CFLAGS@							\
	@LIBSTARTUP_NOTIFICATION_CFLAGS@				\
	@VTE_CFLAGS@

Terminal_LDFLAGS =							\
	-no-undefined							\
	@EXO_LIBS@							\
	@LIBSTARTUP_NOTIFICATION_LIBS@					\
	@VTE_LIBS@

# install symlink to 'terminal'
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	( cd $(DESTDIR)$(bindir) ; ln -sf Terminal terminal )

# Used to regenerate all automatic files
regenerate:
	echo "Regenerating source files"
	( cd $(srcdir) && gdk-pixbuf-csource --static --struct --build-list \
		stock_advanced48 stock-icon-advanced48.png \
		stock_appearance48 stock-icon-appearance48.png \
		stock_closetab16 stock-icon-closetab16.png \
		stock_closetab24 stock-icon-closetab24.png \
		stock_closewindow16 stock-icon-closewindow16.png \
		stock_closewindow24 stock-icon-closewindow24.png \
		stock_colors48 stock-icon-colors48.png \
		stock_compose48 stock-icon-compose48.png \
		stock_fullscreen24 stock-icon-fullscreen24.png \
		stock_general48 stock-icon-general48.png \
		stock_newtab16 stock-icon-newtab16.png \
		stock_newtab24 stock-icon-newtab24.png \
		stock_newwindow16 stock-icon-newwindow16.png \
		stock_newwindow24 stock-icon-newwindow24.png \
		stock_reportbug16 stock-icon-reportbug16.png \
		stock_reportbug24 stock-icon-reportbug24.png \
		stock_shortcuts48 stock-icon-shortcuts48.png \
		stock_showborders24 stock-icon-showborders24.png \
		stock_showmenu24 stock-icon-showmenu24.png ) \
	> stock-icons.h
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __TERMINAL_ENUM_TYPES_H__\n#define __TERMINAL_ENUM_TYPES_H__\n#include <libxfce4util/libxfce4util.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define TERMINAL_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* !__TERMINAL_ENUM_TYPES_H__ */" \
		$(Terminal_headers) ) > xgen \
	&& (cmp -s xgen terminal-enum-types.h || cp xgen terminal-enum-types.h ) \
	&& rm -f xgen
	( cd $(srcdir) && glib-mkenums \
		--fhead "#undef GTK_DISABLE_DEPRECATED\n#define GTK_ENABLE_BROKEN\n#include <terminal/terminal-enum-types.h>\n#include <terminal/terminal-options.h>\n#include <terminal/terminal-preferences.h>" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {"\
		--vprod "\t{ @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "\t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n\treturn type;\n}\n" \
		$(Terminal_headers) ) > xgen \
	&& (cmp -s xgen terminal-enum-types.c || cp xgen terminal-enum-types.c ) \
	&& rm -f xgen
	echo "Regenerated source files"

clean-local:
	rm -f *.core core core.* stamp-*.h

EXTRA_DIST =								\
	terminal-config.h.in

