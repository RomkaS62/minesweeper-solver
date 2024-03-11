#ifndef MINESWEEPER_SOLVER_BITS_H
#define MINESWEEPER_SOLVER_BITS_H

#include <stdint.h>

static inline int popcount(uintmax_t n)
{
	int ret = 0;

	while (n) {
		if (n & 1) ret++;
		n >>= 1;
	}

	return ret;
}

#endif /* MINESWEEPER_SOLVER_BITS_H */
