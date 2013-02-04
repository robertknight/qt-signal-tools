#pragma once

#include "FunctionUtils.h"

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

template <class R, class T1, class T2, class T3, class T4>
struct FunctionTraits<R(T1,T2,T3,T4)> : FunctionTraits<R(T1,T2,T3)>
{
	enum { count = 4 };
	typedef T4 arg3_type;
};

template <class R, class T1, class T2, class T3, class T4, class T5>
struct FunctionTraits<R(T1,T2,T3,T4,T5)> : FunctionTraits<R(T1,T2,T3,T4)>
{
	enum { count = 5 };
	typedef T5 arg4_type;
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

template <class Signature, class Class>
struct ExtractSignature<Signature Class::*>
{
	typedef Signature type;
};

template <class MemberFunc>
struct MemberFuncResultType
{
#ifdef QST_COMPILER_SUPPORTS_DECLTYPE
	typedef decltype(QtSignalTools::mem_fn(static_cast<MemberFunc>(0))) MemFnType;
#else
	typedef __typeof__(QtSignalTools::mem_fn(static_cast<MemberFunc>(0))) MemFnType;
#endif
	typedef typename MemFnType::result_type type;
};


