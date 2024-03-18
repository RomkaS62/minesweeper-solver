#include "combine.h"

void n_choose_k_init(struct n_choose_k_uc_itr *itr, unsigned char nitems, unsigned char nchoices)
{
	unsigned char i;

	if (nchoices > nitems) {
		itr->state = N_CHOOSE_K_DONE;
		return;
	}

	itr->nitems = nitems;
	itr->nchoices = nchoices;
	itr->state = N_CHOOSE_K_HAS_NEXT;

	for (i = 0; i < itr->nchoices; i++)
		itr->choices[i] = i;
}

void n_choose_k_next(struct n_choose_k_uc_itr *itr)
{
	unsigned char i;

	i = itr->nchoices - 1;

	if (itr->state == N_CHOOSE_K_DONE)
		return;

	while (itr->choices[i] >= itr->nitems - itr->nchoices + i && i > 0)
		i--;

	if (i == 0 && itr->choices[i] >= itr->nitems - itr->nchoices) {
		itr->state = N_CHOOSE_K_DONE;
		return;
	}

	itr->choices[i]++;

	for (; i < itr->nchoices - 1; i++)
		itr->choices[i + 1] = itr->choices[i] + 1;

	return;
}
