﻿// Implementation of newlib syscall

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#undef errno
extern int errno;
extern int  _end;

/*This function is used for handle heap option*/
__attribute__((used))
caddr_t _sbrk(int incr)
{
	static unsigned char *heap = NULL;
	unsigned char *prev_heap;

	if (heap == NULL)
	{
		heap = (unsigned char *)&_end;
	}
	prev_heap = heap;

	heap += incr;

	return (caddr_t)prev_heap;
}

__attribute__((used))
int link(char *old, char *new)
{
	(void)old;
	(void)new;
	return -1;
}

__attribute__((used))
int _close(int file)
{
	(void)file;

	return -1;	// почему не 0?
}

__attribute__((used))
int _fstat(int file, struct stat *st)
{
	(void)file;

	st->st_mode = S_IFCHR;
	return 0;
}

__attribute__((used))
int _isatty(int file)
{
	(void)file;

	return 1;
}

__attribute__((used))
int _lseek(int file, int ptr, int dir)
{
	(void)file;
	(void)ptr;
	(void)dir;

	return 0;
}

/*Low layer read(input) function*/
__attribute__((used))
int _read(int file, char *ptr, int len)
{
	(void)file;
	(void)ptr;
	(void)len;

#if 0
	//user code example
	int i;
	(void)file;

	for (i = 0; i < len; i++)
	{
		// UART_GetChar is user's basic input function
		*ptr++ = UART_GetChar();
	}

#endif

	return len;
}

void RTUUartTx(size_t len, void *data);

/*Low layer write(output) function*/

__attribute__((used))
int _write(int file, char *ptr, int len)
{
	(void)file;
//	(void)ptr;
//	(void)len;
	
	RTUUartTx(len, ptr);

#if 0
	//user code example

	int i;
	(void)file;

	for (i = 0; i < len; i++)
	{
		// UART_PutChar is user's basic output function
		UART_PutChar(*ptr++);
	}
#endif

	return len;
}

__attribute__((used))
void abort(void)
{
	/* Abort called */
	while (1);
}

__attribute__((weak))
void _init(void)
{
	return;
}