  * **16 December 2013**: escudo 0.4.0 and esidl 0.4.0 packages for **Fedora 20** are available [here](http://download.esrille.org/).
  * **24 October 2013**: [escudo Web browser 0.4.0](https://code.google.com/p/es-operating-system/downloads/detail?name=escudo-0.4.0.tar.gz) and [esidl Web IDL compiler 0.4.0](https://code.google.com/p/es-operating-system/downloads/detail?name=esidl-0.4.0.tar.gz) released. Binary packages for Fedora 18, 19 and Ubuntu 13.04 and **13.10** are available [here](http://download.esrille.org/).
  * **16 September 2013**: [escudo Web browser 0.3.3](https://code.google.com/p/es-operating-system/downloads/detail?name=escudo-0.3.3.tar.gz) and [esidl Web IDL compiler 0.3.3](https://code.google.com/p/es-operating-system/downloads/detail?name=esidl-0.3.3.tar.gz) released. Binary packages for Fedora 18, 19 and Ubuntu 12.10 and 13.04 are available [here](http://download.esrille.org/).
  * **[Older news](News.md)**

---

The source code repositories for esidl and escudo are now on Github!

https://github.com/esrille

---


ES operating system is a new pure component operating system currently being developed mainly at [Esrille](http://www.esrille.com/). ES operating system project was originally started by Shiki Okasaka and Kyu Ueno at Nintendo largely affected by Rob Pike's "[Systems Software Research is Irrelevant](http://herpolhode.com/rob/utah2000.pdf)" talk in 2000. ES operating system was released under an open source license from Nintendo at [SourceForge.JP](http://nes.sourceforge.jp/) in 2006. The source code repository of the ES operating system was moved from SourceForge.JP to Google Code in 2008 under the copyright of both Nintendo and Google in hope we can reach more people worldwide.

The project goals include but not limited to:
  * a pure component operating system kernel design and development
  * a new [web browser](BuildingWebBrowser.md) design and development including a new CSS/HTML rendering engine
  * a [Web IDL](http://dev.w3.org/2006/webapi/WebIDL/) based component object binding runtime implementation for C++, ECMAScript, and other programming languages
  * a TCP/IP stack implementation based on design pattern

As we realize [ECMAScript](http://www.ecma-international.org/publications/standards/Ecma-262.htm)/Web based applications are becoming very important and useful, ES operating system has been designed to make the Web Apps APIs as the primary operating system interfaces, and to be an extensible operating system by supporting the component technology from the operating system kernel level as originally proposed for the future direction by [Noah Mendelsohn](http://www.arcanedomain.com/) in "[Operating Systems for Component Software Environments](http://www.arcanedomain.com/publications/HOTOS%20Final%20Version.pdf)" in 1997.

In ES operating system, every system API is defined in [Web IDL](http://dev.w3.org/2006/webapi/WebIDL/), a new interface definition language used for defining APIs for Web Apps including [HTML5](http://www.whatwg.org/specs/web-apps/current-work/multipage/), [Web GL](https://cvs.khronos.org/svn/repos/registry/trunk/public/webgl/doc/spec/WebGL-spec.html), and so forth. New APIs defined in Web IDL can be dynamically added to the system by loading the corresponding new software components running at the user level. With a better security model in the operating system, we should be able to bring better extensibility to the computers and devices connected to the Internet. For example, a web browser could be implemented as a seamless composition of various software components from various companies, communities, and organizations. As the Web is becoming the new platform replacing the traditional operating systems, we believe we will need an extensible operating system like we're studying with ES operating system. A unique part in ES operating system is we are using Web IDL to generate interfaces for C++ not only for ECMAScript and Java, and the software components written in C++ can be seamlessly controlled from ECMAScript.

# Introduction #
  * [Introduction](XV_Semana_Informatica.md)
  * [Using the ES operating system](UsingES.md)
  * [Using Escort Web Browser](UsingEscort.md)  (Last updated on: **17 February 2013**)

# How-tos #
  * [Quick Developer Setup](QuickSetup.md)
    * [Setting up the development environment](DeveloperSetup.md)
    * [Building the support libraries](BuildingSupportLibraries.md)
    * [Building the ES operating system](BuildingES.md)
    * [ES Web browser for Linux](BuildingWebBrowser.md) (Last updated on: **26 June 2012**)
  * [Developing ES on a physical machine](PCHowto.md)
  * [Requesting code review](CodeReview.md) (for GSoC students and contributors)

# C++ DOM API #
  * [Web IDL - C++11 binding](Cplusplus0xBinding.md)
  * [C++ DOM API Introduction](CplusplusDOM.md)
  * [Web IDL - C++ binding](CplusplusBinding.md)

# References #
  * [esidl, Web IDL compiler](http://code.google.com/p/es-operating-system/wiki/esidl)
  * [The ES operating system's namespace](http://code.google.com/p/es-operating-system/wiki/Namespace)
  * [ES Coding Style Guideline](http://code.google.com/p/es-operating-system/wiki/Style)

# Design Documents #
  * [The design of the ES pure component kernel](http://code.google.com/p/es-operating-system/wiki/Kernel)
  * [The design of the ES operating system's TCP/IP stack](http://code.google.com/p/es-operating-system/wiki/Conduit)

# Presentation #
  * S. Okasaka. ES Operating System from Google Code. [The XV Semana Informática do Instituto Superior Técnico](http://xv.sinfo.ist.utl.pt/en/), March 12, 2008. ([summary](http://code.google.com/p/es-operating-system/wiki/XV_Semana_Informatica), invited talk)
  * S. Okasaka and K. Ueno. An Extensible Operating System Architecture Using Reflection. In Proceedings of IPSJ SIG OS, Vol. 2007, No. 10, pp.1-8, 2007. ([paper](http://nes.sourceforge.jp/doc/os-104-1.pdf) | [slide](http://nes.sourceforge.jp/doc/os-104-1-vsd.pdf), in Japanese)

# Google Summer of Code™ 2009 #
ES operating system has participated in the [Google Summer of Code™ 2009](http://socghop.appspot.com/org/show/google/gsoc2009/esos).
And [three students](http://socghop.appspot.com/org/home/google/gsoc2009/esos) successfully passed the final evaluation.
Our ideas list for GSoC 2009 is [here](http://code.google.com/p/es-operating-system/wiki/GSoC2009).

# Google Summer of Code™ 2008 #
ES operating system has participated in the [Google Summer of Code™ 2008](http://code.google.com/soc/2008/).
And [two students](http://code.google.com/soc/2008/esos/about.html) successfully passed the final evaluation.
Our ideas list for GSoC 2008 is [here](http://code.google.com/p/es-operating-system/wiki/GSoC2008).

# Contact Us #
Please feel free to post your questions, suggestions, etc., on any topics related to the ES operating system to [our discussion group](http://groups.google.com/group/es-operating-system). We always welcome your input.