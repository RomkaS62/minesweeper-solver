#include "combine.h"

#define CURRENT_CHOICE(__itr) ((__itr)->choices[(__itr)->choice])

void n_choose_k_init(struct n_choose_k_uc_itr *itr, unsigned char nitems, unsigned char nchoices)
{
	unsigned char i;

	if (nchoices > nitems) {
		itr->state = N_CHOOSE_K_DONE;
		return;
	}

	itr->nitems = nitems;
	itr->nchoices = nchoices;
	itr->choice = nchoices - 1;
	itr->state = N_CHOOSE_K_HAS_NEXT;

	for (i = 0; i < itr->nchoices; i++)
		itr->choices[i] = i;
}

void n_choose_k_next(struct n_choose_k_uc_itr *itr)
{
	if (itr->state == N_CHOOSE_K_DONE)
		return;

	while (CURRENT_CHOICE(itr) >= itr->nitems - itr->nchoices + itr->choice
			&& itr->choice > 0)
	{
		itr->choice--;
	}

	if (itr->choice == 0 && CURRENT_CHOICE(itr) >= itr->nitems - itr->nchoices) {
		itr->state = N_CHOOSE_K_DONE;
		return;
	}

	CURRENT_CHOICE(itr)++;

	for (; itr->choice < itr->nchoices - 1; itr->choice++) {
		itr->choices[itr->choice + 1] = CURRENT_CHOICE(itr) + 1;
	}

	itr->choice = itr->nchoices - 1;

	return;
}
