
#define NOINIT __attribute__ ((section (".noinit")))
#define UNUSED(X) ((void)(X))

#if defined(__GNUC__) && (__GNUC__ > 2)
#define likely(expr) (__builtin_expect((expr), 1))
#define unlikely(expr) (__builtin_expect((expr), 0))
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif

#define ISR(VECTOR) extern "C" void VECTOR()

#define USART_BRR(F_BUS, F_UART) ((F_BUS) / (F_UART))

#define sign(X) ((X) < 0 ? -1 : 1)
