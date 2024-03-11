#include "buf.h"

#include <stdarg.h>

int buf_printf(struct gr_buffer *buf, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	return buf_vaprintf(buf, fmt, args);
}

int buf_vaprintf(struct gr_buffer *buf, const char *fmt, va_list args)
{
	va_list arg_copy;
	int chars_to_print;
	char *printbuf;

	va_copy(arg_copy, args);

	chars_to_print = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (chars_to_print < 0) {
		va_end(arg_copy);
		return -1;
	}

	printbuf = malloc(chars_to_print + 1);
	va_end(arg_copy);
	chars_to_print = vsnprintf(printbuf, chars_to_print + 1, fmt, arg_copy);
	gr_buf_append(buf, printbuf, chars_to_print);

	free(printbuf);

	return chars_to_print;
}

void buf_write(const struct gr_buffer *buf, FILE *out)
{
	size_t written = 0;
	size_t total = 0;

	do {
		written = fwrite(buf->buf, 1, buf->length, out);
		total += written;
	} while (total < buf->length);
}

