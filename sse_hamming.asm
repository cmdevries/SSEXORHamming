section .text
    global sse_xor_512

; Fast XOR for 512 bits (64 bytes) using x86 SSE XORPS instructions in x86_64 assembly
;   core2 can dispatch 3 SSE instructions per clock
; C declarartion
;   // pre:
;   //  (p, q and r are pointers to 512 bits of 16 byte aligned memory) AND (r = p ^ q)
;   void sse_xor_512(void* p, void* q, void *r);

sse_xor_512:
    ; allocate stack for 64 byte XOR result
    enter 64, 0

    ; p is in rdi 
    ; q is in rsi
    ; r is in rdx

    movaps xmm0, [rdi]
    movaps xmm1, [rdi + 16]
    movaps xmm2, [rdi + 32]
    movaps xmm3, [rdi + 48]

    ; XOR first 16 bytes
    xorps xmm0, [rsi]

    ; XOR second 16 bytes
    xorps xmm1, [rsi + 16]

    ; XOR third 16 bytes
    xorps xmm2, [rsi + 32]

    ; XOR fourth 16 bytes
    xorps xmm3, [rsi + 48]

    movaps [rdx], xmm0
    movaps [rdx + 16], xmm1
    movaps [rdx + 32], xmm2
    movaps [rdx + 48], xmm3

    leave
    ret
