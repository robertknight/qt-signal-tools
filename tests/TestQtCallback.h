#include <QtTest/QTest>

#include "QtCallback.h"
#include "QtCallbackProxy.h"

class TestQtCallback : public QObject
{
	Q_OBJECT

	private Q_SLOTS:
		void testInvoke();
		void testSignalProxy();
		void testEventProxy();
};

class CallbackTester : public QObject
{
	Q_OBJECT

	public:
		QList<int> values;

		void emitASignal(int arg)
		{
			emit aSignal(arg);
		}

	public Q_SLOTS:
		void addValue(int value)
		{
			values << value;
		}

	Q_SIGNALS:
		void aSignal(int arg);
};
