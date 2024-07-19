## About
Reads files (or all the files in directory if provided, recursively) and
aggregates character occurrences (8/7 bit ASCII).

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
For complete usage run the program with no additional arguments.
