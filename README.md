# SSEXORHamming
Testing Hamming distance performance when optimizing XOR with SSE instructions

I have tested SSE XOR for computing the Hamming distance and it makes no difference. The computation is memory bandwidth bound as we scan over millions of records in main memory. It might work better with caching effects created by a search tree.

I have created a program that compares different approaches and using a 2 byte lookup table seems to be the fastest. As my CPU (intel core2quad) only has 32k L1 data cache, the whole table can not fit in L1 cache. The next generation of intel CPUs codenamed sandybridge still have 32kb L1 data cache. However, it still performs faster than the 1 byte lookup table.

Other people have had the same experience with SSE optimization of bitwise OPS (http://bmagic.sourceforge.net/bmsse2opt.html).

I have attached the code. The assembly is compiled with NASM and has only been tested on x86_64 on Linux using GCC. Though, it should work on x86_64 on OSX using GCC as well.
