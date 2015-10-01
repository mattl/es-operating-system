# Using Escudo Web Browser #

**Note**: Escudo is still at an early phase of the development.
The current alpha (unstable) version is still evolving and some parts are not implemented yet.

## Setup ##

### Fedora 18, 19 / Ubuntu 12.10, 13.04 ###

We provide a yum-style repository for Fedora and
a deb-style repository for Ubuntu
at [download.esrille.org](http://download.esrille.org/).
Please follow the instructions there.

### Other Linux distributions ###

Escudo needs to be built and installed from the source files at this point.
Please refer to the
[setup script](http://es-operating-system.googlecode.com/svn/trunk/setup.escudo)
for Fedora and Ubuntu to see how to configure the Makefile.

### Mac OS X (Lion/Mountain Lion) ###

Since Feb 16, 2013, you can build Escudo on Mac OS X from the master branch of [our GitHub repository](https://github.com/esrille/escudo).

Before building Escudo on OS X, please set up [XCode](https://developer.apple.com/technologies/tools/), Command Line Tools for XCode, and
[XQuartz](http://xquartz.macosforge.org/landing/).

We're also using [Homebrew](http://mxcl.github.com/homebrew/) for installing libraries and tools that are used to build Escudo as below:

```
$ ruby -e "$(curl -fsSL https://raw.github.com/mxcl/homebrew/go)"
$ export PATH=/usr/local/bin:$PATH
$ export HOMEBREW_CC=llvm
$ export HOMEBREW_VERBOSE=1
$ brew install bison
$ brew link bison --force
$ brew install spidermonkey v8 re2c glew giflib jpeg
$ brew install boost --with-c++11
```

If you encounter errors during the above steps, please check HOMEBREW\_CC (e.g. when “Illegal instruction: 4" occurs), and try older versions (e.g. use v8 3.9.24 instead of v8 3.15.11 when it doesn't build).

Then please clone [esidl](https://github.com/esrille/esidl) and [escudo](https://github.com/esrille/escudo) from the GitHub repository, and the following steps should build esidl and escudo:

esidl:
```
$ [srcidr]/esidl/configure CFLAGS=-g CXXFLAGS=-g
$ make
$ make install
```

escudo:
```
$ [srcdir]/escudo/configure CC=clang CFLAGS=-g CXX=clang++ CXXFLAGS=-g  
$ make 
```

To start Escudo web browser, type from the command line like below:
```
$ ./Navigator.test [srcdir]/escudo/data/navigator
```

Please refer to [this page](BuildingWebBrowser.md) for more instructions.

## Starting Escudo ##

Here's a quick introduction for starting Escudo from various shells.

### KDE ###

Click [Kickoff](http://userbase.kde.org/Plasma/Kickoff) application launcher (the blue ∞ icon in Fedora) at the very left of the bottom panel,
and then select **Applications** | **Internet**. Find **Web Browser - Escudo** item in the list and select it.

![http://es-operating-system.googlecode.com/svn/html/Browser/r2840.kde.png](http://es-operating-system.googlecode.com/svn/html/Browser/r2840.kde.png)

### GNOME Shell ###

Select **Activities** | **Applications**. Find the ES logo icon in the list and select it.

<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.gnome.png' width='66.7%' height='66.7%'>

<h3>Ubuntu Unity</h3>

Click the Ubuntu logo in the Launcher to start the <a href='https://help.ubuntu.com/12.04/ubuntu-help/unity-dash-intro.html'>Dash</a>,<br>
and type "escudo" in to the <i>Search</i> bar.<br>
The Dash will show you the ES logo icon. Select the ES logo icon to start Escudo.<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.unity.png' width='66.7%' height='66.7%'>

<h3>Terminal</h3>

Type "escudo" and hit <code>Enter</code>.<br>
<br>
<pre><code>% escudo<br>
</code></pre>

<h2>Using Escudo</h2>

The new Escudo window initially opens about:blank page:<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.blank.png' width='66.7%' height='66.7%'>

There is a toolbar across the bottom of the window:<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.toolbar.png' width='66.7%' height='66.7%'>

The toolbar offers the following items:<br>
<br>
<ul><li>ES logo button<br>
</li><li>Back button<br>
</li><li>Forward button<br>
</li><li>Location field<br>
</li><li>Reload button<br>
</li><li>Cancel button</li></ul>

Clicking the ES logo button brings the about: page (with version 0.2.0):<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.about.png' width='66.7%' height='66.7%'>

There is a blue link, "Esrille Inc.", in the fourth line on the about: page.<br>
Try clicking on the link. Now Escudo tries to bring the home page of Esrille Inc.<br>
After you click on the link, the ES logo button switches to an animating gear icon<br>
to show you that the page transfer is in progress:<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.loading.png' width='66.7%' height='66.7%'>

When the page is completely loaded, the gear icon changes back to the ES logo icon,<br>
and you'll see <a href='http://www.esrille.com/'>http://www.esrille.com/</a> in the location field:<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.esrille.png' width='66.7%' height='66.7%'>

The title bar of the Escudo window shows the title and favicon of the page (if the page has one):<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.titlebar.png' width='66.7%' height='66.7%'>

To bring a web page by specifying a URL, click the location field, and type,<br>
<br>
<blockquote><code>http://www.webstandards.org/files/acid2/test.html</code></blockquote>

which is the URL of the <a href='http://www.webstandards.org/action/acid2/'>Acid2</a> test page:<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.acid2-1.png' width='66.7%' height='66.7%'>

Hit the <code>Enter</code> key to start loading the page.<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.acid2-2.png' width='66.7%' height='66.7%'>

To see the Acid2 test result, click on the blue link that reads, "Take The Acid2 Test":<br>
<br>
<img src='http://es-operating-system.googlecode.com/svn/html/Browser/r2840.acid2.png' width='66.7%' height='66.7%'>

At this point, Escudo web browser implements <a href='http://www.w3.org/TR/CSS2/'>CSS 2.1</a> specification relatively well and<br>
passes the Acid2 test.<br>
It also passes about 90% of the tests in the <a href='http://test.csswg.org/suites/css2.1/20110323/'>CSS2.1 Conformance Test Suite</a>.<br>
We're going to support recent dynamic and rich Web pages and the<br>
<a href='http://www.webstandards.org/action/acid3/'>Acid3</a> test<br>
in the future versions of the Escudo browser.<br>
<br>
<h2>File Organization</h2>

The following is the file organization of the Escudo web browser installation.<br>
<br>
<pre><code>/usr/bin<br>
    + escudo          # shell script that executes the browser core<br>
/usr/libexec/esrille/escudo<br>
    + escudo          # browser core (binary executable)<br>
/usr/share/applications<br>
    + escudo.desktop  # desktop file for freedesktop.org environment<br>
/usr/share/esrille/escudo<br>
    + escudo.html     # browser UI page<br>
    + default.css     # default style sheet<br>
    + escudo.ico      # default window icon<br>
    + etc.            # other image files used by escudo.html<br>
/usr/share/esrille/escudo/about<br>
    + blank           # about:blank page<br>
    + index.html      # about: page<br>
    + etc.            # other image files used by the about URL files<br>
/usr/share/icons/hicolor/*x*/apps<br>
    + escudo.png      # icon files used by the GUI shell<br>
</code></pre>

The user specific configuration files are stored in,<br>
<br>
<pre><code>$HOME/.esrille/escudo<br>
</code></pre>

This is also where Escudo creates its temporary cache files.<br>
<br>
<h2>Escudo's user agent string</h2>

Escudo's user agent string uses the format:<br>
<br>
<pre><code>Escudo/x.y.z<br>
</code></pre>

Where <b>x.y.z.</b> provides the version number of the package (such as "0.2.0").<br>
<br>
<i>Note: The format may be changed in the future versions.</i>

<h1>Contact Us</h1>

Please feel free to post your questions, suggestions, etc., on any topics related to Escudo Web Browser to <a href='http://groups.google.com/group/es-operating-system'>our discussion group</a>. We always welcome your input.