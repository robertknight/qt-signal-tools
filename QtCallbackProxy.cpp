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

bool QtCallbackProxy::checkTypeMatch(const QtMetacallAdapter& callback, const QList<QByteArray>& paramTypes)
{
	int argTypes[10] = {0};
	for (int i=0; i < paramTypes.count(); i++) {
		argTypes[i] = QMetaType::type(paramTypes.at(i).data());
	}
	return callback.canInvoke(argTypes, paramTypes.count());
}

void QtCallbackProxy::bind(QObject* sender, const char* signal, const QtMetacallAdapter& callback)
{
	int signalIndex = qtObjectSignalIndex(sender, signal);
	if (signalIndex < 0) {
		qWarning() << "No such signal" << signal << "for" << sender;
		return;
	}

	Binding binding(sender, signalIndex, callback);
	binding.paramTypes = sender->metaObject()->method(signalIndex).parameterTypes();

	if (!checkTypeMatch(callback, binding.paramTypes)) {
		qWarning() << "Sender and receiver types do not match for" << signal+1;
		return;
	}

	int memberOffset = QObject::staticMetaObject.methodCount();

	if (!QMetaObject::connect(sender, signalIndex, this, memberOffset, Qt::AutoConnection, 0)) {
		qWarning() << "Unable to connect signal" << signal << "for" << sender;
		return;
	}

	m_bindings << binding;
}

void QtCallbackProxy::bind(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter)
{
	if (!callback.canInvoke(0,0)) {
		qWarning() << "Callback does not take 0 arguments";
		return;
	}

	sender->installEventFilter(this);

	EventBinding binding(sender, event, callback, filter);
	m_eventBindings << binding;
}

void QtCallbackProxy::unbind(QObject* sender, const char* signal)
{
	int signalIndex = qtObjectSignalIndex(sender, signal);
	QMutableVectorIterator<Binding> iter(m_bindings);
	while (iter.hasNext())
	{
		const Binding& binding = iter.next();
		if (binding.sender == sender &&
		    binding.signalIndex == signalIndex)
		{
			int memberOffset = QObject::staticMetaObject.methodCount();
			QMetaObject::disconnect(sender, signalIndex, this, memberOffset);
			iter.remove();
		}
	}
}

void QtCallbackProxy::unbind(QObject* sender, QEvent::Type event)
{
	int activeBindingCount = 0;
	QMutableVectorIterator<EventBinding> iter(m_eventBindings);
	while (iter.hasNext())
	{
		const EventBinding& binding = iter.next();
		if (binding.sender == sender)
		{
			++activeBindingCount;
			if (binding.eventType == event)
			{
				--activeBindingCount;
				iter.remove();
			}
		}
	}
	if (activeBindingCount == 0)
	{
		sender->removeEventFilter(this);
	}
}

QtCallbackProxy* installCallbackProxy(QObject* sender)
{
	// We currently create one proxy object per sender.
	//
	// On the one hand, this makes signal/event dispatches cheaper
	// because we only have to match on signals/events for that sender
	// in eventFilter() and qt_metacall().  Within QObject's internals,
	// there are also some operations that are linear in the number of
	// signal/slot connections.
	//
	// The downside of having more proxy objects is the cost per instance
	// and the time required to instantiate a proxy object the first time
	// a callback is installed for a given sender
	// 
	QObject* proxyTarget = sender;
	const char* callbackProperty = "qt_callback_handler";
	QtCallbackProxy* callbackProxy = proxyTarget->property(callbackProperty).value<QtCallbackProxy*>();
	if (!callbackProxy) {
		callbackProxy = new QtCallbackProxy(proxyTarget);
		proxyTarget->setProperty(callbackProperty, QVariant::fromValue(callbackProxy));
	}

	return callbackProxy;
}

void QtCallbackProxy::connectCallback(QObject* sender, const char* signal, const QtMetacallAdapter& callback)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	proxy->bind(sender, signal, callback);
}

void QtCallbackProxy::disconnectCallbacks(QObject* sender, const char* signal)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	proxy->unbind(sender, signal);
}

void QtCallbackProxy::connectEvent(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	proxy->bind(sender, event, callback, filter);
}

void QtCallbackProxy::disconnectEvent(QObject* sender, QEvent::Type event)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	proxy->unbind(sender, event);
}

const QtCallbackProxy::Binding* QtCallbackProxy::matchBinding(QObject* sender, int signalIndex) const
{
	for (int i = 0; i < m_bindings.count(); i++) {
		const Binding& binding = m_bindings.at(i);
		if (binding.sender == sender && binding.signalIndex == signalIndex) {
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

	if (!sender) {
		failInvoke("Unable to determine sender");
	} else if (signalIndex < 0) {
		failInvoke("Unable to determine sender signal");
	}

	methodId = QObject::qt_metacall(call, methodId, arguments);
	if (methodId < 0) {
		return methodId;
	}

	if (call == QMetaObject::InvokeMetaMethod) {
		if (methodId == 0) {
			const Binding* binding = matchBinding(sender, signalIndex);
			if (binding) {
				QGenericArgument args[6] = {
					QGenericArgument(binding->paramType(0), arguments[1]),
					QGenericArgument(binding->paramType(1), arguments[2]),
					QGenericArgument(binding->paramType(2), arguments[3]),
					QGenericArgument(binding->paramType(3), arguments[4]),
					QGenericArgument(binding->paramType(4), arguments[5]),
					QGenericArgument(binding->paramType(5), arguments[6])
				};
				binding->callback.invoke(args, binding->paramTypes.count());
			} else {
				const char* signalName = sender->metaObject()->method(signalIndex).signature();
				failInvoke(QString("Unable to find matching binding for signal %1").arg(signalName));
			}
		}
		--methodId;
	}
	return methodId;
}

bool QtCallbackProxy::eventFilter(QObject* watched, QEvent* event)
{
	Q_FOREACH(const EventBinding& binding, m_eventBindings) {
		if (binding.sender == watched &&
		    binding.eventType == event->type() &&
		    (!binding.filter || binding.filter(watched,event))) {
			binding.callback.invoke(0, 0);
		}
	}
	return QObject::eventFilter(watched, event);
}

