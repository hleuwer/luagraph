include config

all:
	cd src; $(MAKE) $@
	rm -f graph.lua; ln -f -s $(LUAGRAPH) graph.lua

clean depend:
	cd src; $(MAKE) $@

uclean: clean
	rm -f `find . -name "*~"` 
	rm -f `find . -name "#*"` 
	rm -f graph.so graph.lua
	rm -f out.*

install: all
	cd src; mkdir -p $(INSTALL_SHARE)
	cp graph.lua $(INSTALL_SHARE)
	cd src; mkdir -p $(INSTALL_LIB)/graph
	cd src; cp $(LUAGRAPH_SO) $(INSTALL_LIB)/graph/core.$(EXT)

uninstall:
	rm -rf $(INSTALL_SHARE)/graph.lua
	rm -rf $(INSTALL_LIB)/graph

test:
	$(LUABIN) $(TESTLUA)

testd:
	$(LUABIN) $(TESTLUA) DEBUG

tag::
	cvs tag -F latest

cvsdist::
	mkdir -p $(EXPORTDIR)/$(DISTNAME)
	cvs export -r latest -d $(EXPORTDIR)/$(DISTNAME) $(CVSMODULE)
	cd $(EXPORTDIR); tar -cvzf $(DISTNAME).tar.gz $(DISTNAME)/*
	rm -rf $(EXPORTDIR)/$(DISTNAME)

dist::
	svn export $(REPOSITORY)/$(SVNMODULE) $(EXPORTDIR)/$(DISTNAME)
	cd $(EXPORTDIR); tar -cvzf $(DISTARCH) $(DISTNAME)/*
	rm -rf $(EXPORTDIR)/$(DISTNAME)

.PHONY: all webbook tag dist test testd depend clean uclean install uninstall
