SUBDIRS = \
	images \
	po

STYLESHEET = $(PACKAGE).xsl
DOCUMENT = $(PACKAGE).xml
LINGUAS = $(shell cd $(srcdir)/po && echo `/bin/ls *.po | sed 's,[.]po,,g'`)
ALL_LINGUAS = C $(LINGUAS)

# install the html files to the destdir
install-data-local:
	for lang in $(ALL_LINGUAS); do \
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
				ln -sf $(docdir)/C/images $(DESTDIR)$(docdir)/$$lang/images; \
			fi \
		fi \
	done;

# remove installed files from the destdir
uninstall-local:
	for lang in $(ALL_LINGUAS); do \
		dest=$(DESTDIR)$(docdir)/$$lang; \
		if test -d $$dest; then \
			if test -L $$dest/images; then \
				rm -rf $$dest; \
			else \
				rm -f $$dest/*.html; \
			fi \
		fi \
	done

if GENERATE_DOCUMENTATION

# regenerate po(t) files
update-po:
	rm -f $(srcdir)/po/$(GETTEXT_PACKAGE).pot; \
	$(XML2PO) -o $(srcdir)/po/$(GETTEXT_PACKAGE).pot $(srcdir)/$(DOCUMENT); \
	for lang in $(LINGUAS); do \
		pofile=$(srcdir)/po/$$lang.po; \
		$(XML2PO) -u $(srcdir)/po/$$lang.po $(srcdir)/$(DOCUMENT); \
	done

# build the (translated) html help files
html-build.stamp: $(srcdir)/$(DOCUMENT) $(srcdir)/$(STYLESHEET)
	@chmod -R u+w $(srcdir); \
	if test -d $(srcdir)/html; then \
		rm -rf $(srcdir)/html; \
	fi; \
	echo; \
	for lang in $(ALL_LINGUAS); do \
		pofile=$(srcdir)/po/$$lang.po; \
		echo "*** Generating $$lang HTML documentation"; \
		if test x"$$lang" = x"C"; then \
			cp $(srcdir)/$(DOCUMENT) xgen-doc; \
		else \
			echo "Building the translated DocBook XML file"; \
			$(XML2PO) -e -l $$lang -p $$pofile $(srcdir)/$(DOCUMENT) > xgen-doc; \
			rm -f .xml2po.mo; \
		fi; \
		sed -e "s,\@PACKAGE_NAME\@,$(PACKAGE_NAME),g" \
			-e "s,\@PACKAGE_VERSION\@,$(PACKAGE_VERSION),g" \
			-i xgen-doc; \
		echo "Creating the HTML pages"; \
		$(XSLTPROC) --nonet -o $(srcdir)/html/$$lang/ $(srcdir)/$(STYLESHEET) xgen-doc; \
		chmod -R u+w $(srcdir)/html/$$lang; \
		rm -f xgen-doc; \
		echo; \
	done; \
	touch html-build.stamp

dist-check-doc: all

else

update-po:
	@echo "*** You need to compile with --enable-gen-doc in order to update-po"
	@echo "*** This requires both the xml2po and xsltproc executables"
	@false

html-build.stamp:
	@if test ! -d $(srcdir)/html; then \
		echo; \
		echo "*** No generated HTML documentation found. If you want to"; \
		echo "*** generate the (translated) documentation, run configure"; \
		echo "*** with --enable-gen-doc."; \
		echo; \
	fi

dist-check-doc:
	@echo "*** You need to compile with --enable-gen-doc in order to make dist"
	@echo "*** This requires both the xml2po and xsltproc executables"
	@false

endif

# always try to build (if compiling with --enable-build-doc) or print
# a message if no html files were found in the tarball
all: html-build.stamp

# include the html files in the dist
dist-hook: dist-check-doc html-build.stamp
	for lang in $(ALL_LINGUAS); do \
		if test -d $(srcdir)/html/$$lang; then \
			mkdir -p $(distdir)/html/$$lang; \
			cp $(srcdir)/html/$$lang/* $(distdir)/html/$$lang/; \
		fi; \
	done

# css file
cssdir = $(docdir)
css_DATA = \
	terminal.css

EXTRA_DIST = \
	$(css_DATA) \
	$(STYLESHEET) \
	$(DOCUMENT) \
	README

CLEANFILES = \
	xgen-doc \
	html-build.stamp
