#pragma once

// include headers that provide function<> and bind()
#ifdef Q_CC_MSVC
#include <functional>
#else
#include <tr1/functional>
#endif

// tests for relevant C++11 features

// Visual C++
#if (_MSC_VER >= 1600)
#define QST_COMPILER_SUPPORTS_LAMBDAS
#endif

// GCC
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 5) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#define QST_COMPILER_SUPPORTS_LAMBDAS
#endif

// Clang
#if __clang__
#if __has_feature(cxx_lambdas)
#define QST_COMPILER_SUPPORTS_LAMBDAS
#endif
#endif


