### About
Reads files (or all the files inside of directory if provided, recursively) and
aggregates character occurrences. This comes as a single binary file or can be
built as shared library or an archive, it provides basic API for easy embedding.
Entirely written in C (C99).
### Dependencies
- `libpthread` (should already be on your system)
### Build binary
Edit `config.mk` to your needs and then build/install:
```sh
git clone https://github.com/b-jure/cfreq.git
cd cfreq
make # build
make install # install (add sudo in front if needed)
```
### Building shared library
```sh
make library # build
make install-library # install
```
```sh
make uninstall-library # uninstall
```
### Building archive
```sh
make archive # build
```
### Example usage binary
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
For complete usage run the program with no additional arguments or refer to the
manual page.
### Example library API
This counts character frequencies of all files under `/home` directory recursively
using `6` worker threads, returning the result into static `counts`.
```c
#include <stdlib.h>
#include <cfreq.h>

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
