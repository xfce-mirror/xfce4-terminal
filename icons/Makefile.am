SUBDIRS =								\
	16x16								\
	24x24								\
	48x48								\
	scalable

pixmapdir = $(datadir)/pixmaps
pixmap_DATA =								\
	terminal.xpm

EXTRA_DIST =								\
	$(pixmap_DATA)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then					\
                echo "Updating Gtk icon cache.";			\
                $(gtk_update_icon_cache);				\
        else								\
                echo "*** Icon cache not updated. Remember to run:";	\
		echo "***";						\
                echo "***   $(gtk_update_icon_cache)";			\
		echo "***";						\
        fi

# vi:set ts=8 sw=8 noet ai nocindent:
