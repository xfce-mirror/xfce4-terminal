SUBDIRS = \
	po

MAN_STYLESHEET = $(PACKAGE)-man.xsl
MANPAGE = $(PACKAGE).1.xml
LINGUAS = $(shell cd $(srcdir)/po && echo `/bin/ls *.po | sed 's,[.]po,,g'`)
ALL_LINGUAS = C $(LINGUAS)

# install the man files to the destdir
install-data-local:
	for lang in $(ALL_LINGUAS); do \
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

# remove generate manuals from the srcdir
clean-local:
	rm -rf $(srcdir)/man

if GENERATE_DOCUMENTATION

# regenerate po(t) files
update-po:
	rm -f $(srcdir)/po/$(GETTEXT_PACKAGE).pot; \
	$(XML2PO) -o $(srcdir)/po/$(GETTEXT_PACKAGE).pot $(srcdir)/$(MANPAGE); \
	for lang in $(LINGUAS); do \
		pofile=$(srcdir)/po/$$lang.po; \
		$(XML2PO) -u $(srcdir)/po/$$lang.po $(srcdir)/$(MANPAGE); \
	done

# build the translated help files
doc-build.stamp: $(srcdir)/$(MANPAGE) $(srcdir)/$(MAN_STYLESHEET)
	@chmod -R u+w $(srcdir); \
	rm -rf $(srcdir)/man; \
	echo; \
	for lang in $(ALL_LINGUAS); do \
		pofile=$(srcdir)/po/$$lang.po; \
		echo "*** Generating $$lang documentation"; \
		if test x"$$lang" = x"C"; then \
			cp $(srcdir)/$(MANPAGE) xgen-man; \
		else \
			echo "Building the translated DocBook XML files"; \
			$(XML2PO) -e -l $$lang -p $$pofile $(srcdir)/$(MANPAGE) > xgen-man; \
			rm -f .xml2po.mo; \
		fi; \
		sed -e "s,\@PACKAGE_NAME\@,$(PACKAGE_NAME),g" \
			-e "s,\@PACKAGE_VERSION\@,$(PACKAGE_VERSION),g" \
			-i xgen-man; \
		echo "Validating DocBook XML files"; \
		$(XMLLINT) --noout --valid xgen-man || exit 1; \
		echo "Creating the MAN page"; \
		$(XSLTPROC) --nonet -o $(srcdir)/man/$$lang/$(PACKAGE_NAME).1 $(srcdir)/$(MAN_STYLESHEET) xgen-man; \
		chmod -R u+w $(srcdir)/man/$$lang; \
		rm -f xgen-man; \
		echo; \
	done; \
	touch doc-build.stamp

dist-check-doc: all

else

update-po:
	@echo "*** You need to compile with --enable-gen-doc in order to make update-po"
	@false

doc-build.stamp:
	@if test ! -d $(srcdir)/man; then \
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
# a message if no man files were found in the tarball
all: doc-build.stamp

# include the man files in the dist
dist-hook: dist-check-doc doc-build.stamp
	for lang in $(ALL_LINGUAS); do \
		if test -d $(srcdir)/man/$$lang; then \
			mkdir -p $(distdir)/man/$$lang; \
			cp $(srcdir)/man/$$lang/* $(distdir)/man/$$lang/; \
		fi; \
	done

EXTRA_DIST = \
	$(MAN_STYLESHEET) \
	$(MANPAGE) \
	README

CLEANFILES = \
	xgen-man \
	doc-build.stamp
