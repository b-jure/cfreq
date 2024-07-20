## About
Reads files (or all the files in directory if provided, recursively) and
aggregates character occurrences (7 or 8 bit ASCII).
Written in C (C99).

## Dependencies
- `libpthread`

## Build & Install
#### Build as binary
Edit `config.mk` to your needs and then build/install:
```sh
git clone https://github.com/b-jure/cfreq.git
cd cfreq
make
make install # add sudo in front if needed
```
#### Building as shared library
```sh
make library
make install-library
```
#### Building as archive
```sh
make archive
```

## Example usage binary
This will scan all the contents of directory `mydir` recursively, it will also
scan the regular file `myfile.txt` and finally `mysrcfile.c`.
```sh
cfreq mydir myfile.txt mysrcfile.c
```
---
Same as above but this will spawn `8` worker threads if possible.
```sh
cfreq -t 8 mydir myfile.txt mysrcfile.c
```
---
For complete usage run the program with no additional arguments.


## Example library API
This counts character frequencies of all files under `/home` directory recursively
using `6` worker threads, returning the result into static `counts`.
```c
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
```
