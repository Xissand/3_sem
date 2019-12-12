I don't really have time to write comments, so a readme instead.

File used for testing: "enwiki9" is the test data for the Large Text Compression Benchmark is the first 10^9 bytes of the English Wikipedia dump on Mar. 3, 2006.

Source: http://mattmahoney.net/dc/textdata.html

Notes: creating large chunks of shared memory was causing segfaults, which was solved by enabling hugepages via SHM_HUGETLB
