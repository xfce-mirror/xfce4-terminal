apps_pngdir = $(datadir)/icons/hicolor/48x48/apps
apps_png_DATA =								\
	Terminal.png

apps_svgdir = $(datadir)/icons/hicolor/scalable/apps
apps_svg_DATA =								\
	Terminal.svg

mime_svgdir = $(datadir)/icons/hicolor/scalable/mimetypes
mime_svg_DATA =								\
	gnome-mime-application-x-terminal-color-palette.svg

mime_pngdir = $(datadir)/icons/hicolor/22x22/mimetypes
mime_png_DATA =								\
	gnome-mime-application-x-terminal-color-palette.png

pixmapdir = $(datadir)/pixmaps
pixmap_DATA =								\
	terminal.xpm

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(mime_svgdir)
	( cd $(DESTDIR)$(mime_svgdir) && \
	  rm -f mime-application:x-terminal-color-palette.svg && \
	  $(LN_S) gnome-mime-application-x-terminal-color-palette.svg \
	          mime-application:x-terminal-color-palette.svg )
	$(mkinstalldirs) $(DESTDIR)$(mime_pngdir)
	( cd $(DESTDIR)$(mime_pngdir) && \
	  rm -f mime-application:x-terminal-color-palette.png && \
	  $(LN_S) gnome-mime-application-x-terminal-color-palette.png \
	          mime-application:x-terminal-color-palette.png )

EXTRA_DIST =								\
	$(apps_png_DATA)						\
	$(apps_svg_DATA)						\
	$(mime_svg_DATA)						\
	$(pixmap_DATA)
