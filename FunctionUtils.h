#pragma once

// include headers that provide function<> and bind()
#ifdef Q_CC_MSVC
#include <functional>
#include <memory>
#include <type_traits>
#else
#include <tr1/functional>
#include <tr1/memory>
#include <tr1/type_traits>
#endif

// tests for relevant C++11 features

// Visual C++
#if (_MSC_VER >= 1600)
#define COMPILER_SUPPORTS_LAMBDAS
#endif

// GCC
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 5) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#define COMPILER_SUPPORTS_LAMBDAS
#endif

// Clang
#if __clang__
#if __has_feature(cxx_lambdas)
#define COMPILER_SUPPORTS_LAMBDAS
#endif
#endif

#define QST_COMMA ,

template <bool, class T>
struct enable_if;

template <class T>
struct enable_if<true,T>
{
	typedef T type;
};

