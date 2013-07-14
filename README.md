# Qt Signal Tools

qt-signal-tools is a collection of utility classes related to signal and slots in Qt.  It includes:
 * QtCallback - Package up a receiver and slot arguments into an object for invoking later.
 * QtSignalForwarder - Connect signals and events from objects to QtCallback or arbitrary functions.
 * QtMetacallAdapter - Low-level interface for calling a function using a list of QGenericArgument() arguments.
 * safe_bind() - Create a wrapper around a method call which does nothing and returns a default value if
  the object is destroyed before the wrapper is called.

## Requirements

 * Qt 4.6 (earliest version tested, may also work with older Qt 4.x releases) or Qt 5.x
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

### safe_bind()

Compared to using Qt 4's normal signals and slots, a disadvantage of using `bind()` or `function` to
create a callback object which can be run later is that there is no automatically disconnection
if the object is destroyed.

As a solution, the `safe_bind()` function creates a wrapper around an object and a method call.  The
wrapper can then be called with the same arguments as the wrapped method.  When a call happens,
either the wrapped method is called with the provided arguments, or if the object has been destroyed,
nothing happens and a default value is returned.

The wrapper created by `safe_bind()` can be used with `bind()` and `function` and can be used together
with `QtSignalForwarder` to automatically 'disconnect' if the receiver is destroyed.

```cpp
QScopedPointer<QLabel> label(new QLabel);

// create a wrapper around label->setText() which can be run using
// setTextWrapper(text).
function<void(QString)> setTextWrapper = safe_bind(label.data(), &QLabel::setText);

// create a wrapper around label->text() which either calls label->text() and returns
// the same result or returns an empty string if the label has been destroyed
function<QString()> getTextWrapper = safe_bind(label.data(), &QLabel::text);

setTextWrapper("first update"); // sets the label's text to "first update"
qDebug() << "label text" << getTextWrapper(); // prints "first update"
label.reset(); // destroy the label
setTextWrapper("second update"); // does nothing, as the label has been destroyed
qDebug() << "label text" << getTextWrapper(); // prints an empty string
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
