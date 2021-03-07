#include <stdbool.h>
#include <stdarg.h>
#include "semantic.h"
#include "ast.h"
#include "errors.h"

static void
semantic_report(enum se_t se, struct hash_node *node, ...)
{
  va_list va;
  int expected, got;
  const char *side;
  const char *expr;
  enum hashtype_t expectedt, gott;
  int parampos;
  fprintf(stderr, "Semantic error: ");
  switch (se) {
    case se_redcl_t:
      fprintf(stderr, "Identifier %s redeclared\n", node->key);
      break;
    case se_iop_t:
      va_start(va, node);
      expr = va_arg(va, const char *);
      side = va_arg(va, const char *);
      va_end(va);
      fprintf(stderr, "Invalid %s operand for %s\n", side, expr);
      break;
    case se_badargs_t:
      va_start(va, node);
      expected = va_arg(va, int);
      got = va_arg(va, int);
      va_end(va);
      fprintf(stderr, "Expected %d arguments for function %s, got %d\n", expected, node->key, got);
      break;
    case se_badargt_t:
      va_start(va, node);
      expectedt = va_arg(va, enum hashtype_t);
      gott = va_arg(va, enum hashtype_t);
      parampos = va_arg(va, int);
      va_end(va);
      fprintf(stderr, "Expected type %d, got %d, for %dth param of %s\n", expectedt, gott, parampos, node->key);
      break;
    default:
      if (node != NULL)
        fprintf(stderr, "Unmapped semantic error %d at %s\n", se, node->key);
      else
        fprintf(stderr, "Unmapped semantic error %d\n", se);
      break;
  }
}

static enum hashtype_t
semantic_get_type(enum atype_t atype)
{
  switch (atype) {
    case a_kwc_t: return ht_char_t;
    case a_kwi_t: return ht_int_t;
    case a_kwf_t: return ht_float_t;
    case a_kwb_t: return ht_bool_t;
    default: return ht_unknown_t;
  }
}

static int
semantic_check_decl(struct ast_node *head)
{
  int ans = 0;
  // Validations
  ast_validate_children(head, 3, __func__, __LINE__);
  if ((head->children[0]->symbol == NULL) || (head->children[2]->symbol == NULL))
    LOG_NSYM_AND_EXIT1(__func__, __LINE__);
  // Actually do semantic analysis
  if (head->children[0]->symbol->typeinfo.nature != hn_id_t) {
    semantic_report(se_redcl_t, head->children[0]->symbol);
    ans = 1;
  }
  struct hash_typeinfo typeinfo = {
    .nature = hn_var_t,
    .type = semantic_get_type(head->children[1]->atype)
  };
  hash_set_typeinfo(head->children[0]->symbol, typeinfo, __func__, __LINE__);
  hash_set_astinfo(head->children[0]->symbol, head->children[2], __func__, __LINE__);
  return ans;
}

static int
semantic_check_vdecl(struct ast_node *head)
{
  int ans = 0;
  // Validations
  ast_validate_children(head, 2, __func__, __LINE__);
  ast_validate_symbol(head, 1, __func__, __LINE__);
  // Actually do semantic analysis
  if (head->children[0]->symbol->typeinfo.nature != hn_id_t) {
    semantic_report(se_redcl_t, head->children[0]->symbol);
    ans = 1;
  }
  struct hash_typeinfo typeinfo = {
    .nature = hn_vec_t,
    .type = semantic_get_type(head->children[1]->atype)
  };
  hash_set_typeinfo(head->children[0]->symbol, typeinfo , __func__, __LINE__);
  hash_set_astinfo(head->children[0]->symbol, head, __func__, __LINE__);
  // TODO validate vector stuff here
  //char *endptr = NULL;
  //long int vsize = strtol(head->symbol->key, &endptr, 10);
  //if ((((vsize == LONG_MIN) || (vsize == LONG_MAX)) && (errno == ERANGE)) ||
  //    (head->symbol->key == endptr)) {
  //  fprintf(stderr, "Vector size %s is not a valid base 10 integer.\n", head->symbol->key);
  //} else if (vsize > MAX_VEC_SIZE) {
  //  fprintf(stderr, "Vector size %s larger than max %d.\n", head->symbol->key, MAX_VEC_SIZE);
  //} else if (vsize <= 0) {
  //  fprintf(stderr, "Vector size %s <= 0.\n", head->symbol->key);
  //}
  return ans;
}

static int
semantic_check_fdecl(struct ast_node *head)
{
  int ans = 0;
  // Validations
  ast_validate_children(head, 1, __func__, __LINE__);
  if (!(head->children[0]->atype == a_fsig_t)) {
    fprintf(stderr, "Function declaration has no signature.\n");
    exit(E_SYNTAX); // Should not happen
  }
  // Actually do semantic analysis
  struct ast_node *fsig = head->children[0];
  ast_validate_symbol(fsig, 1, __func__, __LINE__);
  if (fsig->children[0]->symbol->typeinfo.nature != hn_id_t) {
    semantic_report(se_redcl_t, fsig->children[0]->symbol);
    ans = 1;
  }
  struct hash_typeinfo typeinfo = {
    .nature = hn_func_t,
    .type = semantic_get_type(fsig->children[2]->atype)
  };
  // Set the nature of this symbol as a function and a ptr to its args
  hash_set_typeinfo(fsig->children[0]->symbol, typeinfo, __func__, __LINE__);
  hash_set_astinfo(fsig->children[0]->symbol, fsig->children[1], __func__, __LINE__);
  // Additionally, set the type of the args, if there are any
  struct ast_node *csv = fsig->children[1];
  while (csv != NULL) {
    ast_validate_children(csv, 1, __func__, __LINE__);
    struct ast_node *arg = csv->children[0];
    ast_validate_children(arg, 2, __func__, __LINE__);
    ast_validate_symbol(arg, 1, __func__, __LINE__);
    typeinfo.nature = hn_arg_t;
    typeinfo.type = semantic_get_type(arg->children[1]->atype);
    hash_set_typeinfo(arg->children[0]->symbol, typeinfo, __func__, __LINE__);
    csv = csv->children[1];
  }
  return ans;
}

static int
semantic_check_and_set_decls(struct ast_node *head)
{
  int ans = 0;

  if (!head)
    return ans;

  switch (head->atype) {
    case a_decl_t:
      ans += semantic_check_decl(head);
      break;
    case a_vdecl_t:
      ans += semantic_check_vdecl(head);
      break;
    case a_fdecl_t:
      ans += semantic_check_fdecl(head);
      break;
    default:
      break; // Ignore, we're only checking declarations here
  }

  // TODO Is this the best way to do this?
  for (size_t nchild = 0; nchild < NUM_CHILDREN; nchild++) {
    if (head->children[nchild])
      ans += semantic_check_and_set_decls(head->children[nchild]);
    else
      break;
  }

  return ans;
}

static enum hashtype_t
semantic_get_expr_type(struct ast_node *head)
{
  switch (head->atype) {
    case a_op_t:
    case a_paren_t:
      ast_validate_children(head, 1, __func__, __LINE__);
      return semantic_get_expr_type(head->children[0]);
    case a_add_t:
    case a_sub_t:
    case a_mul_t:
    case a_div_t:
    case a_pow_t:
      return ht_int_t; // Interchangable with float and char
    case a_lt_t:
    case a_gt_t:
    case a_le_t:
    case a_ge_t:
    case a_eq_t:
    case a_ne_t:
      return ht_bool_t;
    case a_sym_t:
      if (head->symbol == NULL)
        LOG_NSYM_AND_EXIT1(__func__, __LINE__);
       return head->symbol->typeinfo.type;
    case a_call_t:
    case a_vsym_t:
       ast_validate_children(head, 1, __func__, __LINE__);
       return semantic_get_expr_type(head->children[0]);
    default:
       return ht_unknown_t;
  }
}

static bool
semantic_check_int_or_float(struct ast_node *head)
{
  switch (head->atype) {
    case a_op_t:
    case a_paren_t:
      ast_validate_children(head, 1, __func__, __LINE__);
      return semantic_check_int_or_float(head->children[0]);
    case a_add_t:
    case a_sub_t:
    case a_mul_t:
    case a_div_t:
    case a_pow_t:
      return true;
    case a_lt_t:
    case a_gt_t:
    case a_le_t:
    case a_ge_t:
    case a_eq_t:
    case a_ne_t:
      return false;
    case a_sym_t:
      if (head->symbol == NULL)
        LOG_NSYM_AND_EXIT1(__func__, __LINE__);
       return (
           (head->symbol->typeinfo.nature == hn_char_t) ||
           (head->symbol->typeinfo.nature == hn_int_t) ||
           (head->symbol->typeinfo.nature == hn_float_t) ||
           (head->symbol->typeinfo.type == ht_char_t) ||
           (head->symbol->typeinfo.type == ht_int_t) ||
           (head->symbol->typeinfo.type == ht_float_t)
       );
    case a_call_t:
    case a_vsym_t:
       ast_validate_children(head, 1, __func__, __LINE__);
       return semantic_check_int_or_float(head->children[0]);
    default:
       return false;
  }
}

static int
semantic_check_arit(struct ast_node *head, const char *optype)
{
  ast_validate_children(head, 2, __func__, __LINE__);

  int ans = 0;

  if (!semantic_check_int_or_float(head->children[0])) {
    ans++;
    semantic_report(se_iop_t, head->symbol, optype, "left");
  }

  if (!semantic_check_int_or_float(head->children[1])) {
    ans++;
    semantic_report(se_iop_t, head->symbol, optype, "right");
  }

  return ans;
}

static int
semantic_check_ops(struct ast_node *head)
{
  int ans = 0;

  if (!head)
    return ans;

  char *optype = NULL;

  // "fall thtough" is a gcc marker to not issue -Wimplicit-fallthrough, it
  // must be in its own line and come right before a case statement
  switch (head->atype) {
    case a_add_t:
      if (!optype)
        optype = "add";
      // fall through
    case a_sub_t:
      if (!optype)
        optype = "sub";
      // fall through
    case a_mul_t:
      if (!optype)
        optype = "mul";
      // fall through
    case a_div_t:
      if (!optype)
        optype = "div";
      // fall through
    case a_pow_t:
      if (!optype)
        optype = "pow";
      // fall through
    case a_lt_t:
      if (!optype)
        optype = "lt";
      // fall through
    case a_gt_t:
      if (!optype)
        optype = "gt";
      // fall through
    case a_le_t:
      if (!optype)
        optype = "le";
      // fall through
    case a_ge_t:
      if (!optype)
        optype = "ge";
      // fall through
    case a_eq_t:
      if (!optype)
        optype = "eq";
      // fall through
    case a_ne_t:
      if (!optype)
        optype = "ne";
      ans += semantic_check_arit(head, optype);
      break;
    default:
      break;
  }

  // TODO Is this the best way to do this?
  for (size_t nchild = 0; nchild < NUM_CHILDREN; nchild++) {
    if (head->children[nchild])
      ans += semantic_check_ops(head->children[nchild]);
    else
      break;
  }

  return ans;
}

static int
semantic_get_paramct(struct ast_node *head)
{
  int ans = 0;

  if (!head)
    return ans;

  ans++;

  ans += semantic_get_paramct(head->children[1]);

  return ans;
}

static bool
semantic_check_paramct(struct ast_node *decl, struct ast_node *call, int *expected, int *got)
{
  *expected = semantic_get_paramct(decl);
  *got = semantic_get_paramct(call);
  return (*expected == *got);
}

static bool
semantic_is_interchangeable(enum hashtype_t a, enum hashtype_t b)
{
  if ((a == ht_float_t) || (a == ht_char_t) || (a == ht_int_t))
    return ((b == ht_float_t) || (b == ht_char_t) || (b == ht_int_t));
  else
    return a == b;
}

static bool
semantic_check_paramtype(struct hash_node *fdecl, struct ast_node *callcsv)
{
  // Assumes both with same param count, possibly zero (both NULL)
  int ans = 0;
  int parampos = 0;

  struct ast_node *declcsv = fdecl->astinfo;

  while ((declcsv != NULL) && (callcsv != NULL)) {
    ast_validate_children(declcsv, 1, __func__, __LINE__);
    struct ast_node *declarg = declcsv->children[0];
    //ast_validate_children(declarg, 2, __func__, __LINE__);
    ast_validate_symbol(declarg, 1, __func__, __LINE__);
    ast_validate_children(callcsv, 1, __func__, __LINE__);

    enum hashtype_t expected = declarg->children[0]->symbol->typeinfo.type,
                    got = semantic_get_expr_type(callcsv->children[0]);

    if (!semantic_is_interchangeable(expected, got)) {
      semantic_report(se_badargt_t, fdecl, expected, got, parampos);
      ans++;
    }

    declcsv = declcsv->children[1];
    callcsv = callcsv->children[1];
    parampos++;
  }

  return ans;
}

static int
semantic_check_fcalls(struct ast_node *head)
{
  int ans = 0;
  int expected, got;

  if (!head)
    return ans;

  if (head->atype == a_call_t) {
    // does not report undeclared identifiers
    ast_validate_symbol(head, 1, __func__, __LINE__);
    struct hash_node *fdecl = hash_find(head->children[0]->symbol->key);
    if (fdecl) {
      // Check param count
      if (semantic_check_paramct(fdecl->astinfo, head->children[1], &expected, &got)) {
        // We have the same number of params, check their types
        ans += semantic_check_paramtype(fdecl, head->children[1]);
      } else {
        semantic_report(se_badargs_t, head->children[0]->symbol, expected, got);
        ans++;
      }
    }
  }

  for (size_t nchild = 0; nchild < NUM_CHILDREN; nchild++) {
    if (head->children[nchild])
      ans += semantic_check_fcalls(head->children[nchild]);
    else
      break;
  }

  return ans;
}

int
semantic_analyze(struct ast_node *head)
{
  int ans = 0;

  if (!head)
    return ans;

  // These must be called in this order
  ans += semantic_check_and_set_decls(head);
  ans += hash_fprint_ids(stderr, "Semantic error: Identifier undeclared:");
  ans += semantic_check_ops(head);
  ans += semantic_check_fcalls(head); // TODO can be optimized to not need its own pass through the ast

  return ans;
}
