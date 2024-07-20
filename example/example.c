#include <stdlib.h>

#include "cfreq.h"

static size_t counts[CFREQ_TABLESIZE] = { 0 };

void *myrealloc(void *block, void *userdata, size_t osize, size_t nsize) {
	(void)userdata;(void)osize;
	if (nsize == 0) {
		free(block);
		return NULL;
	}
	return realloc(block, nsize);
}

int main(int argc, char **argv) {
	(void)argc;(void)argv;

	cfreq_State *state = cfreq_newstate(myrealloc, NULL);
	if (state == NULL)
		exit(EXIT_FAILURE);
	cfreq_addfilepath(state, "/home");
	cfreq_count(state, 6, counts);
	for (int i = 0; i < CFREQ_TABLESIZE; i++)
		printf("%03d  %zu\n", i, counts[i]);
	fflush(stdout);
	cfreq_free(state);
}
