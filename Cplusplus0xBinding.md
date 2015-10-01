# Web IDL - C++11 binding #

Note: This document is based on the [Web IDL W3C Editor’s Draft](http://dev.w3.org/2006/webapi/WebIDL/).

## _28 April 2011_ ##

This document describes how interfaces described with Web IDL correspond to constructs in C++11 by esidl Web IDL compiler (rev. 1738 or later).

The Web IDL C++11 binding is the new C++ language binding being developed for the use of the next version of the ES operating system.
Currently the Web IDL C++11 binding can also be used with [JSAPI](https://developer.mozilla.org/en/JSAPI_Reference),
[V8 API](https://developers.google.com/v8/embed),
and [NPAPI](https://developer.mozilla.org/en/Gecko_Plugin_API_Reference).
So applications are not limited to the operating systems or Web browsers.
If you want to make your applications support scripting in JavaScript, the Web IDL C++11 binding might be used in your applications along with TraceMonkey JavaScript engine over JSAPI, or V8 JavaScript engine over V8 API, etc.

Web IDL C++11 binding is very different from the previous versions of our Web IDL C++ bindings.
In the C++11 binding, IDL interfaces are directly mapped to C++ concrete classes instead of C++ abstract classes.
We used to use C++ abstract classes in hope that those abstract classes would not change very frequently,
and we would be able to keep binary compatibilities among its applications while refining the implementation classes for these interfaces.

However, it turns out this is not true.
Old versioning schemes like assigning globally unique ID to each interface, or introducing new namespace, etc., do not seem to work anymore.
Hixie described this situation around HTML in [the WHATWG blog](http://blog.whatwg.org/html-is-the-new-html5) as,

"_we moved to a new development model, where the technology is not versioned and instead we just have a living document that defines the technology as it evolves._"

In short, concrete classes are not that concrete where interfaces are defined in Web IDL.

What are not very fragile are attributes, operations, and constants defined in interfaces, but not interfaces themselves.
Although occasionally minor interfaces or interface members are classified as obsolete due to various reasons,
things very widely accepted and used are not easily taken back anyway.

Back to 1982, it seems Alan Kay mentioned that
"_SmallTalk is object-oriented but should have been message-oriented_" in his talk at "Creative Think" seminar (`*`).
He revisited this issue  [in 1998](http://lists.squeakfoundation.org/pipermail/squeak-dev/1998-October/017019.html) as,

"_The key in making great and growable systems is much more to design how its
modules communicate rather than what their internal properties and
behaviors should be._"

It is hard to fully understand what exactly he meant by that for us, but the current situation seems to be very similar.
We could see messages as the means for accessing properties in interfaces.
Interfaces can only illustrate snapshots of objects at certain point, and definitions of objects are changing as they evolve.
We used to emphasize the interfaces or classes, but what actually matters is each property in interfaces that is in use.

(`*`) Andy Hertzfeld, Revolution in the Valley, O'Reilly, 2004.


The Web IDL C++11 binding is thus designed on a message-oriented architecture.
Before describing its internals, let's see how it is used from applications.
The following JavaScript function excerpted from a tiny paint application using the canvas API,

```
function down(e)
{
    var rect = e.target.getBoundingClientRect();
    var context = e.target.getContext("2d");
    context.beginPath();
    context.moveTo(e.clientX - rect.left, e.clientY - rect.top);
    e.target.addEventListener('mousemove', move, true);
}
```

can be written in C++ using the Web IDL C++11 binding as below:

```
void Paint::down(Event evt)
{
    MouseEvent mouse = interface_cast<MouseEvent>(evt);
    HTMLCanvasElement canvas = interface_cast<HTMLCanvasElement>(mouse.getTarget());
    ClientRect rect = canvas.getBoundingClientRect();
    CanvasRenderingContext2D context = interface_cast<CanvasRenderingContext2D>(canvas.getContext("2d"));
    context.beginPath();
    context.moveTo(mouse.getClientX() - rect.getLeft(), mouse.getClientY() - rect.getTop());
    canvas.addEventListener("mousemove", moveHandler, true);
}
```

Since this is written in C++, you need to explicitly specify the types of objects.
Any object can be accessed using any interface via `interface_cast<>`.
Currently, unknown messages are simply ignored.
This is somewhat like doing dynamic duck typing in C++.
Alternatively, you can explicitly check if `evt` is actually a MouseEvent object by calling,

```
    if (MouseEvent::hasInstance(evt))
        blah blah blah...
```

Otherwise names of functions are same.
As for properties, you need to use member functions that begin with "get" or "set" rather than directly accessing properties as in JavaScript
(e.g., .clientX -> .getClientX()).
No explicit memory management processing like invoking addRef(), release(), etc. is necessary.
Internally, each C++ class generated by esidl compiler from Web IDL interface definition acts more or less like a smart pointer.


Each interface object class has the Object class as its root.
The Web IDL C++11 binding does not support interface multiple inheritance following
[the recent discussion](http://lists.w3.org/Archives/Public/public-script-coord/2010OctDec/0081.html) at W3C.
The complete definition of the Object class can be seen [here](http://code.google.com/p/es-operating-system/source/browse/trunk/esidl/jsapi/include/object.h),
and the following is a simplified excerpt:

```
class Object
{
    Object* object;  // target object
public:
    Object();
    Object(Object* other);
    Object(const Object& other);
    virtual ~Object();
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv) {
        if (object && object != this)
            return object->message_(selector, id, argc, argv);
        return Any();
    }
    Object& operator=(const Object& other);
    bool operator!() const;
    operator void*( ) const;
    bool operator==(const Object& other) const;
    bool operator!=(const Object& other) const;
    bool operator<(const Object& other) const;
    // snip
};
```

The Object class is just keeping a pointer to the target object, which is, in most cases, an object of the actual implementation class.
The heart of the Object class is `message_` member function, which sends a message to the target object and returns the result.
It takes four arguments.
`selector` is a unique hash value generated from `id`, which is the name of the interface property to access.
`argc` is the number of arguments held in `argv`.
Negative `argc` values are reserved for the special operations required for ECMAScript binding support, etc.
The `Any` type corresponds to the Web IDL any type which can keep a all possible Web IDL type value.

Now that we have seen the basics of the Object class, let us turn to an example of a C++ concrete class generated from a Web IDL interface.
For the following EventTarget interface definition,

```
interface EventTarget {
  void addEventListener(DOMString type, EventListener listener, optional boolean capture);
  void removeEventListener(DOMString type, EventListener listener, optional boolean capture);
  boolean dispatchEvent(Event event);
};
```

esidl compiler generates the corresponding C++ class definition like below,

```
class EventTarget : public Object
{
public:
    // EventTarget
    void addEventListener(std::u16string type, EventListener listener);
    void addEventListener(std::u16string type, EventListener listener, bool capture);
    void removeEventListener(std::u16string type, EventListener listener);
    void removeEventListener(std::u16string type, EventListener listener, bool capture);
    bool dispatchEvent(Event event);
    EventTarget(Object* object);
    EventTarget(const EventTarget& object);
    EventTarget& operator=(const EventTarget& object);

    static bool hasInstance(Object& object);

    template <class IMP>
    static Any dispatch(IMP* self, unsigned selector, const char* id, int argc, Any* argv) {
        // snip
    }
    // snip
};
```

and member function definitions.
Here's one of the generate member functions:

```
bool EventTarget::dispatchEvent(Event event)
{
    Any arguments_[1];
    arguments_[0] = event;
    return static_cast<bool>(message_(0xd642a126, "dispatchEvent", 1, arguments_));
}
```

What's important here is that the generated member functions are simply packing arguments and sending a message to the target object.
The same is true for member functions for accessing interface attributes.

C++ classes generated by using the Web IDL C++11 binding look like ordinary C++ classes from outside,
but in fact those are more like syntax sugar in C++ for a classic message dispatching subsystem.
By doing this we would lose some performance, but we've already described its benefits and this is absolutely intentional.

We've described how `hasInstance` can be used.
We will visit `dispatch` static template member function in the generated EventTarget class later.

Now Let us move on to what an implementation class would look like for EventTarget.
Here's one possible version.

```
class EventTargetImp : public Object
{
    Object* object;
public:
    EventTargetImp() : object(this) {}
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv) {
        switch (selector) {
        case 0xd642a126:  // dispatchEvent
             blah blah blah...
        case ...
         }
    // snip
    }
};
```

One thing needs to be emphasize here is EventTargetImp does not have any inheritance relationship with EventTarget.
EventTargetImp is an object that simply understands messages defined with EventTarget.
The definition of EventTarget changes as its specification document changes, and there would be multiple versions of EventTarget class in use.
However, a single EventTargetImp class still supports all of them.

The above code would work, but we know not everyone wants to hand-decode selectors like this way.
This is where `dispatch` static template member function generated by esidl compiler is intended to be used.
With the `dispatch` static template member function, the above EventTargetImp class can be written like below:

```
class EventTargetImp : public ObjectMixin<EventTargetImp>
{
public:
    void addEventListener(std::u16string type, events::EventListener listener, bool capture = false);
    void removeEventListener(std::u16string type, events::EventListener listener, bool capture = false);
    bool dispatchEvent(events::Event event);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv) {
        return EventTarget::dispatch(this, selector, id, argc, argv);
    }
    // snip
};
```

The `dispatch` static template member function is basically a huge switch statement,
but it also deals with special operations, overload resolution, etc., as defined in Web IDL specification.
So whenever Web IDL C++11 binding is used with ECMAScript,
you should use the generated `dispatch` static template member functions.

In the above refined code, we also used ObjectMixin template class with the curiously recurring template pattern (CRTP).
This hides the common object implementations that are required for every implementation class.

For the ease of application development, esidl compiler can generate these skeleton implementation classes for the correspoding interface classes.
So the actual development practice would be like generating header and source files with esidl from Web IDL definitions,
and then adding details, e.g., private members, member function definitions, constructors, and so forth, to the generated implementation files.

The rest of this document is the reference of Web IDL C++11 binding.

### Modules ###

Every IDL module corresponds to a C++ namespace. The name of that namespace is based on module's prefixed name.

#### IDL ####

```
  module dom {
  };
```

#### C++ ####

```
  namespace org {
    namespace w3c {
      namespace dom {
```

#### IDL ####

```
  [Prefix=::com]
  module getfirebug {
```

#### C++ ####

```
  namespace com {
    namespace getfirebug {
```

Note: Historically each W3C specification introduced its own module name. However, the recent specifications tend to omit the module specification in the IDL definition. At least it seems one module name per spec doesn't make sense any more.

> cf. http://lists.w3.org/Archives/Public/public-webapps/2009AprJun/1380.html

### Primitive types ###

| IDL | C++ |
|:----|:----|
| boolean | bool |
| byte | signed char |
| octet | unsigned char |
| short | short |
| unsigned short | unsigned short |
| long | int |
| unsigned long | unsigned int |
| long long | long long |
| unsigned long long | unsigned long long |
| float | float |
| double | double |

### Any ###

The any type is the union of all other possible types.

#### IDL ####

```
  any
```

#### C++ ####

```
  class Any {
   public:
    Any();
    Any(const Any& value);
    template <typename T>
    Any(const T& x);
    Any& operator=(const Any& x);
    template<typename T>
    Any& operator=(const T& x);
    template<typename T>
    operator T() const;
    // snip
  };
```

Example

#### IDL ####

```
  interface CanvasRenderingContext2D {
    attribute any fillStyle;
  };
```

#### C++ ####

```
  class CanvasRenderingContext2D : public Object {
  public:
    Any getFillStyle();
    void setFillStyle(Any fillStyle);
  };

  canvasRenderingContext2D->setFillStyle("black");
```

### DOMString ###

The DOMString type corresponds to the set of all possible sequences of 16 bit unsigned integer code units to be interpreted as UTF-16 encoded strings.

| IDL | C++ |
|:----|:----|
| DOMString | std::u16string |

### Date ###

The Date type corresponds to a 64 bit unsigned integer value. In C++ DOM API, the DOMTimeStamp type is defined as Date following Chrome's implementation as specified in DOM Level3 Core (`*`).

| IDL | C++ |
|:----|:----|
| Date	| unsigned long long |

(`*`) It appears currently Chrome is the only browser that treats the DOMTimeStamp type as the Date object in ECMAScript.

### Object ###

The object type corresponds to the set of all possible object references, plus the special value null, which indicates no object reference.

| IDL | C++ |
|:----|:----|
| object | Object |
| null	| 0   |


### Interfaces ###

An interface is a specification of a set of interface members, which are the constants, attributes and operations.

#### IDL ####

```
  interface CanvasRenderingContext2D {
  };
```

#### C++ ####

```
  class CanvasRenderingContext2D : public Object {
  };
```

### Nullable types - T? ###

A nullable type is an IDL type that can represent an existing type (called the inner type) values, plus an additional value null.

| IDL | C++ |
|:----|:----|
| inner-type? | `Nullable<inner-type>` |

#### C++ ####

```
  template <typename T>
  class Nullable {
   public:
    bool hasValue() const;
    T value() const;
    Nullable();
    Nullable(const T& value);
    Nullable(const Nullable<T>& nullable);
    Nullable(const Any& any);
    // snip
  };
```

Example

#### IDL ####

```
  attribute DOMString? background;
```

#### C++ ####

```
  Nullable<std::string> getBackground();
  void setBackground(Nullable<std::string> background);
```

### Sequences - `sequence<T>` ###

The `sequence<T>` type is a parameterized type whose values are (possibly zero-length) sequences of values of type T.

| IDL | C++ |
|:----|:----|
| `sequence<T>` | `Sequence<T>` |

#### C++ ####

```
  template <typename T>
  class Sequence
  {
   public:
    Sequence();
    Sequence(const T* array, unsigned int size);
    Sequence(std::initializer_list<T> list);
    Sequence(const Sequence& value);
    ~Sequence();
    T operator[](int index) const;
    T& operator[](int index);
    unsigned int getLength() const;
    // snip
  };
```
#### IDL ####

```
  typedef stylesheets::StyleSheetList StyleSheetList;

  readonly attribute StyleSheetList styleSheets;
```

#### C++ ####

```
  typedef Sequence<StyleSheet*> StyleSheetList;

  StyleSheetList getStyleSheets();
```

### Constants ###

Constants can be defined in interfaces and exceptions.

#### IDL ####

```
  interface MediaError {
    const unsigned short MEDIA_ERR_ABORTED = 1;
    const unsigned short MEDIA_ERR_NETWORK = 2;
    const unsigned short MEDIA_ERR_DECODE = 3;
    const unsigned short MEDIA_ERR_NONE_SUPPORTED = 4;
  };
```

#### C++ ####

```
    class MediaError : public Object {
    public:
      static const unsigned short MEDIA_ERR_ABORTED = 1;
      static const unsigned short MEDIA_ERR_NETWORK = 2;
      static const unsigned short MEDIA_ERR_DECODE = 3;
      static const unsigned short MEDIA_ERR_NONE_SUPPORTED = 4;
    };
```

### Operations ###

Each operation defined in the IDL interface will be mapped to one or more member functions in the C++ interface class.

#### IDL ####

```
  interface CanvasRenderingContext2D {
    void fillRect(float x, float y, float w, float h);
  };
```

#### C++ ####

```
  class CanvasRenderingContext2D : public Object {
   public:
    void fillRect(float x, float y, float w, float h);
  };
```

#### Optional argument ####

If the "optional" keyword appears on an operation argument, it indicates that the operation can be invoked by passing values only for the those arguments appearing before the optional argument in the operation’s argument list.

#### IDL ####

```
  interface ColorCreator {
    object createColor(float v1, optional float v2, float v3, optional float alpha);
  };
```

#### C++ ####

```
  class ColorCreator {
  public:
    Object createColor(float v1);
    Object createColor(float v1, float v2, float v3);
    Object createColor(float v1, float v2, float v3, float alpha);
  };  
```

#### Variadic operation ####

If the final argument uses the "..." terminal, it indicates that the operation is variadic, and can be passed zero or more arguments after the regular arguments. In C++,  a varying number of arguments are represent as a Variadic value.

Example

#### IDL ####

```
  interface IntegerSet {
    readonly attribute unsigned long cardinality;

    void union(long... ints);
    void intersection(long... ints);
  };
```

#### C++ ####

```
  class IntegerSet {
  public:
    unsigned int getCardinality();
    void union(Variadic<int> ints = Variadic<int>());
    void intersection(Variadic<int> ints = Variadic<int>());
  };
```

```
  template <typename T>
  class Variadic
  {
public:
    Variadic();
    Variadic(const Any* variadic, size_t length);
    Variadic(std::initializer_list<T> list);
    T operator[](int index) const;
    size_t size() const;
  };
```

### Operations with no identifier ###

While it has been agreed to remove unnamed getters/setters at W3C WebApps WG (`*`), operations with no identifier are still in use in several W3C specifications. In C++, the default operation name as shown in the following table is used for operations with no identifier.


| IDL | C++ |
|:----|:----|
| getter | getElement |
| setter | setElement |
| creator | createElement |
| deleter | deleteElement |
| stringifier | toString |

Example

#### IDL ####

```
  interface CanvasPixelArray {
    readonly attribute unsigned long length;
    getter octet (in unsigned long index);
    setter void (in unsigned long index, in octet value);
  };
```

#### C++ ####

```
  class CanvasPixelArray : public Object {
   public:
    // CanvasPixelArray
    unsigned int getLength();
    unsigned char getElement(unsigned int index);
    void setElement(unsigned int index, unsigned char value);
    // snip
  };
```

(`*`) http://www.w3.org/2009/11/02-webapps-minutes.html#action07

### Attributes ###

For each attribute defined on the IDL interface, a getter method is declared. For each attribute defined on the IDL interface that is not read only, a corresponding setter method is also declared.

#### IDL ####

```
  interface CanvasRenderingContext2D {
    attribute float globalAlpha;
  };
```

#### C++ ####

```
  class CanvasRenderingContext2D : public Object {
  public:
    float getGlobalAlpha();
    void setGlobalAlpha(float globalAlpha);
  };
```

Note: A getter is prefixed with "get", and the setter is prefixed with "set" following the Web IDL Java binding.

### Exceptions ###

An exception is used to declare a type of exception that can be thrown by implementation.

#### IDL ####

```
  exception DOMException {
    const unsigned short INDEX_SIZE_ERR = 1;
    const unsigned short DOMSTRING_SIZE_ERR = 2;
    unsigned short code;
  };
```

#### C++ ####

```
  struct DOMException {
    const unsigned short INDEX_SIZE_ERR = 1;
    const unsigned short DOMSTRING_SIZE_ERR = 2;
    unsigned short code;
  };
```

Note: The use of exceptions can be turned off with esidl.

### Implements statements ###

An implements statement declares the all objects implementing an interface A (the first name) MUST also implement interface B (the second name).
In C++, interface members in the mixin interface B are mixed into the primary interface A.

Example

#### IDL ####

```
  interface ElementTraversal {
    readonly attribute Element        firstElementChild;
    readonly attribute Element        lastElementChild;
    readonly attribute Element        previousElementSibling;
    readonly attribute Element        nextElementSibling;
  };

  Element implements ElementTraversal;

  interface Element : Node {
    // snip
  };
```

#### C++ ####

```
  class Element : public Node
  {
   public:
    // snip
    Element getFirstElementChild();
    Element getLastElementChild();
    Element getPreviousElementSibling();
    Element getNextElementSibling();
    // snip
```

> cf. http://lists.w3.org/Archives/Public/public-script-coord/2010OctDec/0071.html

## Extended attributes ##

### `[`Constructor`]`, `[`NamedConstructor`]` ###

If the `[Constructor]` extended attribute appears on an interface, the corresponding C++ constructor(s) are defined.

#### IDL ####

```
  [Constructor,
   Constructor(HTMLFormElement form)]
  interface FormData {
    void append(DOMString name, Blob value);
    void append(DOMString name, DOMString value);
  };
```

#### C++ ####

```
  class FormData : public Object
  {
   public:
    // FormData
    void append(std::u16string name, file::Blob value);
    void append(std::u16string name, std::u16string value);

    // [Constructor]
    FormData();
    FormData(html::HTMLFormElement form);
    
    // snip
  };
```

Note: Unlike ECMAScript, you do not have to use the "new" operator to create an instance of the interface with  the `[Constructor]` extended attribute.

### `[`PutForwards`]` ###

If the `[`PutForwards`]` extended attribute appears on a read only attribute declaration whose type is an object implementing an interface, it indicates that assigning to the attribute will have specific behavior. Namely, the assignment is “forwarded” to the attribute (specified by the extended attribute argument) on the object that is currently referenced by the attribute being assigned to.

#### IDL ####

```
  interface Name {
    attribute DOMString full;
    attribute DOMString family;
    attribute DOMString given;
  };

  interface Person {
    [PutForwards=full] readonly attribute Name name;
  };
```

#### C++ ####

```
  class Person {
  public:
    Name* getName();
    void setName(std::string name);
    // snip
  };
```

### `[`Prefix`]` ###

See Modules.

### `[`Supplemental`]` ###

cf. http://lists.w3.org/Archives/Public/public-webapps/2009JulSep/0528.html

Example

#### IDL ####

```
  interface Element : Node {
    readonly attribute DOMString tagName;
    // snip
  };

  [Supplemental] interface Element {
    ClientRectList getClientRects();
    ClientRect getBoundingClientRect();
  };
```

#### C++ ####

```
  class Element : public Node {
   public:
    std::string getTagName();
    // snip
    ClientRectList getClientRects();
    ClientRect getBoundingClientRect();
  };
```