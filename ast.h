#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include "hash.h"
#include "errors.h"

// Número fixo de filhos por simplicidade. 10 deve ser mais do que o suficiente
#define NUM_CHILDREN 10
#define MAX_ARGC 50

#define LOG_NSYM_AND_EXIT\
  do{\
    fprintf(stderr, "NULL head symbol at %s %s:%d\n", __func__, __FILE__, __LINE__);\
    exit(E_SYNTAX);\
  }while(0)

#define LOG_NHEAD_AND_EXIT\
  do{\
    fprintf(stderr, "NULL head at %s %s:%d\n", __func__, __FILE__, __LINE__);\
    exit(E_SYNTAX);\
  }while(0)

#define LOG_NSYM_AND_EXIT1(s,n)\
  do{\
    fprintf(stderr, "NULL head symbol at %s %s:%d\n", (s), __FILE__, (n));\
    exit(E_SYNTAX);\
  }while(0)

#define LOG_NHEAD_AND_EXIT1(s,n)\
  do{\
    fprintf(stderr, "NULL head at %s %s:%d\n", (s), __FILE__, (n));\
    exit(E_SYNTAX);\
  }while(0)

enum atype_t {
  // symbbol
  a_sym_t,
  // expr ops
  a_add_t,
  a_sub_t,
  a_mul_t,
  a_div_t,
  a_lt_t,
  a_gt_t,
  a_le_t,
  a_ge_t,
  a_eq_t,
  a_ne_t,
  a_pow_t,
  a_or_t,
  a_and_t,
  a_not_t,
  a_paren_t,
  a_op_t,
  // attr
  a_attr_t,
  a_vattr_t,
  // call
  a_call_t,
  // cmds
  a_read_t,
  a_print_t,
  a_ret_t,
  // cond
  a_cond_t,
  // while
  a_while_t,
  // for
  a_for_t,
  // lists
  a_csv_t,
  a_cmdl_t,
  a_vlist_t,
  a_plist_t,
  // blocos
  a_block_t,
  //decl
  a_decl_t,
  a_vdecl_t,
  // vector access
  a_vsym_t,
  // function decl
  a_fdecl_t,
  a_fsig_t,
  // kw
  a_kwc_t,
  a_kwi_t,
  a_kwf_t,
  a_kwb_t,
  // typed var
  a_tvar_t
};

struct ast_node {
  enum atype_t atype;
  struct hash_node *symbol;
  struct ast_node *children[NUM_CHILDREN];
};

struct ast_node *AST_HEAD;

/*
 * Retorna um novo nodo ast alocado dinamicamente com tipo atype, símbolo
 * symbol, e nchildren do tipo struct ast_node * (passados em __VA_LIST__)
 */
struct ast_node *
ast_create(enum atype_t atype, struct hash_node *symbol, size_t nchildren, ...);

/*
 * Imprime para stdout o nodo passado e toda sua sub-árvore, identando level
 */
void
ast_print(struct ast_node *head, int level);

/*
 * Imprime para stdout o nodo passado, identando level
 */
void
ast_print_node(struct ast_node *head, int level);

/*
 * Imprime para stdout um programa baseado na ast
 */
void
ast_print_disassemble(FILE *out, struct ast_node *head);

/*
 * Se head não tem pelo menos nchildren != NULL, loga erro e exit
 */
void
ast_validate_children(struct ast_node *head, size_t nchildren, const char *caller, int line);

/*
 * Se head não tem pelo menos nchildren != NULL && cujo symbol != NULL, loga
 * erro e exit
 */
void
ast_validate_symbol(struct ast_node *head, size_t nchildren, const char *caller, int line);
