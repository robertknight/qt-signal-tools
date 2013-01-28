#include "TestQtCallback.h"

#include <QtCore/QDebug>

#include <iostream>

using namespace std::tr1;
using namespace std::tr1::placeholders;

void TestQtCallback::testInvoke()
{
	CallbackTester tester;
	QtCallback callback1(&tester, SLOT(addValue(int)));
	callback1.bind(42);
	callback1.invoke();
	QCOMPARE(tester.values, QList<int>() << 42);
	tester.values.clear();

	QtCallback1<int> callback2(&tester, SLOT(addValue(int)));
	callback2.invoke(27);
	QCOMPARE(tester.values, QList<int>() << 27);
}

void TestQtCallback::testSignalProxy()
{
	CallbackTester tester;
	QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)),
	  QtCallback(&tester, SLOT(addValue(int))));
	tester.emitASignal(32);

	QCOMPARE(tester.values, QList<int>() << 32);
	tester.values.clear();

	QtCallbackProxy::disconnectCallbacks(&tester, SIGNAL(aSignal(int)));

	tester.emitASignal(15);
	QCOMPARE(tester.values, QList<int>());
	tester.values.clear();

	QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)),
	  QtCallback(&tester, SLOT(addValue(int))).bind(10));
	tester.emitASignal(11);
	QCOMPARE(tester.values, QList<int>() << 10);
	tester.values.clear();

	QtCallbackProxy::connectCallback(&tester, SIGNAL(noArgSignal()),
	  QtCallback(&tester, SLOT(addValue(int))));
	tester.emitNoArgSignal();
	QCOMPARE(tester.values, QList<int>());
}

void TestQtCallback::testEventProxy()
{
	CallbackTester tester;
	QtCallbackProxy::connectEvent(&tester, QEvent::MouseButtonPress,
	  QtCallback(&tester, SLOT(addValue(int))).bind(1));
	QMouseEvent event(QEvent::MouseButtonPress, QPoint(0,0), Qt::LeftButton, Qt::LeftButton, 0);
	QCoreApplication::sendEvent(&tester, &event);
	QCOMPARE(tester.values, QList<int>() << 1);
	tester.values.clear();

	QtCallbackProxy::disconnectEvent(&tester, QEvent::MouseButtonPress);
	QCoreApplication::sendEvent(&tester, &event);
	QCOMPARE(tester.values, QList<int>());
}

void TestQtCallback::testSignalToFunctionObject()
{
	CallbackTester tester;
	QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)),
	  function<void()>(bind(&CallbackTester::addValue, &tester, 18)));
	tester.emitASignal(42);
	QCOMPARE(tester.values, QList<int>() << 18);
	tester.values.clear();

	QtCallbackProxy::disconnectCallbacks(&tester, SIGNAL(aSignal(int)));
	tester.emitASignal(19);
	QCOMPARE(tester.values, QList<int>());
	tester.values.clear();

	QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)),
	  function<void(int)>(bind(&CallbackTester::addValue, &tester, _1)));
	tester.emitASignal(39);
	QCOMPARE(tester.values, QList<int>() << 39);

	// check that a signal arg count mismatch
	// is caught
	tester.values.clear();
	QtCallbackProxy::connectCallback(&tester, SIGNAL(noArgSignal()),
	  function<void(int)>(bind(&CallbackTester::addValue, &tester, _1)));
	tester.emitNoArgSignal();
	QCOMPARE(tester.values, QList<int>());
}

int sumInputs(int value)
{
	static int total = 0;
	total += value;
	return total;
}

void TestQtCallback::testSignalToPlainFunc()
{
	CallbackTester tester;
	QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)), sumInputs);
	tester.emitASignal(5);
	tester.emitASignal(8);
	QCOMPARE(sumInputs(0), 13);
}

void TestQtCallback::testArgCast()
{
	// use function and bind() to perform a cast of the argument
	// when it is emitted.
	QList<qint64> list;
	CallbackTester tester;
	QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)),
	  function<void(int)>(bind(&QList<qint64>::push_back, &list, _1)));
	tester.emitASignal(42);
	QCOMPARE(list, QList<qint64>() << 42LL);
}

void floatFunc(float) {}
void intFunc(int) {}
void noArgsFunc() {}
void twoArgsFunc(int,int) {}

void TestQtCallback::testArgTypeCheck()
{
	CallbackTester tester;
	QVERIFY(QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)), intFunc));
	QVERIFY(!QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)), floatFunc));
	QVERIFY(QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)), noArgsFunc));
	QVERIFY(!QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)), twoArgsFunc));
}

void fiveArgFunc(int,bool,float,char,double) {}

void TestQtCallback::testArgLimit()
{
	QtMetacallAdapter adapter(fiveArgFunc);
	QtMetacallArgsArray args = {-1};
	int count = adapter.getArgTypes(args);
	QStringList argList;
	for (int i=0; i < count; i++) {
		argList << QMetaType::typeName(args[i]);
	}
	QCOMPARE(argList, QStringList() << "int" << "bool" << "float" << "char" << "double");
}

void TestQtCallback::testSignalToLambda()
{
#ifdef COMPILER_SUPPORTS_LAMBDAS
    CallbackTester tester;
    int sum = 0;
    QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)), function<void(int)>(
                                         [&](int value) { sum += value; }
    ));
    tester.emitASignal(12);
    tester.emitASignal(7);
    QCOMPARE(sum, 19);
#endif
}

void TestQtCallback::testSenderDestroyed()
{
	QScopedPointer<CallbackTester> tester(new CallbackTester);
	QtCallbackProxy proxy;
	proxy.bind(tester.data(), SIGNAL(aSignal(int)),
	  noArgsFunc);
	QCOMPARE(proxy.bindingCount(), 1);
	tester.reset();
	QCOMPARE(proxy.bindingCount(), 0);

	tester.reset(new CallbackTester);
	proxy.bind(tester.data(), QEvent::MouseButtonPress, noArgsFunc);
	QCOMPARE(proxy.bindingCount(), 1);
	tester.reset();
	QCOMPARE(proxy.bindingCount(), 0);
}

QTEST_MAIN(TestQtCallback)
