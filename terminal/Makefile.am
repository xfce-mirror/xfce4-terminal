INCLUDES =								\
	-DEXO_API_SUBJECT_TO_CHANGE					\
	-DG_LOG_DOMAIN=\"Terminal\"					\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
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
	terminal-options.h						\
	terminal-preferences.h						\
	terminal-preferences-dialog.h					\
	terminal-shortcut-editor.h					\
	terminal-tab-header.h						\
	terminal-widget.h						\
	terminal-window.h

Terminal_SOURCES =							\
	$(Terminal_build_sources)					\
	$(Terminal_headers)						\
	main.c								\
	stock-icons.h							\
	terminal-accel-map.c						\
	terminal-app.c							\
	terminal-options.c						\
	terminal-preferences.c						\
	terminal-preferences-dialog.c					\
	terminal-shortcut-editor.c					\
	terminal-tab-header.c						\
	terminal-widget.c						\
	terminal-window.c

Terminal_CFLAGS =							\
	@EXO_CFLAGS@							\
	@VTE_CFLAGS@

Terminal_LDFLAGS =							\
	-no-undefined							\
	@EXO_LIBS@							\
	@VTE_LIBS@

stock-icons.h: stamp-stock-icons.h
	@true
stamp-stock-icons.h: stock-icon-newtab.png stock-icon-newwindow.png \
                     stock-icon-reportbug.png Makefile
	( cd $(srcdir) && gdk-pixbuf-csource --static --struct --build-list \
		stock_newtab stock-icon-newtab.png \
		stock_newwindow stock-icon-newwindow.png \
		stock_reportbug stock-icon-reportbug.png > xgen) \
	&& cp xgen stock-icons.h \
	&& rm -f xgen \
	&& echo timestamp > $(@F)

terminal-enum-types.h: stamp-terminal-enum-types.h
	@true
stamp-terminal-enum-types.h: $(Terminal_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __TERMINAL_ENUM_TYPES_H__\n#define __TERMINAL_ENUM_TYPES_H__\n#include <libxfce4util/libxfce4util.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define TERMINAL_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* !__TERMINAL_ENUM_TYPES_H__ */" \
		$(Terminal_headers) ) >> xgen \
	&& (cmp -s xgen terminal-enum-types.h || cp xgen terminal-enum-types.h ) \
	&& rm -f xgen \
	&& echo timestamp > $(@F)

terminal-enum-types.c: $(Terminal_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#undef GTK_DISABLE_DEPRECATED\n#define GTK_ENABLE_BROKEN\n#include <terminal/terminal-enum-types.h>\n#include <terminal/terminal-options.h>\n#include <terminal/terminal-preferences.h>" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {"\
		--vprod "\t{ @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "\t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n\treturn type;\n}\n" \
		$(Terminal_headers) ) > xgen \
	&& cp xgen terminal-enum-types.c  \
	&& rm -f xgen

clean-local:
	rm -f *.core core core.* stamp-*.h

EXTRA_DIST =								\
	stock-icon-newtab.png						\
	stock-icon-newwindow.png

