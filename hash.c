#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "logging.h"

struct _hash_node_and_addr {
  struct hash_node *node;
  size_t addr;
};


/*
 * Retorna um endreço dada uma chave.
 */
static size_t
hash_address(char *key)
{
  size_t ans = 1;
  for (size_t i = 0; i < strlen(key); i++)
    ans = (ans * (size_t)key[i]) % HASH_SIZE + 1;
  return ans - 1;
}

void
hash_init(void)
{
  for (size_t i = 0; i < HASH_SIZE; i++)
    HASH_TABLE[i] = NULL;
}

/*
 * Procura uma chave na hash, preenchendo a estrutura out, pré-alocada, com o
 * nodo encontrado e seu endereço. Se não encontrar, o nodo é NULL e o endereço
 * é zero. Para uso interno.
 */
static void
_hash_find(char *key, struct _hash_node_and_addr *out)
{
  size_t addr = hash_address(key);
  struct hash_node *node = HASH_TABLE[addr];
  // isto não é muito inteligente, mas funciona
  while (node != NULL) {
    if (strcmp(node->key, key) == 0) {
      out->node = node;
      out->addr = addr;
      return;
    } else {
      node = node->next;
    }
  }
  out->node = NULL;
  out->addr = 0;
  return;
}

struct hash_node *
hash_find(char *key)
{
  struct _hash_node_and_addr node_and_addr;
  _hash_find(key, &node_and_addr);
  return node_and_addr.node;
}

struct hash_node *
hash_insert(char *key, struct hash_typeinfo typeinfo)
{
  // ve se o nodo já está na hash
  struct _hash_node_and_addr node_and_addr;
  _hash_find(key, &node_and_addr);
  if (node_and_addr.node != NULL)
    return node_and_addr.node;

  // não achamos o nodo, cria e adiciona
  struct hash_node *ans = malloc(sizeof(*ans));
  if (!ans)
    REPORT_AND_EXIT;
  size_t addr = hash_address(key);
  ans->typeinfo = typeinfo;
  ans->astinfo = NULL;
  ans->key = strdup(key);
  ans->next = HASH_TABLE[addr];
  HASH_TABLE[addr] = ans;
  return ans;
}

static char *
hash_nature_to_str(enum hashnature_t nature)
{
  switch (nature) {
    case hn_id_t: return "id";
    case hn_int_t: return "int";
    case hn_float_t: return "float";
    case hn_char_t: return "char";
    case hn_bool_t: return "bool";
    case hn_str_t: return "str";
    case hn_var_t: return "var";
    case hn_func_t: return "id";
    case hn_vec_t: return "vec";
    default: return "unknown";
  }
}

static char *
hash_type_to_str(enum hashtype_t type)
{
  switch (type) {
    case ht_int_t: return "int";
    case ht_float_t: return "float";
    case ht_char_t: return "char";
    case ht_bool_t: return "bool";
    case ht_str_t: return "str";
    case ht_unknown_t:
    default:
      return "unknown";
  }
}

static void
hash_print_typeinfo(struct hash_typeinfo typeinfo)
{
  printf("nature=%s; type=%s; ", hash_nature_to_str(typeinfo.nature),
      hash_type_to_str(typeinfo.type));
}

void
hash_print(void)
{
  struct hash_node *node = NULL;
  for (size_t i = 0; i < HASH_SIZE; i++) {
    node = HASH_TABLE[i];
    printf("address %zu; ", i);
    while (node != NULL) {
      printf("key=%s; ", node->key);
      hash_print_typeinfo(node->typeinfo);
      node = node->next;
    }
    printf("\n");
  }
}

void
hash_free(void)
{
  for (size_t i = 0; i < HASH_SIZE; i++) {
    struct hash_node *node = HASH_TABLE[i];
    while (node != NULL) {
      struct hash_node *next = node->next;
      if (node->key != NULL)
        free(node->key);
      free(node);
      node = next;
    }
  }
}

void
hash_set_typeinfo(struct hash_node *node, struct hash_typeinfo typeinfo, const char *caller, int line)
{
  LOG_DEBUG("nature: %d->%d; type: %d->%d @ %s:%d\n", node->typeinfo.nature, typeinfo.nature, node->typeinfo.type, typeinfo.type, caller, line);
  node->typeinfo = typeinfo;
}

int
hash_fprint_ids(FILE *f, const char *prefix)
{
  int ans = 0;
  for (size_t i = 0; i < HASH_SIZE; i++) {
    struct hash_node *node = HASH_TABLE[i];
    while (node != NULL) {
      struct hash_node *next = node->next;
      if ((node->key != NULL) && (node->typeinfo.nature == hn_id_t)) {
        fprintf(f, "%s %s\n", prefix, node->key);
        ans++;
      }
      node = next;
    }
  }
  return ans;
}

void
hash_set_astinfo(struct hash_node *node, struct ast_node *astinfo, const char *caller, int line)
{
  // accepts astinfo == NULL
  LOG_DEBUG("astinfo: %p->%p; @ %s:%d\n", (void *)(node->astinfo), (void *)astinfo, caller, line);
  node->astinfo = astinfo;
}

struct hash_node *
hash_create_dummy(void)
{
  static int DUMMYCT = 0;
  char key[9] = "dummyXXX"; // Reminder: \0
  snprintf(key, 9, "dummy%d", DUMMYCT++ % 999);
  struct hash_typeinfo typeinfo = { hn_var_t, ht_unknown_t };
  return hash_insert(strdup(key), typeinfo);
}

struct hash_node *
hash_create_label(void)
{
  static int LABELCT = 0;
  char key[9] = "labelXXX"; // Reminder: \0
  snprintf(key, 9, "label%d", LABELCT++ % 999);
  struct hash_typeinfo typeinfo = { hn_label_t, ht_unknown_t };
  return hash_insert(strdup(key), typeinfo);
}


bool
hash_is_str(struct hash_node *node)
{
  if (!node)
    return false;

  return ((node->typeinfo.nature == hn_str_t) || (node->typeinfo.type == ht_str_t));
}
