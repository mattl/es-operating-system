# C++ DOM API Introduction #

## _17 May 2010_ ##

The C++ DOM API is a library for writing Web applications in C++ over NPAPI/Pepper
using the [Web IDL C++ binding](CplusplusBinding.md) for accessing DOM objects inside the Web browser.
This document describes how to write your own Web applications in C++ using the C++ DOM API.

In June, 2008, right after the Web IDL draft specification is published from W3C,
we showed it is technically possible to create Web applications running inside the Web browsers in C++ `[`[4](http://groups.google.com/group/es-operating-system/browse_thread/thread/037d65a0f614d0d8)`]`.
Today, along with [Native Client](http://code.google.com/p/nativeclient-sdk/),
[NPAPI Pepper Extensions](https://wiki.mozilla.org/NPAPI:Pepper), and Chrome 5 beta 3 (5.0.375) or later, it is becoming a real story.

Note: this document assumes you are using an x86 Linux machine.
However, your Web applications using the C++ DOM API should run on Windows and OS X as well
since these are compiled and linked as Native Client applications.

## Prerequisite ##

The following packages are required to set up the C++ DOM API development environment:

  * bison
  * chrome (Chrome 5 beta 3 (5.0.375) or later)
  * flex
  * gcc-c++
  * glibc
  * libtool
  * python
  * subversion
  * wget

## Setting up ##

The following steps will set up the C++ DOM API development environment,
and launch Chrome to test drive C++ DOM demo applications.
A shell script for doing this is available [here](http://es-operating-system.googlecode.com/svn/trunk/esidl/setup.pepper)
so you don't have to type them manually.

```
#!/bin/bash -v
cd
mkdir nacl
cd nacl
# download NaCl SDK
wget http://build.chromium.org/buildbot/nacl_archive/nacl_new_sdk/naclsdk_linux.tgz
echo export NACL_SDK=`pwd`/`tar -tzf naclsdk_linux.tgz | head -n 1 | tr -d /` > naclvars
echo export 'PATH=$NACL_SDK/toolchain/linux_x86/bin:$PATH' >> naclvars
source naclvars
tar -zxvf naclsdk_linux.tgz
# download esidl source code
mkdir esidl
cd esidl
svn checkout http://es-operating-system.googlecode.com/svn/trunk/esidl src
# build esidl Web IDL compiler and install it into the NaCl SDK
mkdir obj
cd obj
../src/configure --prefix=$NACL_SDK/toolchain/linux_x86 --disable-java --disable-cplusplus --disable-npapi
make
make install
cd ..
# generate C++ DOM API header files with esidl, and build C++ DOM API library and example applications
mkdir nacl
cd nacl
CXXFLAGS='-fno-rtti -fno-exceptions' ../src/configure --prefix=$NACL_SDK/toolchain/linux_x86 --htmldir=$NACL_SDK/../esidl/examples --disable-java --host=nacl --target=nacl
make
# install html and nexe files to $NACL_SDK/../esidl/examples
make install
cd ../examples
# start a local HTTP server on port 8000
python -m CGIHTTPServer &
# launch Chrome with the --enable-nacl flag
/opt/google/chrome/google-chrome --enable-nacl http://localhost:8000/
kill %python
```

If everything goes fine, you will see the Chrome window like below:

![http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/chrome.png](http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/chrome.png)

From this page, you can start the following application pages:

  * md5sum.nacl.html
  * paint.nacl.html
  * test.nacl.html
  * xhr.nacl.html

In the following sections, paint, xhr, and md5sum applications are described in more details.

Note: With Chrome 5.0.375 beta launched with the --enable-nacl flag,
its renderer process occasionally crashes typically after closing those pages.
It it happens, please go back to http://localhost:8000/.

Hint: it is very easy to use the Native Client SDK with GNU autoconf and automake. Just add the following lines,

```
   nacl)
       basic_machine=$basic_machine-unknown
       ;;
```

under the following sections in your config.sub,

```
   case $basic_machine in
```

Then add AC\_CANONICAL\_TARGET in your configure.ac for using NaCl gcc tool chain with the generated configure script.

## Demo applications ##

### Paint application ###

![http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/paint.png](http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/paint.png)

[paint.nacl.html](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/paint.nacl.html)
([paint.h](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/paint.h) and
[paint.cc](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/paint.cc)) is
a very tiny paint application using the canvas element.
Compare this with [paint.js.html](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/paint.js.html) to see how JavaScript code can be converted into C++ code;
usually it is very straightforward as shown below:

**Example**

#### JavaScript ####

```
function down(event) {
  var rect = event.target.getBoundingClientRect();
  var context = event.target.getContext("2d");
  context.beginPath();
  context.moveTo(event.clientX - rect.left, event.clientY - rect.top);
  event.target.addEventListener('mousemove', move, true);
}
```

#### C++ ####

```
using namespace org::w3c::dom;

void PaintInstance::down(events::Event* event) {
  events::MouseEvent* mouse = interface_cast<events::MouseEvent*>(event);
  if (!mouse)
    return;
  html::HTMLCanvasElement* canvas = interface_cast<html::HTMLCanvasElement*>(mouse->getTarget());
  if (!canvas)
    return;
  ClientRect* rect = canvas->getBoundingClientRect();
  if (!rect)
    return;
  html::CanvasRenderingContext2D* context = interface_cast<html::CanvasRenderingContext2D*>(canvas->getContext("2d"));
  if (context) {
    context->beginPath();
    float x = mouse->getClientX() - rect->getLeft();
    float y = mouse->getClientY() - rect->getTop();
    context->moveTo(x, y);
  }
  events::EventTarget* eventTarget = interface_cast<events::EventTarget*>(canvas);
  if (eventTarget)
    eventTarget->addEventListener("mousemove", moveHandler, true);
}
```

Note: interface\_cast<> is replaceable with dynamic\_cast<> if C++ RTTI is enabled. esidl generates queryInterface() function in each generated C++ class so that interface\_cast<> can be used without RTTI.

Please refer to Web IDL C++ binding specification `[`[1](CplusplusBinding.md)`]` to see how Web IDL interfaces are converted to C++ classes in more details.

In C++ code, DOM objects obtained from the browser are automatically reclaimed after each function,
e.g., initialize() and event handlers, down(), move(), up(), and select(),
completes its execution (`*`).
The window object in PluginInstance class is the only object that is retained from the plugin instantiation to its termination by default.

It would be better to manually release DOM objects created in a plugin module like downHander, moveHandler, and so forth,
when those become unnecessary like in paint.cc.
However, in C++ DOM API implementation over NPAPI,
the browser will automatically release those objects after the plugin instance is destroyed anyway.

(`*`) It is possible to manually keep DOM objects across multiple function invocations using Object’s retain() and release() methods.
The use of these methods should be done with great caution, and generally should be avoided.

### XHR application ###

![http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/xhr.png](http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/xhr.png)

[xhr.nacl.html](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/xhr.nacl.html)
([xhr.h](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/xhr.h) and
[xhr.cc](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/xhr.cc)) illustrates how to use the XMLHttpRequest object in C++.
In JavaScript, you can create an XMLHttpRequest instance by,

```
   var xhr = new XMLHttpRequest();
```

In C++, you cannot do this, and it takes a few more steps as show below:

```
 XMLHttpRequest_Constructor* xmlHttpRequest =
   interface_cast<XMLHttpRequest_Constructor*>(
     window->getElement("XMLHttpRequest"));
 if (xmlHttpRequest) {
   XMLHttpRequest* xhr = xmlHttpRequest->createInstance();
```

As you see, the XMLHttpRequest object is a distinct, constructor object inside the browser.

In C++ DOM API, the class name of a constructor object is the concatenation of the type name of the base interface and the string "`_Constructor`".
You can create other objects from constructors in the same manner.

### MD5 application ###

![http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/md5.png](http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/md5.png)

While the previous two demo applications, paint and xhr, illustrated we can create Web applications entirely in C++,
[md5sum.nacl.html](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/md5sum.nacl.html)
([md5.idl](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/md5.idl),
[md5sum.h](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/md5sum.h) and
[md5sum.cc](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/md5sum.cc))
shows that the C++ DOM API can also be used to empower JavaScript programming environment in the browser.

In md5sum.nacl.html, an MD5 context object is created in JavaScript by,

```
var md5 = new MD5();
```

This md5 instance refers to an MD5Object instance created in md5sum.h.
The MD5 constructor object is created and exposed to JavaScript in md5sum.cc by,

```
md5class = new (std::nothrow) MD5Class(this);
if (md5class) {
  window->setElement("MD5", md5class);
}
```

The MD5Object and MD5Class classes implement the com::rsa::MD5 and com::rsa::MD5\_Constructor interface classes generated by esidl from md5.idl:

```
[Prefix=::com]
module rsa {
   [Constructor]
   interface MD5 {
       void update(DOMString input);
       DOMString final();
   };
};
```

By this way, you can expose as many constructor objects as you wish to the JavaScript environment.
It would be a good strategy when creating a real application to use JavaScript where productivity matters and C++ where the CPU performance matters.

Please refer to W3C Web IDL Editor’s Draft `[`2`]` for how to write Web IDL definitions.

### Calling arbitrary JavaScript functions from C++ ###

Occasionally you may want to invoke arbitrary JavaScript functions from C++.
In the C++ DOM API, an unknown JavaScript function is represented as a
[Function](http://www.whatwg.org/specs/web-apps/current-work/multipage/webappapis.html#function) interface defined in HTML5.

So a JavaScript function like,

```
 function testFunction(a, b) {
 }
```

can be invoked from C++ by doing this,

```
 html::Function* function =
   interface_cast<html::Function*>(window->getElement("testFunction"));
 function->call(Sequence<Any>({ window, "hello, ", "world." }));
```

Note the 1st argument, window, is used as 'this' value in testFunction() in JavaScript.

[test.nacl.html](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/test.nacl.html)
([test.h](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/test.h) and
[test.cpp](http://es-operating-system.googlecode.com/svn/trunk/esidl/npapi/sample/test.cpp)) contains
the actual code of doing this.

### NPP function implementation for C++ DOM API ###

By using C++ DOM API, we can mostly ignore the fact that these are running over NPAPI.
However there are several exceptions.
Each .nexe file needs be linked with np\_entry.cpp and npp\_gatet.cpp that come with the demo applications.
Plus you’ll need to implement NPP\_Initialize(), NPP\_Shutdown(), NPP\_New(), and NPP\_Destroy() separately as described in the following subsections.

#### NPP\_Initialize() ####

C++ DOM API uses a reflection runtime to interact with the DOM objects inside the browser.
To do so, the metadata of the DOM objects needs to be registered to the reflection runtime.
A typical NPP\_Initialize() would look like,

```
NPError NPP_Initialize()
{
   initializeMetaData();
   initializeHtmlMetaData();
   return NPERR_NO_ERROR;
}
```

Since the total size of metadata for the entire WebApps APIs can be huge, the metadata can be registered separately per API basis by using the following functions:

```
void initializeMetaData();
void initializeFileMetaData();
void initializeGeolocationMetaData();
void initializeHtmlMetaData();
void initializeIndexedDBMetaData();
void initializeSvgMetaData();
void initializeWebDatabaseMetaData();
void initializeWebGLMetaData();
void initializeWorkersMetaData();
```

For example, if your application needs to use Geolocation API, initializeGeolocationMetaData() should be called as well in NPP\_Initialize().

#### NPP\_New() ####

NPP\_New() is where your plugin instance is created.
A typical NPP\_New() would look like,

```
NPError NPP_New(NPMIMEType pluginType, NPP npp, uint16_t mode,
               int16_t argc, char* argn[], char* argv[],
               NPSavedData* saved) {
 if (!npp) {
   return NPERR_INVALID_INSTANCE_ERROR;
 }
 NPObject* window;
 NPN_GetValue(npp, NPNVWindowNPObject, &window);
 YourInstance* instance = new (std::nothrow) YourInstance(npp, window);
 npp->pdata = instance;
 if (!instance) {
   NPN_ReleaseObject(window);
   return NPERR_INVALID_INSTANCE_ERROR;
 }
 // Reclaim DOM objects being used in YourInstance constructor.
 if (ProxyControl* proxyControl = instance->getProxyControl()) {
   proxyControl->leave();
 }
 return NPERR_NO_ERROR;
}
```

YourInstance class must have PluginInstance class as its base class like,

```
class YourInstance : public PluginInstance {
public:
 YourInstance(NPP npp, NPObject* window)
     : PluginInstance(npp, window) {
   initialize();
 }
 ~YourInstance();
 // snip
};
```

The PluginInstance class provides a garbage collection subsystem to your C++ DOM application instance.
After the C++ DOM API runtime invokes an event handler in your plugin instance,
it automatically invokes the garbage collector as mentioned before.
However, the garbage collector is not automatically invoked after the constructor of YourInstance is invoked.
The following lines in NPP\_New() manually does this,

```
 // Reclaim DOM objects being used in YourInstance constructor.
 if (ProxyControl* proxyControl = instance->getProxyControl()) {
   proxyControl->leave();
 }
```

In addition to this case, there are two more places you need to explicitly interact with the garbage collection subsystem in the PluginInstance class.
For objects exposed to the browser from your application module, like `events::EventListener` objects,
Object’s retain() and release() methods must be implemented this way to retain those as long as the browser refers to them through EventTarget objects and so forth:

```
class EventHandler : public org::w3c::dom::events::EventListener {
public:
 EventHandler(PaintInstance* instance,
              void (PaintInstance::*handler)(org::w3c::dom::events::Event*))
     : instance(instance),
       handler(handler) {
 }
 virtual void handleEvent(org::w3c::dom::events::Event* evt) {
   (instance->*handler)(evt);
 }
 unsigned int retain() {
   return instance->retain(this);  // Invoke PluginInstance’s retain().
 };
 unsigned int release() {
   return instance->release(this);  // Invoke PluginInstance’s release().
 };

private:
 PaintInstance* instance;
 void (PaintInstance::*handler)(org::w3c::dom::events::Event* evt);
};
```

#### NPP\_Destroy() ####

NPP\_Destroy() is where your plugin instance is destroyed.
A typical NPP\_Destroy() would look like this:

```
NPError NPP_Destroy(NPP npp, NPSavedData** save) {
 if (npp == NULL) {
   return NPERR_INVALID_INSTANCE_ERROR;
 }
 PluginInstance* instance = static_cast<PluginInstance*>(npp->pdata);
 if (instance) {
   delete instance;
   npp->pdata = 0;
 }
 return NPERR_NO_ERROR;
}
```

Please refer to the NPAPI documentation `[`3`]` for more details about NPP and NPN functions.

## Using C++ DOM API with Firefox ##

![http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/firefox.png](http://es-operating-system.googlecode.com/svn/html/CPlusPlusDOM/firefox.png)

C++ DOM API can be used with Firefox for the ordinary NPAPI plugin development.
To do so, remove --disable-cplusplus --disable-npapi options from the first configure line in the Setting up section:

```
  ../src/configure --prefix=$NACL_SDK/toolchain/linux_x86 --disable-java
```

Then after make install, .so plugin files are created under $NACL\_SDK/toolchain/linux\_x86/lib. You can copy these .so files into ~/.mozilla/plugins/ for testing.
When testing, please use paint.html instead of paint.nacl.html, and so forth.
You may also find this useful when debugging your C++ DOM application.
For example, if you like to use kdbg gdb GUI front-end, you can start firefox with gdb by,

```
$ firefox -g -d kdbg
```

Please consult firefox and other documentations for how to install the firefox debug information, etc.

## W3C/WHATWG specifications covered by C++ DOM API ##

As of writing, the following W3C/WHATWG specifications are covered by C++ DOM API:

  * CSSOM, W3C Editor's Draft 31 March 2010, http://dev.w3.org/csswg/cssom/
  * CSSOM View Module, W3C Editor's Draft 2 February 2010, http://dev.w3.org/csswg/cssom-view/
  * DOM Level 2 CSS, http://www.w3.org/TR/2000/REC-DOM-Level-2-Style-20001113/css.idl
  * DOM Level 2 Ranges, http://www.w3.org/TR/2000/REC-DOM-Level-2-Traversal-Range-20001113/ranges.idl
  * DOM Level 2 Traversal, http://www.w3.org/TR/2000/REC-DOM-Level-2-Traversal-Range-20001113/traversal.idl
  * DOM Level 3 Core, http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/idl/dom.idl
  * DOM Level 3 Events, http://dev.w3.org/2006/webapi/DOM-Level-3-Events/html/DOM3-Events.html?rev=1.129
  * DOM Level 3 Load and Save, http://www.w3.org/TR/2004/REC-DOM-Level-3-LS-20040407/ls.idl
  * DOM Level 3 Validation, http://www.w3.org/TR/2004/REC-DOM-Level-3-Val-20040127/validation.idl
  * DOM Level 3 XPath, http://www.w3.org/TR/2004/NOTE-DOM-Level-3-XPath-20040226/xpath.idl
  * Element Traversal Specification, W3C Recommendation 22 December 2008, http://www.w3.org/TR/2008/REC-ElementTraversal-20081222/
  * File API, W3C Working Draft, http://www.w3.org/TR/2009/WD-FileAPI-20091117/
  * Geolocation API, Editor's Draft 10 February 2010, http://dev.w3.org/geo/api/spec-source.html
  * HTML Canvas 2D Context, W3C Working Draft 4 March 2010, http://www.w3.org/TR/2010/WD-2dcontext-20100304/
  * HTML Microdata, W3C Working Draft 4 March 2010, http://www.w3.org/TR/2010/WD-microdata-20100304/
  * HTML5, W3C Working Draft 4 March 2010, http://www.w3.org/TR/2010/WD-html5-20100304/Overview.html
  * HTML5 Web Messaging, W3C Editor's Draft 12 April 2010, http://dev.w3.org/html5/postmsg/
  * Indexed Database API, W3C Editor's Draft 15 April 2010, http://dev.w3.org/2006/webapi/WebSimpleDB/
  * Progress Events 1.0, W3C Working Draft 21 May 2008, http://www.w3.org/TR/2008/WD-progress-events-20080521/
  * SMIL Animation, http://www.w3.org/TR/2001/REC-smil-animation-20010904/
  * Scalable Vector Graphics 1.1, W3C Recommendation 14 January 2003, http://www.w3.org/TR/2003/REC-SVG11-20030114/
  * Selectors API Level 1, W3C Candidate Recommendation 22 December 2009, http://www.w3.org/TR/2009/CR-selectors-api-20091222/
  * Server-Sent Events, W3C Working Draft 22 December 2009, http://www.w3.org/TR/eventsource/
  * Web Notifications, W3C Editor's Draft 22 April 2010, http://dev.w3.org/2006/webapi/WebNotifications/publish/
  * Web Sockets API, W3C Working Draft 22 December 2009, http://www.w3.org/TR/websockets/
  * Web SQL Database, W3C Editor's Draft 4 March 2010, http://dev.w3.org/html5/webdatabase/, (This specification has reached an impasse)
  * Web Storage, W3C Working Draft 22 December 2009, http://www.w3.org/TR/webstorage/
  * Web Workers, W3C Working Draft 22 December 2009, http://www.w3.org/TR/workers/
  * WebGL, Khronos Working Draft, Working Draft 12 May 2010, https://cvs.khronos.org/svn/repos/registry/trunk/public/webgl/doc/spec/WebGL-spec.html
  * XMLHttpRequest Level 2, W3C Editor's Draft 19 February 2010, http://dev.w3.org/2006/webapi/XMLHttpRequest-2/

For specifications those are still using OMG IDL, C++ DOM API uses the IDL definitions rewritten in Web IDL by Cameron McCormack, the Web IDL spec editor:
> http://lists.w3.org/Archives/Public/public-script-coord/2009OctDec/0022.html

Note: Many of these specifications are still drafts, and revised frequently.
Your Web browser may not have implemented the features specified in the above specifications yet.
In that case, those features cannot be used with C++ DOM API as well.
The situation is almost exactly same as with JavaScript.
The C++ DOM API continues to incorporate the updated specifications, too.

**References**

`[`1`]` Web IDL - C++ binding, http://code.google.com/p/es-operating-system/wiki/CplusplusBinding

`[`2`]` W3C Web IDL Editor’s Draft, http://dev.w3.org/2006/webapi/WebIDL/

`[`3`]` Gecko Plugin API Reference, https://developer.mozilla.org/en/Gecko_Plugin_API_Reference

`[`4`]` http://groups.google.com/group/es-operating-system/browse_thread/thread/037d65a0f614d0d8