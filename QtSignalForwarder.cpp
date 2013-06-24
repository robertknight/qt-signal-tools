#include "QtSignalForwarder.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtCore/QTimer>

// method index of QObject::destroyed(QObject*) signal
const int DESTROYED_SIGNAL_INDEX = 0;

// minimum ID for method IDs used in signal bindings.
//
// These IDs are used by QtSignalForwarder::qt_metacall() to
// determine which binding to invoke.
const int BINDING_METHOD_MIN_ID = 1000;

// limit on number of signal bindings per proxy.
// Internally Qt stores receiver method IDs for signal connections
// in a 16-bit uint, so there is a constraint that
// BINDING_METHOD_MIN_ID + MAX_BINDINGS_PER_PROXY <= 2^16
const int MAX_BINDINGS_PER_PROXY = 10000;

// dummy function for use with the sentinel callback for when
// an object is destroyed. It should never actually be called.
void destroyBindingFunc()
{
	Q_ASSERT(false);
}
QtMetacallAdapter QtSignalForwarder::s_senderDestroyedCallback(destroyBindingFunc);

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

// returns true if called on the main app thread
bool isMainThread()
{
	return QThread::currentThread() == QCoreApplication::instance()->thread();
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
	if (!m_senderSignalBindingIds.contains(sender)) {
		bind(sender, SIGNAL(destroyed(QObject*)), s_senderDestroyedCallback);
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

	if (!canAddSignalBindings()) {
		qWarning() << "Limit of bindings per proxy has been reached";
		return false;
	}

	if (m_freeSignalBindingIds.isEmpty()) {
		m_freeSignalBindingIds << BINDING_METHOD_MIN_ID + m_signalBindings.count();
	}
	int bindingId = m_freeSignalBindingIds.takeFirst();
	Q_ASSERT(!m_signalBindings.contains(bindingId));

	// we use Qt::DirectConnection here, so the callback will always be invoked on the same
	// thread that the signal was delivered.  This ensures that we can rely on the object
	// still existing in the qt_metacall() implementation.  This also means that we don't
	// retain any QObject* pointers in the internal maps once the destroyed(QObject*) signal
	// has been emitted.
	//
	// If the binding's callback uses QtCallback, that will use a queued connection if the receiver
	// actually lives in a different thread.
	//
	if (!QMetaObject::connect(sender, signalIndex, this, bindingId, Qt::DirectConnection, 0)) {
		qWarning() << "Unable to connect signal" << signal << "for" << sender;
		return false;
	}

	m_signalBindings.insert(bindingId, binding);

	if (callback != s_senderDestroyedCallback) {
		// listen for destroyed(QObject*) signal to remove
		// all bindings.  setupDestroyNotify() in turn calls bind()
		// with s_senderDestroyedCallback as the callback
		setupDestroyNotify(sender);
	}
	
	m_senderSignalBindingIds.insertMulti(sender, bindingId);

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
	QHash<QObject*,int>::iterator iter = m_senderSignalBindingIds.find(sender);
	while (iter != m_senderSignalBindingIds.end() && iter.key() == sender) {
		Q_ASSERT(m_signalBindings.contains(*iter));
		const Binding& binding = m_signalBindings.value(*iter);
		if (binding.signalIndex == signalIndex) {
			m_freeSignalBindingIds << *iter;
			m_signalBindings.remove(*iter);
			iter = m_senderSignalBindingIds.erase(iter);
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
	QHash<QObject*,int>::iterator iter = m_senderSignalBindingIds.find(sender);
	while (iter != m_senderSignalBindingIds.end() && iter.key() == sender) {
		m_freeSignalBindingIds << *iter;
		m_signalBindings.remove(*iter);
		iter = m_senderSignalBindingIds.erase(iter);
	}
	m_eventBindings.remove(sender);

	sender->removeEventFilter(this);
	disconnect(sender, 0, this, 0);
}

bool QtSignalForwarder::canAddSignalBindings() const
{
	return m_signalBindings.count() < MAX_BINDINGS_PER_PROXY;
}

QtSignalForwarder* QtSignalForwarder::sharedProxy(QObject* sender)
{
	// QtSignalForwarder methods are currently not thread-safe, so
	// use of shared proxies is restricted to the main thread.
	Q_ASSERT(isMainThread());

	Q_UNUSED(sender);

	// We try to use a small number of shared proxy objects to minimize
	// the overhead of each binding.
	//
	// There are some issues to consider when re-using proxies however:
	//
	// - Some operations in QObject's internals are linear in the number of
	//   connected senders, eg. sender(), senderSignalIndex()
	// - There is a limit on the number of signal bindings for any given proxy
	// - When using Qt::AutoConnection to connect the sender and receiver, the
	//   delivery method depends on the sender/receiver threads
	//
	static QList<QtSignalForwarder*> proxies;
	if (proxies.isEmpty() || !proxies.last()->canAddSignalBindings()) {
		QtSignalForwarder* newProxy = new QtSignalForwarder(QCoreApplication::instance());
		proxies << newProxy;
	}
	return proxies.last();
}

bool QtSignalForwarder::connect(QObject* sender, const char* signal, const QtMetacallAdapter& callback)
{
	return sharedProxy(sender)->bind(sender, signal, callback);
}

void QtSignalForwarder::disconnect(QObject* sender, const char* signal)
{
	sharedProxy(sender)->unbind(sender, signal);
}

bool QtSignalForwarder::connect(QObject* sender, QEvent::Type event, const QtMetacallAdapter& callback, EventFilterFunc filter)
{
	return sharedProxy(sender)->bind(sender, event, callback, filter);
}

void QtSignalForwarder::disconnect(QObject* sender, QEvent::Type event)
{
	sharedProxy(sender)->unbind(sender, event);
}

void QtSignalForwarder::failInvoke(const QString& error)
{
	qWarning() << "Failed to invoke callback" << error;
}

void QtSignalForwarder::invokeBinding(const Binding& binding, void** arguments)
{
	const int MAX_ARGS = 10;
	int argCount = binding.paramTypes.count();
	QGenericArgument args[MAX_ARGS];
	for (int i=0; i < argCount; i++) {
		args[i] = QGenericArgument(binding.paramType(0), arguments[i+1]);
	}
	binding.callback.invoke(args, argCount);
}

int QtSignalForwarder::qt_metacall(QMetaObject::Call call, int methodId, void** arguments)
{
	if (methodId >= BINDING_METHOD_MIN_ID && call == QMetaObject::InvokeMetaMethod) {
		// signal binding invocation
		//
		// note: Avoid using sender() and senderSignalIndex() here as:
		// - The cost is linear in the number of connected senders
		// - Both functions involve a mutex lock on the sender
		// - The functions do not work for queued signals
		//
		QHash<int,Binding>::const_iterator iter = m_signalBindings.find(methodId);
		if (iter != m_signalBindings.end()) {
			if (iter->callback == s_senderDestroyedCallback) {
				unbind(iter->sender);
			} else {
				invokeBinding(*iter, arguments);
			}
		} else {
			failInvoke(QString("Unable to find matching binding for signal %1").arg(methodId));
		}
		return -1;
	} else {
		// standard qt_metacall() implementation
		return QObject::qt_metacall(call, methodId, arguments);
	}
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
	int totalSignalBindings = 0;
	Q_FOREACH(const Binding& binding, m_signalBindings) {
		if (binding.callback != s_senderDestroyedCallback) {
			++totalSignalBindings;
		}
	}
	return totalSignalBindings + m_eventBindings.size();
}

bool QtSignalForwarder::isConnected(QObject* sender) const
{
	QHash<QObject*,int>::const_iterator signalBindingIter = m_senderSignalBindingIds.find(sender);
	while (signalBindingIter != m_senderSignalBindingIds.end() &&
	       signalBindingIter.key() == sender) {
		Q_ASSERT(m_signalBindings.contains(*signalBindingIter));
		const Binding& binding = m_signalBindings.value(*signalBindingIter);
		if (binding.callback != s_senderDestroyedCallback) {
			return true;
		}
		++signalBindingIter;
	}
	return m_eventBindings.contains(sender);
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


