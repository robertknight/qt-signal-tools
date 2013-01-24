#include "TestQtCallback.h"

#include <QtCore/QDebug>

#ifdef ENABLE_QTCALLBACK_TR1_FUNCTION
using namespace std::tr1;
#endif

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
#ifdef ENABLE_QTCALLBACK_TR1_FUNCTION
	CallbackTester tester;
	QtCallbackProxy::connectCallback(&tester, SIGNAL(aSignal(int)),
	  function<void()>(bind(&CallbackTester::addValue, &tester, 18)));
	tester.emitASignal(42);
	QCOMPARE(tester.values, QList<int>() << 18);
	tester.values.clear();

	QtCallbackProxy::disconnectCallbacks(&tester, SIGNAL(aSignal(int)));
	tester.emitASignal(19);
	QCOMPARE(tester.values, QList<int>());
#endif
}


QTEST_MAIN(TestQtCallback)
