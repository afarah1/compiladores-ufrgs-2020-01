#pragma once
#include <stdio.h>
#include "tac.h"

/*
 * Dado uma lista de TACs, imprime ASM
 */
void
asm_print(FILE *out, struct tac_node *thead, struct hash_node **hhead, size_t hsize);
