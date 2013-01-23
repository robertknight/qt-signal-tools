#pragma once

#include <QtCore/QMetaMethod>
#include <QtCore/QMetaType>
#include <QtCore/QPointer>
#include <QtCore/QSharedData>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QVarLengthArray>
#include <QtCore/QVariant>

class QtCallbackBase
{
	public:
		QtCallbackBase();
		QtCallbackBase(QObject* receiver, const char* method);
		QtCallbackBase(const QtCallbackBase& other);

		void bind(int index, const QVariant& value);
		void bind(const QVariant& value);

		bool invokeWithArgs(const QGenericArgument& arg1 = QGenericArgument(),
		                    const QGenericArgument& arg2 = QGenericArgument(),
					        const QGenericArgument& arg3 = QGenericArgument(),
							const QGenericArgument& arg4 = QGenericArgument(),
							const QGenericArgument& arg5 = QGenericArgument(),
							const QGenericArgument& arg6 = QGenericArgument()) const;

		int parameterCount() const;

	private:
		struct Data : public QSharedData
		{
			struct Arg
			{
				int position;
				QVariant value;
			};
			QPointer<QObject> receiver;
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

