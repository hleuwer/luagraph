local gr = require "graph"

--
-- Define the graph
--
local g = gr.graph{"G",
  -- Need 'record' as shape attribute
  node = {shape = "record"},
  -- Classical graphviz record definition
  gr.node{"n1", 
    label="<f0> left|<f1> mid\ dle|<f2> right"};
  gr.node{"n2", 
    label = "<f0> one|<f1> two"},
  gr.node{"n3", 
    label = "hello\\nworld 1|{ b |{c|<here> d|e}|f}|<g> g|{h|z}"},

  gr.edge{"n1:f2", "n2:f1"},
  gr.edge{"n1", "n3"},
  gr.edge{"n2:f0:n", "n3:here:sw"},
  gr.edge{"n1:f0", "n3:w"},
  -- Luayats record definition using vbox and hbox function
  gr.record{"n4",
    gr.hbox{
      "hello\\nworld 2",
      gr.vbox{
	"b",
	gr.hbox{"c", "<here> d", "e"},
	"f"
      },
      "<g> g",
      gr.vbox{"h", "z"}
    }
  },
  -- Same elements but starting top to down
  gr.record{"n5",
    gr.vbox{
      "hello\\nworld 3",
      gr.hbox{
	"b",
	gr.vbox{"c", "<here> d", "e"},
	"f"
      },
      "<g> g",
      gr.hbox{"h", "z"}
    }
  },
  -- Connect two ports
  gr.edge{"n4:here", "n5:e"}
}

--
-- Layout using 'dot' (default) engine
--
g:layout()
g:write()

--
-- Render into different formats
--
g:render("pdf", "out.pdf")
g:render("gif", "out.gif")
g:render("svg", "out.svg")
g:render("jpg", "out.jpg")
g:render("png", "out.png")
g:show()
--
-- That's it
--
g:close()
