# Creating event managers and console services #

Here are the steps to create the event manager and console as a more concrete example of [creating software components](ComponentHowto.md).

The event manager and console service register interface pointers provided by services in the system namespace meaning input from the keyboard and mouse can be displayed as text on a SVGA monitor through other processes that retrieve that interface pointer from the namespace.

## The event manager ##

The event manager is a component which controls input from the keyboard and mouse. The executable file (eventManager.elf) built by [eventManager.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/cmd/eventManager.cpp) is the event manager.

In versions of the ES operating system before 0.0.11, which did not have an event manager, low-level input from the keyboard and mouse had to be dealt with by applications. For example, Squeak directly monitors low-level input from the keyboard and mouse in a separate thread. However, it's not good to have all applications individually monitoring input and several processes fighting for that input, so we decided to encapsulate it into an event manager component that controls keyboard and mouse input which can then be used by other applications.

We made the event manager by re-using the implementation for Squeak that monitored keyboard and mouse input. In [previous versions of Squeak](http://code.google.com/p/es-operating-system/source/browse/trunk/cmd/squeak-3.7.1/es/vm/sqEsInput.cpp), keyboard and mouse input was governed by an EventQueue class. To make this into a component, we re-defined an [EventQueue interface](http://code.google.com/p/es-operating-system/source/browse/trunk/cmd/IEventQueue.idl) and in the [EventManager](http://code.google.com/p/es-operating-system/source/browse/trunk/cmd/eventManager.cpp) class made from a cut-down EventQueue, we implemented the [EventQueue interface](http://code.google.com/p/es-operating-system/source/browse/trunk/cmd/IEventQueue.idl). Then we enabled other processes to use the event manager by registering the EventQueue interface provided by the event manager in the namespace. Using the namespace is explained in the next section.

The event manager, after registering the interface, uses the EventManager class to start monitoring input from the keyboard and mouse.

### Registering in the namespace ###

Here we'll explain how to register component interface pointers in the namespace.

In the ES operating system, you can register a name and interface pointer together in the namespace. (For further details, please refer to [The ES operating system namespace](Namespace.md). )

Registering in the namespace is easy. Firstly, call System()->getRoot() to get the Context interface pointer to the root namespace. You can give a name to an interface and register it in the namespace by using the bind method.

In the following example, we name the Stream interface pointer as `console` and register it in the namespace's /device/console.

```
Handle<Stream> stream = ... // Preparing the Stream interface pointer.
Handle<Context> nameSpace = System()->getRoot(); // Getting the root namespace.
Handle<Context> device = nameSpace->lookup("device"); // Searching for the existing namespace /device.
device->bind("console", stream); // Registering stream below /device with the name console.
```

### Registering the interface ###

The Stream interface used in the above example is an interface initialized by the kernel. Meanwhile, the EventQueue interface is individually defined by the event manager. Because of this, registration in the kernel has to be done before registration in the namespace.

The registration steps are as follows: (For further details, please refer to "Interface Extension" in [Creating software components](ComponentHowto.md))

```
Handle<Context> nameSpace = System()->getRoot();
// Register EventQueue interface.
Handle<InterfaceStore> interfaceStore = nameSpace->lookup("interface");
interfaceStore->add(EventQueueInfo, EventQueueInfoSize);
```

Preparation is now complete. The EventQueue interface is registered in the "/device/event" namespace as below:

```
// Create Event manager object.
Handle<EventQueue> eventQueue = new EventManager;
// register the event queue.
Handle<Context> device = nameSpace->lookup("device");
device->bind("event", static_cast<EventQueue*>(eventQueue));
```

The event manager component initially performs the following two types of registration:

  * EventQueue interface registration in the kernel.
  * EventQueue interface pointer registration in the "/device/event" namespace.

### Using the component ###

Before using the event manager, we have to run the event manager component (eventManager.elf) as a new process.

```
Handle<Process> eventProcess;
eventProcess = reinterpret_cast<Process*>(
esCreateInstance(CLSID_Process, Process::iid())); // Creating the process
Handle<File> eventElf = nameSpace->lookup("file/eventManager.elf"); // Reading the component
eventProcess->setRoot(nameSpace); // Setting the root of the new process's namespace
eventProcess->setIn(esReportStream()); // Setting standard input
eventProcess->setOut(esReportStream()); // Setting standard output
eventProcess->setError(esReportStream()); // Setting standard error output
eventProcess->start(file); // Starting the event manager
```

After this, EventQueue can be obtained and used from the namespace.

```
Handle<Context> nameSpace = System()->getRoot();
Handle<EventQueue> eventQueue = nameSpace->lookup("device/event"); // Getting EventQueue
int x;
int y;
eventQueue->getMousePoint(x, y); // Getting the mouse coordinates
int stroke;
eventQueue->getKeystroke(&stroke); // Getting keyboard input
```

## The console service ##

As the second example of a software component, we'll explain the console service. The console has the following features:

  * Displaying text to the screen (Stream's write method)
  * Saving inputted keystrokes from the event manager to the buffer
  * Getting inputted keystrokes saved in the buffer (Stream's read method)

Console services use [FreeType](http://www.freetype.org/) to render fonts to an SVGA monitor.

When [console.elf](http://code.google.com/p/es-operating-system/source/browse/trunk/cmd/console.cpp) is started as a process, the Stream interface pointer that provides the console features is registered in the "/device/console" namespace. We get the console's interface pointer as follows:

```
Handle<Stream> console = nameSpace->lookup("device/console"); // Getting the console
```

Also, because the console provides the Stream interface, we can set the standard input and output of new processes to the console.

```
Handle<Process> process;
process = reinterpret_cast<Process*>(
esCreateInstance(CLSID_Process, Process::iid()));
Handle<File>file = nameSpace->lookup("file/esjs.elf"); // Reading an arbitrary component
process->setRoot(nameSpace);
process->setIn(console); // Setting standard input to the console
process->setOut(console); // Setting standard output to the console
process->setError(console); // Setting standard error output to the console
process->start(file);
```

esjs, the ECMAScript interpreter which runs the ES operating system's command shell, sets the standard input and output and starts the console as above in [es.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/init/es.cpp).