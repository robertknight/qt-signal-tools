#include "QtSignalForwarder.h"

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QTimer>

// method index of QObject::destroyed(QObject*) signal
const int DESTROYED_SIGNAL_INDEX = 0;

struct ProxyMap
{
	QHash<QObject*,QtSignalForwarder*> map;
	QMutex mutex;
};

Q_GLOBAL_STATIC(ProxyMap, theProxyMap);

static int targetMethodIndex()
{
	return QObject::staticMetaObject.methodCount();
}

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

QtSignalForwarder::QtSignalForwarder(QObject* parent)
	: QObject(parent)
{
}

bool QtSignalForwarder::checkTypeMatch(const QtMetacallAdapter& callback, const QList<QByteArray>& paramTypes)
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

void QtSignalForwarder::setupDestroyNotify(QObject* sender)
{
	if (!m_bindings.contains(sender) &&
	    !m_eventBindings.contains(sender))
	{
		QMetaObject::connect(sender, DESTROYED_SIGNAL_INDEX,
		  this, targetMethodIndex(), Qt::AutoConnection, 0);
	}
}

bool QtSignalForwarder::bind(QObject* sender, const char* signal, const QtMetacallAdapter& callback)
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

	if (!matchBinding(sender, signalIndex)) {
		if (!QMetaObject::connect(sender, signalIndex, this, targetMethodIndex(), Qt::AutoConnection, 0)) {
			qWarning() << "Unable to connect signal" << signal << "for" << sender;
			return false;
		}
	}

	setupDestroyNotify(sender);
	m_bindings.insertMulti(sender, binding);
	return true;
}

bool QtSignalForwarder::bind(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter)
{
	if (!checkTypeMatch(callback, QList<QByteArray>())) {
		qWarning() << "Callback does not take 0 arguments";
		return false;
	}

	setupDestroyNotify(sender);
	sender->installEventFilter(this);

	EventBinding binding(sender, event, callback, filter);
	m_eventBindings.insertMulti(sender, binding);

	return true;
}

void QtSignalForwarder::unbind(QObject* sender, const char* signal)
{
	int signalIndex = qtObjectSignalIndex(sender, signal);
	QHash<QObject*,Binding>::iterator iter = m_bindings.find(sender);
	while (iter != m_bindings.end() && iter.key() == sender) {
		if (iter->signalIndex == signalIndex) {
			QMetaObject::disconnect(sender, signalIndex, this, targetMethodIndex());
			iter = m_bindings.erase(iter);
		} else {
			++iter;
		}
	}
	if (!isConnected(sender)) {
		// disconnect destruction notifications
		unbind(sender);
	}
}

void QtSignalForwarder::unbind(QObject* sender, QEvent::Type event)
{
	QHash<QObject*,EventBinding>::iterator iter = m_eventBindings.find(sender);
	while (iter != m_eventBindings.end() && iter.key() == sender) {
		if (iter->eventType == event) {
			iter = m_eventBindings.erase(iter);
		} else {
			++iter;
		}
	}
	if (!isConnected(sender)) {
		// disconnect destruction notifications
		unbind(sender);
	}
}

void QtSignalForwarder::unbind(QObject* sender)
{
	m_bindings.remove(sender);
	m_eventBindings.remove(sender);

	sender->removeEventFilter(this);
	disconnect(sender, 0, this, 0);

	if (installProxy(sender) == this) {
		removeProxy(sender);
	}
}

QtSignalForwarder* QtSignalForwarder::installProxy(QObject* sender)
{
	// We currently create one proxy object per sender.
	//
	// The upside is that some operations are cheaper as within QObject's
	// internals, there are methods which are linear in the number of connected
	// senders (eg. sender(), senderSignalIndex())
	//
	// The downside of having more proxy objects is the cost per instance
	// and the time required to instantiate a proxy object the first time
	// a callback is installed for a given sender
	// 
	QtSignalForwarder* proxy = 0;
	{
		QMutexLocker lock(&theProxyMap()->mutex);
		proxy = theProxyMap()->map.value(sender);
	}

	if (!proxy) {
		proxy = new QtSignalForwarder(sender);
		QMutexLocker lock(&theProxyMap()->mutex);
		theProxyMap()->map.insert(sender,proxy);
	}
	return proxy;
}

void QtSignalForwarder::removeProxy(QObject* sender)
{
	QMutexLocker lock(&theProxyMap()->mutex);
	theProxyMap()->map.remove(sender);
}

bool QtSignalForwarder::connect(QObject* sender, const char* signal, const QtMetacallAdapter& callback)
{
	return installProxy(sender)->bind(sender, signal, callback);
}

void QtSignalForwarder::disconnect(QObject* sender, const char* signal)
{
	installProxy(sender)->unbind(sender, signal);
}

bool QtSignalForwarder::connect(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter)
{
	return installProxy(sender)->bind(sender, event, callback, filter);
}

void QtSignalForwarder::disconnect(QObject* sender, QEvent::Type event)
{
	installProxy(sender)->unbind(sender, event);
}

const QtSignalForwarder::Binding* QtSignalForwarder::matchBinding(QObject* sender, int signalIndex) const
{
	QHash<QObject*,Binding>::const_iterator iter = m_bindings.find(sender);
	for (;iter != m_bindings.end() && iter.key() == sender;++iter) {
		const Binding& binding = iter.value();
		if (binding.sender == sender && binding.signalIndex == signalIndex) {
			return &binding;
		}
	}
	return 0;
}

void QtSignalForwarder::failInvoke(const QString& error)
{
	qWarning() << "Failed to invoke callback" << error;
}

int QtSignalForwarder::qt_metacall(QMetaObject::Call call, int methodId, void** arguments)
{
	// performance note - QObject::sender() and senderSignalIndex()
	// are linear in the number of connections to this object
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
			bool foundBinding = false;
			QHash<QObject*,Binding>::const_iterator iter = m_bindings.find(sender);
			for (;iter != m_bindings.end() && iter.key() == sender;++iter) {
				const Binding& binding = iter.value();
				if (binding.sender == sender && binding.signalIndex == signalIndex) {
					foundBinding = true;
					const int MAX_ARGS = 10;
					int argCount = binding.paramTypes.count();
					QGenericArgument args[MAX_ARGS];
					for (int i=0; i < argCount; i++) {
						args[i] = QGenericArgument(binding.paramType(0), arguments[i+1]);
					}
					binding.callback.invoke(args, argCount);
				}
			}
			
			if (!foundBinding && signalIndex != DESTROYED_SIGNAL_INDEX) {
				const char* signalName = sender->metaObject()->method(signalIndex).signature();
				failInvoke(QString("Unable to find matching binding for signal %1").arg(signalName));
			}
		}
		--methodId;
	}

	if (signalIndex == DESTROYED_SIGNAL_INDEX) {
		unbind(sender);
	}

	return methodId;
}

bool QtSignalForwarder::eventFilter(QObject* watched, QEvent* event)
{
	QHash<QObject*,EventBinding>::iterator iter = m_eventBindings.find(watched);
	for (;iter != m_eventBindings.end() && iter.key() == watched; iter++) {
		const EventBinding& binding = iter.value();
		if (binding.eventType == event->type() &&
		    (!binding.filter || binding.filter(watched,event))) {
			binding.callback.invoke(0, 0);
		}
	}
	return QObject::eventFilter(watched, event);
}

int QtSignalForwarder::bindingCount() const
{
	return m_bindings.size() + m_eventBindings.size();
}

bool QtSignalForwarder::isConnected(QObject* sender) const
{
	return m_bindings.contains(sender) || m_eventBindings.contains(sender);
}

void QtSignalForwarder::delayedCall(int ms, const QtMetacallAdapter& adapter)
{
	QTimer* timer = new QTimer;
	timer->setSingleShot(true);
	timer->setInterval(ms);
	QtSignalForwarder::connect(timer, SIGNAL(timeout()), adapter);
	connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));
	timer->start();
}


