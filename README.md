LuaGRAPH
--------
  
  LuaGRAPH is a binding to the graphviz library.

  LuaGRAPH requires the graphviz library version 2.26 or later, which
  can be downloaded from:
  
  [http://www.graphviz.org/](http://www.graphviz.org/)


  You must have Lua version 5.1, 5.2 or 5.3. Lua can be downloaded 
  from its home page:
  
  [http://www.tecgraf.puc-rio.br/lua/](http://www.tecgraf.puc-rio.br/lua/)

Documentation
-------------

  Documentation of LuaGRAPH comes with the LuaGRAPH distribution. I recommend also to read the graphviz documentation, which can be found here:
  
[http://www.graphviz.org/Documentation.php](http://www.graphviz.org/Documentation.php)
	

Legal matters
-------------

  See the license agreement in the file LICENSE.

Download
--------
  LuaGRAPH can be downloaded from github at
  
  [https://github.com/hleuwer/luagraph](https://github.com/hleuwer/luagraph)  

  or install with luarocks. See below under "Installation".
  
Configuration
-------------

  Before building LuaGRAPH you have to adopt the config file by
  adjusting various constants to the installed Lua and Graphviz version.

Status
------
  Newest version is LuaGRAPH 2.0 now supports the `cgraphÂ´ library and all
  Lua versions from Lua 5.1 to Lua 5.3. Lua 5.0 is no longer supported.
  It has been tested on MacOS Sierra version 10.12.13, Linux Debian and
  Windows 10 running as guest in a Virtual Box virtual machine under MacOS.

  Under Windows luarocks has only been tested with Lua 5.1 using the
  LuaForWindows installation.
  
Installation
------------
  
**Install via Luarocks (MacOS and Linux only)**

  Simply type the following:

     $ sudo luarocks GRAPHVIZ_DIR=/opt/local install luagraph

  You may have to change the value for the GRAPHVIZ_DIR variable to ensure
  that luarocks finds your GRAPHVIZ installation. On MacOS it is typically
  installed in /opt/local.

**Manual Installation under MacOS or Linux**

  In order to build LuaGRAPH on a Linux, MacOS or Cygwin/Mingw based
  POSIX like system:

  1. Make sure you have Lua 5.1 to 5.3. 
     The file test.lua in the test subdirectory uses lualogging which 
     can be installed via Luarocks at http://www.luarocks.org.

  2. With graphviz version 2.12 you might have to use the `ltdl' library 
     that comes with graphviz in order to have the rendering plugins to 
     work properly. Version 2.40 is currently the last graphviz version
     that is supported.

  3. LuaGRAPH comes with a makefile and a simple config file to adopt
     the make process to your target system.

  4. Adopt build configuration to your platform by editing the file
     config according to you needs.

  5. Compile the package

     Type 
	make

     If make succeeds you get:

     * a Lua extension library "graph.so.x.0" in the src sub-directory.
     * a copy "core.so" of the same Lua extension library in ./graph

  6. Type "make install" as user root in order to install all relevant
     files in standard places. The directory /usr/local is the default
     install prefix.

**Manual Build and Installation under Windows**

  In order to build LuaGRAPH on Windows, use the Visual Studio 2008
  project file that comes with source code. Load the project and build
  a Release version inside the Visual Sudio IDE. To make the luagraph
  module available copy the file Release/luagraph-2.0.0-2.dll into
  graph/core.dll. Sorry for this inconvenience, which I may remove
  once I have a really large amount of time.

Environment Variables
---------------------

  None.


Comments and bug reports
------------------------

  Please send your comments and bug reports to the Lua mailing list.

***December 2006 (January 2017)***

***Have fun!***