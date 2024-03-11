#include <stdint.h>
#include <string.h>

#include <gramas/buf.h>
#include <gramas/line_reader.h>

#include "board.h"
#include "bits.h"

static int board_assume_clear(struct minesweeper_board *board, int row, int col);
static int board_assume_mine(struct minesweeper_board *board, int row, int col);
static int board_assume_neighbors_clear(struct minesweeper_board *board, int row, int col);
static int board_assume_neighbors_mines(struct minesweeper_board *board, int row, int col);
static int board_deduce_from_tile(struct minesweeper_board *board, int row, int col);
static int board_deduce_guaranteed_cases(struct minesweeper_board *board);
static int board_deduce_partial_cases(struct minesweeper_board *board);
static int board_deduce_partial_from_tile(struct minesweeper_board *board, int row, int col);
static int board_fill_empty_tiles(struct minesweeper_board *board, int row, int col);
static int board_is_solved(struct minesweeper_board *board);
static void board_resize_if_needed(struct minesweeper_board *board, int row, int col);
static void board_set_deduced_as_known(struct minesweeper_board *board);
static int board_solve_from_tile(struct minesweeper_board *board, int row, int col);
static int board_solve_iteration(struct minesweeper_board *board);
static int tile_to_string(const unsigned char tile, struct gr_buffer *strbuf);

void board_init(struct minesweeper_board *board, int rows, int cols)
{
	board->rows = rows;
	board->cols = cols;

	board->row_capacity = 1;
	board->col_capacity = 1;

	while (board->row_capacity < board->rows)
		board->row_capacity *= 2;

	while (board->col_capacity < board->cols)
		board->col_capacity *= 2;

	board->tiles = malloc(board->row_capacity * board->col_capacity * sizeof(board->tiles[0]));
	memset(board->tiles, TILE_UNKNOWN, board->row_capacity * board->col_capacity * sizeof(board->tiles[0]));
}

void board_destroy(struct minesweeper_board *board)
{
	free(board->tiles);
}

void board_set_r(struct minesweeper_board *board, int row, int col, unsigned char state)
{
	board_resize_if_needed(board, row, col);
	BOARD_AT(board, row, col) = state;
}

static void board_resize_if_needed(struct minesweeper_board *board, int row, int col)
{
	int new_col_cap;
	int new_row_cap;
	unsigned char *new_board;
	int i;
	int j;

	if (row < board->rows && col < board->cols)
		return;

	if (row < board->row_capacity && row >= board->rows)
		board->rows = row + 1;

	if (col < board->col_capacity && col >= board->cols)
		board->cols = col + 1;

	if (col >= board->col_capacity || row >= board->row_capacity) {
		new_col_cap = board->col_capacity;
		new_row_cap = board->row_capacity;

		while (new_col_cap <= col) new_col_cap *= 2;
		while (new_row_cap <= row) new_row_cap *= 2;

		new_board = malloc(new_col_cap * new_row_cap * sizeof(new_board[0]));

		for (i = 0; i < board->rows; i++)
			for (j = 0; j < board->cols; j++)
				new_board[i * new_col_cap + j] = BOARD_AT(board, i, j);

		free(board->tiles);
		board->tiles = new_board;
		board->col_capacity = new_col_cap;
		board->row_capacity = new_row_cap;

		if (col >= board->cols) board->cols = col + 1;
		if (row >= board->rows) board->rows = row + 1;
	}
}

int board_is_full(const struct minesweeper_board *board)
{
	int i;
	int j;

	for (i = 0; i < board->rows; i++) {
		for (j = 0; j < board->cols; j++) {
			if (BOARD_AT(board, i, j) == TILE_MINE) continue;
			if (BOARD_AT(board, i, j) == TILE_CLEAR) continue;

			return 0;
		}
	}

	return 1;
}

int board_is_partial(const struct minesweeper_board *board)
{
	int i;
	int j;

	for (i = 0; i < board->rows; i++) {
		for (j = 0; j < board->cols; j++) {
			if (BOARD_AT(board, i, j) == TILE_MINE) continue;
			if (BOARD_AT(board, i, j) == TILE_UNKNOWN) continue;
			if (BOARD_AT(board, i, j) <= 8) continue;

			return 0;
		}
	}

	return 1;
}

int board_mine_numbers_consistent(const struct minesweeper_board *board)
{
	int i;
	int j;
	int k;
	int l;
	int mine_count;
	int neighbors;
	unsigned char *tile;

	for (i = 0; i < board->rows; i++) {
		for (j = 0; j < board-> cols; j++) {
			if (BOARD_AT(board, i, j) > 8)
				continue;

			mine_count = 0;
			neighbors = 0;

			BOARD_FOREACH_NEIGHBOR(board, i, j, k, l, tile) {
				if (*tile & TILE_MINE) mine_count++;
				neighbors++;
			}

			if (mine_count > BOARD_AT(board, i, j)) return 0;
			if (mine_count > neighbors) return 0;
		}
	}

	return 1;
}

void board_read(struct minesweeper_board *board, FILE *file)
{
	struct file_line_itr_s itr = {0};
	const char *line = NULL;
	size_t length = 0;
	size_t i = 0;
	int row = 0;
	int col = 0;
	unsigned char tile;

	board_init(board, 1, 1);

	FOREACH_LINE_IN_FILE(&itr, file, &line, &length) {
		for (i = 0; i < length; i++) {
			if (line[i] == '?') {
				tile = TILE_UNKNOWN;
			} else if (line[i] == '.') {
				tile = TILE_CLEAR;
			} else if (line[i] == '#') {
				tile = TILE_MINE;
			} else if (line[i] >= '1' && line[i] <= '8') {
				tile = line[i] - '0';
			} else {
				continue;
			}

			board_set_r(board, row, col++, tile);
		}

		row++;
		col = 0;
	}

	file_line_itr_delete(&itr);
}

int board_to_string_buf(const struct minesweeper_board *board, struct gr_buffer *strbuf)
{
	int i;
	int j;
	int ret = 0;

	for (i = 0; i < board->rows; i++) {
		for (j = 0; j < board->cols; j++) {
			if (tile_to_string(BOARD_AT(board, i, j), strbuf)) {
				fprintf(stderr, "Unknown tile %i,%i!\n", i, j);
				exit(1);
			}

			gr_buf_append_char(strbuf, ' ');
		}

		gr_buf_append_char(strbuf, '\n');
	}

	return ret;
}

#define BOLD(__str) "\033[1m" __str "\033[0m"
#define COLOR(__r, __g, __b, __str) "\033[38;2;" #__r ";" #__g ";" #__b "m" __str "\033[0m"

static int tile_to_string(const unsigned char tile, struct gr_buffer *strbuf)
{
	const char *str = NULL;

	if (tile & TILE_BUGGERED) {
		switch (tile & ~(TILE_DEDUCED | TILE_UNKNOWN | TILE_BUGGERED)) {
		case TILE_CLEAR: str = BOLD(COLOR(255, 0, 0, ".")); break;
		case 1: str = BOLD(COLOR(255, 0, 0, "1")); break;
		case 2: str = BOLD(COLOR(255, 0, 0, "2")); break;
		case 3: str = BOLD(COLOR(255, 0, 0, "3")); break;
		case 4: str = BOLD(COLOR(255, 0, 0, "4")); break;
		case 5: str = BOLD(COLOR(255, 0, 0, "5")); break;
		case 6: str = BOLD(COLOR(255, 0, 0, "6")); break;
		case 7: str = BOLD(COLOR(255, 0, 0, "7")); break;
		case 8: str = BOLD(COLOR(255, 0, 0, "8")); break;
		case TILE_MINE: str = BOLD(COLOR(255, 0, 0, "#")); break;
		}
	} else if (tile & TILE_DEDUCED) {
		switch (tile & ~(TILE_DEDUCED | TILE_UNKNOWN)) {
		case TILE_CLEAR: str = COLOR(255, 255, 0, "."); break;
		case 1: str = COLOR(255, 255, 0, "1"); break;
		case 2: str = COLOR(255, 255, 0, "2"); break;
		case 3: str = COLOR(255, 255, 0, "3"); break;
		case 4: str = COLOR(255, 255, 0, "4"); break;
		case 5: str = COLOR(255, 255, 0, "5"); break;
		case 6: str = COLOR(255, 255, 0, "6"); break;
		case 7: str = COLOR(255, 255, 0, "7"); break;
		case 8: str = COLOR(255, 255, 0, "8"); break;
		case TILE_MINE: str = COLOR(255, 255, 0, "#"); break;
		}
	} else if (tile & TILE_UNKNOWN) {
		str = COLOR(0, 0, 255, "?");
	} else {
		switch (tile) {
		case TILE_CLEAR: str = COLOR(40, 40, 40, "."); break;
		case 1: str = COLOR(60, 255, 0, "1"); break;
		case 2: str = COLOR(120, 255, 0, "2"); break;
		case 3: str = COLOR(180, 255, 0, "3"); break;
		case 4: str = COLOR(255, 255, 0, "4"); break;
		case 5: str = COLOR(255, 180, 0, "5"); break;
		case 6: str = COLOR(255, 120, 0, "6"); break;
		case 7: str = COLOR(255, 60, 0, "7"); break;
		case 8: str = COLOR(255, 0, 0, "8"); break;
		case TILE_MINE: str = COLOR(255, 0, 0, "#"); break;
		}
	}

	if (str == NULL) {
		return 1;
	}

	gr_buf_append(strbuf, str, strlen(str));
	return 0;
}

int board_solve_full(struct minesweeper_board *board, int row, int col)
{
	int ret = BOARD_SOLVE_SUCCESS;
	struct gr_buffer strbuf;
	int i = 0;
	int j = 0;
	int k;
	int l;
	unsigned char *tile;
	unsigned char mines;

	gr_buf_init(&strbuf, 64);

	for (i = 0; i < board->rows; i++)
		for (j = 0; j < board->cols; j++)
			BOARD_AT(board, row, col) |= TILE_UNKNOWN;

	BOARD_AT(board, row, col) &= ~TILE_UNKNOWN;

	for (i = 0; i < board->rows; i++) {
		for (j = 0; j < board->cols; j++) {
			if (!TILE_IS_CLEAR(BOARD_AT(board, i, j))) {
				fputs("* ", stdout);
				continue;
			}

			mines = 0;
			BOARD_AT(board, i, j) &= 0xF0;

			BOARD_FOREACH_NEIGHBOR(board, i, j, k, l, tile)
				if (*tile & TILE_MINE) mines++;

			BOARD_AT(board, i, j) |= mines;

			printf("%i ", BOARD_AT(board, i, j) & 0xF);
		}
		puts("");
	}

	i = 0;

	do {
		gr_buf_clear(&strbuf);
		buf_printf(&strbuf, "-- Step #%zu --\n", i);
		board_to_string_buf(board, &strbuf);
		buf_write(&strbuf, stdout);
		board_set_deduced_as_known(board);

		if (i > board->rows + board->cols) {
			ret = BOARD_SOLVE_BUG;
			goto end;
		}

		i++;
	} while ((ret = board_solve_iteration(board)) == BOARD_SOLVE_PARTIAL);

end:
	gr_buf_clear(&strbuf);
	buf_printf(&strbuf, "-- Step #%zu --\n", i);
	board_to_string_buf(board, &strbuf);
	buf_write(&strbuf, stdout);
	gr_buf_delete(&strbuf);

	return ret;
}

static void board_set_deduced_as_known(struct minesweeper_board *board)
{
	int i;
	int j;

	for (i = 0; i < board->rows; i++)
		for (j = 0; j < board->cols; j++)
			if (BOARD_AT(board, i, j) & TILE_DEDUCED)
				BOARD_AT(board, i, j) &= ~(TILE_DEDUCED | TILE_UNKNOWN);
}

static int board_solve_iteration(struct minesweeper_board *board)
{
	int i;
	int j;
	int deduced_anything = 0;
	int cont = 1;
	struct gr_buffer strbuf;

	while (cont) {
		for (i = 0; i < board->rows; i++) {
			for (j = 0; j < board->cols; j++) {
				switch(board_solve_from_tile(board, i, j)) {
				case BOARD_SOLVE_TILE_SUCCESS:
					cont = 1;
					deduced_anything |= 1;
					break;
				case BOARD_SOLVE_TILE_NOTHING:
					cont = 0;
					break;
				case BOARD_SOLVE_TILE_ERROR:
					cont = 0;

					fprintf(stderr, "Buggered %i,%i\n", i, j);
					BOARD_AT(board, i, j) |= TILE_BUGGERED;

					gr_buf_init(&strbuf, 1024);
					board_to_string_buf(board, &strbuf);
					buf_write(&strbuf, stderr);
					gr_buf_delete(&strbuf);

					return BOARD_SOLVE_BUG;
				}
			}
		}
	}

	if (board_is_solved(board)) return BOARD_SOLVE_SUCCESS;
	if (deduced_anything) return BOARD_SOLVE_PARTIAL;

	return BOARD_SOLVE_MUST_GUESS;
}

static int board_is_solved(struct minesweeper_board *board)
{
	int i;
	int j;

	for (i = 0; i < board->rows; i++)
		for (j = 0; j < board->cols; j++)
			if (BOARD_AT(board, i, j) & TILE_UNKNOWN)
				return 0;

	return 1;
}

static int board_solve_from_tile(struct minesweeper_board *board, int row, int col)
{
	int i;
	int j;
	int unknown_neighbors = 0;
	int known_mines = 0;
	unsigned char *tile;
	unsigned char surrounding_mines = 0;

	tile = &BOARD_AT(board, row, col);

	if (*tile > 8) return 0;

	surrounding_mines = *tile;

	BOARD_FOREACH_NEIGHBOR(board, row, col, i, j, tile) {
		if (*tile == TILE_MINE) known_mines++;
		if (*tile & TILE_UNKNOWN) unknown_neighbors++;
	}

	if (!unknown_neighbors) return 0;

	if (surrounding_mines == 0) {
		if (board_fill_empty_tiles(board, row, col)) {
			fputs("Error while filling empty tiles!\n", stderr);
			return BOARD_SOLVE_TILE_ERROR;
		}
	} else if (surrounding_mines == known_mines) {
		if (board_assume_neighbors_clear(board, row, col)) {
			fputs("Wrongly assumed all neighbors are clear!\n", stderr);
			return BOARD_SOLVE_TILE_ERROR;
		}
	} else if (unknown_neighbors + known_mines == surrounding_mines) {
		if (board_assume_neighbors_mines(board, row, col)) {
			fputs("Wrongly assumed all neighbors are mines!\n", stderr);
			return BOARD_SOLVE_TILE_ERROR;
		}
	} else {
		return BOARD_SOLVE_TILE_NOTHING;
	}

	return BOARD_SOLVE_TILE_SUCCESS;
}

static int board_assume_neighbors_mines(struct minesweeper_board *board, int row, int col)
{
	int i;
	int j;
	unsigned char *tile;

	BOARD_FOREACH_NEIGHBOR(board, row, col, i, j, tile)
		if (*tile & TILE_UNKNOWN)
			if (board_assume_mine(board, row + i, col + j))
				return 1;

	return 0;
}

static int board_assume_neighbors_clear(struct minesweeper_board *board, int row, int col)
{
	int i;
	int j;
	unsigned char *tile;

	BOARD_FOREACH_NEIGHBOR(board, row, col, i, j, tile)
		if (*tile & TILE_UNKNOWN)
			if (board_assume_clear(board, row + i, col + j))
				return 1;

	return 0;
}

static int board_assume_mine(struct minesweeper_board *board, int row, int col)
{
	if (BOARD_AT(board, row, col) & TILE_MINE) {
		BOARD_AT(board, row, col) |= TILE_DEDUCED;
		return 0;
	} else {
		return 1;
	}
}

static int board_assume_clear(struct minesweeper_board *board, int row, int col)
{
	if (!(BOARD_AT(board, row, col) & TILE_MINE)) {
		BOARD_AT(board, row, col) |= TILE_DEDUCED;
		return 0;
	} else {
		return 1;
	}
}

static int board_fill_empty_tiles(struct minesweeper_board *board, int row, int col)
{
	struct tile_coordinate_s {
		int row;
		int col;
	};

	struct tile_coordinate_s *tiles_to_fill;
	struct tile_coordinate_s *current;
	struct tile_coordinate_s *write_head;
	int ro;
	int co;
	unsigned char *tile;

	if (BOARD_AT(board, row, col) != TILE_CLEAR)
		return 1;

	tiles_to_fill = malloc(sizeof(tiles_to_fill[0]) * board->rows * board->cols);

	current = tiles_to_fill;
	current->row = row;
	current->col = col;
	write_head = current + 1;

	while (current != write_head) {
		BOARD_FOREACH_NEIGHBOR(board, current->row, current->col, ro, co, tile) {
			if (*tile == (TILE_UNKNOWN | TILE_CLEAR)) {
				write_head->row = current->row + ro;
				write_head->col = current->col + co;
				*tile |= TILE_DEDUCED;
				write_head++;
			} else if (*tile & TILE_UNKNOWN) {
				*tile |= TILE_DEDUCED;
			}
		}

		current++;
	}

	free(tiles_to_fill);

	return 0;
}

#define BUF_APPEND_STR(__buf, __str) do { gr_buf_append(__buf, __str, sizeof(__str) - 1); } while (0)

void board_deduce_partial(struct minesweeper_board *board)
{
	int ret = BOARD_SOLVE_MUST_GUESS;
	struct gr_buffer strbuf;

	gr_buf_init(&strbuf, 64);

	while (1) {
		switch (board_deduce_guaranteed_cases(board)) {
		case BOARD_SOLVE_SUCCESS:
		case BOARD_SOLVE_PARTIAL:
			ret = BOARD_SOLVE_SUCCESS;
			break;
		case BOARD_SOLVE_MUST_GUESS:
			switch (board_deduce_partial_cases(board)) {
			case BOARD_SOLVE_SUCCESS:
				ret = BOARD_SOLVE_SUCCESS;
				goto again;
			case BOARD_SOLVE_MUST_GUESS: goto end;
			case BOARD_SOLVE_BUG: goto bug;
			}

			if (ret != BOARD_SOLVE_SUCCESS)
				puts("Nothing can be deduced.");

			goto end;
		default:
			goto bug;
		}

again:
		continue;
	}

end:
	BUF_APPEND_STR(&strbuf, "Deduced:\n");
	board_to_string_buf(board, &strbuf);
	buf_write(&strbuf, stdout);

	gr_buf_delete(&strbuf);

	return;

bug:
	fputs("BUG!", stderr);
	ret = BOARD_SOLVE_BUG;
	goto end;
}

static int board_deduce_guaranteed_cases(struct minesweeper_board *board)
{
	int i;
	int j;
	int ret = BOARD_SOLVE_MUST_GUESS;

	for (i = 0; i < board->rows; i++) {
		for (j = 0; j < board->cols; j++) {
			switch (board_deduce_from_tile(board, i, j)) {
			case BOARD_SOLVE_TILE_SUCCESS:
				ret = BOARD_SOLVE_SUCCESS;
				break;
			case BOARD_SOLVE_TILE_NOTHING:
				break;
			default:
				ret = BOARD_SOLVE_BUG;
				goto end;
			}
		}
	}

end:
	return ret;
}

/* This function solves simple cases like the following:
 *
 *	1 1 1	1 2 2
 *	1 ? ?	1 ? ?
 *	1 ? ?	1 ? ?
 *
 * Just by looking at top left tile you can determine which tiles are mines and
 * which are clear. This function only looks at the situation from the
 * perspective of one tile.
 * */
static int board_deduce_from_tile(struct minesweeper_board *board, int row, int col)
{
	int i;
	int j;
	int unknown_neighbors = 0;
	int known_mines = 0;
	unsigned char *tile;
	unsigned char surrounding_mines = 0;

	tile = &BOARD_AT(board, row, col);

	if (*tile > 8) return BOARD_SOLVE_TILE_NOTHING;

	surrounding_mines = *tile;

	BOARD_FOREACH_NEIGHBOR(board, row, col, i, j, tile) {
		if ((*tile & ~TILE_DEDUCED) == TILE_MINE)
			known_mines++;

		if ((*tile & (TILE_UNKNOWN | TILE_DEDUCED)) == TILE_UNKNOWN)
			unknown_neighbors++;
	}

	if (!unknown_neighbors) return BOARD_SOLVE_TILE_NOTHING;

	if (surrounding_mines == 0 || surrounding_mines == known_mines) {
		BOARD_FOREACH_NEIGHBOR(board, row, col, i, j, tile)
			if ((*tile & (TILE_UNKNOWN | TILE_DEDUCED)) == TILE_UNKNOWN)
				*tile = TILE_CLEAR | TILE_DEDUCED;

		return BOARD_SOLVE_TILE_SUCCESS;
	} else if (unknown_neighbors + known_mines == surrounding_mines) {
		BOARD_FOREACH_NEIGHBOR(board, row, col, i, j, tile)
			if ((*tile & (TILE_UNKNOWN | TILE_DEDUCED)) == TILE_UNKNOWN)
				*tile = TILE_MINE | TILE_DEDUCED;

		return BOARD_SOLVE_TILE_SUCCESS;
	} else {
		return BOARD_SOLVE_TILE_NOTHING;
	}

	return BOARD_SOLVE_TILE_SUCCESS;
}

static int board_deduce_partial_cases(struct minesweeper_board *board)
{
	int i;
	int j;
	int ret = BOARD_SOLVE_MUST_GUESS;

	if (board->rows < 3 || board->cols < 3)
		goto end;

	for (i = 1; i < board->rows - 1; i++) {
		for (j = 1; j < board->cols - 1; j++) {
			switch (board_deduce_partial_from_tile(board, i, j)) {
			case BOARD_SOLVE_TILE_SUCCESS:
				ret = BOARD_SOLVE_SUCCESS;
				break;
			case BOARD_SOLVE_TILE_NOTHING:
				break;
			case BOARD_SOLVE_TILE_ERROR:
				ret = BOARD_SOLVE_BUG;
				goto end;
			}
		}
	}

end:
	return ret;
}

static int board_count_unknown_outside_neighbors(
		const struct minesweeper_board *board,
		int from_row, int from_col,
		int for_row, int for_col);

static int board_count_known_outside_mines(
		const struct minesweeper_board *board,
		int from_row, int from_col,
		int for_row, int for_col);

/* This is meant to solve the more difficult cases, like:
 *
 *      . 1 ?
 *      2 4 ?
 *      # # ?
 *
 * A partial solution can be found by enumarating all technically possible
 * solutions and picking out which tiles are always mines and which are always
 * clear.
 *
 * This function works only on 3x3 areas.
 * */
static int board_deduce_partial_from_tile(struct minesweeper_board *board, int row, int col)
{
	/* Patterns that mask out mine bits irrelevant to calculating the
	 * neighbor mine count of a particular tile
	 * */
	static const uint16_t mine_count_masks[] = {
		3 | (3 << 3),
		7 | (7 << 3),
		6 | (6 << 3),

		3 | 3 << 3 | 3 << 6,
		~(~0U << 9),
		6 | 6 << 3 | 6 << 6,

		3 << 3 | 3 << 6,
		7 << 3 | 7 << 6,
		6 << 3 | 6 << 6
	};

	int i;
	int j;
	int ret = BOARD_SOLVE_TILE_NOTHING;
	int ro;	/* Neighbor row offset */
	int co;	/* Neighbor column offset*/
	int board_too_small;
	int too_close_to_edge;
	int viable_solutions_exist;
	int tile_idx;
	uint16_t always_mine;
	uint16_t always_clear;
	uint16_t mines;
	uint16_t mine_mask = 0;		/* These mines must be present */
	uint16_t unknown_mask = 0;	/* Mines can be placed here */
	unsigned char *tile;
	unsigned char total_mines;
	unsigned char expected_mine_counts[9];
	unsigned char unknown_outside_neighbors[9] = { 0 };

	if (BOARD_AT(board, row, col) > 8)
		goto end;

	board_too_small = board->rows < 3 || board->cols < 3;

	too_close_to_edge = row < 1 || col < 1
		|| row >= board->rows - 1 || col >= board->cols - 1;

	if (board_too_small || too_close_to_edge)
		goto end;

	total_mines = BOARD_AT(board, row, col);
	expected_mine_counts[4] = total_mines;

	BOARD_FOREACH_NEIGHBOR(board, row, col, ro, co, tile) {
		tile_idx = (ro + 1) * 3 + co + 1;

		if (*tile <= 8) {
			expected_mine_counts[tile_idx] = *tile - board_count_known_outside_mines(
				board, row, col, row + ro, col + co);
		}

		if ((*tile & (TILE_UNKNOWN | TILE_DEDUCED)) == TILE_UNKNOWN)
			unknown_mask |= 1 << (tile_idx);

		if (((*tile & ~TILE_DEDUCED) & (TILE_UNKNOWN | TILE_MINE)) == TILE_MINE)
			mine_mask |= 1 << (tile_idx);

		unknown_outside_neighbors[tile_idx] = board_count_unknown_outside_neighbors(
				board, row, col, row + ro, col + co);
	}

	if (popcount(mine_mask) == total_mines || !unknown_mask)
		goto end;

	/* We can only deduce information that was not previously known */
	always_mine = unknown_mask;
	always_clear = unknown_mask;

	viable_solutions_exist = 0;

	/* Enumarate all mine placements */
	for (mines = ~(~0U << total_mines); mines < (1 << 9) && (always_mine || always_clear); mines++) {
		/* Mines that must exist do not */
		if ((mines & mine_mask) != mine_mask)
			continue;

		/* Mines are misplaced */
		if ((mines & (mine_mask | unknown_mask)) != mines)
			continue;

		/* Too many or too few mines */
		if (popcount(mines) != total_mines)
			continue;

		/* Check if current layout produces the expected mine neighbor
		 * counts.
		 * */
		for (i = 0; (size_t)i < sizeof(expected_mine_counts); i++) {
			/* Not a clear tile. Next. */
			if ((1 << i) & (mine_mask | unknown_mask))
				continue;

			if (expected_mine_counts[i] - popcount(mines & mine_count_masks[i])
					> unknown_outside_neighbors[i]
					|| popcount(mines & mine_count_masks[i]) > expected_mine_counts[i])
			{
				goto enumerate_next;
			}
		}

		viable_solutions_exist = 1;

		/* This layout produces the same mine counts. Update positions
		 * that always remain mines or clear.
		 * */
		always_mine &= mines;
		always_clear &= ~mines;

enumerate_next:
		continue;
	}

	if (!viable_solutions_exist)
		goto end;

	/* We got tiles that throughout every technically possible solution
	 * always remained mines. Therefore, they must be mines.
	 * */
	for (i = 0; i < 3 && always_mine; i++) {
		for (j = 0; j < 3; j++) {
			if (!(BOARD_AT(board, row - 1 + i, col - 1 + j) & TILE_UNKNOWN))
				continue;

			if (always_mine & (1 << (i * 3 + j)))
				BOARD_AT(board, row - 1 + i, col - 1 + j) = TILE_DEDUCED | TILE_MINE;
		}
	}

	/* Same as above but for clear tiles */
	for (i = 0; i < 3 && always_clear; i++) {
		for (j = 0; j < 3; j++) {
			if (!(BOARD_AT(board, row - 1 + i, col - 1 + j) & TILE_UNKNOWN))
				continue;

			if (always_clear & (1 << (i * 3 + j)))
				BOARD_AT(board, row - 1 + i, col - 1 + j) = TILE_DEDUCED | TILE_CLEAR;
		}
	}

	if ((always_clear || always_mine) && viable_solutions_exist)
		ret = BOARD_SOLVE_TILE_SUCCESS;

end:
	return ret;
}

#ifndef ABS
#	define ABS(__a) (((__a) < 0) ? -(__a) : (__a))
#endif

static int board_count_unknown_outside_neighbors(
		const struct minesweeper_board *board,
		int from_row, int from_col,
		int for_row, int for_col)
{
	int ret = 0;
	int ro;
	int co;
	unsigned char *tile;

	BOARD_FOREACH_NEIGHBOR(board, for_row, for_col, ro, co, tile) {
		if (ABS(for_row + ro - from_row) < 2 && ABS(for_col + co - from_col) < 2)
			continue;

		if ((*tile & (TILE_UNKNOWN | TILE_DEDUCED)) == TILE_UNKNOWN)
			ret++;
	}

	return ret;
}

static int board_count_known_outside_mines(
		const struct minesweeper_board *board,
		int from_row, int from_col,
		int for_row, int for_col)
{
	int ret = 0;
	int ro;
	int co;
	unsigned char *tile;

	BOARD_FOREACH_NEIGHBOR(board, for_row, for_col, ro, co, tile) {
		if (ABS(for_row + ro - from_row) < 2 && ABS(for_col + co - from_col) < 2)
			continue;

		if ((*tile & ~TILE_DEDUCED) == TILE_MINE)
			ret++;
	}

	return ret;
}
