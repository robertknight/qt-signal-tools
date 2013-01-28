#include "QtCallbackProxy.h"

#include <QtCore/QDebug>

int qtObjectSignalIndex(const QObject* object, const char* signal)
{
	const QMetaObject* metaObject = object->metaObject();
	int signalIndex = metaObject->indexOfMethod(signal + 1);
	if (signalIndex < 0) {
		QByteArray normalizedSignature = QMetaObject::normalizedSignature(signal + 1);
		signalIndex = metaObject->indexOfMethod(normalizedSignature.constData());
	}
	return signalIndex;
}

QtCallbackProxy::QtCallbackProxy(QObject* parent)
	: QObject(parent)
{
}

bool QtCallbackProxy::checkTypeMatch(const QtMetacallAdapter& callback, const QList<QByteArray>& paramTypes)
{
	int receiverArgTypes[QTMETACALL_MAX_ARGS] = {-1};
	int receiverArgCount = callback.getArgTypes(receiverArgTypes);

	for (int i=0; i < receiverArgCount; i++) {
		if (i >= paramTypes.count()) {
			qWarning() << "Missing argument" << i << ": "
			  << "Receiver expects" << QLatin1String(QMetaType::typeName(receiverArgTypes[i]));
			return false;
		}
		int type = QMetaType::type(paramTypes.at(i).data());
		if (type != receiverArgTypes[i]) {
			qWarning() << "Type mismatch for argument" << i << ": "
			  << "Signal sends" << QLatin1String(QMetaType::typeName(type))
			  << "receiver expects" << QLatin1String(QMetaType::typeName(receiverArgTypes[i]));
			return false;
		}
	}

	return true;
}

bool QtCallbackProxy::bind(QObject* sender, const char* signal, const QtMetacallAdapter& callback)
{
	int signalIndex = qtObjectSignalIndex(sender, signal);
	if (signalIndex < 0) {
		qWarning() << "No such signal" << signal << "for" << sender;
		return false;
	}

	Binding binding(sender, signalIndex, callback);
	binding.paramTypes = sender->metaObject()->method(signalIndex).parameterTypes();

	if (!checkTypeMatch(callback, binding.paramTypes)) {
		qWarning() << "Sender and receiver types do not match for" << signal+1;
		return false;
	}

	int memberOffset = QObject::staticMetaObject.methodCount();

	if (!QMetaObject::connect(sender, signalIndex, this, memberOffset, Qt::AutoConnection, 0)) {
		qWarning() << "Unable to connect signal" << signal << "for" << sender;
		return false;
	}

	m_bindings.insert(sender, binding);
	return true;
}

bool QtCallbackProxy::bind(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter)
{
	if (!checkTypeMatch(callback, QList<QByteArray>())) {
		qWarning() << "Callback does not take 0 arguments";
		return false;
	}

	sender->installEventFilter(this);

	EventBinding binding(sender, event, callback, filter);
	m_eventBindings.insert(sender, binding);

	return true;
}

void QtCallbackProxy::unbind(QObject* sender, const char* signal)
{
	int signalIndex = qtObjectSignalIndex(sender, signal);
	QMutableHashIterator<QObject*,Binding> iter(m_bindings);
	while (iter.hasNext())
	{
		iter.next();
		const Binding& binding = iter.value();
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
	QMutableHashIterator<QObject*,EventBinding> iter(m_eventBindings);
	while (iter.hasNext())
	{
		iter.next();
		const EventBinding& binding = iter.value();
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

bool QtCallbackProxy::connectCallback(QObject* sender, const char* signal, const QtMetacallAdapter& callback)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	return proxy->bind(sender, signal, callback);
}

void QtCallbackProxy::disconnectCallbacks(QObject* sender, const char* signal)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	proxy->unbind(sender, signal);
}

bool QtCallbackProxy::connectEvent(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	return proxy->bind(sender, event, callback, filter);
}

void QtCallbackProxy::disconnectEvent(QObject* sender, QEvent::Type event)
{
	QtCallbackProxy* proxy = installCallbackProxy(sender);
	proxy->unbind(sender, event);
}

const QtCallbackProxy::Binding* QtCallbackProxy::matchBinding(QObject* sender, int signalIndex) const
{
	QHash<QObject*,Binding>::const_iterator iter = m_bindings.find(sender);
	for (;iter != m_bindings.end() && iter.key() == sender;++iter) {
		const Binding& binding = *iter;
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
				const int MAX_ARGS = 10;
				int argCount = binding->paramTypes.count();
				QGenericArgument args[MAX_ARGS];
				for (int i=0; i < argCount; i++) {
					args[i] = QGenericArgument(binding->paramType(0), arguments[i+1]);
				}
				binding->callback.invoke(args, argCount);
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

