# Qt Signal Tools

qt-signal-tools is a collection of utility classes related to signal and slots in Qt.  It includes:
 * QtCallback - Package up a receiver and slot arguments into an object for invoking later.
 * QtSignalForwarder - Connect signals and events from objects to QtCallback or arbitrary functions.

## Requirements

 * Qt 4.8 (could be adapted for earlier Qt versions if necessary)
 * The TR1 standard library (for C++03 compilers) or the C++11 standard library
  (for newer compilers when C++11 support is enabled).

## Classes

### QtCallback

QtCallback is a binder class which provides a way to create callbacks that invoke a signal or slot
when invoked, using a mixture of pre-bound arguments and arguments passed to QtCallback::invoke().

Usage:
```cpp
QtCallback1<int> callback(myWidget, SLOT(someSlot(int,QString)));
callback.bind(42);

// invokes the MyWidget::someSlot() slot with arguments (42, "Hello World")
callback.invoke("Hello World");

void MyWidget::someSlot(int firstArg, const QString& secondArg)
{
}
```

### QtSignalForwarder

QtSignalForwarder provides a way to invoke callbacks when an object emits a signal or receives
a particular type of event.  The callbacks can be signals and slots
(via `QtCallback`) or arbitrary functions using `tr1::function`, `std::function`, `boost::function` or
a similar wrapper.

Qt 5 provides support for connecting signals to arbitrary functions out of the box and to lambdas
when using C++11.  QtSignalForwarder emulates this for Qt 4.

As well as being able to connect signals to functions that are not slots, this also provides
a way to pass additional arguments to the receiver other than those from the signal using `QtCallback::bind()`
or `std::tr1::bind()`.

Usage:

Connecting a signal to a slot with pre-bound arguments:
```cpp
MyObject receiver;
QPushButton button;
QtSignalForwarder::connect(&button, SIGNAL(clicked(bool)),
  QtCallback(&receiver, SLOT(buttonClicked(int))).bind(42));

// invokes MyObject::buttonClicked() slot with arguments (42)
button.click();
```

Connecting a signal to an arbitrary function:
```cpp
using namespace std::tr1;
using namespace std::tr1::placeholders;

SomeObject receiver;
QLineEdit editor;

// function which calls someMethod() with the first-argument fixed (42) and the
// second string argument from the signal
function<void(int,QString)> callback(bind(&SomeObject::someMethod, &receiver, 42, _1));

QtSignalForwarder::connect(&editor, SIGNAL(textChanged(QString)), callback);
  
// invokes SomeObject::someMethod(42, "Hello World")
editor.setText("Hello World");
```

### QtMetacallAdapter

QtMetacallAdapter is a low-level wrapper around a function or function object (eg. `std::function`)
which can be used to invoke the function with a list of QGenericArgument (created by the Q_ARG() macro)
and introspect the function's argument types at runtime.

## License

qt-signal-tools is licensed under the BSD license.

## Related Projects & Reading

 * Qt Signal Adapters - Library for connecting signals to Boost function objects: http://sourceforge.net/projects/qtsignaladapter/
 * sigfwd - Library for connecting signals to function objects.  Uses Boost. https://bitbucket.org/edd/sigfwd/wiki/Home
 * Qt 5 meta-object system changes - http://blog.qt.digia.com/blog/2012/06/22/changes-to-the-meta-object-system-in-qt-5/
