include config

.PHONY: all clean depend uclean
all:
	cd src && $(MAKE) $@

clean depend:
	cd src && $(MAKE) $@

uclean: clean
	rm -f `find . -name "*~"` 
	rm -f `find . -name "#*"` 
	rm -f graph/core.so $(LUAGRAPH_SO) 
	rm -f out.*
	rm -rf outdir

.PHONY: install uninstall install-doc uninstall-doc

install: all
	mkdir -p $(INSTALL_SHARE) $(INSTALL_LIB)/graph
	$(INSTALL_DATA) graph.lua $(INSTALL_SHARE)
	cd src && $(INSTALL_COPY) $(LUAGRAPH_SO) $(INSTALL_LIB)/graph/core.$(EXT)

uninstall:
	rm -rf $(INSTALL_SHARE)/graph.lua
	rm -rf $(INSTALL_LIB)/graph

install-doc:
	mkdir -p $(INSTALL_DOC)/html
	cd doc && $(INSTALL_COPY) * $(INSTALL_DOC)/html

uninstall-doc:
	rm -rf $(INSTALL_DOC)

.PHONY: test testd

test:
	$(LUABIN) test/test.lua

testd: 
	$(LUABIN) test/test.lua DEBUG

.PHONY: tag tag-git 
tag: tag-git

tag-git:
	git tag -F latest


show::
	@echo "System shortname: "$(SYSTEM)

.PHONY: dist dist-git dist-cvs dist-svn
dist: dist-git

dist-git:
	mkdir -p $(EXPORTDIR)
	git archive --format=tar --prefix=$(DISTNAME)/ HEAD | gzip >$(EXPORTDIR)/$(DISTARCH)
