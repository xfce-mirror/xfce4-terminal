
/* Generated data (by glib-mkenums) */

#undef GTK_DISABLE_DEPRECATED
#define GTK_ENABLE_BROKEN
#include <terminal/terminal-enum-types.h>
#include <terminal/terminal-options.h>
#include <terminal/terminal-preferences.h>

/* enumerations from "terminal-options.h" */
GType
terminal_options_mask_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
	static const GFlagsValue values[] = {
	{ TERMINAL_OPTIONS_MASK_COMMAND, "TERMINAL_OPTIONS_MASK_COMMAND", "mask-command" },
	{ TERMINAL_OPTIONS_MASK_TITLE, "TERMINAL_OPTIONS_MASK_TITLE", "mask-title" },
	{ TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY, "TERMINAL_OPTIONS_MASK_WORKING_DIRECTORY", "mask-working-directory" },
	{ TERMINAL_OPTIONS_MASK_DISPLAY, "TERMINAL_OPTIONS_MASK_DISPLAY", "mask-display" },
	{ TERMINAL_OPTIONS_MASK_SCREEN, "TERMINAL_OPTIONS_MASK_SCREEN", "mask-screen" },
	{ TERMINAL_OPTIONS_MASK_FLAGS, "TERMINAL_OPTIONS_MASK_FLAGS", "mask-flags" },
	{ TERMINAL_OPTIONS_MASK_GEOMETRY, "TERMINAL_OPTIONS_MASK_GEOMETRY", "mask-geometry" },
	{ 0, NULL, NULL }
	};
	type = g_flags_register_static ("TerminalOptionsMask", values);
  }
	return type;
}

GType
terminal_flags_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
	static const GFlagsValue values[] = {
	{ TERMINAL_FLAGS_OPENTAB, "TERMINAL_FLAGS_OPENTAB", "opentab" },
	{ TERMINAL_FLAGS_COMPACTMODE, "TERMINAL_FLAGS_COMPACTMODE", "compactmode" },
	{ TERMINAL_FLAGS_FULLSCREEN, "TERMINAL_FLAGS_FULLSCREEN", "fullscreen" },
	{ TERMINAL_FLAGS_DISABLESERVER, "TERMINAL_FLAGS_DISABLESERVER", "disableserver" },
	{ TERMINAL_FLAGS_SHOWVERSION, "TERMINAL_FLAGS_SHOWVERSION", "showversion" },
	{ TERMINAL_FLAGS_SHOWHELP, "TERMINAL_FLAGS_SHOWHELP", "showhelp" },
	{ 0, NULL, NULL }
	};
	type = g_flags_register_static ("TerminalFlags", values);
  }
	return type;
}


/* enumerations from "terminal-preferences.h" */
GType
terminal_scrollbar_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
	static const GEnumValue values[] = {
	{ TERMINAL_SCROLLBAR_NONE, "TERMINAL_SCROLLBAR_NONE", "none" },
	{ TERMINAL_SCROLLBAR_LEFT, "TERMINAL_SCROLLBAR_LEFT", "left" },
	{ TERMINAL_SCROLLBAR_RIGHT, "TERMINAL_SCROLLBAR_RIGHT", "right" },
	{ 0, NULL, NULL }
	};
	type = g_enum_register_static ("TerminalScrollbar", values);
  }
	return type;
}

GType
terminal_title_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
	static const GEnumValue values[] = {
	{ TERMINAL_TITLE_REPLACE, "TERMINAL_TITLE_REPLACE", "replace" },
	{ TERMINAL_TITLE_PREPEND, "TERMINAL_TITLE_PREPEND", "prepend" },
	{ TERMINAL_TITLE_APPEND, "TERMINAL_TITLE_APPEND", "append" },
	{ TERMINAL_TITLE_HIDE, "TERMINAL_TITLE_HIDE", "hide" },
	{ 0, NULL, NULL }
	};
	type = g_enum_register_static ("TerminalTitle", values);
  }
	return type;
}

GType
terminal_background_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
	static const GEnumValue values[] = {
	{ TERMINAL_BACKGROUND_SOLID, "TERMINAL_BACKGROUND_SOLID", "solid" },
	{ TERMINAL_BACKGROUND_IMAGE, "TERMINAL_BACKGROUND_IMAGE", "image" },
	{ TERMINAL_BACKGROUND_TRANSPARENT, "TERMINAL_BACKGROUND_TRANSPARENT", "transparent" },
	{ 0, NULL, NULL }
	};
	type = g_enum_register_static ("TerminalBackground", values);
  }
	return type;
}

GType
terminal_erase_binding_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
	static const GEnumValue values[] = {
	{ TERMINAL_ERASE_BINDING_ASCII_BACKSPACE, "TERMINAL_ERASE_BINDING_ASCII_BACKSPACE", "ascii-backspace" },
	{ TERMINAL_ERASE_BINDING_ASCII_DELETE, "TERMINAL_ERASE_BINDING_ASCII_DELETE", "ascii-delete" },
	{ TERMINAL_ERASE_BINDING_DELETE_SEQUENCE, "TERMINAL_ERASE_BINDING_DELETE_SEQUENCE", "delete-sequence" },
	{ 0, NULL, NULL }
	};
	type = g_enum_register_static ("TerminalEraseBinding", values);
  }
	return type;
}


/* Generated data ends here */

