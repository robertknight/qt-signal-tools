#pragma once

#include "QtCallback.h"

#include <QtCore/QMetaObject>
#include <QtCore/QScopedPointer>

#include <QtCore/QDebug>

template <class T>
struct RemovePtr
{
	typedef T type;
};

template <class T>
struct RemovePtr<T*>
{
	typedef T type;
};

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

// interface for implementations of QtMetacallAdapter
struct QtMetacallAdapterImplIface
{
	virtual ~QtMetacallAdapterImplIface() {}
	virtual bool invoke(const QGenericArgument* args, int count) const = 0;
	virtual QtMetacallAdapterImplIface* clone() const = 0;
	virtual bool canInvoke(int* args, int count) const = 0;
};

struct QtCallbackImpl : public QtMetacallAdapterImplIface
{
	QtCallback callback;

	QtCallbackImpl(const QtCallback& _callback)
	: callback(_callback)
	{}

	virtual bool invoke(const QGenericArgument* _args, int count) const {
		const int MAX_ARGS = 6;
		QGenericArgument args[MAX_ARGS];
		for (int i=0; i < MAX_ARGS; i++) {
			if (i < count) {
				args[i] = _args[i];
			} else {
				args[i] = QGenericArgument();
			}
		}
		return callback.invokeWithArgs(args[0], args[1], args[2], args[3], args[4], args[5]);
	}

	virtual QtMetacallAdapterImplIface* clone() const {
		return new QtCallbackImpl(callback);
	}

	virtual bool canInvoke(int* args, int count) const {
		int unboundCount = callback.unboundParameterCount();
		if (count < unboundCount) {
			return false;
		}
		for (int i=0; i < unboundCount; i++) {
			if (callback.unboundParameterType(i) != args[i]) {
				return false;
			}
		}
		return true;
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

	template <class T>
	static bool argMatch(int arg) {
		return qMetaTypeId<T>() == arg;
	}
};

template <class Functor, class Signature, int>
struct QtMetacallAdapterImpl;

template <class Functor, class Signature>
struct QtMetacallAdapterImpl<Functor,Signature,0>
  : QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,Signature,0> >
{
	typedef QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,Signature,0> > Base;
	QtMetacallAdapterImpl(const Functor& functor) : Base(functor) {}

	virtual bool invoke(const QGenericArgument*,int) const {
		Base::functor();
		return true;
	}

	virtual bool canInvoke(int*, int) const {
		return true;
	}
};

// extract the Nth argument from a QGenericArgument array and cast
// to the type expected by the Nth functor parameter
#define QMA_CAST_ARG(N) *reinterpret_cast<typename FunctionTraits<Signature>::arg##N##_type*>(args[N].data())

// check that the Nth argument type from a QGenericArgument array matches
// the Nth parameter type expected by the functor
#define QMA_CHECK_ARG_TYPE(N) Base::template argMatch<typename FunctionTraits<Signature>::arg##N##_type>(args[N])

template <class Functor, class Signature>
struct QtMetacallAdapterImpl<Functor,Signature,1>
  : QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,Signature,1> >
{
	typedef QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,Signature,1> > Base;
	QtMetacallAdapterImpl(const Functor& functor) : Base(functor) {}

	virtual bool invoke(const QGenericArgument* args, int count) const {
		if (count < 1) {
			return false;
		}
		Base::functor(QMA_CAST_ARG(0));
		return true;
	}

	virtual bool canInvoke(int* args, int count) const {
		if (count < 1) {
			return false;
		}
		return QMA_CHECK_ARG_TYPE(0);
	}
};

template <class Functor, class Signature>
struct QtMetacallAdapterImpl<Functor,Signature,2> 
  : QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,Signature,2> >
{
	typedef QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,Signature,2> > Base;
	QtMetacallAdapterImpl(const Functor& functor) : Base(functor) {}

	virtual bool invoke(const QGenericArgument* args, int count) const {
		if (count < 2) {
			return false;
		}
		Base::functor(QMA_CAST_ARG(0), QMA_CAST_ARG(1));
		return true;
	}

	virtual bool canInvoke(int* args, int count) const {
		if (count < 2) {
			return false;
		}
		return QMA_CHECK_ARG_TYPE(0) && QMA_CHECK_ARG_TYPE(1);
	}
};

template <class Functor, class Signature>
struct QtMetacallAdapterImpl<Functor,Signature,3>
  : QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,Signature,3> >
{
	typedef QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,Signature,3> > Base;
	QtMetacallAdapterImpl(const Functor& functor) : Base(functor) {}

	virtual bool invoke(const QGenericArgument* args, int count) const {
		if (count < 3) {
			return false;
		}
		Base::functor(QMA_CAST_ARG(0), QMA_CAST_ARG(1), QMA_CAST_ARG(2));
		return true;
	}

	virtual bool canInvoke(int* args, int count) const {
		if (count < 3) {
			return false;
		}
		return QMA_CHECK_ARG_TYPE(0) && QMA_CHECK_ARG_TYPE(1) && QMA_CHECK_ARG_TYPE(2);
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

	/** Construct a QtMetacallAdapter which invokes a function object
	 * (eg. std::function or boost::function)
	 */
	template <template <class Signature> class FunctionObject, class Signature>
	QtMetacallAdapter(const FunctionObject<Signature>& t)
	: m_impl(new QtMetacallAdapterImpl<FunctionObject<Signature>,Signature,FunctionTraits<Signature>::count>(t))
	{
	}

	template <class Functor>
	QtMetacallAdapter(Functor f)
	: m_impl(new QtMetacallAdapterImpl<Functor,typename RemovePtr<Functor>::type,
	         FunctionTraits<typename RemovePtr<Functor>::type>::count>(f))
	{
	}

	QtMetacallAdapter(const QtMetacallAdapter& other)
	: m_impl(other.m_impl->clone())
	{}

	QtMetacallAdapter& operator=(const QtMetacallAdapter& other)
	{
		m_impl.reset(other.m_impl->clone());
		return *this;
	}

	/** Attempts to invoke the receiver with a given set of arguments from
	 * a signal invocation.
	 */
	bool invoke(const QGenericArgument* args, int count) const
	{
		return m_impl->invoke(args, count);
	}

	/** Checks whether the receiver an be invoked with a given list of
	 * argument types.  @p args are the argument types returned for
	 * a given type by QMetaType::type()
	 */
	bool canInvoke(int* args, int count) const
	{
		return m_impl->canInvoke(args, count);
	}
	
private:
	QScopedPointer<QtMetacallAdapterImplIface> m_impl;
};

