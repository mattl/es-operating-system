#This is a summary of the talk for the XV Semana Informática do Instituto Superior Técnico of Lisbon on March 12, 2008.

# ES operating system from Google Code #

## Introduction ##

In 2004, I started a new operating system project in Nintendo largely affected by Rob Pike's "Systems Software Research is Irrelevant" talk in 2000 `[5]`. It took about two year before we released the first ES operating system from SourceForge.jp, which is the Japanese version of SourceForge.net.

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/stats-graph.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/stats-graph.png)

This is the chart of the download numbers and page view counts of the our former project website at SourceForge.jp from August 2006 to February 2008. The red line is for the page views and the blue line is for downloads.

Initially, we just released the ES kernel and a Squeak port for ES. It took about three months before somehow we got noticed by a media. The first peak was raised just after the news post to Slashdot Japan.

But then the access numbers went back to pretty low level again. Even though we had been releasing new features regularly like SMP support, [TCP/IP stack](Conduit.md), ECMAScript interpreter and so on, it didn't affect to the page views and downloads very much.

Moving a new open source project forward is not that easy, particularly in its early stages because it is irrelevant to the majority of people except those who have strong interests in the source code of the ES operating system.

One thing I'm doing occasionally is searching blogs writing about our projects. Their comments is of course interesting part, but what's more interesting is reading what they are writing about other than ES in their blogs because that helps me to understand what are the real problems for them who also have some interests in ES. I adjust our goals and priorities following those their indirect feedbacks.

The second peak appeared near the end of the last year. OSNews.com picked up the ES operating system in their articles in late November; They gave a link to a machine-translation of our project's home page, which was translated by Google Translate BETA. One of the interesting comments was "Who wrote this? Yoda??". :-) Anyways, many followup articles appeared on the net about ES after it was picked by OSNews. And this time there were really many people who seriously took a look into what we were doing. I read really many blog posts writing about ES.

And now this project is hosted in Google Code under the copyright of both Google and Nintendo in hope we can reach more people worldwide. Now I know this site is actually accessed from over a hundred countries by using Google Analytics.

> "Who needs new operating systems, anyway?"

> Rob Pike,
> Bell Labs, Lucent Technologies, 2000 `[5]`

Probably this is also the question you might have. So I'd like to explain the background before I started the ES project, and hopefully I can share with you our ideas. And then I'd like to describe the design and development of the current ES operating system.

## Background: Monolithic Operating Systems ##

Today, there are many operating systems, and many application programs for each operating system.

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/mono.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/mono.png)

In the today's desktop PC market, we have Windows, Mac, and Linux. In the game console market, we have Wii, XBox, and PlayStation. It's maybe just a coincidence, but it seems these consumer market is not big enough to have a large fourth party. Remaining spot is so small. We need to do something different, something new to remain or to enter the market.

So because of the market competition, these operating systems tend to become bigger and bigger to support more attractive features for customers. One big issue is how rapidly we can extend the operating system features. But ironically, the other big issue is as the system becomes bigger, it becomes much harder to add more features.

How did the operating system researchers handle these issues?

## Background: Microkernels ##

The microkernel architecture is one of the possible solution. Theoretically, you can extend the OS features by just adding server softwares in userland.

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/microkernel.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/microkernel.png)

However, a lot of researches addressed something different:

> "New operating systems today tend to be just ways of reimplementing Unix. How can operating systems research be relevant when the resulting operating systems are all indistinguishable?"

> Rob Pike,
> Bell Labs, Lucent Technologies, 2000 `[5]`

I think the most important lesson we learned through microkernel research is that with a very thoughtful design you can actually create your own, modern, useful operating system by yourself or by a really small team. If I have never seen the MINIX operating system, maybe I wouldn't even think about starting a new operating system project. And today I think the odds is actually higher than in the 1980's or 90's. We can easily find very useful open source tools and libraries, which we can use for constructing a new operating system.

## Background: The Internet ##

What's new today? The Internet. Quite obvious. Applications running in web browsers are becoming popular, useful, and powerful.

> "With the enormous growth of the worldwide web and the growth of the standards for it like HTTP, HTML, XML, etc., the importance of a new OS structure was considerably diminished."

> Jim Mitchell,
> Sun Microsystems, Inc.,
> 2001 `[4]`

In early 90's, Sun has a microkernel based, research operating system called Spring, but they shifted their engineering resource to Java because their original goal for Spring was to "reimplement UNIX in an object-oriented fashion", not to create a totally new operating system. So I think the more appropriate way to put this quote was:

> "The importance of a new **Unix** structure was considerably diminished."

But several key technologies they developed like IDL, or a fast IPC mechanism are still very important to the today's design and implementation of the ES operating system.

And what's really hot today is JavaScript applications running in the browsers.

> "There is no reason for a JavaScript application to run any slower than a comparable Smalltalk application (Smalltalk is an equally dynamic programming language), or significantly slower than a comparable Java application."

> Dan Ingalls, et al.,
> Sun Microsystems, Inc., 2007 `[2]`

It's just amazing how rapidly people can now create a new web desk top environment within a web browser. I'm totally sure more application programs will be written in JavaScript, ActionScript or ECMAScript, whatever it's called today.

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/browser.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/browser.png)

But if you look at this diagram carefully, don't you think this is too complicated? Microkernels were designed to support various server pieces to extend the operating system features. The web browsers are doing the same thing: supporting various plug-ins to extend its features, but using their own component management scheme. Most of the features of the operating system's windowing system is also duplicated in today's web browsers.

How can we simplify this whole picture, not just under the traditional operating system layer?

## Background: Component Operating Systems ##

Eleven years ago, Mr. Noah Mendelsohn were questioning us if we can construct a new operating system for component software environment, which can seamlessly host application software components and system software components.

> "Components: a revolution in application development. But most operating systems aren't designed to host component based applications."

> Noah Mendelsohn,
> Lotus Development Corporation, 1997 `[3]`

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/components.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/components.png)

I think Microsoft's COM architecture is a pretty close answer to his question. One unfortunate part for COM was, I think, it was introduced in the middle of the evolution of the C++ programming language. Even though COM uses a very similar runtime object model to C++, the C++ didn't have a standard language specification nor standard application binary interface when COM was introduced.

Evolution of C++:
  * Itanium C++ ABI, 2001
  * ISO C++ 2nd ed., 2003
  * GCC 3.4, 2004
  * GCC 4.0, 2005
  * C++ TR1, 2005

This is the time line of the recent evolution of C++. And COM was introduce in 1990's. Without the standard ABI, it's just impossible to write a portable operating system kernel in C++. And even application programmers occasionally had to write COM components in C those days, which was a pain and unproductive.

But today we can clearly see that in the future we will use JavaScript for writing much more applications which will be more universally accessible. And today, we have the standard C++ language specification and the standard C++ ABI. We have even free C++ compilers that follow these standards. What if we design a component operating system all over again, using the latest C++, and integrating JavaScript virtual machine from the very beginning?

## Components: Pros and Cons ##

On components, there has been really long discussions about what is the appropriate level of separation of components. I don't think there is any "standard" for that, yet.

In early 1980's, Prof. David Clark wrote this in his forward to a text book on TCP/IP by Prof. Douglas Comer:

> "The process is the fundamental structuring component provided by most systems. It is natural to try to map the basic module of the specification to the basic component of the system; this maps layer to process. The result, as least in our experience with protocols, is almost always substantially inefficient."

> "The technique we used was to implement Swift in a high level typesafe language with garbage collection."

> David. D. Clark,
> Massachusetts Institute of Technology, 1985 `[1]`

There were network protocol stack implementations that tried to map each network layer to a separate process, like one process for the IP layer, one for UDP, and one for TCP, something like that. You might say it is apparently inefficient by today's perspective. But the TCP/IP stack is a really complicated software and it has been enhanced over years and it's being improved even now.  It's natural that people tried to decouple it into more tiny pieces of manageable components.

This was written in 1985, well before microkernel researchers were discussing about their RPC performance issues on microkernels.

> "There has been much talk about component architectures but only one true success: Unix pipes. It should be possible to build interactive and distributed applications from piece parts."

> Rob Pike,
> Bell Labs, Lucent Technologies, 2000 `[5]`

We all know Unix pipes are great invention for processing data in sequence by small software tools running in separate processes.

So we know there is a component architecture that works well, and we know it is very difficult to decouple a really complex system into appropriate pieces of software component that run fast enough.

## Top-down design for the ES operating system ##

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/es-design.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/es-design.png)

This is the current design goals of the ES operating system. It is not a goal building a simplified, decoupled UNIX implementation on top of the microkernel in the ES operating system. The primary goal has been making it possible for developers to easily create rich applications, like 3D games, using ECMAScript.

We provide a unified programming environment for application, service, and kernel development, which means you can use the same programming style, the same programming conventions, and the same software libraries for writing your code no matter where you want to put your component. You can configure where to put your components without modifying your code.

We make the ES operating system interfaces extensible just by loading the metadata of the interface definitions. In Unix, philosophically the right way to extend the operating system feature is inventing a new way to represent the new features as files and directories. So what you need to provide as system call interfaces are the same old open, read, write, close, and stuff like that. But it's like you say you can write any web applications just using the socket interfaces. In ES, we want to provide more meaningful programming interfaces to the programmers from the kernel level, and we want to make the interfaces dynamically extensible.

And we make every software component seamlessly controllable through the ECMAScript interpreter. So we've been designing new, simple, interface definitions for file I/O, sockets, threads, name spaces, etc., that can be used from scripts written in ECMAScript. And we are using C++ to implement the objects that support these new interfaces.

## Interfaces ##

Interfaces are really important pieces of design. So how do we define interfaces?

Originally we were using our own custom IDL grammar, based on DCE IDL specification. Using IDL occasionally discouraged the developers due to its cumbersome, extra procedures. But today, for the most important APIs for web applications like DOM, HTML5, etc., a precise, language-independent specification of the interfaces are already defined in OMG IDL. So we've switched to using the OMG IDL grammar last year.

```
interface CanvasRenderingContext2D {

  attribute float lineWidth;       // (default 1)

  // path API
  void moveTo(in float x, in float y);
  void lineTo(in float x, in float y);
    :
    :
};
```

This is the example of the interface definition from the HTML5 specification. CanvasRenderingContext2D is for 2D graphics. It looks very simple. There is also an on-going work defining CanvasRenderingContext3D for 3D graphics like OpenGL.

So we simply follow them. Actually, the current OMG IDL is insufficient to fully express the semantics behind the design in machine readable form, like HTMLCollection. So I'm expecting some enhancements in the IDL grammar and I'm hoping it won't be too difficult to follow very quickly.

We are defining all the system interfaces in OMG IDL, which makes all the system calls naturally invokable from the ECMAScript.

We don't define any legacy "C" bindings any more. You can still use many principal posix functions in ES user processes, but they are just for importing the highly portable, existing open source libraries like Cairo, FreeType, etc. These posix functions are implemented as library functions in userland and none of them are the system calls in ES.

## esidl - an IDL compiler ##

We've been using our own IDL compiler named esidl.

It converts the interfaces definitions into C++ header files and metadata files. It doesn't generate RPC stubs. Instead, we dynamically install the generated metadata into the kernel.

```
class ICanvasRenderingContext2D : public IInterface
{
public:
    // lineWidth attribute
    virtual float getLineWidth() = 0;
    virtual void setLineWidth(float lineWidth) = 0;

    // path API
    virtual void moveTo(float x, float y) = 0;
    virtual void lineTo(float x, float y) = 0;
        :
        :
};
```

For C++, we are using very straight forward mappings unlike CORBA. The attributes are converted to a pair of accessors because there is no other means. Nothing's really special here.

In ES, from caller's perspective, there is no difference between system calls and remote procedure calls. So this is the standard way to make system calls and remote procedure calls. It's just invoking a virtual function in the abstract interface classes generated by the IDL compiler.

```
    ICanvasRenderingContext2D* canvas;

    canvas->moveTo(512, 200);
    canvas->setLineWidth(4);
```

In ECMAScript, the attributes defined in IDL can be accessed as ordinary property objects as expected.

```
    canvas.moveTo(512, 200);
    canvas.lineWidth = 4;
```

## ES Kernel ##

The ES kernel joins all the software components seamlessly together, like glue, to make up a unified computing system.

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/es-kernel.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/es-kernel.png)

The ES kernel only handles object APIs defined in IDL, and it allows exchanging object interface pointers among user processes. Each interface pointer acts like a capability associated with a specific object interface.

Unlike most other microkernels, a message passing is not a primitive operation in the ES kernel; RPC stubs are not used at all. Instead, both system calls and upcalls are immediately intercepted by the kernel. Since the ES kernel collects the metadata of the interface definitions inside the kernel, it can processes them at the meta-level, using reflection, to directly invoke the appropriate method of the target object.

In this figure, the green arrows represent the system calls and the red arrows represent the upcalls.
In the case of local RPC, the calling thread directly moves to the server address space and makes an upcall to the server object. So a local RPC is basically an operation consisting of a system call and an upcall.

The ES kernel is fully written in C++ in portable manner following the recent standardization effort of the C++ programing language and the C++ ABI. For example, in ES, a C++ language exception with an integer error code triggered in a server process is correctly reported back to the callee in the client process as an ordinary C++ exception.

The extensibility of the ES kernel is assured by allowing the dynamic loading of the new interface metadata into the kernel. The object calls are the the first-class operations in the ES operating system. You don't have to link separate RPC stubs to each application, and overall development cycle has been very simplified.

## esjs - An ECMAScript interpreter ##

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/esjs.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/esjs.png)

We have our own ECMAScript interpreter implementation, esjs, based on the 3rd edition of the ECMAScript specification, which is really simple enough to implement just by a single person in a few months or so. This interpreter is not for the optimal performance, but rather we wanted to study how simply we could implement an ECMAScript to native C++ object binding runtime. By having our own implementation, we have a lot of flexibility to explore the various ways to do that.

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/prototype.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/prototype.png)

Currently, the interpreter creates a prototype object for each interface on the fly from the interface metadata, and uses reflection to convert ECMAScript function calls and attribute accesses into C++ virtual function calls. This approach works pretty well. Once we have completed the implementation of the meta-level code, we can simply use all the defined object APIs without doing any specific tweaks for each interface.

```
    canvas = new ICanvasRenderingContext2D(unknown);
    canvas.lineWidth = 4;
```

So occasionally people talk about the lack of libraries in JavaScript in other systems, that won't be an issue in the ES operating system. Any component written for ES in C++ are simply accessible from the ECMAScript in ES.

## Bottom-up build of the ES operating system ##

I think the "top-down design, bottom-up build" philosophy is one of the most important philosophy I learned when I was a graduate student.

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/es-0-1-1.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/es-0-1-1.png)

Here's how we've proceeded with our development of the ES operating system.

After we've completed the very basic study about the component operating system, we just wrote a kernel boot loader for a floppy disk drive. Then implemented threads, a few device drivers, and a FAT file subsystem.

Then we ported Squeak, a Smalltalk environment, to that minimal kernel so that we can test all the basic primitives interactively. Actually, the virtual machine code of Squeak was directly linked to the kernel at that phase, and all the code was running in the supervisor mode.

Then we added a virtual memory support, which was the first version of the ES operating system released to open source, in which Squeak was running in userland as an application program.

Then we added support for upcalls, SMP, TCP/IP stack, and other relatively basic stuff. These were actually really difficult parts only interesting for professionals and experts in those area, and basically covered by any other operating systems, too. So just saying that our system is a pure component system is just not enough to attract people. So we just kept releasing software as they got ready like real PC boot support, ECMAScript interpreter, the canvas service, the event manager, and so on.

But as I said earlier, I watched many blog entires about ES, and I could see how people guessed correctly or incorrectly what we were aiming at. I saw the canvas API and cairo were hot topics for people who are also interested in ES. So I just skipped all the complicated part of DOM and HTML, and just released the support for the canvas API using Cairo. After that, I could see that people could more easily guess what we were aiming at without speaking a lot like this today, and we were getting noticed by really many people.

I believe some people attending here today must have really great ideas. But perhaps maybe you have to take many classes, you have to write papers nearing the submission deadlines, and maybe you don't have enough time to write your code. So I'd recommend for you to use the "top-down design, bottom-up build" philosophy, too. If you have a great idea, please implement the most interesting part first, and demonstrate it, demonstrate your concept to the people. The "concept demonstration phase" is a very important phase before starting the massive development when designing a completely new, complex product. But ultimately, what is really important is still to create something substantial and something useful for people.

## Current Status ##

Now you can build ES on both x86 and x64 versions of Linux including Fedora and Ubuntu using the current source code in ES subversion trunk in Google Code. And actually you can now build ES on Mac OS X Leopard with XCode.

![http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/screen-shot.png](http://es-operating-system.googlecode.com/svn/html/XV_Semana_Informatica/screen-shot.png)

And I want to mention that although these charts and colorful texts were drawn by using the CanvasRenderingContext2D interface from a script written in ECMAScript, the actual rendering was done by Cairo and FreeType. Those are one of the open source libraries we are using to construct the ES operating system. We are also using other open source libraries including Newlib, PCRE, Expat, and Fontconfig. And we are also using free TrueType fonts from Redhat and others. Of course we are using binutils, GCC and other tools from GNU for building everything.

Today the open source community has a really rich set of software libraries and tools which allow us to design and develop the more operating system specific area much more easily, if we look back ten years ago.

The development of the ES operating system is still at an early stage. We've just demonstrated our concept by implementing a command line shell, an ed clone text editor, etc. in ECMAScript, and by providing a 2D graphics support based on the CanvasRenderingContext2D interface that recently appeared in the HTML5 specification.

## Development Environment ##

Here's the tools and libraries you need to build and to modify the ES operating system:

  * autoconf 2.61 or newer
  * automake 1.10 or newer
  * bison 2.1 or newer
  * flex 2.5 or newer
  * gcc 4.0 or newer
  * glibc 2.2 or newer for host tools, glibc 2.4 or newer recommended
  * pcrc 7.2 or newer configured with the --enable-utf8 and --enable-unicode-properties options

If you can use a latest Linux machine either x86 or x64, that's really fine. But if you need to use relatively older versions of Linux distributions, you might have to build and install these programs manually, which shouldn't take too long, though.

If you can use glibc 2.4 or newer, you can separately test many software components for ES,  including the TCP/IP stack, on Linux using your favorite debugger as an ordinary Linux application.

If you only have a Mac, you can still build ES and test it with the Q x86 emulator, as I have demonstrated it.

So if you have any of those machines, please try making ES on your computer.

And if you only have a Windows machine, at this point, we do not have any useful information for you. Actually, we have used to use cygwin for building ES, but it turned out it's fairly slower than linux running on the exact same machine. And as we moved to the newer tools like GCC 4, we were unable to use cygwin to build ES any more. If you have any ideas using Windows to build ES, please let me know. Virtual PC had no problems running ES.

You can also use actual PCs like listed below for running ES. In any of these configurations, ES runs much much faster than running on QEMU:

  * Pentium III/440BX + SB16 + RTL8029AS
  * Pentium 4/865G+ICH5 + ES1370 + RTL8029AS
  * Core 2 Duo/945G+ICH7 + ES1370 + RTL8029AS

One way to do this is, there are CF-IDE adapters in PC parts shops which let you use a compact flash card as an ATA hard disk drive. So what you really need to do is copy ES files to the CF card, remove it from the host PC and insert it to the CF-IDE adapter connected to the target PC, and just boot it up. ES operating system is small enough to keep everything in a CF card. And we have written device drivers only for the devices that are emulated in QEMU so far.

## Things to be done ##

There are still many things need to be done from now on:

### A pure component operating system kernel design and development ###

The ES kernel is running well as we've designed. The very basic idea of making a new, pure component operating system is working. We still need to do a lot more work on this to raise the quality of the kernel to the production level. We were actually rather in hurry to demonstrate our concept first in the last few years.

We are also thinking about supporting x64 architecture in addition to x86 since now Newlib has an x64 support. Actually most of the components are already 64-bit ready. Adding a new virtual memory management system for x64 will be most major work for this.

### A component object binding runtime implementation for ECMAScript ###

In our ECMAScript interpreter, there are several uncompleted tasks like supporting function overloading, attribute overloading, etc. Actually the attribute overloading is one of the difficult notion to smartly represent in C++ because it results in overloading by the return value types, if applying a simple IDL-C++ mapping, which is supported very limitedly in C++. So we need to think about a good way to handle this.

Also as we move to the newer edition of ECMAScript standard, we will probably incorporate the other open source ECMAScript implementation as writing very fast virtual machine requires very different skills from writing the operating system. So we will probably concentrate on designing and developing the object binding runtime for ECMAScript.

### An HTML5 rendering engine integration ###

This will be the most complicated part for us over the next few months or so because either which open source implementation of the HTML rendering engines we will use, we will still need to do some modifications to that to make it fit into the ES environment. We will need very careful planning on this.

Dan Ingalls at Sun Labs taught me we wouldn't need every piece of HTML rendering features for running the Lively Kernel. So probably just incorporating the portions that are really necessary to run the Lively Kernel can be a good starting point for this.

### A TCP/IP stack implementation based on design pattern ###

I didn't address any details of our TCP/IP stack implementation today, but it is also a challenging one. Inside the ES kernel, we have our own TCP/IP stack implementation based on Conduit+ model proposed by Hueni, Johnson, and Engel. We are working on finalizing the interface definitions in IDL and then we will work on extensive testing on this. In this implementation, I think we have cleanly separated out IPv4 dependent portions from upper UDP and TCP socket layers, so the IPv6 layer integration should be more easy. Even though we already have a preliminary implementation of IPv6, we will release it in open source after we finish the IPv4 implementation.

Even though we've already released more than a hundred thousand lines of C++ code to open source, we still have a lot of work needs to be done.

## Help Wanted ##

So we really welcome helpers. If you'd like to contribute to the ES project, please talk to me after this session.

> "The ideal project is one where people don't have meetings, they have lunch. The size of the team should be the size of the lunch table."

> Bill Joy,
> cofounder of Sun Microsystems, 2003 `[6]`

I really believe this. And you could be the one.

And right now I'm applying for Google Summer of Code 2008 for the ES project.

I'm not sure if my application will be accepted, or not. But if accepted, I can officially mentor a few accepted students so that you can get experience in developing ES operating system with stipend of 4500 USD that goes to you. So please check the site here and apply for ES operating system if you are intrigued. You can work on any of the four areas I've described in the previous slide. Or, if you have any interesting idea that related to the ES project, that's fine, too.

Even if not accepted for Google Summer of Code, there are no stipend payments, but I still want you to join us.

It's people deciding and shaping future computing systems, and I really hope some of you here will join us to complete this project.

Thank you very much. Obrigado!

## References ##

`[1]` Comer, D., Operating System Design - Vol. II: Internetworking with Xinu. Prentice-Hall, 1987.

`[2]` Ingalls, D., Mikkonen T., Palacz, K., Taivalsaari, A. Sun Labs Lively Kernel FAQ. 2007. http://www.sunlabs.com/projects/lively/faq.html .

`[3]` Mendelsohn, N. Operating Systems for Component Software Environments. In Proceedings of the 6th Workshop on Hot Topics in Operating Systems (HotOS-VI), pp. 49-54, May 1997. http://www.arcanedomain.com/publications.html .

`[4]` Mitchell, J. An Overview of the Spring System (In IEEE COMPCOM '94) - Introduction by Jim Mitchell. In Sun Microsystems Laboratories The First Ten Years: 1991-2001, October 2001. http://research.sun.com/features/tenyears/volcd/papers/mitchell.htm .

`[5]` Pike, R. Systems Software Research is Irrelevant. Invited talk, University of Utah, February 2000.

`[6]` Reiss. S. Hope Is a Lousy Defense. WIRED. December 2003. http://www.wired.com/wired/archive/11.12/billjoy.html .


**Note ES operating system has been accepted in to the Google Summer of Code™ 2008. So If you are a student as of April 14, 2008, we really welcome your application for Google Summer of Code™ 2008 in ES operating system.** - March 17, 2008