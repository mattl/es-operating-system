# Web IDL - C++ binding #

## _14 May 2010_ ##

This document describes how interfaces described with Web IDL correspond to constructs within C++ by esidl Web IDL compiler ([r1672](https://code.google.com/p/es-operating-system/source/detail?r=1672) or later).

An application of this C++ binding for writing Web applications in C++ over [NPAPI/Pepper](https://wiki.mozilla.org/NPAPI:Pepper) is described [here](CplusplusDOM.md).

Note: This document is based on the [Web IDL W3C Editor’s Draft 30 September 2009](http://dev.w3.org/2006/webapi/WebIDL/).

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

Note: Historically each W3C specification introduced its own module name. However, the recent specifications tend to omit the module specification in the IDL definition. It seems one module name per spec doesn't make sense any more.

> cf. http://lists.w3.org/Archives/Public/public-webapps/2009AprJun/1380.html

### Primitive types ###

| IDL | C++ |
|:----|:----|
| boolean | bool |
| byte (`*`) | signed char |
| octet | unsigned char |
| short | short |
| unsigned short | unsigned short |
| long | int |
| unsigned long | unsigned int |
| long long | long long |
| unsigned long long | unsigned long long |
| float | float |
| double | double |

(`*`) The byte type is an extension in esidl, and it has been proposed for addition to the Web IDL specification for the use in Web GL.

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
    Any();  // null

    template <typename T>
    Any(T value);

    Any(const Any& value);
    operator bool() const;
    operator signed char() const;
    operator unsigned char() const;
    operator short() const;
    operator unsigned short() const;
    operator int() const;
    operator unsigned int() const;
    operator long long() const;
    operator unsigned long long() const;
    operator float() const;
    operator double() const;
    operator const std::string() const;
    operator Object*() const;

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
    virtual Any getFillStyle() = 0;
    virtual void setFillStyle(Any fillStyle) = 0;
  };

  canvasRenderingContext2D->setFillStyle("black");
```

### DOMString ###

The DOMString type corresponds to the set of all possible sequences of 16 bit unsigned integer code units to be interpreted as UTF-16 encoded strings.

| IDL | C++ |
|:----|:----|
| DOMString | std::string |

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
  class CanvasRenderingContext2D : public virtual Object {
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
  virtual Nullable<std::string> getBackground() = 0;
  virtual void setBackground(Nullable<std::string> background) = 0;
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
#ifdef __GXX_EXPERIMENTAL_CXX0X__
    Sequence(std::initializer_list<T> list);
#endif
    Sequence(const Sequence& value);
    ~Sequence();
    T operator[](int index) const;
    Ref operator[](int index);
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

  virtual StyleSheetList getStyleSheets() = 0;
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

Each operation defined in the IDL interface will be mapped to one or more virtual member functions in the C++ interface class.

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
    virtual void fillRect(float x, float y, float w, float h) = 0;
  };
```

#### Optional argument ####

If the "optional" keyword appears on an operation argument, it indicates that the operation can be invoked by passing values only for the those arguments appearing before the optional argument in the operation’s argument list.

#### IDL ####

```
  interface ColorCreator {
    Object createColor(float v1, optional float v2, float v3, optional float alpha);
  };
```

#### C++ ####

```
  class ColorCreator {
  public:
    virtual Object* createColor(float v1) = 0;
    virtual Object* createColor(float v1, float v2, float v3) = 0;
    virtual Object* createColor(float v1, float v2, float v3, float alpha) = 0;
  };  
```

#### Variadic operation ####

If the final argument uses the "..." terminal, it indicates that the operation is variadic, and can be passed zero or more arguments after the regular arguments. In C++,  a varying number of arguments are represent as a Sequence value.

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
    virtual unsigned int getCardinality() = 0;
    virtual void union(Sequence<int> ints = Sequence<int>()) = 0;
    virtual void intersection(Sequence<int> ints = Sequence<int>()) = 0;
  };
```

Note: In C++0x, the use of `std::initializer_list<T>` would be a better choice.

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
  class CanvasPixelArray : public virtual Object {
   public:
    // CanvasPixelArray
    virtual unsigned int getLength() = 0;
    virtual unsigned char getElement(unsigned int index) = 0;
    virtual void setElement(unsigned int index, unsigned char value) = 0;
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
    virtual float getGlobalAlpha() = 0;
    virtual void setGlobalAlpha(float globalAlpha) = 0;
  };
```

Note: A getter is prefixed with "get", and the setter is prefixed with "set" followoing the Web IDL Java binding.

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

An implements statement declares the all objects implementing an interface A (the first name) MUST also implement interface B (the second name). In C++, a mixin class is defined for an interface that has extra interfaces needed be implemented. The class name of a mixin class is the concatenation of the type name of the primary interface and the string "`_Mixin`".

Example

#### IDL ####

```
  interface Node {
    // snip
  };

  Node implements EventTarget;

  interface Element : Node {
    // snip
  };
```

#### C++ ####

```
  class Element : public Node {
    // snip
  };

  class Element_Mixin : public Element, public events::EventTarget {
    // snip
  };
```

Note: There is a proposal from Microsoft to mix mixins into the primary interface:
> cf. http://lists.w3.org/Archives/Public/public-webapps/2009JulSep/0903.html
This seems to be a more realistic approach for the browser implementation given that ECMAScript does not support the multiple inheritance. If this proposal is accepted, interface members in B are mixed into the interface A. Hixie also invented `[`Supplemental`]` extended attribute that effectively enforces the proposed behavior without changing the current 'implements' statement behavior, and esidl supports `[`Supplemental`]` extended attribute since it is now widely used in W3C/WHATWG specifications.
> cf. http://lists.w3.org/Archives/Public/public-webapps/2009JulSep/0528.html

## Extended attributes ##

### `[`Constructor`]`, `[`NamedConstructor`]` ###

If the `[Constructor]` extended attribute appears on an interface, a C++ constructor class is defined.  The class name of a constructor class is the concatenation of the type name of the base interface and the string "`_Constructor`".

#### IDL ####

```
  [NamedConstructor=Image(),
   NamedConstructor=Image(unsigned long width),
   NamedConstructor=Image(unsigned long width, unsigned long height)]
  interface HTMLImageElement : HTMLElement {
    // snip
  };
```

#### C++ ####

```
  class HTMLImageElement_Constructor : public Object {
   public:
    virtual HTMLImageElement* createInstance() = 0;
    virtual HTMLImageElement* createInstance(unsigned long width) = 0;
    virtual HTMLImageElement* createInstance(unsigned long width, unsigned long height) = 0;
  };
```

Note: If the `[`NamedConstructor`]` extended attribute appears on an interface, it indicates the ECMAScript global object will have a property with the specified name whose value is a constructor function that can create objects that implement the interface.

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
    virtual Name* getName() = 0;
    virtual void setName(std::string name) = 0;
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
    virtual std::string getTagName() = 0;
    // snip
    virtual ClientRectList* getClientRects() = 0;
    virtual ClientRect* getBoundingClientRect() = 0;
  };
```