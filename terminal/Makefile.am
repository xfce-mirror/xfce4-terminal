SUBDIRS =								\
	stock-icons

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
	terminal-icons.h						\
	terminal-image-loader.h						\
	terminal-options.h						\
	terminal-preferences.h						\
	terminal-preferences-dialog.h					\
	terminal-screen.h						\
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
	terminal-accel-map.c						\
	terminal-app.c							\
	terminal-icons.c						\
	terminal-image-loader.c						\
	terminal-options.c						\
	terminal-preferences.c						\
	terminal-preferences-dialog.c					\
	terminal-screen.c						\
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
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __TERMINAL_ENUM_TYPES_H__\n#define __TERMINAL_ENUM_TYPES_H__\n#include <libxfce4util/libxfce4util.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define TERMINAL_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* !__TERMINAL_ENUM_TYPES_H__ */" \
		$(Terminal_headers) ) > xgen \
	&& (cmp -s xgen terminal-enum-types.h || cp xgen terminal-enum-types.h ) \
	&& rm -f xgen
	( cd $(srcdir) && glib-mkenums \
		--fhead "#undef GTK_DISABLE_DEPRECATED\n#define GTK_ENABLE_BROKEN\n#include <terminal/terminal-enum-types.h>\n#include <terminal/terminal-app.h>\n#include <terminal/terminal-preferences.h>" \
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

