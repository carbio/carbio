/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * License:
 *   Permission is hereby granted, free of charge, to any person
 *   obtaining a copy of this software and associated documentation
 *   files (the "Software"), to deal in the Software without
 *   restriction, including without limitation the rights to use,
 *   copy, modify, merge, publish, distribute, sublicense, and/or
 *   sell copies of the Software, subject to the following
 *   conditions:
 *
 *   The above copyright notice and this permission notice shall
 *   be included in all copies or substantial portions of the
 *   Software.
 *
 * Disclaimer:
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *   WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *   PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *   OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/

#pragma once

#include <csignal>
#include <cstdarg>
#include <cstdio>

#if defined(NDEBUG)
#ifndef CARBIO_ASSERTION_ENABLED
#define CARBIO_ASSERTION_ENABLED (0)
#endif
#ifndef CARBIO_PANIC_ENABLED
#define CARBIO_PANIC_ENABLED (0)
#endif
#else
#ifndef CARBIO_ASSERTION_ENABLED
#define CARBIO_ASSERTION_ENABLED (1)
#endif
#ifndef CARBIO_PANIC_ENABLED
#define CARBIO_PANIC_ENABLED (1)
#endif
#endif

/*!
 * @brief Stop debugger immediately at call site.
 *
 * This function immediately stops debugger at call site.
 * It is implemented as a macro in order to print the actual call site
 * in the debugger instead of a physical function.
 *
 * @par Example
 * @code{.c}
 * // Stop here when debugging
 * stop_debugger_at()
 * @endcode
 */
#ifndef CARBIO_STOP_DEBUGGER_AT
#if defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define CARBIO_STOP_DEBUGGER_AT() (__builtin_debugtrap())
#endif
#elif defined(__arch64__) && defined(__GNUC__)
#define CARBIO_STOP_DEBUGGER_AT() asm("brk 10")
#elif defined(__arm__) && defined(__GNUC__)
#define CARBIO_STOP_DEBUGGER_AT() asm("bkpt 10")
#else
#define CARBIO_STOP_DEBUGGER_AT() *static_cast<int *>(0) = 0
#endif
#endif

/*!
 * @brief Perform invalid memory write which result in exception on most
 * platforms.
 */
#ifndef CARBIO_FORCE_CRASH_AT
#define CARBIO_FORCE_CRASH_AT() *static_cast<volatile int *>(0) = 0
#endif

#if CARBIO_ASSERTION_ENABLED
/*!
 * @brief Perform runtime assertions for non-release builds.
 *
 * When an asserted condition is unmet, stop_debugger_at is called.
 *
 * @param expression A logical expression. Debug assertion fails when this
 * logical expression is evaluated as false.
 */
#ifndef CARBIO_ASSERT
#define CARBIO_ASSERT(expression)                                              \
  do {                                                                         \
    if (!(expression)) [[unlikely]] {                                          \
      std::printf("runtime assertion (%s) failed in %s(%d)\n", #expression,    \
                  __FILE__, __LINE__);                                         \
      CARBIO_STOP_DEBUGGER_AT();                                               \
    } else {                                                                   \
      ((void)0);                                                               \
    }                                                                          \
  } while (0)
#endif /* CARBIO_ASSERT(x) */
/*!
 * @brief Perform runtime assertions for non-release builds.
 *
 * When an asserted condition is unmet, stop_debugger_at is called.
 *
 * @param expression A logical expression. Debug assertion fails when this
 * logical expression is evaluated as false.
 * @param message The message to show when assertion fails.
 */
#ifndef CARBIO_ASSERT_MSG
#define CARBIO_ASSERT_MSG(expression, message)                                 \
  do {                                                                         \
    if (!(expression)) [[unlikely]] {                                          \
      std::printf("runtime assertion (%s) failed in %s(%d): %s\n",             \
                  #expression, __FILE__, __LINE__, message);                   \
      CARBIO_STOP_DEBUGGER_AT();                                               \
    } else {                                                                   \
      ((void)0);                                                               \
    }                                                                          \
  } while (0)
#endif /* CARBIO_ASSERT_MSG(x, m) */
/*!
 * @brief Cause unconditional failure at call site.
 * This function simply stops debugger at call site.
 */
#ifndef CARBIO_FAIL
#define CARBIO_FAIL()                                                          \
  do {                                                                         \
    std::printf("runtime failure in %s(%d)\n", __FILE__, __LINE__);            \
    CARBIO_STOP_DEBUGGER_AT();                                                 \
  } while (0)
#endif /* CARBIO_FAIL() */
/*!
 * @brief Cause unconditional runtime failure at call site.
 * This function simply stops debugger at call site.
 * You can specify extra "log" message along with failure.
 * @param message The message to show at the call site.
 */
#ifndef CARBIO_FAIL_MSG
#define CARBIO_FAIL_MSG(message)                                               \
  do {                                                                         \
    std::printf("runtime failure in %s(%d): %s\n", __FILE__, __LINE__,         \
                message);                                                      \
    CARBIO_STOP_DEBUGGER_AT();                                                 \
  } while (0)
#endif /* CARBIO_FAIL_MSG(message) */
#else  /* !CARBIO_ASSERTION_ENABLED */
#ifndef CARBIO_ASSERT
#define CARBIO_ASSERT(expression) ((void)0)
#endif /* CARBIO_ASSERT(expression) */
#ifndef CARBIO_ASSERT_MSG
#define CARBIO_ASSERT_MSG(expression, message) ((void)0)
#endif /* CARBIO_ASSERT_MSG(expression, message) */
#ifndef CARBIO_FAIL
#define CARBIO_FAIL() ((void)0)
#endif /* CARBIO_FAIL() */
#ifndef CARBIO_FAIL_MSG
#define CARBIO_FAIL_MSG(message) ((void)0)
#endif /* CARBIO_FAIL_MSG(message) */
#endif /* !CARBIO_ASSERTION_ENABLED */

#if CARBIO_PANIC_ENABLED
/*!
 * @brief Perform runtime panic checking in release builds.
 *
 * The difference between assertions and panics are minor; panic fires
 * force_crash_at() function at call site instead of stop_debugger_at().
 *
 * This ensures forced crash on release builds when critical condition or
 * requirement is unmet since regular assertions are disabled for release
 * builds.
 *
 * @param expression A logical expression. Panic occur when this
 * logical expression is evaluated as false.
 */
#ifndef CARBIO_PANIC
#define CARBIO_PANIC(expression)                                               \
  do {                                                                         \
    if (!(expression)) [[unlikely]] {                                          \
      std::printf("runtime panic(%s) failed in %s(%d)\n", #expression,         \
                  __FILE__, __LINE__);                                         \
      CARBIO_FORCE_CRASH_AT();                                                 \
    } else {                                                                   \
      ((void)0);                                                               \
    }                                                                          \
  } while (0)
#endif /* CARBIO_PANIC */
/*!
 * @brief Perform runtime panic checking in release builds.
 *
 * The difference between assertions and panics are minor; panic fires
 * force_crash_at() function at call site instead of stop_debugger_at().
 *
 * This ensures forced crash on release builds when critical condition or
 * requirement is unmet since regular assertions are disabled for release
 * builds.
 *
 * @param expression A logical expression. Panic occur when this
 * logical expression is evaluated as false.
 * @param message The message to show when panic fails.
 */
#ifndef CARBIO_PANIC_MSG
#define CARBIO_PANIC_MSG(expression, message)                                  \
  do {                                                                         \
    if (!(expression)) [[unlikely]] {                                          \
      std::printf("runtime panic(%s) failed in %s(%d): %s\n", #expression,     \
                  __FILE__, __LINE__, message);                                \
      CARBIO_FORCE_CRASH_AT();                                                 \
    } else {                                                                   \
      ((void)0);                                                               \
    }                                                                          \
  } while (0)
#endif /* CARBIO_PANIC_MSG */
#else  /* !CARBIO_PANIC_ENABLED */
#ifndef CARBIO_PANIC
#define CARBIO_PANIC(expression) ((void)0)
#endif /* CARBIO_PANIC(expression) */
#ifndef CARBIO_PANIC_MSG
#define CARBIO_PANIC_MSG(expression, message) ((void)0)
#endif /* CARBIO_PANIC_MSG(expression, message) */
#endif /* !CARBIO_PANIC_ENABLED */
