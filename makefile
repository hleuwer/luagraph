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

.PHONY: tag tag-git tag-cvs tag-svn
tag: tag-git
	cvs tag -F latest

tag-git:
	git tag -F latest

tag-cvs:
	cvs tag -F latest

.PHONY: dist dist-git dist-cvs dist-svn
dist: dist-git

dist-git:
	mkdir -p $(EXPORTDIR)
	git archive --format=tar --prefix=$(DISTNAME)/ HEAD | gzip >$(EXPORTDIR)/$(DISTARCH)

dist-cvs:
	mkdir -p $(EXPORTDIR)/$(DISTNAME)
	cvs export -r latest -d $(EXPORTDIR)/$(DISTNAME) $(CVSMODULE)
	cd $(EXPORTDIR); tar -cvzf $(DISTNAME).tar.gz $(DISTNAME)/*
	rm -rf $(EXPORTDIR)/$(DISTNAME)

dist-svn:
	svn export $(REPOSITORY)/$(SVNMODULE) $(EXPORTDIR)/$(DISTNAME)
	cd $(EXPORTDIR); tar -cvzf $(DISTARCH) $(DISTNAME)/*
	rm -rf $(EXPORTDIR)/$(DISTNAME)
