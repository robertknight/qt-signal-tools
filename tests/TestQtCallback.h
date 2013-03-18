#include <QtTest/QTest>

#include "QtCallback.h"
#include "QtSignalForwarder.h"
#include "FunctionUtils.h"

class TestQtCallback : public QObject
{
	Q_OBJECT

	private Q_SLOTS:
		void testInvoke();
		void testSignalProxy();
		void testEventProxy();
		void testSignalToFunctionObject();
		void testSignalToPlainFunc();
		void testArgCast();
		void testArgTypeCheck();
		void testArgLimit();
		void testSignalToLambda();
		void testSenderDestroyed();
		void testUnbind();
		void testDelayedCall();
		void testSafeBinder();
		void testBindingCount();

		void testConnectPerf();
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

		void emitNoArgSignal()
		{
			emit noArgSignal();
		}

		void emitStringSignal(const QString& arg)
		{
			emit stringSignal(arg);
		}

		// expose protected QObject::receivers() method
		int receiverCount(const char* signal) const
		{
			return receivers(signal);
		}

	public Q_SLOTS:
		void addValue(int value)
		{
			values << value;
			emit valuesChanged();
		}

	Q_SIGNALS:
		void aSignal(int arg);
		void noArgSignal();
		void valuesChanged();
		void stringSignal(const QString& arg);
};
