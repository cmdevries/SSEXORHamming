#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// align to 16 byte boundaries
const size_t align = 16;

// bit vectors are 64 bytes
const size_t size = 64;

// function description in ASM file
void sse_xor_512(void* p, void* q, void* r);

void xor_512(uint64_t* p, uint64_t* q, uint64_t* r) {
    for (int i = 0; i < 8; ++i) {
        r[i] = p[i] ^ q[i];
    }
    // loop unrolling made no difference
    /*
    rr[0] = pp[0] ^ qq[0];
    rr[1] = pp[1] ^ qq[1];
    rr[2] = pp[2] ^ qq[2];
    rr[3] = pp[3] ^ qq[3];
    */
}

// populate a bit count for bytes lookup table
// pre:
//  table points to an array of 256 bytes
// post:
//  index n into table contains the number of bits set in the byte represented by the integer n
void populate_table(uint8_t* table, size_t length) {
    table[0] = 0;
    for (int i = 0; i < length; ++i) {
        table[i] = (i & 1) + table[i / 2];
    }
}

size_t bit_count_table(uint8_t* table, void* p, size_t bytes) {
    int count = 0;
    uint8_t* q = p;
    uint8_t* end = q + bytes;
    for ( ; q != end; ++q) {
        count += table[*q];
    }
    return count;
}

size_t bit_count_table_2bytes(uint8_t* table, void* p, size_t bytes) {
    int count = 0;
    uint16_t* q = p;
    uint16_t* end = q + (bytes/2);
    for ( ; q != end; ++q) {
        count += table[*q];
    }
    return count;
}

size_t count(int64_t b) {
     b = (b & 0x5555555555555555LU) + (b >> 1 & 0x5555555555555555LU);
     b = (b & 0x3333333333333333LU) + (b >> 2 & 0x3333333333333333LU);
     b = b + (b >> 4) & 0x0F0F0F0F0F0F0F0FLU;
     b = b + (b >> 8);
     b = b + (b >> 16);
     b = b + (b >> 32) & 0x0000007F;
     return b;
}

size_t bit_count_non_table(int64_t* p) {
    size_t total = 0;
    for (int i = 0; i < 8; ++i) {
        total += count(p[i]);
    }
    return total;
}

size_t xor_512_non_table_count_inline(uint64_t* p, uint64_t* q, uint64_t* r) {
    size_t total = 0;
    for (int i = 0; i < 8; ++i) {
        r[i] = p[i] ^ q[i];
        total += count(r[i]);
    }
    return total;
}

int main(int argc, char** argv) {
    printf("pointers are %zd bytes\n", sizeof(void*));
    srand(time(NULL));
    
    // bit count lookup tables
    uint8_t table[256];
    populate_table(table, 256);
    uint8_t table_2bytes[65536];
    populate_table(table_2bytes, 65536);

    // mask to compare against
    void* q;
    if (posix_memalign(&q, align, size) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }
    memset(q, 0xFF, size);
    printf("the mask has %zd bits set\n", bit_count_table(table, q, size));

    // allocate aligned memory for SSE
    // many random records
    void* p;
    const size_t repeat = 100000000;
    printf("creating %zd 512-bit random bit strings\n", repeat);
    if (posix_memalign(&p, align, size * repeat) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }
    {
        uint8_t* x = p;
        for (size_t i = 0; i < (size * repeat); ++i) {
            x[i] = rand();
        }
    }

    // memory for xor result
    void* r;
    if (posix_memalign(&r, align, size) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    printf("XOR mask with each of the %zd random records\n", repeat);
    
    // non table
    printf("\nnon table count:\n");
    clock_t start = clock();
    size_t total = 0;
    for (int i = 0; i < repeat; ++i) {
        void* pp = p + (i * size);
        xor_512(pp, q, r);
        total += bit_count_non_table(r);
    }
    printf("took %f seconds\n", ((clock() - start) / (double)CLOCKS_PER_SEC));
    printf("%zd\n", total); 

    // non table with inline count
    printf("\nnon table count (inline count):\n");
    start = clock();
    total = 0;
    for (int i = 0; i < repeat; ++i) {
        void* pp = p + (i * size);
        total += xor_512_non_table_count_inline(pp, q, r);
    }
    printf("took %f seconds\n", ((clock() - start) / (double)CLOCKS_PER_SEC));
    printf("%zd\n", total); 

    // 1 byte table
    printf("\n1byte table count:\n");
    start = clock();
    total = 0;
    for (int i = 0; i < repeat; ++i) {
        void* pp = p + (i * size);
        xor_512(pp, q, r);
        total += bit_count_table(table, r, size);
    }
    printf("took %f seconds\n", ((clock() - start) / (double)CLOCKS_PER_SEC));
    printf("%zd\n", total); 

    // 1 byte table + SSE
    printf("\n1byte table count SSE XOR:\n");
    start = clock();
    total = 0;
    for (int i = 0; i < repeat; ++i) {
        void* pp = p + (i * size);
        sse_xor_512(pp, q, r);
        total += bit_count_table(table, r, size);
    }
    printf("took %f seconds\n", ((clock() - start) / (double)CLOCKS_PER_SEC));
    printf("%zd\n", total); 

    // 2 byte table
    printf("\n2byte table count:\n");
    start = clock();
    total = 0;
    for (int i = 0; i < repeat; ++i) {
        void* pp = p + (i * size);
        xor_512(pp, q, r);
        total += bit_count_table_2bytes(table_2bytes, r, size);
    }
    printf("took %f seconds\n", ((clock() - start) / (double)CLOCKS_PER_SEC));
    printf("%zd\n", total); 
}

