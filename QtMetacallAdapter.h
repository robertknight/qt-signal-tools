#pragma once

#include "FunctionTraits.h"
#include "QtCallback.h"

#include <QtCore/QSharedData>
#include <QtCore/QMetaObject>

static const int QTMETACALL_MAX_ARGS = 6;
typedef int QtMetacallArgsArray[QTMETACALL_MAX_ARGS];

namespace QtSignalTools
{

// interface for implementations of QtMetacallAdapter
struct QtMetacallAdapterImplIface : public QSharedData
{
	virtual ~QtMetacallAdapterImplIface() {}
	virtual bool invoke(const QGenericArgument* args, int count) const = 0;
	virtual int getArgTypes(QtMetacallArgsArray args) const  = 0;
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

	virtual int getArgTypes(QtMetacallArgsArray args) const {
		int count = qMin(callback.unboundParameterCount(), QTMETACALL_MAX_ARGS);
		for (int i=0; i < count; i++) {
			args[i] = callback.unboundParameterType(i);
		}
		return count;
	}
};

template <class Functor>
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

	// helper for checking at runtime that the type of a signal
	// argument matches the type of the receiver's corresponding argument
	template <class T>
	static bool argMatch(int argType) {
		return qMetaTypeId<T>() == argType;
	}

	template <class T>
	static int argType() {
		return qMetaTypeId<T>();
	}

	static int fillArgTypes(QtMetacallArgsArray args, int count, ...) {
		va_list list;
		va_start(list, count);
		for (int i=0; i < count; i++) {
			args[i] = va_arg(list, int);
		}
		va_end(list);
		return count;
	}
};

template <class Functor, int ArgCount>
struct QtMetacallAdapterImpl;

// extract the Nth argument from a QGenericArgument array and cast
// to the type expected by the Nth functor parameter
#define QMA_CAST_ARG(N) *reinterpret_cast<typename Base::traits::arg##N##_type*>(args[N].data())

// returns the type of the Nth argument that the receiver expects
#define QMA_ARG_TYPE(N) Base::template argType<typename Base::traits::arg##N##_type>()

// declares an implementation of QtMetacallAdapterImpl for functors
// that take 'argCount' arguments.
//
// 'invokeExpr' is the expression passed to the functor to call it with
// the appropriate args
//
// 'argTypesExpr' is a comma-separated list of argument type IDs for
// the arguments which the receiver expects.
//
// In C++11, this macro could be replaced with variadic templates
#define QMA_DECLARE_ADAPTER_IMPL(argCount, invokeExpr, argTypesExpr) \
  template <class Functor> \
  struct QtMetacallAdapterImpl<Functor,argCount> \
   : QtMetacallAdapterImplBase<Functor> \
  { \
    typedef QtMetacallAdapterImplBase<Functor> Base;\
    QtMetacallAdapterImpl(const Functor& functor) : Base(functor) {} \
    virtual bool invoke(const QGenericArgument* args, int count) const { \
	  (void)args;\
	  if (count < argCount) {\
	    return false; \
	  }\
	  Base::functor(invokeExpr);\
	  return true;\
	}\
	virtual int getArgTypes(QtMetacallArgsArray args) const {\
		(void)args;\
		return Base::fillArgTypes(args, argCount, argTypesExpr);\
	}\
  };

QMA_DECLARE_ADAPTER_IMPL(0,/* empty */, 0 /* empty */)

QMA_DECLARE_ADAPTER_IMPL(1,
  QMA_CAST_ARG(0),
  QMA_ARG_TYPE(0)
)

#define QMA_COMMA ,

QMA_DECLARE_ADAPTER_IMPL(2,
  QMA_CAST_ARG(0) QMA_COMMA QMA_CAST_ARG(1),
  QMA_ARG_TYPE(0) QMA_COMMA QMA_ARG_TYPE(1)
)

QMA_DECLARE_ADAPTER_IMPL(3,
  QMA_CAST_ARG(0) QMA_COMMA QMA_CAST_ARG(1) QMA_COMMA QMA_CAST_ARG(2),
  QMA_ARG_TYPE(0) QMA_COMMA QMA_ARG_TYPE(1) QMA_COMMA QMA_ARG_TYPE(2)
)
	
QMA_DECLARE_ADAPTER_IMPL(4,
  QMA_CAST_ARG(0) QMA_COMMA QMA_CAST_ARG(1) QMA_COMMA QMA_CAST_ARG(2) QMA_COMMA QMA_CAST_ARG(3),
  QMA_ARG_TYPE(0) QMA_COMMA QMA_ARG_TYPE(1) QMA_COMMA QMA_ARG_TYPE(2) QMA_COMMA QMA_ARG_TYPE(3)
)

QMA_DECLARE_ADAPTER_IMPL(5,
  QMA_CAST_ARG(0) QMA_COMMA QMA_CAST_ARG(1) QMA_COMMA QMA_CAST_ARG(2) QMA_COMMA QMA_CAST_ARG(3) QMA_COMMA QMA_CAST_ARG(4),
  QMA_ARG_TYPE(0) QMA_COMMA QMA_ARG_TYPE(1) QMA_COMMA QMA_ARG_TYPE(2) QMA_COMMA QMA_ARG_TYPE(3) QMA_COMMA QMA_ARG_TYPE(4)
)

}

/** A wrapper around either a QtCallback or a function object (eg.
 * std::tr1::function, boost::function, a C++11 lambda)
 * which can invoke the function given an array of QGenericArgument objects.
 */
class QtMetacallAdapter
{
public:
	QtMetacallAdapter()
	{}

	QtMetacallAdapter(const QtCallback& callback)
	: m_impl(new QtSignalTools::QtCallbackImpl(callback))
	{}

	/** Construct a QtMetacallAdapter which invokes a function object
	 * (eg. std::function or boost::function)
	 */
	template <template <class Signature> class FunctionObject, class Signature>
	QtMetacallAdapter(const FunctionObject<Signature>& t)
	: m_impl(new QtSignalTools::QtMetacallAdapterImpl<FunctionObject<Signature>,QtSignalTools::FunctionTraits<Signature>::count>(t))
	{
	}

	/** Construct a QtMetacallAdapter which invokes a plain function */
	template <class Functor>
	QtMetacallAdapter(Functor f)
	: m_impl(new QtSignalTools::QtMetacallAdapterImpl<Functor, QtSignalTools::FunctionTraits<typename QtSignalTools::ExtractSignature<Functor>::type>::count>(f))
	{
	}

	QtMetacallAdapter(const QtMetacallAdapter& other)
	: m_impl(other.m_impl)
	{}

	/** Attempts to invoke the receiver with a given set of arguments from
	 * a signal invocation.
	 */
	bool invoke(const QGenericArgument* args, int count) const
	{
		return m_impl->invoke(args, count);
	}

	/** Retrieves the count and types of arguments expected by the receiver */
	int getArgTypes(QtMetacallArgsArray args) const
	{
		return m_impl->getArgTypes(args);
	}
	
private:
	QSharedDataPointer<QtSignalTools::QtMetacallAdapterImplIface> m_impl;
};

