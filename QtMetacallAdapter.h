#pragma once

#include "QtCallback.h"

#include <QtCore/QMetaObject>
#include <QtCore/QScopedPointer>

template <class T>
struct FunctionTraits;

template <class R>
struct FunctionTraits<R()>
{
	enum { count = 0 };
	typedef R result_type;
};

template <class R, class T>
struct FunctionTraits<R(T)> : FunctionTraits<R()>
{
	enum { count = 1 };
	typedef T arg0_type;
};

template <class R, class T1, class T2>
struct FunctionTraits<R(T1,T2)> : FunctionTraits<R(T1)>
{
	enum { count = 2 };
	typedef T2 arg1_type;
};

template <class R, class T1, class T2, class T3>
struct FunctionTraits<R(T1,T2,T3)> : FunctionTraits<R(T1,T2)>
{
	enum { count = 3 };
	typedef T3 arg2_type;
};

struct QtMetacallAdapterImplIface
{
	virtual ~QtMetacallAdapterImplIface() {}
	virtual void invoke(const QGenericArgument* args, int count) const = 0;
	virtual QtMetacallAdapterImplIface* clone() const = 0;
};

struct QtCallbackImpl : public QtMetacallAdapterImplIface
{
	QtCallback callback;

	QtCallbackImpl(const QtCallback& _callback)
	: callback(_callback)
	{}

	virtual void invoke(const QGenericArgument* _args, int count) const {
		const int MAX_ARGS = 6;
		QGenericArgument args[MAX_ARGS];
		for (int i=0; i < MAX_ARGS; i++) {
			if (i < count) {
				args[i] = _args[i];
			} else {
				args[i] = QGenericArgument();
			}
		}
		callback.invokeWithArgs(args[0], args[1], args[2], args[3], args[4], args[5]);
	}

	virtual QtMetacallAdapterImplIface* clone() const {
		return new QtCallbackImpl(callback);
	}
};

template <class Functor, class Derived>
struct QtMetacallAdapterImplBase : QtMetacallAdapterImplIface
{
	Functor functor;

	QtMetacallAdapterImplBase(const Functor& _f)
	: functor(_f)
	{}

	virtual QtMetacallAdapterImplIface* clone() const {
		return new Derived(functor);
	}
};

template <class T,int>
struct QtMetacallAdapterImpl;

template <template <class F> class Functor, class F>
struct QtMetacallAdapterImpl<Functor<F>,0>
  : QtMetacallAdapterImplBase<Functor<F>,QtMetacallAdapterImpl<Functor<F>,0> >
{
	typedef QtMetacallAdapterImplBase<Functor<F>,QtMetacallAdapterImpl<Functor<F>,0> > Base;
	QtMetacallAdapterImpl(const Functor<F>& functor) : Base(functor) {}

	virtual void invoke(const QGenericArgument*,int) const {
		Base::functor();
	}
};

template <template <class F> class Functor, class F>
struct QtMetacallAdapterImpl<Functor<F>,1>
  : QtMetacallAdapterImplBase<Functor<F>,QtMetacallAdapterImpl<Functor<F>,1> >
{
	typedef QtMetacallAdapterImplBase<Functor<F>,QtMetacallAdapterImpl<Functor<F>,1> > Base;
	QtMetacallAdapterImpl(const Functor<F>& functor) : Base(functor) {}

	virtual void invoke(const QGenericArgument* args, int count) const {
		Q_UNUSED(count);
		Base::functor(*reinterpret_cast<typename FunctionTraits<F>::arg0_type*>(args[0].data()));
	}
};

template <template <class F> class Functor, class F>
struct QtMetacallAdapterImpl<Functor<F>,2> 
  : QtMetacallAdapterImplBase<Functor<F>,QtMetacallAdapterImpl<Functor<F>,2> >
{
	typedef QtMetacallAdapterImplBase<Functor<F>,QtMetacallAdapterImpl<Functor<F>,2> > Base;
	QtMetacallAdapterImpl(const Functor<F>& functor) : Base(functor) {}

	virtual void invoke(const QGenericArgument* args, int count) const {
		Q_UNUSED(count);
		Base::functor(*reinterpret_cast<typename FunctionTraits<F>::arg0_type*>(args[0].data()),
		              *reinterpret_cast<typename FunctionTraits<F>::arg1_type*>(args[1].data()));
	}
};

template <template <class F> class Functor, class F>
struct QtMetacallAdapterImpl<Functor<F>,3>
  : QtMetacallAdapterImplBase<Functor<F>,QtMetacallAdapterImpl<Functor<F>,3> >
{
	typedef QtMetacallAdapterImplBase<Functor<F>,QtMetacallAdapterImpl<Functor<F>,3> > Base;
	QtMetacallAdapterImpl(const Functor<F>& functor) : Base(functor) {}

	virtual void invoke(const QGenericArgument* args, int count) const {
		Q_UNUSED(count);
		Base::functor(*reinterpret_cast<typename FunctionTraits<F>::arg0_type*>(args[0].data()),
		              *reinterpret_cast<typename FunctionTraits<F>::arg1_type*>(args[1].data()),
				      *reinterpret_cast<typename FunctionTraits<F>::arg2_type*>(args[2].data()));
	}
};

/** A wrapper around either a QtCallback or a function object (such as std::function)
 * which can invoke the function given an array of QGenericArgument objects.
 */
class QtMetacallAdapter
{
public:
	QtMetacallAdapter()
	{}

	QtMetacallAdapter(const QtCallback& callback)
	: m_impl(new QtCallbackImpl(callback))
	{}

	template <template <class F> class FunctionObject, class F>
	QtMetacallAdapter(const FunctionObject<F>& t)
	: m_impl(new QtMetacallAdapterImpl<FunctionObject<F>,FunctionTraits<F>::count>(t))
	{}

	QtMetacallAdapter(const QtMetacallAdapter& other)
	: m_impl(other.m_impl->clone())
	{}

	QtMetacallAdapter& operator=(const QtMetacallAdapter& other)
	{
		m_impl.reset(other.m_impl->clone());
		return *this;
	}

	void invoke(const QGenericArgument* args, int count) const
	{
		m_impl->invoke(args, count);
	}
	
private:
	QScopedPointer<QtMetacallAdapterImplIface> m_impl;
};

