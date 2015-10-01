# Introduction #

If you turn on the PC installed with the ES operating system, the following screen appears on your SVGA monitor.

```
esjs file/shell.js
alloc count: 2400
yypase() ok.
% 
```

Once the ES operating system kernel gets executing, it will start the event manger (eventManager.elf) and the canvas service (console.elf) in userland. The event manager is a user process that records the inputs from the keyboard and from the mouse as events so that the other processes can get the events. The canvas service repeatedly reads the events from the event manager converting the keyboard input events into a stream of characters; The canvas service also accepts the requests from the yet other processes to draw graphics and texts in the screen.

![http://es-operating-system.googlecode.com/svn/html/blockDiagram.png](http://es-operating-system.googlecode.com/svn/html/blockDiagram.png)

The kernel then starts the ECMAScript interpreter (esjs.elf), which execute the command line shell program written in ECMAScript (shell.js). The command line shell provides the interactive environment to the user using the canvas service.

# Using commands #

From the command line shell, you can use the programs like below. Most of the commands are written in ECMAScript:

| Command Name | Description |
|:-------------|:------------|
| `cat [file ...]` | Prints the contents of files to the screen |
| `cd [dir]`   | Sets the current directory to `dir` |
| `clear`      | Clears the canvas for drawing graphics |
| `date [-u]`  | Displays the current date and time. |
| `echo [arg ...]` | Prints the arguments to the screen |
| `edit [file]`  | Edits text files |
| `exit`       | Exits the command line shell |
| `figure`     | Draws chars using the CanvasRenderingContext2D interface (A demo script) |
| `ls [dir ...]` | Lists names of objects in the directory |
| `rm [file ...]` | Removes the files |
| `squeak`     | Squeak - a Smalltalk programming environment |

## [Namespace](Namespace.md) ##

You can list the objects in the current directory by executing the `ls` command without specifying any arguments:

```
% ls
device
network
class
interface
file 
% 
```

You can change the current directory to any place in the namespace using the `cd` command. Initially, the current directory is set to the root of the namespace. In the following example, names of device objects in the `device` directory are shown:

```
% cd device
% ls
rtc
cga
beep
framebuffer
cursor
keyboard
mouse
ata
floppy
loopback
soundInput
soundOutput
ethernet
event
console
%
```

The `event` and `console` device objects are dynamically registered objects by the event manager and the canvas service, respectively. In the ES operating system, you can dynamically extend the system features that can be used from the other programs by executing userland programs like this way.

To set the current directory back to the root, invoke the `cd` comand without any arguments:

```
% cd
% ls
device
network
class
interface
file 
% 
```

Your local disk drive is mounted in the `file` object. To display the names of files in your disk, invoke the `ls` command with an argument `file`:

```
% ls file
es.ldr
es.img
eventManager.elf
console.elf
esjs.elf
shell.js
cat.js
cd.js
date.js
echo.js
edit.js
ls.js
% 
```

You can draw graphics on the screen: type `figure` in the command line and hit enter:

![http://es-operating-system.googlecode.com/svn/html/shell.png](http://es-operating-system.googlecode.com/svn/html/shell.png)

`figure` is another ECMAScript program that invokes the canvas service. You might want to read the program by typing

```
% cat file/figure.js
```

The APIs used in `figure.js` are basically same as the ones defined in the `CanvasRenderingContext2D` interface in the HTML5 specification.

To shut down the computer, just invoke the `exit` command. It will automatically turn off your computer.

```
% exit
```

# Programming in the ES operating system #

To extend the operating system features, you still need to write the programs like the event manger or the canvas service in C or in C++. However, you can easily write your program using those services in ECMAScript. For example, let's take a look into the `date` program:

```
% cat file/date.js
var date = new Date();
var str = date.toString();
stdout.write(str + '\n', str.length + 1);
%
```

It's just three lines of code. In this script, you see a non-standard ECMAScript object is used: `stdout`. In ECMAScript programs invoked from the command line shell - `shell.js` - you can use the following objects:

| Object | Interface | Description |
|:-------|:----------|:------------|
| `System` | ICurrentProcess | The current process |
| `stdin` | IStream   | The standard input |
| `stdout` | IStream   | The standard output |
| `stderr` | IStream   | The standard error output |
| `root` | IContext  | The root namespace |
| `cwd`  | IContext  | The current directory |
| `classStore` | IClassStore | The class store |
| `path` | Array     | Array of pathnames in which programs are looked up by the shell |

The ECMAScript interpreter `esjs` is implemented in a way that it can invoke any programming interfaces defined in the standard interface definition language (IDL). For example, you can invoke any method provided by the ICurrentProcess interface using the `System` object.

To obtain a different interface of an object, just write as below:

```
unknown = iter.next();
file = IFile(unknown);        // get IFile interface for unknown.
```

If you want to edit a tiny script program, you may use the `edit` command:

```
% edit file/date.js
3
1,$p
var date = new Date();
var str = date.toString();
stdout.write(str + '\n', str.length + 1);
q
%
```

The `edit` program is originally introduced by Kernighan and Plauger `[`1, 2`]`. It was written in Ratfor and then in Pascal at that time. In ES, `edit` is another ECMAScript script. ECMAScript is actually a powerful language with the support for exception, regular expression, and more. In the ES operating system, you can easily write a command line shell program or a text editor in ECMAScript; those programs are usually written in C or in C++ in the other operating systems.

# Running ES software components as Linux applications #

You can run and test ES software components as Linux applications without executing QEMU.

![http://es-operating-system.googlecode.com/svn/html/aslinuxapps.png](http://es-operating-system.googlecode.com/svn/html/aslinuxapps.png)

To do this with your computer, change the current directory to
`~es/local/os/testsuite`, and copy `~/es/trunk/esjs/scripts/*.js` and
`~/es/local/esjs/esjs` to there (probably you want to use `ln -s` rather
than `cp`). Then run the following command:

```
$ ./canvas file/esjs shell.js
```

In this configuration, canvas and esjs are running in separate processes.
Please try to execute other commands like figure there.
It runs much faster than on QEMU; it is like you run ES on an actual PC.

**Note**: Currently this configuration is supported only on x86 linux.

# Running the ES networking stack as a Linux application #

The ES networking stack may be tested as a regular application by binding to the appropriate network interface. To do so, you must first register the desired network interface in trunk/os/kernel/posix/posix\_init.cpp:

```
try
    {
        Tap* tap = new Tap("eth1");
        device->bind("ethernet",
        static_cast<es::Stream*>(tap));
}

```

The default network interface is eth1. You may change this to any interface, including wireless interfaces and virtual interfaces (eth0:1). Using a wireless interface may lead to erratic results: you should make sure you are correctly associated to your access point but that your host operating system is not using the interface. Any changes to posix\_init.cpp should not be committed.

Once compiled, you may run the applications in local/os/net/testsuite. The respective source code is contained in trunk/os/net/testsuite. To test whether your interface is correctly bound you may run the 'dhcp' application, which simply requests an IP address through the DHCP protocol, resolves 'www.nintendo.com', and attempts to ping the resulting address three times. Using a protocol analyzer such as Wireshark may be useful to view the output of the ES stack on the wire.

Some applications have static host, router and nameserver addresses. Make sure you alter them to reflect your network configuration and re-compile before attempting to run the applications.


## References ##

`[`1`]` B. W. Kernighan and P. J. Plauger, Software Tools, Addison-Wesley, 1976.

`[`2`]` B. W. Kernighan and P. J. Plauger, Software Tools in Pascal, Addison-Wesley, 1981.