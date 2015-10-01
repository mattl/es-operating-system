# Escort web browser #

Escort web browser, currently being developed as one of the ES operating system projects, contains a totally new CSS/HTML rendering engine written in C++11, and it can be configured with [SpiderMonkey](https://developer.mozilla.org/en/SpiderMonkey) or [V8](http://code.google.com/p/v8/) JavaScript engine along with the [C++11 Web IDL binding](Cplusplus0xBinding.md) of the esidl Web IDL compiler. For the graphics backend, Escort web browser uses [OpenGL](http://www.opengl.org/) for the better performance and portability.

The core components of the Escort web browser will be used like a window system in today's operating systems in the ES operating system, but before doing so, we are experimenting it as a standalone web browser on Linux.
The user interface of the Escort web browser itself is written in HTML/CSS and JavaScript on top of the Escort core components.

Currently Escort web browser can be built on [Fedora](http://fedoraproject.org/) 17 and [Ubuntu](http://www.ubuntu.com/) 12.10 using an easy [setup script](http://es-operating-system.googlecode.com/svn/trunk/setup.escort). Binary packages are also available [here](http://download.esrille.org/).


---


The browser core executables are built as `Navigator.test` and `NavigatorV8.test`;
`Navigator.test` and `NavigatorV8.test` are linked with SpiderMonkey and
V8 JavaScript engine, respectively.

To start Escort web browser, type from the command line like below:

```
$ ./Navigator.test path_to_'escort/data/navigator'
```

`Navigator.test` takes one command line argument that specifies the directory containing the the default CSS style sheet ([default.css](http://code.google.com/p/es-operating-system/source/browse/trunk/escort/data/escort/default.css)),
and the default UI HTML file ([escort.html](http://code.google.com/p/es-operating-system/source/browse/trunk/escort/data/escort/escort.htmll)).
Note the main browser UI is written in HTML and JavaScipt in ES web browser.

If a new window successfully shows up, click the setting icon at the top right corner of the window, which is just a link to the about page at this point.
If everything goes fine, you'll see the window like below:

<img src='http://es-operating-system.googlecode.com/svn-history/r2731/html/Browser/r2695.about.png' width='50%' height='50%'>

Currently we are actively implementing <a href='http://www.w3.org/TR/CSS2/'>CSS 2.1</a> and testing our implementation using <a href='http://test.csswg.org/suites/css2.1/20110323/'>CSS2.1 Conformance Test Suite</a>.<br>
<br>
<a href='http://www.w3.org/Style/CSS/Test/CSS1/current/test5526c.htm'>Acid1</a>

<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2452.acid1.png' width='50%' height='50%'>

<a href='http://www.webstandards.org/files/acid2/test.html'>Acid2</a>

<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2765.acid2.png' width='50%' height='50%'>

Here's a quick introduction about<br>
<a href='http://code.google.com/p/es-operating-system/source/browse/trunk/escort'>the current browser source tree</a>:<br>
<br>
<ul><li>escort - A DOM core and events implementation.<br>
<ul><li>data<br>
<ul><li>escort - Escort web browser UI written in HTML.<br>
</li></ul></li><li>idl - Web IDL definitions customized for the current esidl.<br>
</li><li>src<br>
<ul><li>css - A CSS parser and renderer implementation using OpenGL as a graphics back-end.<br>
</li><li>font - A tiny font manager that converts TrueType fonts to OpenGL textures.<br>
</li><li>html - An implementation of the HTML5 parser and HTML interfaces and elements.<br>
</li><li>http - HTTP 1.1 client built on top of <a href='http://www.boost.org/doc/libs/1_46_1/doc/html/boost_asio.html'>boost asio library</a>.<br>
</li><li>js - JSAPI bridge for using SpiderMonkey.<br>
</li><li>url - URL parser<br>
</li><li>v8 - V8 API bridge for using V8<br>
</li><li>xbl - A simplified HTML based XBL 2.0 implementation.<br>
</li></ul></li><li>testdata - test data files.</li></ul></li></ul>

<hr />
<b>Change history</b>

Brief descriptions of ok revisions.<br>
<br>
<ul><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=2769'>r2769</a> Passed <a href='http://www.webstandards.org/files/acid2/test.html'>Acid2 Test</a>.<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=2743'>r2743</a> Support Fedora 17.<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=2736'>r2736</a> Support Ubuntu 12.04.<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=2599'>r2599</a> Support the V8 JavaScript engine in addition to SpiderMonkey.<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=2578'>r2578</a> Passed 8440 of the 9365 html4 tests (90.1%) in the <a href='http://test.csswg.org/suites/css2.1/20110323/'>CSS 2.1 Conformance Test Suite</a>.<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=2452'>r2452</a> Passed <a href='http://hixie.ch/tests/evil/acid/002-no-data/'>Acid2 Test (a version without data URLs)</a>.<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=1961'>r1961</a> Passed <a href='http://www.w3.org/Style/CSS/Test/CSS1/current/test5526c.htm'>Acid1 Test</a>.<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=1783'>r1783</a> Support x86-64 (only tested on Fedora 15).<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=1777'>r1777</a> esidl has almost switched over to Web IDL W3C Editor’s Draft 4 July 2011 specification, and ES web browser is built from the IDL definitions in the very recent DOM Core and HTML specifications.<br>
</li><li><a href='http://code.google.com/p/es-operating-system/source/detail?r=1753'>r1753</a> The initial open source edition. Tested only on Fedora 15.