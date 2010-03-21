include ../config

OBJS += gr_graph.o gr_node.o gr_edge.o gr_util.o

all: $(LUAGRAPH_SO)

$(LUAGRAPH_SO): $(OBJS) $(LIBSTAT)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)
	mkdir -p ../graph
	cp $(LUAGRAPH_SO) ../graph/core.$(EXT)

clean:
	rm -f $(OBJS) $(LUAGRAPH_SO) 
	rm -rf ../graph
	rm -rf makefile.deps

depend: makefile.deps

makefile.deps: $(OBJS:.o=.c)
	rm -f makefile.deps
	test -z "$(OBJS)" || $(CC) -MM $(CFLAGS) $(OBJS:.o=.c) >> makefile.deps

-include makefile.deps