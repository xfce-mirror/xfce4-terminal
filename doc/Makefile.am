SUBDIRS = \
	images \
	po

HTML_STYLESHEET = $(PACKAGE)-html.xsl
MAN_STYLESHEET = $(PACKAGE)-man.xsl
DOCUMENT = $(PACKAGE).xml
MANPAGE = $(PACKAGE).1.xml
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
				$(INSTALL_DATA) $$file $$dest/; \
			done; \
			if test x"$$lang" != x"C" -a ! -d $(srcdir)/images/$$lang; then \
				echo "--- Creating symlink to C images for $$lang"; \
				ln -sf $(docdir)/C/images $$dest/images; \
			fi \
		fi; \
		file=$(srcdir)/man/$$lang/$(PACKAGE_NAME).1; \
		if test -f $$file ; then \
			if test x"$$lang" = x"C"; then \
				dest=$(DESTDIR)$(mandir)/man1; \
			else \
				dest=$(DESTDIR)$(mandir)/$$lang/man1; \
			fi; \
			$(mkinstalldirs) $$dest; \
			dest=`echo $$dest/$(PACKAGE).1 | sed '$(program_transform_name)'`; \
			$(INSTALL_DATA) $$file $$dest; \
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
		fi; \
		if test x"$$lang" = x"C"; then \
			dest=$(DESTDIR)$(mandir)/man1/$(PACKAGE).1; \
		else \
			dest=$(DESTDIR)$(mandir)/$$lang/man1/$(PACKAGE).1; \
		fi; \
		dest=`echo $$dest | sed '$(program_transform_name)'`; \
		if test -f $$dest; then \
			rm -f $$dest; \
		fi \
	done

# remove generate html from the srcdir
clean-local:
	rm -rf $(srcdir)/html $(srcdir)/man

if GENERATE_DOCUMENTATION

# regenerate po(t) files
update-po:
	rm -f $(srcdir)/po/$(GETTEXT_PACKAGE).pot; \
	$(XML2PO) -o $(srcdir)/po/$(GETTEXT_PACKAGE).pot $(srcdir)/$(DOCUMENT) $(srcdir)/$(MANPAGE); \
	for lang in $(LINGUAS); do \
		pofile=$(srcdir)/po/$$lang.po; \
		$(XML2PO) -u $(srcdir)/po/$$lang.po $(srcdir)/$(DOCUMENT) $(srcdir)/$(MANPAGE); \
	done

# build the translated help files
doc-build.stamp: $(srcdir)/$(DOCUMENT) $(srcdir)/$(MANPAGE) $(srcdir)/$(HTML_STYLESHEET) $(srcdir)/$(MAN_STYLESHEET)
	@chmod -R u+w $(srcdir); \
	rm -rf $(srcdir)/html $(srcdir)/man; \
	echo; \
	echo "*** Generating full XML file ***"; \
	echo "Creating Terminal-options.xml"; \
	sed -e "s,<refsect,<sect,g" \
		-e "s,</refsect,</sect,g" $(srcdir)/$(MANPAGE) > Terminal-options.xml; \
	echo "Creating single XML file for HTML manual"; \
	$(XMLLINT) --nonet --xinclude --postvalid $(srcdir)/$(DOCUMENT) > xgen-xml; \
	echo; \
	for lang in $(ALL_LINGUAS); do \
		pofile=$(srcdir)/po/$$lang.po; \
		echo "*** Generating $$lang documentation"; \
		if test x"$$lang" = x"C"; then \
			cp xgen-xml xgen-doc; \
			cp $(srcdir)/$(MANPAGE) xgen-man; \
		else \
			echo "Building the translated DocBook XML files"; \
			$(XML2PO) -e -l $$lang -p $$pofile xgen-xml > xgen-doc; \
			$(XML2PO) -e -l $$lang -p $$pofile $(srcdir)/$(MANPAGE) > xgen-man; \
			rm -f .xml2po.mo; \
		fi; \
		sed -e "s,\@PACKAGE_NAME\@,$(PACKAGE_NAME),g" \
			-e "s,\@PACKAGE_VERSION\@,$(PACKAGE_VERSION),g" \
			-i xgen-doc xgen-man; \
		echo "Validating DocBook XML files"; \
		$(XMLLINT) --noout --valid xgen-doc xgen-man; \
		echo "Creating the HTML pages"; \
		$(XSLTPROC) --nonet -o $(srcdir)/html/$$lang/ $(srcdir)/$(HTML_STYLESHEET) xgen-doc; \
		echo "Creating the MAN page"; \
		$(XSLTPROC) --nonet -o $(srcdir)/man/$$lang/$(PACKAGE_NAME).1 $(srcdir)/$(MAN_STYLESHEET) xgen-man; \
		chmod -R u+w $(srcdir)/html/$$lang $(srcdir)/man/$$lang; \
		rm -f xgen-doc xgen-man; \
		echo; \
	done; \
	rm -f xgen-xml Terminal-options.xml; \
	touch doc-build.stamp

dist-check-doc: all

else

update-po:
	@echo "*** You need to compile with --enable-gen-doc in order to make update-po"
	@false

doc-build.stamp:
	@if test ! -d $(srcdir)/html -a ! -d $(srcdir)/man; then \
		echo; \
		echo "*** No generated documentation found. If you want to"; \
		echo "*** generate the (translated) documentation, run configure"; \
		echo "*** with --enable-gen-doc."; \
		echo; \
	fi

dist-check-doc:
	@echo "*** You need to compile with --enable-gen-doc in order to make dist"
	@false

endif

# always try to build (if compiling with --enable-build-doc) or print
# a message if no html files were found in the tarball
all: doc-build.stamp

# include the html and man files in the dist
dist-hook: dist-check-doc doc-build.stamp
	for lang in $(ALL_LINGUAS); do \
		if test -d $(srcdir)/html/$$lang; then \
			mkdir -p $(distdir)/html/$$lang; \
			cp $(srcdir)/html/$$lang/* $(distdir)/html/$$lang/; \
		fi; \
		if test -d $(srcdir)/man/$$lang; then \
			mkdir -p $(distdir)/man/$$lang; \
			cp $(srcdir)/man/$$lang/* $(distdir)/man/$$lang/; \
		fi; \
	done

# css file
cssdir = $(docdir)
css_DATA = \
	terminal.css

EXTRA_DIST = \
	$(css_DATA) \
	$(HTML_STYLESHEET) \
	$(MAN_STYLESHEET) \
	$(DOCUMENT) \
	$(MANPAGE) \
	README

CLEANFILES = \
	xgen-xml \
	xgen-doc \
	xgen-man \
	Terminal-options.xml \
	doc-build.stamp
