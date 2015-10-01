# ES Coding Style #

**Be consistent.** In ES operating system, please follow the coding style used in the book [The Practice of Programming](http://books.google.com/books?id=to6M9_dbjosC&printsec=frontcover&dq=The+Practice+of+Programming&ei=9lYTSKKaCIziswOy8pSUCA&sig=XXXeqMTXt_K2fRjutfD2hpXaBLk), with several adjustments described below:

Note: If it doesn't conflict with the ES coding style described here, please also try to follow the style used in [WebKit](http://webkit.org/coding/coding-style.html).

## File Names ##

  * C++ files should end in `.cpp` and header files should end in `.h`.
  * C files should end in `.c` and header files should end in `.h`.
  * ECMAScript files should end in `.js`.
  * IDL files should end in `.idl`.

## File Comments ##

Start each file with the following copyright notice. ES operating system is distributed under the [Apache License 2.0](http://www.apache.org/licenses/):

```
   Copyright [2008] [Google Inc.]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
```

## Interface Comments ##

Every interface definition in .idl and .h should be commented in [Javadoc](http://java.sun.com/j2se/javadoc/) format.

## Space vs. Tabs ##

Use spaces, not tabs. Indent 4 spaces at a time.

**Note**: Indent 8 spaces in `.S` (assembly language), `.ac` (autoconf), and `.am` (automake) files using tabs.

## Names ##

  * Use CamelCase:
    * Capitalize the first letter of a class or struct name;
    * Lower-case the first letter of a struct or class member;
    * Fully capitalize acronyms.
  * Use very short names for local variables in conventional ways.
  * Do not prefix or postfix class members with `"m_"` or `"_"`.

```
    struct Queue;
    size_t numPending;
    class HTMLDocument;
```

```
?   struct queue;
?   size_t num_pending;
?   class HtmlDocument;
```

**Note**: The following Point constructor initializes `Point::x` and `Point::y` with the value of constructor parameter `x` and `y`, respectively:

```
    class Point
    {
        int x;
        int y;
    public:
        Point(int x, int y) :
            x(x), y(y)
        {
        }
```

## Curly Braces ##

Place each curly brace on its own line.

```
   if (cond)
   {
       foo = bar;
   }
```

```
?  if (cond) {
       foo = bar;
   }
```

Curly braces are required around the one-line blocks.

```
?  if (cond) foo = bar;
```

```
?  if (cond)
?      foo = bar;
```

**Note**: Place each brace for the `do-while` statement on the same line as the `do` and `while`.

```
   do {
       x += y;
   } while (cond);
```

## Comparison Operators ##

Prefer less than (`<`) and less than or equal to (`<=`) to greater than (`>`) and greater than or equal to (`>=`).

```
   if (x < y)
```

```
?  if (y > x)
```

## Pointer and Reference Types in C++ ##

Both pointer types and reference types should be written with no space between the type name and the `*` or `&`; do not define more than one pointer or reference variable in a single line.

```
   char* p;
   const string& s;
```

```
?  char *p;
?  const string &s;
?  char* p, *q;
```

## Horizontal Whitespace ##
Do not leave trailing white space at the ends of lines.

## Third-Party Code ##

When modifying code not originated from ES operating system, follow the conventions within that code for consistency.