SUBDIRS =								\
	images

noinst_DATA =								\
	Terminal.xml

docdir = $(datadir)/doc/Terminal/C
doc_DATA =								\
	Terminal.html

EXTRA_DIST =								\
	$(noinst_DATA)							\
	$(doc_DATA)

html:
	xsltproc $(srcdir)/../terminal-nochunk.xsl			\
		$(srcdir)/Terminal.xml
