terminal_sources = [
  'main.c',
  'terminal-app.c',
  'terminal-app.h',
  'terminal-encoding-action.c',
  'terminal-encoding-action.h',
  'terminal-gdbus.c',
  'terminal-gdbus.h',
  'terminal-image-loader.c',
  'terminal-image-loader.h',
  'terminal-options.c',
  'terminal-options.h',
  'terminal-preferences-dialog.c',
  'terminal-preferences-dialog.h',
  'terminal-preferences.c',
  'terminal-preferences.h',
  'terminal-search-dialog.c',
  'terminal-search-dialog.h',
  'terminal-screen.c',
  'terminal-screen.h',
  'terminal-util.c',
  'terminal-util.h',
  'terminal-widget.c',
  'terminal-widget.h',
  'terminal-window-dropdown.c',
  'terminal-window-dropdown.h',
  'terminal-window.c',
  'terminal-window.h',
]

terminal_sources += gnome.genmarshal(
  'terminal-marshal',
  sources: 'terminal-marshal.list',
  prefix: '_terminal_marshal',
  internal: true,
  install_header: false,
)

terminal_sources += configure_file(
  configuration: configuration_data({
    'TERMINAL_VERSION_DBUS': terminal_version_dbus,
  }),
  input: 'terminal-config.h.in',
  output: 'terminal-config.h',
  install: false,
)

terminal_sources += gnome.mkenums(
  'terminal-enum-types.h',
  install_header: false,
  sources: 'terminal-preferences.h',
  fhead: '#ifndef TERMINAL_ENUM_TYPES_H\n#define TERMINAL_ENUM_TYPES_H\n#include <gtk/gtk.h>\nG_BEGIN_DECLS\n',
  fprod: '/* enumerations from "@filename@" */\n',
  vhead: 'GType @enum_name@_get_type (void);\n#define TERMINAL_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n',
  ftail: 'G_END_DECLS\n\n#endif /* !TERMINAL_ENUM_TYPES_H */',
)
terminal_sources += gnome.mkenums(
  'terminal-enum-types.c',
  install_header: false,
  sources: 'terminal-preferences.h',
  fhead: '#include "terminal-enum-types.h"\n#include "terminal-app.h"\n#include "terminal-preferences.h"',
  fprod: ' \n/* enumerations from "@filename@" */',
  vhead: 'GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {',
  vprod: ' \t{ @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
  vtail: ' \t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static ("@EnumName@", values);\n  }\n\treturn type;\n}\n',
)

executable(
  'xfce4-terminal',
  terminal_sources,
  sources: xfce_revision_h,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-terminal'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gio,
    gtk,
    vte,
    pcre2,
    libxfce4ui,
    libxfce4kbd,
    libxfce4util,
    xfconf,
    x11_deps,
    wayland_deps,
    gtk_layer_shell,
    libutempter,
    libprocstat,
    libutil,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)
