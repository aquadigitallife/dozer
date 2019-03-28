// Implementation of newlib syscall

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#undef errno
extern int errno;
extern int  _end;


void RTUUartTx(size_t len, void *data);
void GSMUartTx(size_t len, void *data);
size_t GSMUartRx(size_t len, void *data);

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
/*
	Возможные значения flags:
#define	O_RDONLY	0		// +1 == FREAD
#define	O_WRONLY	1		// +1 == FWRITE
#define	O_RDWR		2		// +1 == FREAD|FWRITE
#define	O_APPEND	_FAPPEND
#define	O_CREAT		_FCREAT
#define	O_TRUNC		_FTRUNC
#define	O_EXCL		_FEXCL
*/
__attribute__((used))
int _open(const char *name, int flags)
{
	(void)flags;
	if (strcmp(name, "RTU") == 0) return 1;
	if (strcmp(name, "GSM") == 0) return 3;
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
	int retval;
	switch (file) {
		case 3:
			for (retval = 0; retval < len; retval++) {
				GSMUartRx(1, &ptr[retval]);
				if (ptr[retval] == '\n') {retval++; break;}
			}

//			retval = GSMUartRx(len, ptr);
			break;
//		default:
//			return len;
	}
//	RTUUartTx(retval, ptr);
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

	return retval;
}


/*Low layer write(output) function*/

__attribute__((used))
int _write(int file, char *ptr, int len)
{

	switch (file) {
		case 3:
			GSMUartTx(len, ptr);
			break;
		default:
			RTUUartTx(len, ptr);	//RTU
	}

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

int fgetc(FILE *fd)
{
	int c = 0;
	switch (fd->_file) {
		case 3:
			GSMUartRx(1, &c);
	}
	return c;
}