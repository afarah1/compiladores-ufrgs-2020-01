#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dry.h"
#include "tac.h"
#include "hash.h"
#include "logging.h"

struct tac_node *
tac_create(enum ttype_t ttype, DRY(struct hash_node *, ans, op1, op2))
{
  struct tac_node *ret = malloc(sizeof(*ret));

  ret->ttype = ttype;
  ret->ans = ans;
  ret->op1 = op1;
  ret->op2 = op2;
  ret->prev = NULL;
  ret->next = NULL;

  return ret;
}

static void
tac_print_node(struct tac_node *head)
{
  // Assumes head <> NULL

  switch (head->ttype) {
    case t_print_t:
      printf("TAC_PRINT");
      break;
    case t_ret_t:
      printf("TAC_RET");
      break;
    case t_read_t:
      printf("TAC_READ");
      break;
    case t_le_t:
      printf("TAC_LE");
      break;
    case t_ge_t:
      printf("TAC_GE");
      break;
    case t_gt_t:
      printf("TAC_GT");
      break;
    case t_lt_t:
      printf("TAC_LT");
      break;
    case t_eq_t:
      printf("TAC_EQ");
      break;
    case t_ne_t:
      printf("TAC_NE");
      break;
    case t_or_t:
      printf("TAC_OR");
      break;
    case t_and_t:
      printf("TAC_AND");
      break;
    case t_pow_t:
      printf("TAC_POW");
      break;
    case t_not_t:
      printf("TAC_NOT");
      break;
    case t_copy_t:
      printf("TAC_COPY");
      break;
    case t_vcopy_t:
      printf("TAC_VCOPY");
      break;
    case t_vread_t:
      printf("TAC_VREAD");
      break;
    case t_sym_t:
      if (LOG_LEVEL == LOG_LEVEL_DEBUG) {
        printf("TAC_SYM");
        break;
      } else {
        return;
      }
    case t_label_t:
      printf("TAC_LABEL");
      break;
    case t_add_t:
      printf("TAC_ADD");
      break;
    case t_sub_t:
      printf("TAC_SUB");
      break;
    case t_mul_t:
      printf("TAC_MUL");
      break;
    case t_div_t:
      printf("TAC_DIV");
      break;
    case t_call_t:
      printf("TAC_CALL");
      break;
    case t_fstart_t:
      printf("TAC_FUNC_START");
      break;
    case t_fend_t:
      printf("TAC_FUNC_END");
      break;
    case t_arg_t:
      printf("TAC_ARG");
      break;
    case t_jmp_t:
      printf("TAC_JUMP");
      break;
    case t_jmpf_t:
      printf("TAC_JUMP_FALSE");
      break;
    default:
      fprintf(stderr, "Unknown tac type %d\n", head->ttype);
      break;
  }

  printf(", %s, %s, %s\n",
      (head->ans ? head->ans->key : ""),
      (head->op1 ? head->op1->key : ""),
      (head->op2 ? head->op2->key : ""));
}

void
tac_print(struct tac_node *head)
{
  while (head) {
    tac_print_node(head);
    head = head->next;
  }
}

void
tac_printb(struct tac_node *head)
{
  while (head) {
    tac_print_node(head);
    head = head->prev;
  }
}

static void
tac_validate_children(struct tac_node **tarr, size_t nchildren, const char *caller, int line)
{
  if ((NUM_CHILDREN < nchildren) || (tarr == NULL))
    LOG_NHEAD_AND_EXIT1(caller, line);

  for (size_t i = 0; i < nchildren; i++) {
    if (tarr[i] == NULL)
      LOG_NHEAD_AND_EXIT1(caller, line);
  }
}

struct tac_node *
tac_cat_tails(DRY(struct tac_node *, tail1, tail2))
{
  if (!tail1)
    return tail2;

  if (!tail2)
    return tail1;

  if ((tail1->next != NULL) || (tail2->next != NULL)) {
    fprintf(stderr, "Not a tail. t1n=%p, t2n=%p\n", (void *)(tail1->next), (void *)(tail2->next));
    exit(E_SYNTAX);
  }

  // Retrocede ao início da lista 2
  struct tac_node *head = tail2;
  while (head->prev != NULL)
    head = head->prev;

  // Concatena e retorna a cauda
  head->prev = tail1;
  tail1->next = head;
  return tail2;
}

static struct tac_node *
tac_cat_tails_arr(struct tac_node **tarr, int nchildren)
{
 // Inicializa
  struct tac_node *ans = NULL;
  int nchild = 0;

  // Avança ao último filho + 1
  while ((nchild < nchildren) && (tarr[nchild] != NULL))
    nchild++;

  // Concatena todos
  for (int i = nchild - 1; i >= 0; i--) {
    ans = tac_cat_tails(tarr[i], ans);
  }

  return ans;
}

static enum ttype_t
tac_ttype_from_atype(enum atype_t atype)
{
  switch (atype) {
    case a_sym_t:
      return t_sym_t;
    case a_vsym_t:
      return t_vread_t;
    case a_add_t:
      return t_add_t;
    case a_sub_t:
      return t_sub_t;
    case a_mul_t:
      return t_mul_t;
    case a_div_t:
      return t_div_t;
    case a_le_t:
      return t_le_t;
    case a_ge_t:
      return t_ge_t;
    case a_lt_t:
      return t_lt_t;
    case a_gt_t:
      return t_gt_t;
    case a_eq_t:
      return t_eq_t;
    case a_ne_t:
      return t_ne_t;
    case a_or_t:
      return t_or_t;
    case a_and_t:
      return t_and_t;
    case a_pow_t:
      return t_pow_t;
    case a_not_t:
      return t_not_t;
    case a_attr_t:
      return t_copy_t;
    case a_vattr_t:
      return t_vcopy_t;
    default:
     return t_unk_t;
  }
}

static bool
tac_is_expr(enum ttype_t ttype)
{
  switch (ttype) {
    case t_sym_t:
    case t_vread_t:
    case t_add_t:
    case t_sub_t:
    case t_mul_t:
    case t_div_t:
    case t_le_t:
    case t_ge_t:
    case t_gt_t:
    case t_lt_t:
    case t_eq_t:
    case t_ne_t:
    case t_or_t:
    case t_and_t:
    case t_pow_t:
    case t_not_t:
    case t_call_t:
      return true;
    default:
      return false;
  }
}

static int
tac_count_dups(struct tac_node *vmemb)
{
  int ans = 0;

  if (!vmemb)
    return ans;

  switch (vmemb->ttype) {
    case t_add_t:
    case t_sub_t:
    case t_mul_t:
    case t_div_t:
    case t_le_t:
    case t_ge_t:
    case t_gt_t:
    case t_lt_t:
    case t_eq_t:
    case t_ne_t:
    case t_or_t:
    case t_and_t:
    case t_pow_t:
    case t_not_t:
    case t_vread_t:
      // two operands, each of which may have two operands as well
      ans += tac_count_dups(vmemb->prev);
      ans += tac_count_dups(vmemb->prev->prev);
      ans += 2;
      // fall through
    default:
      break;
  }

  return ans;
}

static struct tac_node *
tac_gencode_expr(enum atype_t atype, struct tac_node **tarr)
{
  tac_validate_children(tarr, 2, __func__, __LINE__);
  return tac_cat_tails(tac_cat_tails(tarr[0], tarr[1]),
    tac_create(tac_ttype_from_atype(atype), hash_create_dummy(), tarr[0]->ans, tarr[1]->ans));
}

static struct tac_node *
tac_gencode_sym(struct ast_node *head, size_t nchild)
{
  char *key = "";

  // Folha na AST, valida que não tem nada abaixo dele
  if (nchild > 0) {
    if (head->symbol != NULL)
      key = head->symbol->key;
    fprintf(stderr, "a_sym_t && nchild=%zu > 0, key=%s\n", nchild, key);
    exit(E_SYNTAX);
  }

  // Como é o folha, simplesmente cria a TAC e retorna
  return tac_create(t_sym_t, head->symbol, NULL, NULL);
}

static struct tac_node *
tac_gencode_vsym(struct tac_node **tarr)
{
  tac_validate_children(tarr, 2, __func__, __LINE__);
  return tac_cat_tails(tac_cat_tails(tarr[0], tarr[1]),
      tac_create(t_vread_t, hash_create_dummy(), tarr[0]->ans, tarr[1]->ans));
}

static struct tac_node *
tac_gencode_attr(struct tac_node **tarr)
{
  tac_validate_children(tarr, 2, __func__, __LINE__);
  return tac_cat_tails(tac_cat_tails(tarr[0], tarr[1]),
      tac_create(t_copy_t, tarr[0]->ans, tarr[1]->ans, NULL));
}

static struct tac_node *
tac_gencode_vattr(struct tac_node **tarr)
{
  tac_validate_children(tarr, 3, __func__, __LINE__);
  return tac_cat_tails(tac_cat_tails(tarr[0], tarr[1]),
      tac_cat_tails(tarr[2], tac_create(t_vcopy_t, tarr[0]->ans, tarr[1]->ans, tarr[2]->ans)));
}

static struct tac_node *
tac_gencode_call(struct tac_node **tarr)
{
  tac_validate_children(tarr, 1, __func__, __LINE__);

  int ndups = 0;

  struct tac_node *vmemb = tarr[1],
                  *prev = tarr[1],
                  *ans = NULL;

  while (vmemb != NULL) {
    ans = tac_cat_tails(prev, tac_create(t_arg_t, vmemb->ans, tarr[0]->ans, NULL));
    // TODO is there a better way?
    for (ndups = tac_count_dups(vmemb); ndups > 0; ndups--)
      vmemb = vmemb->prev;
    vmemb = vmemb->prev;
    prev = ans;
  }

  return tac_cat_tails(ans, tac_create(t_call_t, hash_create_dummy(), tarr[0]->ans, NULL));
}

static struct tac_node *
tac_gencode_fdecl(struct ast_node *head, struct tac_node **tarr)
{
  tac_validate_children(tarr, 1, __func__, __LINE__);
  ast_validate_children(head, 2, __func__, __LINE__);
  ast_validate_symbol(head->children[0], 1, __func__, __LINE__);
  // we could use tarr->prev for everyhting but head->children is less confusing
  struct tac_node *ans = tac_cat_tails(tac_cat_tails(tarr[0], tarr[1]),
      tac_create(t_fend_t, head->children[0]->children[0]->symbol, NULL, NULL));

  return tac_cat_tails(tac_create(t_fstart_t, head->children[0]->children[0]->symbol, NULL, NULL), ans);
}

static struct tac_node *
tac_gencode_cond(struct tac_node **tarr)
{
  tac_validate_children(tarr, 1, __func__, __LINE__);

  struct hash_node *hlabel = hash_create_label();
  struct tac_node *tlabel = tac_create(t_label_t, hlabel, NULL, NULL);

  // apenas para legibilidade
  struct tac_node *texpr = tarr[0],
                  *tcmd  = tarr[1],
                  *telse = tarr[2],
                  *tjmpf = tac_create(t_jmpf_t, hlabel, texpr->ans, NULL);

  if (telse == NULL) {
    struct tac_node *tarr2[4] = {
      texpr,
      tjmpf,
      tcmd,
      tlabel
    };
    return tac_cat_tails_arr(tarr2, 4);
  } else {
    struct hash_node *hlabel2 = hash_create_label();
    struct tac_node *tlabel2 = tac_create(t_label_t, hlabel2, NULL, NULL);

    // apenas para legibilidade
    struct tac_node *tjmp = tac_create(t_jmp_t, hlabel2, NULL, NULL);

    struct tac_node *tarr2[7] = {
      texpr,
      tjmpf,
      tcmd,
      tjmp,
      tlabel,
      telse,
      tlabel2
    };

    return tac_cat_tails_arr(tarr2, 7);
  }
}

static struct tac_node *
tac_gencode_loop(struct tac_node **tarr)
{
  tac_validate_children(tarr, 4, __func__, __LINE__);

  struct hash_node *hlabel_check = hash_create_label(),
                   *hlabel_end = hash_create_label();

  //return tac_cat_tails(tac_cat_tails(tarr[0], tarr[1]),
  // tac_create(tac_ttype_from_atype(atype), hash_create_dummy(), tarr[0]->ans, tarr[1]->ans));

  // for (id : ini, endc, inc)
  //   cmd
  //
  // cp id, ini
  // label_check:
  // lt dummy1, id, endc
  // jf label_end dummy1
  // cmd
  // add id id, inc
  // jmp label_check
  // label_end:

  // apenas para legibilidade
  struct tac_node *tlabel_check = tac_create(t_label_t, hlabel_check, NULL, NULL),
                  *tlabel_end = tac_create(t_label_t, hlabel_end, NULL, NULL),
                  *tid = tarr[0],
                  *tini = tarr[1],
                  *tendc = tarr[2],
                  *tinc = tarr[3],
                  *tcmd = tarr[4], // might be NULL
                  *tattr = tac_create(t_copy_t, tid->ans, tini->ans, NULL),
                  *tlt = tac_create(t_lt_t, hash_create_dummy(), tid->ans, tendc->ans),
                  *tjf = tac_create(t_jmpf_t, hlabel_end, tlt->ans, NULL),
                  *tadd = tac_create(t_add_t, tid->ans, tid->ans, tinc->ans),
                  *tj = tac_create(t_jmp_t, hlabel_check, NULL, NULL);

  if (tcmd == NULL) {
    struct tac_node *tarr2[11] = {
      tid,
      tini,
      tendc,
      tinc,
      tattr,
      tlabel_check,
      tlt,
      tjf,
      tadd,
      tj,
      tlabel_end
    };

    return tac_cat_tails_arr(tarr2, 11);
  } else {
    struct tac_node *tarr2[12] = {
      tid,
      tini,
      tendc,
      tinc,
      tattr,
      tlabel_check,
      tlt,
      tjf,
      tcmd,
      tadd,
      tj,
      tlabel_end
    };

    return tac_cat_tails_arr(tarr2, 12);

  }
}

static struct tac_node *
tac_gencode_read(struct tac_node **tarr)
{
  tac_validate_children(tarr, 1, __func__, __LINE__);

  return tac_cat_tails(tarr[0], tac_create(t_read_t, tarr[0]->ans, NULL, NULL));
}

static struct tac_node *
tac_gencode_whiledo(struct tac_node **tarr)
{
  tac_validate_children(tarr, 1, __func__, __LINE__);

  struct hash_node *hlabel_check = hash_create_label(),
                   *hlabel_end = hash_create_label();

  struct tac_node *tlabel_check = tac_create(t_label_t, hlabel_check, NULL, NULL),
                  *tlabel_end = tac_create(t_label_t, hlabel_end, NULL, NULL),
                  *texpr = tarr[0],
                  *tcmd = tarr[1], // can be NULL
                  *tjf = tac_create(t_jmpf_t, hlabel_end, texpr->ans, NULL),
                  *tj = tac_create(t_jmp_t, hlabel_check, NULL, NULL);

  // while ( expr ) cmd
  //
  // label_check:
  // eval dummy expr
  // jf dummy label_end
  // cmd
  // j label_check
  // label_end:

  if (tcmd != NULL) {
    struct tac_node *tarr2[6] = {
      tlabel_check,
      texpr,
      tjf,
      tcmd,
      tj,
      tlabel_end
    };

    return tac_cat_tails_arr(tarr2, 6);
  } else {
    struct tac_node *tarr2[5] = {
      tlabel_check,
      texpr,
      tjf,
      tj,
      tlabel_end
    };

    return tac_cat_tails_arr(tarr2, 5);
  }
}

static struct tac_node *
tac_gencode_ret(struct tac_node **tarr)
{
  tac_validate_children(tarr, 1, __func__, __LINE__);

  return tac_cat_tails(tarr[0], tac_create(t_ret_t, tarr[0]->ans, NULL, NULL));
}

static struct tac_node *
tac_gencode_print(struct tac_node **tarr)
{
  tac_validate_children(tarr, 1, __func__, __LINE__);

  struct tac_node *tcsvi = tarr[0],
                  *ans = NULL;
  do {
    if (tac_is_expr(tcsvi->ttype)) { // string is t_sym_t as well
      ans = tac_cat_tails(ans, tac_create(t_print_t, tcsvi->ans, NULL, NULL));
      tcsvi = tcsvi->prev;
    } else {
      tcsvi = NULL;
    };
  } while (tcsvi != NULL);

  return ans;
}

struct tac_node *
tac_gencode(struct ast_node *head)
{
  struct tac_node *ans = NULL;

  if (!head)
    return ans;

  struct tac_node *_tarr[NUM_CHILDREN] = { NULL },
                  *tarr[NUM_CHILDREN] = { NULL };

  size_t nchild = 0;
  while ((nchild < NUM_CHILDREN) && (head->children[nchild] != NULL)) {
    _tarr[nchild] = tac_gencode(head->children[nchild]);
    nchild++;
  }

  // Remove NULL from the middle of tarr (empty block, etc)
  size_t j = 0;
  for (size_t i = 0; i < NUM_CHILDREN; i++) {
    if (_tarr[i] != NULL)
      tarr[j++] = _tarr[i];
  }

  switch (head->atype) {
    case a_sym_t: // done
      ans = tac_gencode_sym(head, nchild);
      break;
    case a_vsym_t: // done
      ans = tac_gencode_vsym(tarr);
      break;
    case a_add_t:
    case a_sub_t:
    case a_mul_t:
    case a_div_t:
    case a_lt_t :
    case a_gt_t :
    case a_or_t :
    case a_pow_t:
    case a_not_t:
    case a_and_t:
    case a_le_t :
    case a_ge_t :
    case a_eq_t :
    case a_ne_t :
      ans = tac_gencode_expr(head->atype, tarr);
      break;
    case a_attr_t: // done
      ans = tac_gencode_attr(tarr);
      break;
    case a_vattr_t: // done
      ans = tac_gencode_vattr(tarr);
      break;
    case a_call_t: // done
      ans = tac_gencode_call(tarr);
      break;
    case a_fdecl_t: // done
      ans = tac_gencode_fdecl(head, tarr);
      break;
    case a_cond_t: // done
      ans = tac_gencode_cond(tarr);
      break;
    case a_for_t: // done
      ans = tac_gencode_loop(tarr);
      break;
    case a_read_t: // done
      ans = tac_gencode_read(tarr);
      break;
    case a_while_t: // done
      ans = tac_gencode_whiledo(tarr);
      break;
    case a_ret_t: // done
      ans = tac_gencode_ret(tarr);
      break;
    case a_print_t: // done
      ans = tac_gencode_print(tarr);
      break;
    case a_paren_t:
    case a_op_t:
    case a_kwc_t:
    case a_kwi_t:
    case a_kwf_t:
    case a_kwb_t:
    case a_vlist_t:
    case a_plist_t:
    case a_cmdl_t:
    case a_block_t:
    case a_fsig_t:
    case a_tvar_t:
    case a_csv_t:
    case a_decl_t:
    case a_vdecl_t:
      // Esses não geram código, mas vamos gerar uma TAC para não deixar uma
      // tac vazia no meio do array, o que adicionaria complexidade
      ans = tac_cat_tails_arr(tarr, NUM_CHILDREN);
      break;
  }

  return ans;
}

struct tac_node *
tac_get_head(struct tac_node *tail)
{
  struct tac_node *ans = tail,
                  *prev = ans;

  while (prev != NULL) {
    ans = prev;
    prev = ans->prev;
  }

  return ans;
}

void
tac_validate_ops(struct tac_node *tnode, size_t nops, const char *caller, int line)
{
  if (!tnode)
    LOG_NHEAD_AND_EXIT1(caller, line);
  if (nops >= 1) {
    if (tnode->ans == NULL)
      LOG_NHEAD_AND_EXIT1(caller, line);
    if (nops >= 2) {
      if (tnode->op1 == NULL)
        LOG_NHEAD_AND_EXIT1(caller, line);
      if (nops == 3) {
        if (tnode->op2 == NULL)
          LOG_NHEAD_AND_EXIT1(caller, line);
      }
    }
  }
}
