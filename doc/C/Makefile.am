SUBDIRS =								\
	images

noinst_DATA =								\
	Terminal.xml

docdir = $(datadir)/doc/Terminal/C
doc_DATA =								\
	copyright.html							\
	getting-started.html						\
	index.html							\
	preferences.html						\
	support.html							\
	usage.html

EXTRA_DIST =								\
	$(noinst_DATA)							\
	$(doc_DATA)

html:
	xsltproc $(srcdir)/../terminal.xsl				\
		$(srcdir)/Terminal.xml
