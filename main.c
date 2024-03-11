#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "board.h"

static int parse_i(const char *str, int base, int *ret);

int main(const int argc, const char **argv)
{
	struct minesweeper_board board = {0};
	struct gr_buffer strbuf;
	int ret = 0;
	int row = 0;
	int col = 0;

	gr_buf_init(&strbuf, 64);

	if (argc == 3) {
		if (parse_i(argv[1], 0, &row) || parse_i(argv[2], 0, &col)) {
			ret = 1;
			goto end;
		}
	} else if (argc != 1) {
		ret = 1;
		goto end;
	}

	board_read(&board, stdin);

	board_to_string_buf(&board, &strbuf);
	fwrite(strbuf.buf, 1, strbuf.length, stdout);

	if (board_is_full(&board)) {
		puts("Board is full. Attempting to solve from start.");

		switch (board_solve_full(&board, row, col)) {
		case BOARD_SOLVE_SUCCESS:
			puts("Board solved.");
			break;
		case BOARD_SOLVE_MUST_GUESS:
			puts("Board cannot be solved without guessing.");
			break;
		default: puts("BUG!");
			 ret = 1;
			 goto end;
		}
	} else if (board_is_partial(&board)) {
		puts("A partial solution is given.");

		if (!board_mine_numbers_consistent(&board)) {
			puts("Board mine number inconsistent!");
			ret = 1;
			goto end;
		}

		puts("Mine numbers consistent. Attempting to deduce next moves.");
		board_deduce_partial(&board);
	}

end:
	gr_buf_delete(&strbuf);
	board_destroy(&board);

	return ret;
}

static int parse_i(const char *str, int base, int *ret)
{
	long maybe_ret;
	int err = 0;
	char *endptr;

	errno = 0;
	maybe_ret = strtol(str, &endptr, base);

	if (*endptr || errno || maybe_ret < INT_MIN || maybe_ret > INT_MAX)
		return 1;

	*ret = (int)maybe_ret;

	return err;
}
