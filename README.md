# Qt Signal Tools

qt-signal-tools is a collection of utility classes related to signal and slots in Qt.

## Requirements

 * Qt 4.8*
 * For connecting signals and events to arbitrary functions, a function wrapper
is needed, such as `std::tr1::function` (C++03 compilers with a TR1 library), `boost::function` or
`std::function` (C++11 std library).

(* Could be adapted for earlier versions of Qt 4 if necessary)

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

### QtCallbackProxy

QtCallbackProxy provides a way to invoke callbacks when an object emits a signal or receives
a particular type of event.  The callbacks can be signals and slots
(via `QtCallback`) or arbitrary functions using `tr1::function`, `std::function`, `boost::function` or
a similar wrapper.

Qt 5 provides support for connecting signals to arbitrary functions out of the box and to lambdas
when using C++11.  QtCallbackProxy emulates this for Qt 4.

As well as being able to connect signals to functions that are not slots, this also provides
a way to pass additional arguments to the receiver other than those from the signal using `QtCallback::bind()`
or `std::tr1::bind()`.

Usage:
```cpp
MyObject receiver;
QPushButton button;
QtCallbackProxy::connectCallback(&button, SIGNAL(clicked(bool)), callback,
  QtCallback(&receiver, SLOT(buttonClicked(int))).bind(42));

// invokes MyObject::buttonClicked() slot with arguments (42)
button.click();
```

## License

qt-signal-tools is licensed under the BSD license.

## Related Projects

Qt Signal Adapters - Library for connecting signals to Boost function objects: http://sourceforge.net/projects/qtsignaladapter/
