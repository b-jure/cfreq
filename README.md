## About
Reads files (or all the files in directory if provided, recursively) and
aggregates character occurrences (8 bit ASCII).
After `cfreq` command specify files/directories you would like to count character
occurrences for.
It is multithreaded so you can specify how many worker threads you would
like to use with `-t` flag, keep in mind that this number will be clamped
automatically to the number of online CPU cores your system has.
By omitting the `-t` flag or specifying `1` as the thread count, `cfreq` will
run on a single thread.
Doubt this will prove to be useful to anyone, I just made it in order to
generate ASCII tables for Huffman encoding tree in my compression program.
Also I doubt this is portable outside of systems currently running
GNU/Linux.

## Build & Install
Before building/installing edit `config.mk` to your needs.
Build (and install):
```sh
git clone https://github.com/b-jure/cfreq.git
cd cfreq
make # this builds it
make install # this installs (add sudo in front if needed)
```

## Example usage
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
