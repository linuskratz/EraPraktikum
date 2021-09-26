.intel_syntax noprefix
.global salsa20_core_naive
.global salsa20_core
    
salsa20_core_naive:
    xor rax, rax    
copy:                                   //copies matrix from input to output
    mov rcx, [rsi+4*rax]
    mov [rdi+4*rax], rcx
    add rax, 2
    cmp rax, 16
    jl copy
    
    mov r8, 12                          //index of the current element in the matrix which is processed
    mov rax, 20
round:                                  //start of a new round
    
    mov rdx, 4
rotate7:                                //four add-rotate-xor operations with a rotation by 7
    mov ecx, [rdi+4*r8]
    add r8, 4
    and r8, 0xF
    add ecx, [rdi+4*r8]
    rol ecx, 7
    add r8, 4
    and r8, 0xF
    xor [rdi+4*r8], ecx
    
    add r8, 13
    and r8, 0xF
    dec rdx
    jnz rotate7
    
    mov rdx, 4
rotate9:                                //four add-rotate-xor operations with a rotation by 9
    mov ecx, [rdi+4*r8]
    add r8, 4
    and r8, 0xF
    add ecx, [rdi+4*r8]
    rol ecx, 9
    add r8, 4
    and r8, 0xF
    xor [rdi+4*r8], ecx
    
    add r8, 13
    and r8, 0xF
    dec rdx
    jnz rotate9
    
    mov rdx, 4
rotate13:                               //four add-rotate-xor operations with a rotation by 13
    mov ecx, [rdi+4*r8]
    add r8, 4
    and r8, 0xF
    add ecx, [rdi+4*r8]
    rol ecx, 13
    add r8, 4
    and r8, 0xF
    xor [rdi+4*r8], ecx
    
    add r8, 13
    and r8, 0xF
    dec rdx
    jnz rotate13
    
    mov rdx, 4
rotate18:                               //four add-rotate-xor operations with a rotation by 18
    mov ecx, [rdi+4*r8]
    add r8, 4
    and r8, 0xF
    add ecx, [rdi+4*r8]
    rol ecx, 18
    add r8, 4
    and r8, 0xF
    xor [rdi+4*r8], ecx
    
    add r8, 13
    and r8, 0xF
    dec rdx
    jnz rotate18

    xor r8, r8
transpose:                              //transpose matrix
    mov r9, r8
    add r9, 4
transpose2:                             //inner loop of transpose
    mov r10, rdi
    add r10, r9
    mov ecx,[r10+4*r8]
    
    mov r11, rdi
    add r11, r8
    mov edx, [r11+4*r9]
    
    mov [r10+4*r8], edx                 //exchange elements
    mov [r11+4*r9], ecx
    
    add r9, 4
    cmp r9, 16
    jl transpose2                       //end of inner loop of transpose
    
    add r8, 4
    cmp r8, 12
    jl transpose                        //end of transpose
    
    dec rax
    jnz round                           //end of round
    
    xor rdx, rdx
addstart:                               //add original matrix after 20 rounds
    mov ecx, [rsi+4*rdx]
    add [rdi+4*rdx], ecx
    
    inc rdx
    cmp rdx, 16
    jl addstart
    
    ret





salsa20_core:

copy_simd:
    movdqu xmm0, [rsi]
    movdqu xmm1, [rsi + 16]
    movdqu xmm2, [rsi + 32]
    movdqu xmm3, [rsi + 48]             //mov matrix from memory into xmm registers 0-3
    movdqa xmm6, xmm0
    movdqa xmm7, xmm1
    movdqa xmm8, xmm2
    movdqa xmm9, xmm3                   //save a copy of the matrix in xmm registers 6-9

shuffle:    
    movdqa xmm4, xmm0
    pblendw xmm4, xmm1, 0xcc            //xmm4 := (x_(1,1), x_(2,2), x_(1,3), x_(2,4))
    
    pblendw xmm0, xmm1, 0x33            //xmm0 := (x_(2,1), x_(1,2), x_(2,3), x_(1,4))
    
    movdqa xmm1, xmm2
    pblendw xmm1, xmm3, 0xcc            //xmm1 := (x_(3,1), x_(4,2), x_(3,3), x_(4,4))
    
    pblendw xmm3, xmm2, 0xcc            //xmm3 := (x_(4,1), x_(3,2), x_(4,3), x_(3,4))
    
    movdqa xmm2, xmm3
    pblendw xmm2, xmm0, 0x3c            //xmm2 := (x_(4,1), x_(1,2), x_(2,3), x_(3,4))
    
    pblendw xmm0, xmm3, 0x3c            //xmm0 := (x_(2,1), x_(3,2), x_(4,3), x_(1,4))
    
    movdqa xmm3, xmm4
    pblendw xmm3, xmm1, 0xF0            //xmm3 := (x_(1,1), x_(2,2), x_(3,3), x_(4,4))
    
    pblendw xmm1, xmm4, 0xF0            //xmm1 := (x_(3,1), x_(4,2), x_(1,3), x_(2,4))
    
    mov rax, 20
    
round_simd:                             //execute 20 rounds
    
rotate7_simd:
    movdqa xmm4, xmm2
    paddd xmm4, xmm3
    movdqa xmm5, xmm4
    pslld xmm4, 7
    psrld xmm5, 25
    pxor xmm0, xmm4
    pxor xmm0, xmm5                     //add-rotate-xor operations with a rotation by 7
    
rotate9_simd:
    movdqa xmm4, xmm3
    paddd xmm4, xmm0
    movdqa xmm5, xmm4
    pslld xmm4, 9
    psrld xmm5, 23
    pxor xmm1, xmm4
    pxor xmm1, xmm5                     //add-rotate-xor operations with a rotation by 9
    
rotate13_simd:
    movdqa xmm4, xmm0
    paddd xmm4, xmm1
    movdqa xmm5, xmm4
    pslld xmm4, 13
    psrld xmm5, 19
    pxor xmm2, xmm4
    pxor xmm2, xmm5                     //add-rotate-xor operations with a rotation by 13

rotate18_simd:  
    movdqa xmm4, xmm1
    paddd xmm4, xmm2
    movdqa xmm5,xmm4
    pslld xmm4, 18
    psrld xmm5, 14
    pxor xmm3, xmm4
    pxor xmm3, xmm5                     //add-rotate-xor operations with a rotation by 18

transpose_simd: 
    movdqa xmm4, xmm0
    pshufd xmm0, xmm2, 0x39
    pshufd xmm2, xmm4, 0x93             //rotate xmm0 and xmm2 by 64 and exchange them
    pshufd xmm1, xmm1, 0x4e             //rotate xmm1 by 64
                                        //matrix is now transposed
    dec rax
    jnz round_simd

reorder:                           
    movdqa xmm4, xmm3                   
    pblendw xmm4, xmm2, 0xcc           
                                       
    pblendw xmm3, xmm2, 0x33
    
    movdqa xmm2, xmm1
    pblendw xmm2, xmm0, 0xcc
    
    pblendw xmm0, xmm1, 0xcc
    
    movdqa xmm1, xmm0
    pblendw xmm1, xmm3, 0x3c
    
    pblendw xmm3, xmm0, 0x3c
    
    movdqa xmm0, xmm4
    pblendw xmm0, xmm2, 0xF0
    
    pblendw xmm2, xmm4, 0xF0           //bring matrix elements back into original order          
                                       //the same computation from the beginning is needed but xmm0 and xmm3 are exchanged and xmm1 and xmm2 also

addstart_simd:           
    paddd xmm0, xmm6
    paddd xmm1, xmm7
    paddd xmm2, xmm8
    paddd xmm3, xmm9                   //add result to original matrix    
    
    movdqu [rdi], xmm0
    movdqu [rdi + 16], xmm1
    movdqu [rdi + 32], xmm2
    movdqu [rdi + 48], xmm3            //store resulting matrix at the destination
    
    ret
