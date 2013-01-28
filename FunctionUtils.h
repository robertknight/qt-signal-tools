#pragma once

// include headers that provide function<> and bind()
#ifdef Q_CC_MSVC
#include <functional>
#else
#include <tr1/functional>
#endif

// tests for relevant C++11 features
#if (_MSC_VER >= 1600)
#define COMPILER_SUPPORTS_LAMBDAS
#endif
