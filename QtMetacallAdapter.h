#pragma once

#include "QtCallback.h"

#include <QtCore/QMetaObject>
#include <QtCore/QScopedPointer>

// extract the argument count and types
// from a function signature
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

// extract the function signature from an input type T,
// where T may be a function pointer or function object
template <class T>
struct ExtractSignature
{
	typedef T type;
};

template <class T>
struct ExtractSignature<T*>
{
	typedef T type;
};

template <template <class S> class Function, class S>
struct ExtractSignature<Function<S> >
{
	typedef S type;
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
	// 'traits' type provides access to the number and types of
	// the functor's arguments
	typedef FunctionTraits<typename ExtractSignature<Functor>::type> traits;

	// the function pointer or function object to invoke
	Functor functor;

	QtMetacallAdapterImplBase(const Functor& _f)
	: functor(_f)
	{}

	virtual QtMetacallAdapterImplIface* clone() const {
		return new Derived(functor);
	}

	// helper for checking at runtime that the type of a signal
	// argument matches the type of the receiver's corresponding argument
	template <class T>
	static bool argMatch(int argType) {
		return qMetaTypeId<T>() == argType;
	}
};

template <class Functor, int ArgCount>
struct QtMetacallAdapterImpl;

// extract the Nth argument from a QGenericArgument array and cast
// to the type expected by the Nth functor parameter
#define QMA_CAST_ARG(N) *reinterpret_cast<typename Base::traits::arg##N##_type*>(args[N].data())

// check that the Nth argument type from a QGenericArgument array matches
// the Nth parameter type expected by the functor
#define QMA_CHECK_ARG_TYPE(N) Base::template argMatch<typename Base::traits::arg##N##_type>(args[N])

// declares an implementation of QtMetacallAdapterImpl for functors
// that take 'argCount' arguments.
//
// 'invokeExpr' is the expression passed to the functor to call it with
// the appropriate args
//
// 'checkExpr' is the expression which verifies that the argument types
// for a call match the receiver's types
//
// In C++11, this macro could be replaced with variadic templates
#define QMA_DECLARE_ADAPTER_IMPL(argCount, invokeExpr, checkExpr) \
  template <class Functor> \
  struct QtMetacallAdapterImpl<Functor,argCount> \
   : QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,argCount> > \
  { \
    typedef QtMetacallAdapterImplBase<Functor,QtMetacallAdapterImpl<Functor,argCount> > Base;\
    QtMetacallAdapterImpl(const Functor& functor) : Base(functor) {} \
    virtual bool invoke(const QGenericArgument* args, int count) const { \
	  (void)args;\
	  if (count < argCount) {\
	    return false; \
	  }\
	  Base::functor(invokeExpr);\
	  return true;\
	}\
    virtual bool canInvoke(int* args,int count) const {\
	  (void)args;\
	  if (count < argCount) {\
	    return false; \
	  }\
	  return checkExpr;\
	}\
  };

QMA_DECLARE_ADAPTER_IMPL(0,/* empty */, true /* can always invoke functor */)

QMA_DECLARE_ADAPTER_IMPL(1,
  QMA_CAST_ARG(0),
  QMA_CHECK_ARG_TYPE(0)
)

QMA_DECLARE_ADAPTER_IMPL(2,
  (QMA_CAST_ARG(0), QMA_CAST_ARG(1)),
  (QMA_CHECK_ARG_TYPE(0) && QMA_CHECK_ARG_TYPE(1))
)

QMA_DECLARE_ADAPTER_IMPL(3,
  (QMA_CAST_ARG(0), QMA_CAST_ARG(1), QMA_CAST_ARG(2)),
  (QMA_CHECK_ARG_TYPE(0) && QMA_CHECK_ARG_TYPE(1) && QMA_CHECK_ARG_TYPE(2))
)
	
QMA_DECLARE_ADAPTER_IMPL(4,
  (QMA_CAST_ARG(0), QMA_CAST_ARG(1), QMA_CAST_ARG(2), QMA_CAST_ARG(3)),
  (QMA_CHECK_ARG_TYPE(0) && QMA_CHECK_ARG_TYPE(1) && QMA_CHECK_ARG_TYPE(2) && QMA_CHECK_ARG_TYPE(3))
)

QMA_DECLARE_ADAPTER_IMPL(5,
  (QMA_CAST_ARG(0), QMA_CAST_ARG(1), QMA_CAST_ARG(2), QMA_CAST_ARG(3), QMA_CAST_ARG(4)),
  (QMA_CHECK_ARG_TYPE(0) && QMA_CHECK_ARG_TYPE(1) && QMA_CHECK_ARG_TYPE(2) && QMA_CHECK_ARG_TYPE(3) && QMA_CHECK_ARG_TYPE(4))
)

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
	: m_impl(new QtMetacallAdapterImpl<FunctionObject<Signature>,FunctionTraits<Signature>::count>(t))
	{
	}

	/** Construct a QtMetacallAdapter which invokes a plain function */
	template <class Functor>
	QtMetacallAdapter(Functor f)
	: m_impl(new QtMetacallAdapterImpl<Functor, FunctionTraits<typename ExtractSignature<Functor>::type>::count>(f))
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

