// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <inc/types.h>
#include <inc/stdio.h>
#include <inc/stdarg.h>


static void
putch(int ch, int *cnt)
{
	cputchar(ch);	// 向控制台输出一个字符
	*cnt++;			// 起作用的就只是cnt++吧？
}

int
vcprintf(const char *fmt, va_list ap)
{
	int cnt = 0;

	vprintfmt((void*)putch, &cnt, fmt, ap);
	return cnt;
}

int
cprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;

	va_start(ap, fmt);		// 初始化ap以检索fmt后面的参数，fmt是格式控制字符串
	cnt = vcprintf(fmt, ap);
	va_end(ap);

	return cnt;
}

