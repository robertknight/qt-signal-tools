#pragma once

#include "QtCallback.h"

#include <QtCore/QVector>

/** Utility class which provides a way to invoke a QtCallback when an
 * object emits a signal.
 * 
 * Often when a slot is invoked in response to a signal, it would be useful
 * to be able to pass additional arguments to the slot.
 *
 * In Qt 5, especially with C++11, this is made much easier by the new signal-slot syntax:
 * http://qt-project.org/wiki/New_Signal_Slot_Syntax
 *
 * QtCallbackProxy provides a way to simulate this under Qt 4 with C++03 by installing
 * a proxy object between the original sender of the signal and the receiver.  The proxy
 * receives the signal, combines the arguments from the signal with those from a QtCallback
 * and then invokes the callback.
 *
 * Think of this as a more powerful version of QSignalMapper.
 *
 *  MyObject receiver;
 *  QPushButton button;
 *  QtCallback1<int> callback(&receiver, SLOT(buttonClicked(int)));
 *  callback.bind(42);
 *  QtCallbackProxy::connectCallback(&button, SIGNAL(clicked(bool)), callback);
 *  button.click();
 *
 * Will invoke Receiver::buttonClicked(42)
 */
class QtCallbackProxy : public QObject
{
	// no Q_OBJECT macro here - see qt_metacall() re-implementation
	// below
	
	public:
		QtCallbackProxy(QObject* parent = 0);

		/** Set up a binding so that @p callback is invoked when
		 * @p sender emits @p signal.  If @p signal has default arguments,
		 * they must be specified.  eg. Use SLOT(clicked(bool)) for a button
		 * rather than SLOT(clicked()).
		 */
		void bind(QObject* sender, const char* signal, const QtCallbackBase& callback);

		// re-implemented from QObject (this method is normally declared via the Q_OBJECT
		// macro and implemented by the code generated by moc)
		virtual int qt_metacall(QMetaObject::Call call, int methodId, void** arguments);

		/** Install a proxy which invokes @p callback when @p sender emits @p signal.
		 */
		static void connectCallback(QObject* sender, const char* signal, const QtCallbackBase& callback);

	private:
		struct Binding
		{
			Binding(QObject* _sender = 0, int _signalIndex = -1, const QtCallbackBase& _callback = QtCallbackBase())
				: sender(_sender)
				, signalIndex(_signalIndex)
				, callback(_callback)
			{}

			const char* paramType(int index) const
			{
				if (index >= paramTypes.count())
				{
					return 0;
				}
				return paramTypes.at(index).constData();
			}

			QObject* sender;
			int signalIndex;
			QtCallbackBase callback;
			QList<QByteArray> paramTypes;
		};

		const Binding* matchBinding(QObject* sender, int signalIndex) const;
		void failInvoke(const QString& error);

		QVector<Binding> m_bindings;
};

Q_DECLARE_METATYPE(QtCallbackProxy*)

