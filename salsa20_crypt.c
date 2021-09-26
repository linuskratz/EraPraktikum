#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include "salsa20_core.h"

/*Function to left rotate n by d bits
source: https://www.geeksforgeeks.org/rotate-bits-of-an-integer/ (accessed on 7.6.21)
*/
int32_t leftRotate(uint32_t n, uint8_t d)
{
  /* In n<<d, last d bits are 0. To
     put first 3 bits of n at
    last, do bitwise or of n<<d
    with n >>(INT_BITS - d) */
  return (n << d) | (n >> (32 - d));
}




void calc_salsa_arr(uint32_t *output, const uint32_t *input)
{
  uint32_t *a = malloc(sizeof(uint32_t) * 16);
  memcpy(a, input, 64);
  if (a)
  {
    uint8_t rotateVal[4] = {7, 9, 13, 18};
    for (size_t iterations = 0; iterations < 10; iterations++)
    {

      // non-transposed matrix
      for (size_t i = 0; i < 4; i++)
      {
        size_t icord = (i + 1) % 4; // i coordinate
        for (size_t j = 0; j < 4; j++)
        { // j coordinate
          //example first iteration a(1,0) = ((a(0,0) + a(3, 0)) <<< 7) ^ a(1,0) 
          a[icord * 4 + j] = leftRotate((a[((icord - 1) % 4) * 4 + j] + a[((icord + 2) % 4) * 4 + j]), rotateVal[i]) ^ a[icord * 4 + j];
          //move down a row 
          icord = (icord + 1) % 4;
        }
      }

      //transposed matrix 
      for (size_t j = 0; j < 4; j++)
      {
        size_t jcord = (j + 1) % 4; 
        for (size_t i = 0; i < 4; i++)
        {
          //transposing the matrix by switching indizes
          a[i * 4 + jcord] = leftRotate((a[i * 4 + ((jcord - 1) % 4)] + a[i * 4 + ((jcord + 2) % 4)]), rotateVal[j]) ^ a[i * 4 + jcord];
          jcord = (jcord + 1) % 4;
        }
      }
    }
    //add matrices to output
    for (size_t i = 0; i < 4; i++)
    {
      for (size_t j = 0; j < 4; j++)
      {
        output[i * 4 + j] = a[i * 4 + j] + input[i * 4 + j];
      }
    }
  }
}


/*
input = ((a1, d2, c3, b4),
         (b1, a2, d3, c4),
         (c1, b2, a3, d4),
         (d1, c2, b3, a4))
*/
void calc_salsa_intrinsics(uint32_t *output, const uint32_t *input)
{
  // reg 1-4 for input; reg 5-6 for calculations; reg A-D for diagonal coordinates

  //(a1, d2, c3, b4)
  __m128i reg1 = _mm_loadu_si128((__m128i *)input);

  //(b1, a2, d3, c4)
  __m128i reg2 = _mm_loadu_si128((__m128i *)(input + 4));

  //(c1, b2, a3, d4)
  __m128i reg3 = _mm_loadu_si128((__m128i *)(input + 8));

  //(d1, c2, b3, a4)
  __m128i reg4 = _mm_loadu_si128((__m128i *)(input + 12));

  //there is no blend_epi32
  //(a1, a2, c3, c4)
  __m128i reg5 = _mm_blend_epi16(reg1, reg2, 0xcc);

  //(b1, b2, d3, d4)
  __m128i reg6 = _mm_blend_epi16(reg2, reg3, 0xcc);

  //(c1, c2, a3, a4)
  __m128i regC = _mm_blend_epi16(reg3, reg4, 0xcc);

  //(d1, d2, b3, b4)
  __m128i regD = _mm_blend_epi16(reg4, reg1, 0xcc);

  //(a1,a2,a3,a4)
  __m128i regA = _mm_blend_epi16(reg5, regC, 0xF0);

  //(c1,c2,c3,c4)
  regC = _mm_blend_epi16(regC, reg5, 0xF0);

  //(b1,b2,b3,b4)
  __m128i regB = _mm_blend_epi16(reg6, regD, 0xF0);

  //(d1,d2,d3,d4)
  regD = _mm_blend_epi16(regD, reg6, 0xF0);

  for (size_t i = 0; i < 20; i++)
  {

    //B = ((A + D <<< 7) xor B)
    reg5 = _mm_add_epi32(regA, regD);
    reg6 = _mm_srli_epi32(reg5, 25); // 32-7
    reg5 = _mm_slli_epi32(reg5, 7);
    reg5 = _mm_or_si128(reg5, reg6);
    regB = _mm_xor_si128(reg5, regB);
    //C = ((A + B <<< 9) xor C)
    reg5 = _mm_add_epi32(regA, regB);
    reg6 = _mm_srli_epi32(reg5, 23);
    reg5 = _mm_slli_epi32(reg5, 9);
    reg5 = _mm_or_si128(reg5, reg6);
    regC = _mm_xor_si128(reg5, regC);
    //D = ((C+B <<< 13) xor D)
    reg5 = _mm_add_epi32(regC, regB);
    reg6 = _mm_srli_epi32(reg5, 19);
    reg5 = _mm_slli_epi32(reg5, 13);
    reg5 = _mm_or_si128(reg5, reg6);
    regD = _mm_xor_si128(reg5, regD);
    //A = ((D+C <<< 18) xor A)
    reg5 = _mm_add_epi32(regD, regC);
    reg6 = _mm_srli_epi32(reg5, 14);
    reg5 = _mm_slli_epi32(reg5, 18);
    reg5 = _mm_or_si128(reg5, reg6);
    regA = _mm_xor_si128(reg5, regA);
    //transpose matrix
    // shuffle dwords in C by 2
    regC = _mm_shuffle_epi32(regC, 0x4e);

    // rotate dwords in B(right), D(left) by 1 and exchange B and D
    reg5 = _mm_shuffle_epi32(regB, 0x93);
    reg6 = _mm_shuffle_epi32(regD, 0x39);
    regB = reg6;
    regD = reg5;
  }

  // reverse blend to receive original matrix
  // step1
  //(a1, a2, c3, c4)
  reg5 = _mm_blend_epi16(regA, regC, 0xF0);
  //(c1, c2, a3, a4)
  reg6 = _mm_blend_epi16(regC, regA, 0xF0);
  //(b1, b2, d3, d4)
  regC = _mm_blend_epi16(regB, regD, 0xF0);
  //(d1, d2, b3, b4)
  regA = _mm_blend_epi16(regD, regB, 0xF0);

  //step2
  //(d1, c2, b3, a4)
  regD = _mm_blend_epi16(regA, reg6, 0xcc);

  //(a1, d2, c3, b4)
  regA = _mm_blend_epi16(reg5, regA, 0xcc);

  //(b1, a2, d3, c4)
  regB = _mm_blend_epi16(regC, reg5, 0xcc);

  //(c1, b2, a3, d4)
  regC = _mm_blend_epi16(reg6, regC, 0xcc);

  // add output = input + A
  regA = _mm_add_epi32(regA, reg1);
  regB = _mm_add_epi32(regB, reg2);
  regC = _mm_add_epi32(regC, reg3);
  regD = _mm_add_epi32(regD, reg4);

  _mm_storeu_si128((__m128i *)output, regA);
  _mm_storeu_si128((__m128i *)(output + 4), regB);
  _mm_storeu_si128((__m128i *)(output + 8), regC);
  _mm_storeu_si128((__m128i *)(output + 12), regD);
}
