# unlu - lbr extractor

## Synopsis

**unlu** [-C dir] [-v] [-t|-l|-x] [-f archive] [files ...]

Extract (-x), list (-l) or test (-t) the specified *files* the contents of an old CP/M or MSDOS lbr archive.
m
For extraction the files can extracted into another directory (-C dir.)
The crc16 checksum of the archive members are verified by (-t.)
More verbose output to standard output (-v.)
If the archive is omitted then standard input is assumed.

The archive members (*files*) to which the operation will be restricted may be (optionally) specified are **fnmatch** (*3*) patterns which are basically **sh** (*1*) glob patterns.

## Examples
```` C
	# extract all members from src1.lbr into /tmp/working
	unlu -C /tmp/working -x -v -f src1.lbr

	# list file matching the glob pattern NB quoting
	unlu -l -v -f src1.lbr '[a-h]*.?'

	# test the CRC16 checksum of com and exe files in progs.lbr
	unlu -t v -f progs.lbr '*.com' '*.exe'
````

## Notes

The whole archive is read into memory before extraction or testing so unlu can be used as part of pipeline. No file seeks are used.  Owing to the sixteen bit integers used to specify the offsets in 128 byte sectors and file size also in sectors the largest possible lbr archive is necessarily less than 16 Mb.

## See also 
[ Wikipedia LBR (file format) ](https://en.wikipedia.org/wiki/LBR_%28file_format%29)
