#include "TestQtCallback.h"

#include <QtCore/QDebug>

#include <tr1/functional>
#include <iostream>

using namespace std;
using namespace std::tr1;

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

void TestQtCallback::testSignalProxyTr1()
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
	  function<void(int)>(bind(&CallbackTester::addValue, &tester, placeholders::_1)));
	tester.emitASignal(39);
	QCOMPARE(tester.values, QList<int>() << 39);

	// check that a signal arg count mismatch
	// is caught
	tester.values.clear();
	QtCallbackProxy::connectCallback(&tester, SIGNAL(noArgSignal()),
	  function<void(int)>(bind(&CallbackTester::addValue, &tester, placeholders::_1)));
	tester.emitNoArgSignal();
	QCOMPARE(tester.values, QList<int>());
}

void TestQtCallback::testArgCast()
{
	// use function and bind() to perform a cast of the argument
	// when it is emitted.
	QList<qint64> list;
	CallbackTester tester;
	QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)),
	  function<void(int)>(bind(&QList<qint64>::push_back, &list, placeholders::_1)));
	tester.emitASignal(42);
	QCOMPARE(list, QList<qint64>() << 42LL);
}

QTEST_MAIN(TestQtCallback)
