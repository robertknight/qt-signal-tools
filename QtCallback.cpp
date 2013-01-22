#include "QtCallback.h"

#include <QtCore/QDebug>
#include <QtCore/QMetaMethod>
#include <QtCore/QMetaObject>

QtCallbackBase::QtCallbackBase()
	: d(new Data)
{
}

QtCallbackBase::QtCallbackBase(QObject* receiver, const char* method)
	: d(new Data)
{
	Q_ASSERT(receiver);
	Q_ASSERT(method);

	d->receiver = receiver;

	int methodIndex = receiver->metaObject()->indexOfMethod(method+1);
	if (methodIndex >= 0)
	{
		d->method = receiver->metaObject()->method(methodIndex);
	}
	else
	{
		qWarning() << "Receiver" << receiver << "has no such method" << method;
	}
}

int QtCallbackBase::parameterCount() const
{
	return d->method.parameterTypes().count();
}

QtCallbackBase::QtCallbackBase(const QtCallbackBase& other)
	: d(other.d)
{
}

void QtCallbackBase::bind(int index, const QVariant& value)
{
	Q_ASSERT(index < parameterCount());

	for (int i=0; i < d->args.count(); i++)
	{
		if (d->args[i].position == index)
		{
			d->args[i].value = value;
			return;
		}
	}
	Data::Arg newArg;
	newArg.position = index;
	newArg.value = value;
	d->args << newArg;
}

void QtCallbackBase::bind(const QVariant& value)
{
	int minUnusedArg = 0;
	for (int i=0; i < d->args.count(); i++)
	{
		if (d->args[i].position == minUnusedArg)
		{
			++minUnusedArg;
		}
	}
	Q_ASSERT(minUnusedArg < parameterCount());
	bind(minUnusedArg, value);
}

bool QtCallbackBase::invokeWithArgs(const QGenericArgument& a1, const QGenericArgument& a2, const QGenericArgument& a3,
                                    const QGenericArgument& a4, const QGenericArgument& a5, const QGenericArgument& a6)
{
	if (!d->receiver)
	{
		// receiver was destroyed before callback could be invoked
		qWarning() << "Unable to invoke callback.  Receiver was destroyed";
		return false;
	}
	if (d->method.methodIndex() < 0)
	{
		// method was not found on receiver
		qWarning() << "Unable to invoke callback.  Method not found";
		return false;
	}

	int invokeArgIndex = 0;
	QGenericArgument invokeArgs[] = {a1,a2,a3,a4,a5,a6};
	QGenericArgument args[7] = {QGenericArgument()};
	QList<QByteArray> params = d->method.parameterTypes();
	
	int paramCount = parameterCount();
	for (int i = 0; i < paramCount; i++)
	{
		for (int k = 0; k < d->args.count(); k++)
		{
			const Data::Arg& boundArg = d->args.at(k);
			if (boundArg.position == i)
			{
				// in Qt 4, QMetaMethod only provides access to the type name string.
				// in Qt 5 we could compare the type IDs instead
				const char* actualType = QMetaType::typeName(boundArg.value.userType());
				const char* expectedType = params[i].constData();
				if (strcmp(actualType, expectedType))
				{
					qWarning() << "Unable to invoke callback.  Type of bound arg at index" << i << QString(actualType)
					           << "does not match expected type" << QString(expectedType);
					return false;
				}

				args[i] = QGenericArgument(actualType, boundArg.value.constData());
				break;
			}
		}
		if (!args[i].data())
		{
			args[i] = invokeArgs[invokeArgIndex];
			if (!args[i].data())
			{
				// not enough arguments supplied
				qWarning() << "Unable to invoke callback.  Argument" << i << "was not bound";
				return false;
			}
			++invokeArgIndex;
		}
	}

	if (!d->method.invoke(d->receiver.data(), args[0], args[1], args[2], args[3], args[4], args[5], args[6]))
	{
		qWarning() << "Failed to invoke method" << d->method.signature();
		return false;
	}
	return true;
}

