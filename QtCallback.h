#pragma once

#include <QtCore/QMetaMethod>
#include <QtCore/QMetaType>
#include <QtCore/QPointer>
#include <QtCore/QSharedData>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVarLengthArray>
#include <QtCore/QVariant>
#include <QtCore/QWeakPointer>

/** QtCallbackBase is an object which stores the receiver, method name
 * and optionally, some of the arguments for a Qt signal or slot function call.
 *
 * The method can then be invoked on the receiver using the invokeWithArgs() method.
 *
 * This is conceptually similar to std::function and std::bind except:
 *
 *  - If the receiver is destroyed before the bound function is invoked, attempting to
 *    call the method has no effect.
 *  - Type matches between the supplied arguments and those expected by the method
 *    are not checked until the call is made.
 *
 * QtCallbackBase can be invoked with any number of arguments and the types are not
 * checked at compile time.  The QtCallback<N> subclasses provide function objects
 * which take N arguments that are passed to the method.
 *
 * Example Usage:
 *
 *   QtCallbackBase callback(myLabel, SLOT(setText(QString)));
 *   callback.invokeWithArgs(Q_ARG(QString, "Hello World"))
 *
 * Or using QtCallback:
 *
 *   QtCallback1<QString> callback(myLabel, SLOT(setText(QString)));
 *   callback("Hello World");
 */
class QtCallbackBase
{
	public:
		QtCallbackBase();

		/** Constructs a callback which will call @p method on @p receiver.
		 */
		QtCallbackBase(QObject* receiver, const char* method);

		QtCallbackBase(const QtCallbackBase& other);

		/** Bind the @p index'th parameter in the method to call to @p value. */
		void bind(int index, const QVariant& value);

		/** Bind the next currently unbound parameter in the method to call to @p value. */
		void bind(const QVariant& value);

		/** Attempt to invoke the stored method on the receiver.
		 * Returns false if the types do not match or the receiver has been destroyed.
		 */
		bool invokeWithArgs(const QGenericArgument& arg1 = QGenericArgument(),
		                    const QGenericArgument& arg2 = QGenericArgument(),
					        const QGenericArgument& arg3 = QGenericArgument(),
							const QGenericArgument& arg4 = QGenericArgument(),
							const QGenericArgument& arg5 = QGenericArgument(),
							const QGenericArgument& arg6 = QGenericArgument()) const;

		/** Returns the number of parameters that the bound method has. */
		int parameterCount() const;

		/** Returns the Qt type ID of the @p index'th parameter to the bound method. */
		int parameterType(int index) const;

		/** Returns the number of parameters to the method which have not been bound
		 * with bind()
		 */
		int unboundParameterCount() const;

		/** Returns the Qt type ID of the @p index'th parameter to the method
		 * which has not yet been bound.
		 */
		int unboundParameterType(int index) const;

		/** Returns true if the @p index'th argument to the bound method
		 * has been set using bind()
		 */
		bool isBound(int index) const;

	private:
		struct Data : public QSharedData
		{
			struct Arg
			{
				int position;
				QVariant value;
			};

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
			QPointer<QObject> receiver;
#else
			// QWeakPointer is much more efficient under Qt 4
			// when large numbers of weak pointers exist
			QWeakPointer<QObject> receiver;
#endif
			QMetaMethod method;
			QVarLengthArray<Arg,5> args;
		};
		QSharedDataPointer<Data> d;
};

template <class T>
QGenericArgument makeQtArg(const T& arg)
{
	return QGenericArgument(QMetaType::typeName(qMetaTypeId<T>()), &arg);
}

/** Declare a QtCallback<> variant with |typeCount| type arguments called
 *  QtCallback<typeCount>
 */
#define DECLARE_QTCALLBACK_TEMPLATE(typeCount, typeList, argList, makeQtArgList) \
	typeList \
	class QtCallback ## typeCount : public QtCallbackBase \
	{ \
		public:\
			   QtCallback ## typeCount() {}\
			   QtCallback ## typeCount(QObject* receiver, const char* method)\
				: QtCallbackBase(receiver,method) {} \
			   bool invoke(argList) const \
			   { return invokeWithArgs(makeQtArgList); } \
			   bool operator()(argList) const \
			   { return invokeWithArgs(makeQtArgList); } \
			   using QtCallbackBase::bind; \
			   template <class T> \
		       QtCallback ## typeCount& bind(const T& value) \
			   { bind(QVariant::fromValue(value)); return *this; } \
    }

#define MACRO_COMMA ,

DECLARE_QTCALLBACK_TEMPLATE(,,,);
DECLARE_QTCALLBACK_TEMPLATE(1, template<class T1>, const T1& arg1, makeQtArg(arg1));
DECLARE_QTCALLBACK_TEMPLATE(2, template<class T1 MACRO_COMMA class T2>,
                 const T1& arg1 MACRO_COMMA const T2& arg2,
                 makeQtArg(arg1) MACRO_COMMA makeQtArg(arg2));
DECLARE_QTCALLBACK_TEMPLATE(3, template<class T1 MACRO_COMMA class T2 MACRO_COMMA class T3>,
                 const T1& arg1 MACRO_COMMA const T2& arg2 MACRO_COMMA const T3& arg3,
                 makeQtArg(arg1) MACRO_COMMA makeQtArg(arg2) MACRO_COMMA makeQtArg(arg3));
DECLARE_QTCALLBACK_TEMPLATE(4, template<class T1 MACRO_COMMA class T2 MACRO_COMMA class T3 MACRO_COMMA class T4>,
                 const T1& arg1 MACRO_COMMA const T2& arg2 MACRO_COMMA const T3& arg3 MACRO_COMMA const T4& arg4,
                 makeQtArg(arg1) MACRO_COMMA makeQtArg(arg2) MACRO_COMMA makeQtArg(arg3) MACRO_COMMA makeQtArg(arg4));

