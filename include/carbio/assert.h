/**********************************************************************
 * Project   : Vehicle access control through biometric
 *             authentication
 * Author    : Rajmund Kail
 * Institute : Óbuda University
 * Faculty   : John von Neumann Faculty of Informatics
 * Dept.     : Computer Science Engineering
 * Year      : 2025
 *
 * Description:
 *   This software was developed as part of a thesis project at
 *   Óbuda University – John von Neumann Faculty of Informatics.
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

#ifndef CARBIO_ASSERT_H
#define CARBIO_ASSERT_H

#include <cstdarg>

/*!
 * @brief Stop debugger immediately at call site.
 */
#ifndef ct_debug_break
#if defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define ct_debug_break() (__builtin_debugtrap())
#else
#define ct_debug_break() (raise(SIGTRAP))
#endif
#else
#define ct_debug_break() (__builtin_trap())
#endif
#endif

/*!
 * @brief Perform invalid memory write which result in exception on most platforms.
 */
#ifndef ct_crash
#define ct_crash() *(volatile int *)(0) = 0
#endif

/*!
 * @brief Define ct_assertion_enabled / ct_panic_enabled macros if undefined
 * Control assertion macros in production and non-production environments.
 */
#if !defined(NDEBUG)
#ifndef ct_assertion_enabled
#define ct_assertion_enabled 1
#endif
#ifndef ct_panic_enabled
#define ct_panic_enabled 1
#endif
#else
#ifndef ct_assertion_enabled
#define ct_assertion_enabled 0
#endif
#ifndef ct_panic_enabled
#define ct_panic_enabled 1
#endif
#endif

/*!
 * @brief Perform assertions on runtime conditions.
 * Assertions only taking action in debug builds.
 */
#if defined(ct_assertion_enabled)
#include <csignal>
#include <cstdio>
#ifndef ct_assert
#define ct_assert(expr)                                                      \
  do                                                                         \
  {                                                                          \
    if (!(expr)) [[unlikely]]                                                \
    {                                                                        \
      printf("condition(%s) failed in %s(%d)\n", #expr, __FILE__, __LINE__); \
      ct_debug_break();                                                      \
    }                                                                        \
    else                                                                     \
    {                                                                        \
      ((void)0);                                                             \
    }                                                                        \
  } while (0)
#endif // ct_assert
#ifndef ct_assert_msg
#define ct_assert_msg(expr, msg)                                                      \
  do                                                                                  \
  {                                                                                   \
    if (!(expr)) [[unlikely]]                                                         \
    {                                                                                 \
      printf("condition(%s) failed in %s(%d): %s\n", #expr, __FILE__, __LINE__, msg); \
      ct_debug_break();                                                               \
    }                                                                                 \
    else                                                                              \
    {                                                                                 \
      ((void)0);                                                                      \
    }                                                                                 \
  } while (0)
#endif // ct_assert_msg
#ifndef ct_fail
#define ct_fail()                                     \
  do                                                  \
  {                                                   \
    printf("failed in %s(%d)\n", __FILE__, __LINE__); \
    ct_debug_break();                                 \
  } while (0)
#endif // ct_fail
#ifndef ct_fail_msg
#define ct_fail_msg(msg)                                       \
  do                                                           \
  {                                                            \
    printf("failed in %s(%d): %s\n", __FILE__, __LINE__, msg); \
    ct_debug_break();                                          \
  } while (0)
#endif // ct_fail_msg
#else
#define pt_assert(expr)          ((void)0)
#define pt_assert_msg(expr, msg) ((void)0)
#endif

/*!
 * @brief Perform conditional panic check.
 * Panic immediately stop program execution if constraint is broken.
 */
#if defined(ct_panic_enabled)
#include <cstdio>
#ifndef ct_panic
#define ct_panic(expr)                                                       \
  do                                                                         \
  {                                                                          \
    if (!(expr)) [[unlikely]]                                                \
    {                                                                        \
      printf("condition(%s) failed in %s(%d)\n", #expr, __FILE__, __LINE__); \
      ct_debug_break();                                                      \
    }                                                                        \
    else                                                                     \
    {                                                                        \
      ((void)0);                                                             \
    }                                                                        \
  } while (0)
#endif
#ifndef ct_panic_msg
#define ct_panic_msg(expr, msg)                                                       \
  do                                                                                  \
  {                                                                                   \
    if (!(expr)) [[unlikely]]                                                         \
    {                                                                                 \
      printf("condition(%s) failed in %s(%d): %s\n", #expr, __FILE__, __LINE__, msg); \
      ct_debug_break();                                                               \
    }                                                                                 \
    else                                                                              \
    {                                                                                 \
      ((void)0);                                                                      \
    }                                                                                 \
  } while (0)
#endif
#else
#ifndef ct_panic
#define ct_panic(expr) ((void)0)
#endif
#ifndef ct_panic_msg
#define ct_panic_msg(expr, msg) ((void)0)
#endif
#endif

#endif // CARBIO_ASSERT_H
