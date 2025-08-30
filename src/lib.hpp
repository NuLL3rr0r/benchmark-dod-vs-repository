/**
 * Copyright (c) 2025 Mamadou Babaei
 *
 * Author: Mamadou Babaei <info@babaei.net>
 *
 */


#pragma once

/*******************************************************************************
* Include directives
*******************************************************************************/

#include <chrono>
#include <cstddef>

/*******************************************************************************
* Macros
*******************************************************************************/

#if defined(__clang__)
#define COMPILER_CLANG 1
#else   /* defined(__clang__) */
#define COMPILER_CLANG 0
#endif  /* defined(__clang__) */

#if defined(__GNUC__) && !defined(__clang__)
#define COMPILER_GCC 1
#else   /* defined(__GNUC__) && !defined(__clang__) */
#define COMPILER_GCC 0
#endif  /* defined(__GNUC__) && !defined(__clang__) */

#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#else   /* defined(_MSC_VER) */
#define COMPILER_MSVC 0
#endif  /* defined(_MSC_VER) */

#ifndef HAS_CPP_ATTR
#if defined(__has_cpp_attribute)
#define HAS_CPP_ATTR(x) __has_cpp_attribute(x)
#else   /* defined(__has_cpp_attribute) */
#define HAS_CPP_ATTR(x) 0
#endif  /* defined(__has_cpp_attribute) */
#endif  /* HAS_CPP_ATTR */

#if COMPILER_MSVC
#if HAS_CPP_ATTR(msvc::noinline)
#define FORCE_NOINLINE [[msvc::noinline]]
#else   /* HAS_CPP_ATTR(msvc::noinline) */
#define FORCE_NOINLINE __declspec(noinline)
#endif  /* HAS_CPP_ATTR(msvc::noinline) */
#elif COMPILER_CLANG || COMPILER_GCC
#if HAS_CPP_ATTR(gnu::noinline)
#define FORCE_NOINLINE [[gnu::noinline]]
#else   /* HAS_CPP_ATTR(gnu::noinline) */
#define FORCE_NOINLINE __attribute__((noinline))
#endif  /* HAS_CPP_ATTR(gnu::noinline) */
#else   /* COMPILER_MSVC */
#define FORCE_NOINLINE
#endif  /* COMPILER_MSVC */

#if COMPILER_MSVC
#define RESTRICT_ALIAS __restrict
#elif COMPILER_CLANG || COMPILER_GCC
#define RESTRICT_ALIAS __restrict__
#else   /* COMPILER_MSVC */
#define RESTRICT_ALIAS
#endif  /* COMPILER_MSVC */

/*******************************************************************************
* Templates
*******************************************************************************/

template <class F>
double MeasureExecutionTime(const std::size_t iterations, F&& f) {
    const std::chrono::time_point<std::chrono::steady_clock> start{
        std::chrono::steady_clock::now()
    };

    volatile float sink = 0.0f;
    for (std::size_t i = 0; i < iterations; ++i) {
        sink = f();
    }

    const std::chrono::time_point<std::chrono::steady_clock> end{
        std::chrono::steady_clock::now()
    };

    (void)sink;

    const double time = std::chrono::duration<double>(end - start).count();
    return time;
}
