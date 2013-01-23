# Qt Signal Tools

qt-signal-tools is a collection of utility classes related to signal and slots in Qt.

## Requirements

Requires Qt 4.8

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

QtCallbackProxy provides a way to invoke QtCallback objects when an object emits a signal.
This provides a way to pass additional arguments than those provided by the signal to a slot
when the signal is emitted.

In Qt 5 with C++11 support this is made much easier by the ability to connect signals to lambdas
and functions (or via Qt 5 with tr1::bind and tr1::function);

QtCallbackProxy emulates this for Qt 4.

Usage:
```cpp
MyObject receiver;
QPushButton button;
QtCallbackProxy::connectCallback(&button, SIGNAL(clicked(bool)), callback,
  QtCallback(&receiver, SLOT(buttonClicked(int))).bind(42));

// invokes MyObject::buttonClicked() slot with arguments (42)
button.click();
```

