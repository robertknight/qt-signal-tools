# Qt Signal Tools

qt-signal-tools is a collection of utility classes related to signal and slots in Qt.

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
