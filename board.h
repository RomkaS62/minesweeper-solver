#ifndef MINESWEEPER_SOLVER_H
#define MINESWEEPER_SOLVER_H

#include <stdio.h>

#include <gramas/buf.h>

#include "buf.h"

#define TILE_CLEAR		0
#define TILE_MINE		(1 << 4)
#define TILE_UNKNOWN		(1 << 5)
#define TILE_DEDUCED		(1 << 6)
#define TILE_BUGGERED		(1 << 7)
#define TILE_DEDUCED_MINE	(TILE_MINE | TILE_DEDUCED)
#define TILE_DEDUCED_CLEAR	(TILE_CLEAR | TILE_DEDUCED)

#define TILE_IS_CLEAR(__tile)		(((__tile) & ~(TILE_DEDUCED | TILE_UNKNOWN)) == TILE_CLEAR)
#define TILE_IS_MINE(__tile)		(((__tile) & ~(TILE_DEDUCED | TILE_UNKNOWN)) == TILE_MINE)
#define TILE_IS_KNOWN_MINE(__tile)	(((__tile) & ~TILE_DEDUCED) == TILE_MINE)
#define TILE_IS_KNOWN_CLEAR(__tile)	(((__tile) & ~TILE_DEDUCED) == TILE_CLEAR)
#define TILE_NEIGHBOR_MINES(__tile)	((__tile) & 0xF)

struct minesweeper_board {
	int rows;
	int cols;
	int row_capacity;
	int col_capacity;
	unsigned char *tiles;
};

void board_init(struct minesweeper_board *board, int rows, int cols);
void board_destroy(struct minesweeper_board *board);

static inline int __within_bounds(
		const struct minesweeper_board *board,
		int row, int col,
		int row_ofst, int col_ofst)
{
	return row + row_ofst < board->rows && col + col_ofst < board->cols
		&& row + row_ofst >= 0 && col + col_ofst >= 0;
}

#define BOARD_FOREACH_IN_NEIGHBORHOOD(__board, __row, __col, __distance, __i, __j, __tile)	\
	for ((__i) = -(__distance); (__i) <= (__distance); (__i)++)	\
		for ((__j) = -(__distance); (__j) <= (__distance); (__j)++)	\
			if (__within_bounds((__board), (__row), (__col), (__i), (__j)))	\
				if ((__tile) = &BOARD_AT((__board), (__row) + (__i), (__col) + (__j)),	\
						!((__i) == 0 && (__j) == 0))

#define BOARD_FOREACH_NEIGHBOR(__board, __row, __col, __i, __j, __tile)	\
	BOARD_FOREACH_IN_NEIGHBORHOOD(__board, __row, __col, 1, __i, __j, __tile)

#define BOARD_AT(__board, __row, __col)	\
	((__board)->tiles[(__row) * (__board)->col_capacity + (__col)])

void board_set_r(struct minesweeper_board *board,
		int row, int col, unsigned char state);

int board_is_full(const struct minesweeper_board *board);
int board_is_partial(const struct minesweeper_board *board);
int board_mine_numbers_consistent(const struct minesweeper_board *board);

void board_read(struct minesweeper_board *board, FILE *file);
int board_to_string_buf(const struct minesweeper_board *board, struct gr_buffer *buf);

#define BOARD_SOLVE_SUCCESS	0
#define BOARD_SOLVE_PARTIAL	1
#define BOARD_SOLVE_MUST_GUESS	2
#define BOARD_SOLVE_BUG		3

#define BOARD_SOLVE_TILE_SUCCESS	0
#define BOARD_SOLVE_TILE_NOTHING	1
#define BOARD_SOLVE_TILE_ERROR		2

int board_solve_full(struct minesweeper_board *board, int row, int col);
void board_deduce_partial(struct minesweeper_board *board);

#endif /* MINESWEEPER_SOLVER_H */
