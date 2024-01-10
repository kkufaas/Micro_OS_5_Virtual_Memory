#ifndef CUSTOM_LIBC_MATH_H
#define CUSTOM_LIBC_MATH_H

#ifndef INFINITY
#define INFINITY (__builtin_inff())
#endif
#ifndef NAN
#define NAN (__builtin_nanf(""))
#endif

/* Many math functions are provided as builtins by GCC.
 * We can simply defer to them. */

#ifndef abs
#define abs(x) __builtin_abs(x)
#endif
#ifndef labs
#define labs(x) __builtin_labs(x)
#endif
#ifndef llabs
#define llabs(x) __builtin_absll(x)
#endif
#ifndef imaxabs
#define imaxabs(x) __builtin_imaxabs(x)
#endif
#ifndef fabs
#define fabs(x) __builtin_fabs(x)
#endif
#ifndef fabsf
#define fabsf(x) __builtin_fabsf(x)
#endif
#ifndef fabsl
#define fabsl(x) __builtin_fabsl(x)
#endif

#ifndef signbit
#define signbit(x) __builtin_signbit(x)
#endif
#ifndef signbitf
#define signbitf(x) __builtin_signbitf(x)
#endif
#ifndef signbitl
#define signbitl(x) __builtin_signbitl(x)
#endif

#ifndef copysign
#define copysign(x, y) __builtin_copysign(x, y)
#endif
#ifndef copysignf
#define copysignf(x, y) __builtin_copysignf(x, y)
#endif
#ifndef copysignl
#define copysignl(x, y) __builtin_copysignl(x, y)
#endif

/* Floating-point classifications */

#define FP_NORMAL    0
#define FP_SUBNORMAL 1
#define FP_ZERO      2
#define FP_INFINITE  3
#define FP_NAN       4

#ifndef fpclassify
#define fpclassify(x) \
    __builtin_fpclassify( \
            FP_NAN, FP_INFINITE, FP_NORMAL, FP_SUBNORMAL, FP_ZERO, x \
    )
#endif

#ifndef iszero
#define iszero(x) (fpclassify(x) == FP_ZERO)
#endif

#endif /* CUSTOM_LIBC_MATH_H */
