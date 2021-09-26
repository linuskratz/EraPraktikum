#ifndef SALSA_H
#define SALSA_H

#include <stddef.h>

//Assembler implementation
void salsa20_core_naive(u_int32_t *output, const u_int32_t *input);
void salsa20_core(u_int32_t *output, const u_int32_t *input);

/*C implementation*/
void calc_salsa_arr(u_int32_t *output, const u_int32_t *input);
void calc_salsa_intrinsics(u_int32_t *output, const u_int32_t *input);

#endif
