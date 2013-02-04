#pragma once

// tests for relevant C++11 features

// Visual C++
#if (_MSC_VER >= 1600)
#define QST_COMPILER_SUPPORTS_LAMBDAS
#define QST_COMPILER_SUPPORTS_DECLTYPE
#endif

// GCC
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 5) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#define QST_COMPILER_SUPPORTS_LAMBDAS
#define QST_COMPILER_SUPPORTS_DECLTYPE
#endif

// Clang
#if __clang__
#if __has_feature(cxx_lambdas)
#define QST_COMPILER_SUPPORTS_LAMBDAS
#define QST_COMPILER_SUPPORTS_DECLTYPE
#endif
#endif

#ifdef QST_COMPILER_SUPPORTS_LAMBDAS
// sets whether the C++11 standard libraries should
// be used.  If not, we fall back to the TR1 versions.
// A similar define could also be used to use Boost instead.
#define QST_USE_CPP11_LIBS
#endif

// include headers that provide function<> and bind()
#if defined(_MSC_VER) || defined(QST_USE_CPP11_LIBS)
#include <functional>
#include <memory>
#include <type_traits>

#ifndef _MSC_VER
// even under C++11, we are still using the TR1 version
// of result_of<>
#include <tr1/functional>
#endif

#else
#include <tr1/functional>
#include <tr1/memory>
#include <tr1/type_traits>
#endif

#define QST_COMMA ,

namespace QtSignalTools
{

// enable_if is actually provided by all
// supported compilers, but is under an extension
// namespace under GCC
template <bool, class T>
struct enable_if;

template <class T>
struct enable_if<true,T>
{
	typedef T type;
};

#if defined(QST_USE_CPP11_LIBS)
using std::is_base_of;
using std::shared_ptr;
#else
using std::tr1::is_base_of;
using std::tr1::shared_ptr;
#endif

}

