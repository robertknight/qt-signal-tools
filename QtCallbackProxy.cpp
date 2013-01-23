#include "QtCallbackProxy.h"

#include <QtCore/QDebug>

int qtObjectSignalIndex(const QObject* object, const char* signal)
{
	QByteArray normalizedSignature = QMetaObject::normalizedSignature(signal + 1);
	const QMetaObject* metaObject = object->metaObject();
	return metaObject->indexOfMethod(normalizedSignature.constData());
}

QtCallbackProxy::QtCallbackProxy(QObject* parent)
	: QObject(parent)
{
}

void QtCallbackProxy::bind(QObject* sender, const char* signal, const QtCallbackBase& callback)
{
	int signalIndex = qtObjectSignalIndex(sender, signal);
	if (signalIndex < 0)
	{
		qWarning() << "No such signal" << signal << "for" << sender;
		return;
	}

	Binding binding(sender, signalIndex, callback);
	binding.paramTypes = sender->metaObject()->method(signalIndex).parameterTypes();

	int memberOffset = QObject::staticMetaObject.methodCount();

	if (!QMetaObject::connect(sender, signalIndex, this, memberOffset, Qt::AutoConnection, 0))
	{
		qWarning() << "Unable to connect signal" << signal << "for" << sender;
		return;
	}

	m_bindings << binding;
}

QtCallbackProxy* installCallbackProxy(QObject* sender)
{
	const char* callbackProperty = "qt_callback_handler";
	QtCallbackProxy* callbackProxy = sender->property(callbackProperty).value<QtCallbackProxy*>();
	if (!callbackProxy)
	{
		callbackProxy = new QtCallbackProxy(sender);
		sender->setProperty(callbackProperty, QVariant::fromValue(callbackProxy));
	}
	return callbackProxy;
}

void QtCallbackProxy::connectCallback(QObject* sender, const char* signal, const QtCallbackBase& callback)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	proxy->bind(sender, signal, callback);
}

const QtCallbackProxy::Binding* QtCallbackProxy::matchBinding(QObject* sender, int signalIndex) const
{
	for (int i = 0; i < m_bindings.count(); i++)
	{
		const Binding& binding = m_bindings.at(i);
		if (binding.sender == sender && binding.signalIndex == signalIndex)
		{
			return &binding;
		}
	}
	return 0;
}

void QtCallbackProxy::failInvoke(const QString& error)
{
	qWarning() << "Failed to invoke callback" << error;
}

int QtCallbackProxy::qt_metacall(QMetaObject::Call call, int methodId, void** arguments)
{
	QObject* sender = this->sender();
	int signalIndex = this->senderSignalIndex();

	if (!sender)
	{
		failInvoke("Unable to determine sender");
	}
	else if (signalIndex < 0)
	{
		failInvoke("Unable to determine sender signal");
	}

	methodId = QObject::qt_metacall(call, methodId, arguments);
	if (methodId < 0)
	{
		return methodId;
	}

	if (call == QMetaObject::InvokeMetaMethod)
	{
		if (methodId == 0)
		{
			const Binding* binding = matchBinding(sender, signalIndex);
			if (binding)
			{
				QGenericArgument args[6] = {
					QGenericArgument(binding->paramType(0), arguments[0]),
					QGenericArgument(binding->paramType(1), arguments[1]),
					QGenericArgument(binding->paramType(2), arguments[2]),
					QGenericArgument(binding->paramType(3), arguments[3]),
					QGenericArgument(binding->paramType(4), arguments[4]),
					QGenericArgument(binding->paramType(5), arguments[5])
				};
				binding->callback.invokeWithArgs(args[0], args[1], args[2], args[3], args[4], args[5]);
			}
			else
			{
				const char* signalName = sender->metaObject()->method(signalIndex).signature();
				failInvoke(QString("Unable to find matching binding for signal %1").arg(signalName));
			}
		}
		--methodId;
	}
	return methodId;
}

