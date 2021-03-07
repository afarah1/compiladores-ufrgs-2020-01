#pragma once

#include <stdio.h>
#include <stdbool.h>

/* Utilize um primo para o tamanho para bom hashing */
#define HASH_SIZE 997
struct hash_node *HASH_TABLE[HASH_SIZE];

enum hashnature_t {
  hn_id_t, hn_int_t, hn_float_t, hn_char_t, hn_bool_t, hn_str_t, hn_arg_t,
  hn_var_t, hn_func_t, hn_vec_t, hn_label_t
};

enum hashtype_t {
  ht_int_t, ht_float_t, ht_char_t, ht_bool_t, ht_str_t,
  ht_unknown_t
};

struct hash_typeinfo {
  enum hashnature_t nature;
  enum hashtype_t type;
};

struct ast_node;

struct hash_node {
  struct hash_typeinfo typeinfo;
  struct ast_node *astinfo;
  char *key;
  struct hash_node *next;
};

bool
hash_is_str(struct hash_node *node);

/*
 * Setter para o valor do nodo. Usado para debug...
 */
void
hash_set_typeinfo(struct hash_node *node, struct hash_typeinfo typeinfo, const char *caller, int line);

/*
 * Setter para o valor do nodo. Usado para debug...
 */
void
hash_set_astinfo(struct hash_node *node, struct ast_node *astinfo, const char *caller, int line);

/*
 * Insere um nodo na tabela com chave key e valor val, retorna o nodo inserido.
 * Caso já exista nodo com essa chave, retorna o existente, sem editar o valor.
 */
struct hash_node *
hash_insert(char *key, struct hash_typeinfo typeinfo);

/*
 * Inicializa a tabela
 */
void
hash_init(void);

/*
 * Imprime todo o conteúdo da tabela
 */
void
hash_print(void);

/*
 * Libera todo o conteúdo da tabela
 */
void
hash_free(void);

/*
 * Retorna um nodo para a chave, nulo se nenhum.
 */
struct hash_node *
hash_find(char *key);

/*
 * Imprime os itens da hash cujo valor é hn_id_t
 */
int
hash_fprint_ids(FILE *f, const char *prefix);

/*
 * Retorna um nodo dummy
 */
struct hash_node *
hash_create_dummy(void);

/*
 * Retorna um nodo label (dummy)
 */
struct hash_node *
hash_create_label(void);
