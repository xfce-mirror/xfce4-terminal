SUBDIRS = \
	images

STYLESHEET = $(PACKAGE).xsl
DOCUMENT = $(PACKAGE).xml
DOC_LINGUAS = ca da fr gl id it ja zh_CN

# install the html files to the destdir
install-data-local:
	for lang in C $(DOC_LINGUAS); do \
		if test -d $(srcdir)/html/$$lang; then \
			dest=$(DESTDIR)$(docdir)/$$lang; \
			$(mkinstalldirs) $$dest; \
			installfiles=`echo $(srcdir)/html/$$lang/*`; \
			for file in $$installfiles; do \
				echo "--- Installing "$$file; \
				$(INSTALL_DATA) $$file $$dest; \
			done; \
			if test x"$$lang" != x"C" -a ! -d $(srcdir)/images/$$lang; then \
				echo "--- Creating symlink to C images for $$lang"; \
				ln -sf $(DESTDIR)$(docdir)/C/images $(DESTDIR)$(docdir)/$$lang/images; \
			fi \
		else \
			echo "--- Nothing to install for language $$lang"; \
		fi \
	done;

# remove the html files from the destdir
uninstall-local:
	rm -rf $(DESTDIR)$(docdir)

if BUILD_DOCUMENTATION

# build the (translated) html help files
html-build.stamp: $(srcdir)/$(DOCUMENT) $(srcdir)/$(STYLESHEET)
	if test -d $(srcdir)/html; then \
		rm -rf $(srcdir)/html; \
	fi; \
	chmod -R u+w $(srcdir); \
	for lang in C $(DOC_LINGUAS); do \
		pofile=$(top_srcdir)/po-doc/$$lang.po; \
		if test x"$$lang" = x"C" -o -f $$pofile; then \
			echo "*** Building $$lang HTML ***"; \
			if test x"$$lang" = x"C"; then \
				cp $(srcdir)/$(DOCUMENT) xgen-doc; \
			else \
				echo "--- Translating XML into $$lang"; \
				$(XML2PO) -e -l $$lang -p $$pofile $(srcdir)/$(DOCUMENT) > xgen-doc; \
				rm -f .xml2po.mo; \
			fi; \
			sed -e "s,\@PACKAGE_NAME\@,$(PACKAGE_NAME),g" \
				-e "s,\@PACKAGE_VERSION\@,$(PACKAGE_VERSION),g" \
				-i xgen-doc; \
			$(XSLTPROC) --nonet -o $(srcdir)/html/$$lang/ $(srcdir)/$(STYLESHEET) xgen-doc; \
			rm -f xgen-doc; \
			echo; \
		else \
			echo "Error: No po file was found for the $$lang documentation"; \
			false; \
		fi \
	done; \
	touch html-build.stamp

dist-check-doc: all

else

html-build.stamp:
	if test ! -d $(srcdir)/html; then \
		@echo "No HTML documentation found. If you want the $(PACKAGE_NAME)"; \
		@echo "documentation, recompile with --enable-build-doc, this"; \
		@echo "This requires both the xml2po and xsltproc executables"; \
	fi; \
	touch html-build.stamp

dist-check-doc:
	@echo "*** You need to compile with --enable-build-doc in order to make dist"
	@echo "*** This requires both the xml2po and xsltproc executables"
	@false

endif

# always try to build (if compiling with --enable-build-doc) or print
# a message if no html files were found in the tarball
all: html-build.stamp

# include the html files in the dist
dist-hook: dist-check-doc html-build.stamp
	for lang in C $(DOC_LINGUAS); do \
		mkdir -p $(distdir)/html/$$lang; \
		cp $(srcdir)/html/$$lang/* $(distdir)/html/$$lang/; \
	done;

# css file
cssdir = $(docdir)
css_DATA = \
	terminal.css

EXTRA_DIST = \
	$(css_DATA) \
	$(STYLESHEET) \
	$(DOCUMENT)

CLEANFILES = \
	xgen-doc \
	html-build.stamp
