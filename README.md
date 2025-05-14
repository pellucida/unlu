# unlu - lbr extractor

## Synopsis

**unlu** [-C *dir*] [-v] [-t|-l|-x] [-f *archive*] 

Extract (-x), list (-l) or test (-t) the contents of an old CP/M or MSDOS lbr archive.
For extraction the files may be extracted into another directory (-C dir.)
The crc16 checksum of the archive members are verified by (-t.)
More verbose output to standard output (-v.)
If the archive is omitted then standard input is assumed.

## Notes

The whole archive is read into memory before extraction or testing so unlu can be used as part of pipeline. No file seeks are used.  The largest possible lbr archive must be less than 16 Mb.

The archive format was taken from *ftp://oak.oakland.edu/pub/cpm/arc-lbr/ludef5.doc* with a copy under doc/

Extraction is currently all or nothing - specifying the members to be extracted with or without wildcards isn't implemented.
No provision for creating new archive is provided as that facility is unlikely to be used although fairly easily added.

*Lu.exe* was the MSDOS program included with the archives I wanted to extract thus *unlu*.
