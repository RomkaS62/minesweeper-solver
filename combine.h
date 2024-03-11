#ifndef MINESWEEPER_SOLVER_COMBINE_H
#define MINESWEEPER_SOLVER_COMBINE_H

#define N_CHOOSE_K_DONE		0
#define N_CHOOSE_K_HAS_NEXT	1

struct n_choose_k_uc_itr {
	unsigned char nitems;
	unsigned char nchoices;
	unsigned char choice;
	unsigned char state;

	/* Unique combination of choices of integers between 0 and nitems - 1.
	 * Use these as indices into external array of things you are choosing
	 * between.
	 * */
	unsigned char choices[0xFF];
};

void n_choose_k_init(struct n_choose_k_uc_itr *itr, unsigned char nitems, unsigned char nchoices);
void n_choose_k_next(struct n_choose_k_uc_itr *itr);

#define FOREACH_N_CHOOSE_K(__itr, __items, __choices)	\
	for (n_choose_k_init((__itr), (__items), (__choices));	\
				(__itr)->state == N_CHOOSE_K_HAS_NEXT;	\
				n_choose_k_next((__itr)))

#endif /* MINESWEEPER_SOLVER_COMBINE_H */
