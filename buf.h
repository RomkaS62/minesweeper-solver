#ifndef MINESWEEPER_SOLVER_BUF_H
#define MINESWEEPER_SOLVER_BUF_H

#include <stdio.h>

#include <gramas/buf.h>

int buf_printf(struct gr_buffer *buf, const char *fmt, ...);
int buf_vaprintf(struct gr_buffer *buf, const char *fmt, va_list args);
void buf_write(const struct gr_buffer *buf, FILE *out);

#endif /* MINESWEEPER_SOLVER_BUF_H */
