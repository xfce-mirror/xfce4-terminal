IMAGES =								\
	 stock_advanced_48.png						\
	 stock_appearance_48.png					\
	 stock_browser_48.png						\
	 stock_closetab_16.png						\
	 stock_closetab_24.png						\
	 stock_closewindow_16.png					\
	 stock_closewindow_24.png					\
	 stock_colors_48.png						\
	 stock_compose_48.png						\
	 stock_fullscreen_24.png					\
	 stock_general_48.png						\
	 stock_mailer_48.png						\
	 stock_newtab_16.png						\
	 stock_newtab_24.png						\
	 stock_newwindow_16.png						\
	 stock_newwindow_24.png						\
	 stock_reportbug_16.png						\
	 stock_reportbug_24.png						\
	 stock_shortcuts_48.png						\
	 stock_showborders_24.png					\
	 stock_showmenu_24.png

VARIABLES_HELPER =							\
	stock_browser_48	$(srcdir)/stock_browser_48.png		\
	stock_mailer_48		$(srcdir)/stock_mailer_48.png

VARIABLES_MAIN =							\
	stock_closetab_16	$(srcdir)/stock_closetab_16.png		\
	stock_closewindow_16	$(srcdir)/stock_closewindow_16.png	\
	stock_general_48	$(srcdir)/stock_general_48.png		\
	stock_newtab_16		$(srcdir)/stock_newtab_16.png		\
	stock_newwindow_16	$(srcdir)/stock_newwindow_16.png	\
	stock_reportbug_16	$(srcdir)/stock_reportbug_16.png

VARIABLES_PREFERENCES =							\
	stock_advanced_48	$(srcdir)/stock_advanced_48.png		\
	stock_appearance_48	$(srcdir)/stock_appearance_48.png	\
	stock_colors_48		$(srcdir)/stock_colors_48.png		\
	stock_compose_48	$(srcdir)/stock_compose_48.png		\
	stock_shortcuts_48	$(srcdir)/stock_shortcuts_48.png

VARIABLES_TOOLBAR =							\
	stock_closetab_24	$(srcdir)/stock_closetab_24.png		\
	stock_closewindow_24	$(srcdir)/stock_closewindow_24.png	\
	stock_fullscreen_24	$(srcdir)/stock_fullscreen_24.png	\
	stock_newtab_24		$(srcdir)/stock_newtab_24.png		\
	stock_newwindow_24	$(srcdir)/stock_newwindow_24.png	\
	stock_reportbug_24	$(srcdir)/stock_reportbug_24.png	\
	stock_showborders_24	$(srcdir)/stock_showborders_24.png	\
	stock_showmenu_24	$(srcdir)/stock_showmenu_24.png

noinst_DATA =								\
	terminal-stock-pixbufs.h

terminal-stock-pixbufs.h: $(IMAGES)
	gdk-pixbuf-csource --raw --static --build-list			\
		$(VARIABLES_HELPER) > terminal-stock-pixbufs.h ||	\
		( rm -f terminal-stock-pixbufs.h && false)
	gdk-pixbuf-csource --raw --static --build-list			\
		$(VARIABLES_MAIN) >> terminal-stock-pixbufs.h ||	\
		( rm -f terminal-stock-pixbufs.h && false)
	gdk-pixbuf-csource --raw --static --build-list			\
		$(VARIABLES_PREFERENCES) >> terminal-stock-pixbufs.h ||	\
		( rm -f terminal-stock-pixbufs.h && false)
	gdk-pixbuf-csource --raw --static --build-list			\
		$(VARIABLES_TOOLBAR) >> terminal-stock-pixbufs.h ||	\
		( rm -f terminal-stock-pixbufs.h && false)

CLEANFILES =								\
	$(noinst_DATA)

EXTRA_DIST =								\
	$(IMAGES)

