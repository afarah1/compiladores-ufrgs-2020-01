#pragma once

#include "hash.h"
#include "dry.h"
#include "ast.h"

enum ttype_t {
  // symbbol
  t_sym_t,
  t_vread_t,
  // expr ops
  t_add_t,
  t_sub_t,
  t_mul_t,
  t_div_t,
  t_le_t,
  t_ge_t,
  t_gt_t,
  t_lt_t,
  t_eq_t,
  t_ne_t,
  t_or_t,
  t_and_t,
  t_pow_t,
  t_not_t,
  // attr
  t_copy_t,
  t_vcopy_t,
  // call
  t_call_t,
  t_arg_t,
  t_fstart_t,
  t_fend_t,
  t_ret_t,
  // jmp
  t_jmp_t,
  t_jmpf_t,
  t_label_t,
  // etc
  t_read_t,
  t_print_t,
  t_unk_t
};

struct tac_node {
  enum ttype_t ttype;
  struct hash_node *ans, *op1, *op2;
  struct tac_node *prev, *next;
};

/*
 * Aloca dinâmicamente um novo node inicializado com os params passados
 */
struct tac_node *
tac_create(enum ttype_t ttype, DRY(struct hash_node *, ans, op1, op2));

/*
 * Concatena duas listas e retorna a causa. Isto é, se temos:
 *
 * head1 <-> tail1
 * head2 <-> tail2
 *
 * O resultado será
 *
 * head1 <-> tail1 <-> head2 <-> tail2
 * ans = tail2
 *
 * Se tail1 é nulo, retorna tail2, e vice-versa.
 *
 */
struct tac_node *
tac_cat_tails(DRY(struct tac_node *, tail1, tail2));

/*
 * Gera código dado a AST, retornando a cabeça da lista gerada
 */
struct tac_node *
tac_gencode(struct ast_node *head);

/*
 * Imprime a lista da frente para trás
 */
void
tac_print(struct tac_node *head);

/*
 * Imprime a lista de trás para frente
 */
void
tac_printb(struct tac_node *head);

/*
 * Dado a cauda de uma lista, retorna a cabeça
 */
struct tac_node *
tac_get_head(struct tac_node *tail);

/*
 * Seja nodo = [ANS, OP1, OP2], aborta se `for OP in min(len(nodo), nops), !OP`
 */
void
tac_validate_ops(struct tac_node *tnode, size_t nops, const char *caller, int line);
