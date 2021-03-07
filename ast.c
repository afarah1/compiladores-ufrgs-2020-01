#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "logging.h"
#include "ast.h"
#include "errors.h"

static void
ast_print_disassemble_node(FILE *out, struct ast_node *head);

static char *
ast_kw_to_str(struct ast_node *head)
{
  switch (head->atype) {
    case a_kwc_t: return "char";
    case a_kwi_t: return "int";
    case a_kwf_t: return "float";
    case a_kwb_t: return "bool";
    default: return "ERROR";
  }
}

void
ast_validate_children(struct ast_node *head, size_t nchildren, const char *caller, int line)
{
  if ((NUM_CHILDREN < nchildren) || (head == NULL) || (head->children == NULL))
    LOG_NHEAD_AND_EXIT1(caller, line);

  for (size_t i = 0; i < nchildren; i++) {
    if (head->children[i] == NULL)
      LOG_NHEAD_AND_EXIT1(caller, line);
  }
}

void
ast_validate_symbol(struct ast_node *head, size_t nchildren, const char *caller, int line)
{
  if ((NUM_CHILDREN < nchildren) || (head == NULL) || (head->children == NULL))
    LOG_NHEAD_AND_EXIT1(caller, line);

  for (size_t i = 0; i < nchildren; i++) {
    if (head->children[i] == NULL) {
      LOG_NHEAD_AND_EXIT1(caller, line);
    } else if (head->children[i]->symbol == NULL) {
      LOG_NSYM_AND_EXIT1(caller, line);
    }
  }
}

struct ast_node *
ast_create(enum atype_t atype, struct hash_node *symbol, size_t nchildren, ...)
{
  if (nchildren > NUM_CHILDREN) {
    fprintf(stderr, "nchildren (%zu) > NUM_CHILDREN (%d)\n", nchildren, NUM_CHILDREN);
    exit(E_SYNTAX);
  }

  struct ast_node *ans = malloc(sizeof(*ans));
  if (!ans)
    REPORT_AND_EXIT;

  ans->atype = atype;
  ans->symbol = symbol;

  va_list va;
  va_start(va, nchildren);

  for (size_t n = 0; n < nchildren; n++) {
    ans->children[n] = va_arg(va, struct ast_node *);
  }

  va_end(va);

  for (size_t n = nchildren; n < NUM_CHILDREN; n++) {
    ans->children[n] = NULL;
  }

  return ans;
}

void
ast_print_node(struct ast_node *head, int level)
{
  // assumes head <> NULL
  char *key = "";
  if (head->symbol != NULL)
    key = head->symbol->key;

  // Provavelmente tem uma maneira melhor de fazer isso
  for (int i = 0; i < level; i++) {
    printf(" ");
  }

  switch (head->atype) {
    // symbbol
    case a_sym_t : printf("a_sym_t %s\n", key); break;
    // expr ops
    case a_paren_t : printf("a_paren_t%s\n", key); break;
    case a_op_t : printf("a_op_t%s\n", key); break;
    case a_add_t : printf("a_add_t%s\n", key); break;
    case a_sub_t : printf("a_sub_t%s\n", key); break;
    case a_mul_t : printf("a_mul_t%s\n", key); break;
    case a_div_t : printf("a_div_t%s\n", key); break;
    case a_lt_t  : printf("a_lt_t%s\n",  key); break;
    case a_gt_t  : printf("a_gt_t%s\n",  key); break;
    case a_or_t  : printf("a_or_t%s\n",  key); break;
    case a_pow_t : printf("a_pow_t%s\n", key); break;
    case a_not_t : printf("a_not_t%s\n", key); break;
    case a_and_t : printf("a_and_t%s\n", key); break;
    case a_le_t  : printf("a_le_t%s\n",  key); break;
    case a_ge_t  : printf("a_ge_t%s\n",  key); break;
    case a_eq_t  : printf("a_eq_t%s\n",  key); break;
    case a_ne_t  : printf("a_ne_t%s\n",  key); break;
    // decl
    case a_decl_t : printf("a_decl_t%s\n", key); break;
    case a_vdecl_t : printf("a_vdecl_t %s\n", key); break;
    // attrg
    case a_attr_t : printf("a_attr_t%s\n", key); break;
    case a_vattr_t : printf("a_vattr_t%s\n", key); break;
    // facllg
    case a_call_t : printf("a_call_t%s\n", key); break;
    // condg
    case a_cond_t : printf("a_cond_t%s\n", key); break;
    // whileg
    case a_while_t : printf("a_while_t%s\n", key); break;
    // forg
    case a_for_t : printf("a_for_t%s\n", key); break;
    // listsg
    case a_cmdl_t   : printf("a_cmdl_t%s\n", key); break;
    case a_csv_t   : printf("a_csv_t%s\n", key); break;
    case a_vlist_t   : printf("a_vlist_t%s\n", key); break;
    case a_plist_t : printf("a_plist_t%s\n", key); break;
    // blocksgno key needed
    case a_block_t : printf("a_block_t%s\n", key); break;
    // cmdsgno key needed
    case a_read_t  : printf("a_read_t%s\n", key); break;
    case a_print_t : printf("a_print_t%s\n", key); break;
    case a_ret_t   : printf("a_ret_t%s\n", key); break;
    // vector accessgno key needed
    case a_vsym_t  : printf("a_vsym_t%s\n", key); break;
    // fsiggno key
    case a_fsig_t  : printf("a_fsig_t%s\n", key); break;
    case a_fdecl_t  : printf("a_fdecl_t%s\n", key); break;
    // kw
    case a_kwc_t  : printf("a_kwc_t %s\n", key); break;
    case a_kwi_t  : printf("a_kwi_t %s\n", key); break;
    case a_kwf_t  : printf("a_kwf_t %s\n", key); break;
    case a_kwb_t  : printf("a_kwb_t %s\n", key); break;
    // tvar
    case a_tvar_t  : printf("a_tvar_t %s\n", key); break;
    default: printf("UNKNOWN %s\n", key);
  }
}

void
ast_print(struct ast_node *head, int level)
{
  if (head == NULL)
    return;

  ast_print_node(head, level);


  // Provavelmente tem um jeito melhor de fazer este laço
  for (size_t nchild = 0; nchild < NUM_CHILDREN; nchild++) {
    struct ast_node *child = head->children[nchild];
    if (child == NULL)
      break;
    ast_print(child, level + 1);
  }
}

static void
ast_print_disassemble_expr(FILE *out, struct ast_node *head)
{
  switch (head->atype) {
    case a_paren_t:
    case a_op_t:
      ast_validate_children(head, 1, __func__, __LINE__);
      break;
    default:
      ast_validate_children(head, 2, __func__, __LINE__);
  }

  if (head->atype == a_paren_t) {
    fprintf(out, "(");

    ast_print_disassemble_node(out, head->children[0]);

    fprintf(out, ")");
  } else {
    ast_print_disassemble_node(out, head->children[0]);
    switch (head->atype) {
      case a_add_t : fprintf(out, " + "); break;
      case a_sub_t : fprintf(out, " - "); break;
      case a_mul_t : fprintf(out, " * "); break;
      case a_div_t : fprintf(out, " / "); break;
      case a_lt_t  : fprintf(out, " < "); break;
      case a_gt_t  : fprintf(out, " > "); break;
      case a_or_t  : fprintf(out, " | "); break;
      case a_pow_t : fprintf(out, " ^ "); break;
      case a_not_t : fprintf(out, " ~ "); break;
      case a_and_t : fprintf(out, " & "); break;
      case a_le_t  : fprintf(out, " <= "); break;
      case a_ge_t  : fprintf(out, " >= "); break;
      case a_eq_t  : fprintf(out, " == "); break;
      case a_ne_t  : fprintf(out, " != "); break;
      case a_op_t  : return; // ugh...
      default      : fprintf(stderr, "unknown expression type %d\n", head->atype);
    }
    ast_print_disassemble_node(out, head->children[1]);
  }
}

static void
ast_print_disassemble_vlist(FILE *out, struct ast_node *head)
{
  if (head == NULL)
    return;

  ast_validate_children(head, 2, __func__, __LINE__);

  ast_validate_symbol(head, 1, __func__, __LINE__);

  fprintf(out, "%s ", head->children[0]->symbol->key);

  ast_print_disassemble_node(out, head->children[1]);
}

static void
ast_print_disassemble_decl(FILE *out, struct ast_node *head)
{
  if (head->atype == a_vdecl_t) {
    ast_validate_children(head, 2, __func__, __LINE__);
    ast_validate_symbol(head, 1, __func__, __LINE__);

    fprintf(out, "%s = %s [ %s ]",
        head->children[0]->symbol->key,
        ast_kw_to_str(head->children[1]),
        head->symbol->key);

    if (head->children[2] != NULL) {
      fprintf(out, " : ");
      ast_print_disassemble_node(out, head->children[2]);
    }
    fprintf(out, ";\n");
  } else {
    ast_validate_children(head, 3, __func__, __LINE__);
    if ((head->children[0]->symbol == NULL) || (head->children[2]->symbol == NULL))
      LOG_NSYM_AND_EXIT1(__func__, __LINE__);

    fprintf(out, "%s = %s : %s;\n",
        head->children[0]->symbol->key,
        ast_kw_to_str(head->children[1]),
        head->children[2]->symbol->key);
  }
}

static void
ast_print_disassemble_attr(FILE *out, struct ast_node *head)
{
  ast_validate_symbol(head, 1, __func__, __LINE__);

  if (head->atype == a_vattr_t) {
    ast_validate_children(head, 3, __func__, __LINE__);
    fprintf(out, "%s [", head->children[0]->symbol->key);
    ast_print_disassemble_expr(out, head->children[1]);
    fprintf(out, "] = ");
    ast_print_disassemble_expr(out, head->children[2]);
  } else {
    ast_validate_children(head, 2, __func__, __LINE__);
    fprintf(out, "%s = ", head->children[0]->symbol->key);
    ast_print_disassemble_expr(out, head->children[1]);
  }

}

static void
ast_print_disassemble_arg(FILE *out, struct ast_node *head)
{
  if (head == NULL)
    return;

  ast_validate_children(head, 2, __func__, __LINE__);
  ast_validate_symbol(head, 1, __func__, __LINE__);

  fprintf(out, "%s = %s", head->children[0]->symbol->key, ast_kw_to_str(head->children[1]));
}

static void
ast_print_disassemble_csv(FILE *out, struct ast_node *head)
{
  if (head == NULL)
    return;

  ast_validate_children(head, 0, __func__, __LINE__);

  if (head->children[0] != NULL) {
    ast_print_disassemble_node(out, head->children[0]);

    if (head->children[1] != NULL) {
      fprintf(out, ", ");
      ast_print_disassemble_node(out, head->children[1]);
    }
  }
}

static void
ast_print_disassemble_fsig(FILE *out, struct ast_node *head)
{
  ast_validate_symbol(head, 1, __func__, __LINE__);

  if ((head->children[0]->symbol == NULL) || ((head->children[2] == NULL) || (head->children[2] == NULL)))
    LOG_NSYM_AND_EXIT;

  fprintf(out, "%s(", head->children[0]->symbol->key);

  ast_print_disassemble_csv(out, head->children[1]);

  fprintf(out, ") = %s ", ast_kw_to_str(head->children[2]));
}

static void
ast_print_disassemble_cmdl(FILE *out, struct ast_node *head)
{
  if (head == NULL)
    return;

  ast_validate_children(head, 0, __func__, __LINE__);

  if (head->children[0] != NULL) {
    ast_print_disassemble_node(out, head->children[0]);

    fprintf(out, "\n");

    if (head->children[1] != NULL) {
      ast_print_disassemble_node(out, head->children[1]);
    }
  }
}

static void
ast_print_disassemble_block(FILE *out, struct ast_node *head)
{
  fprintf(out, "{\n");

  ast_validate_children(head, 0, __func__, __LINE__);

  if (head->children[0] != NULL)
    ast_print_disassemble_node(out, head->children[0]);

  fprintf(out, "\n} ");
}

static void
ast_print_disassemble_fdecl(FILE *out, struct ast_node *head)
{
  ast_validate_children(head, 2, __func__, __LINE__);

  ast_print_disassemble_fsig(out, head->children[0]);

  ast_print_disassemble_block(out, head->children[1]);

  fprintf(out, ";\n");
}

static void
ast_print_disassemble_plist(FILE *out, struct ast_node *head)
{
  if (head == NULL)
    return;

  ast_validate_children(head, 0, __func__, __LINE__);

  if (head->children[0] != NULL) {
    ast_print_disassemble_node(out, head->children[0]);

    // fdecl e decl são quem imprimem porque é o que está no parser.y
    //fprintf(out, ";\n");

    if (head->children[1] != NULL)
      ast_print_disassemble_plist(out, head->children[1]);
  }
}

static void
ast_print_disassemble_call(FILE *out, struct ast_node *head)
{
  ast_validate_symbol(head, 1, __func__, __LINE__);

  fprintf(out, "%s(", head->children[0]->symbol->key);

  ast_print_disassemble_csv(out, head->children[1]);

  fprintf(out, ") ");
}

static void
ast_print_disassemble_for(FILE *out, struct ast_node *head)
{
  ast_validate_children(head, 4, __func__, __LINE__);

  ast_validate_symbol(head, 1, __func__, __LINE__);

  fprintf(out, "loop (%s: ", head->children[0]->symbol->key);

  ast_print_disassemble_expr(out, head->children[1]);

  fprintf(out, ", ");

  ast_print_disassemble_expr(out, head->children[2]);

  fprintf(out, ", ");

  ast_print_disassemble_expr(out, head->children[3]);

  fprintf(out, ") ");

  if (head->children[4] != NULL) {
    ast_print_disassemble_node(out, head->children[4]);
  }
}

static void
ast_print_disassemble_while(FILE *out, struct ast_node *head)
{
  ast_validate_children(head, 1, __func__, __LINE__);

  fprintf(out, "while (");

  ast_print_disassemble_expr(out, head->children[0]);

  fprintf(out, ") ");

  if (head->children[1] != NULL) {
    ast_print_disassemble_node(out, head->children[1]);
  }
}

static void
ast_print_disassemble_cond(FILE *out, struct ast_node *head)
{
  ast_validate_children(head, 1, __func__, __LINE__);

  fprintf(out, "if (");

  ast_print_disassemble_expr(out, head->children[0]);

  fprintf(out, ") then\n");

  if (head->children[1] != NULL) {
    ast_print_disassemble_node(out, head->children[1]);

    if (head->children[2] != NULL) {
      fprintf(out, "\nelse\n");
      ast_print_disassemble_node(out, head->children[2]);
    }
  }
}

static void
ast_print_disassemble_read(FILE *out, struct ast_node *head)
{
  ast_validate_symbol(head, 1, __func__, __LINE__);

  fprintf(out, "read %s", head->children[0]->symbol->key);
}

static void
ast_print_disassemble_vsym(FILE *out, struct ast_node *head)
{
  ast_validate_children(head, 2, __func__, __LINE__);
  ast_validate_symbol(head, 1, __func__, __LINE__);

  fprintf(out, "%s[", head->children[0]->symbol->key);

  ast_print_disassemble_expr(out, head->children[1]);

  fprintf(out, "]");
}

static void
ast_print_disassemble_return(FILE *out, struct ast_node *head)
{
  ast_validate_children(head, 1, __func__, __LINE__);

  fprintf(out, "return ");

  ast_print_disassemble_expr(out, head->children[0]);
}

static void
ast_print_disassemble_print(FILE *out, struct ast_node *head)
{
  ast_validate_children(head, 1, __func__, __LINE__);

  fprintf(out, "print ");

  ast_print_disassemble_csv(out, head->children[0]);
}

static void
ast_print_disassemble_node(FILE *out, struct ast_node *head)
{
  char *key = "";
  if (head->symbol != NULL)
    key = head->symbol->key;

  switch (head->atype) {
    // symbbol
    case a_kwc_t  :
      fprintf(out, "char"); break;
    case a_kwi_t  :
      fprintf(out, "int"); break;
    case a_kwf_t  :
      fprintf(out, "float"); break;
    case a_kwb_t  :
      fprintf(out, "bool"); break;
    case a_sym_t :
      fprintf(out, "%s", key); break;
    // expr ops
    case a_add_t :
    case a_sub_t :
    case a_mul_t :
    case a_div_t :
    case a_lt_t  :
    case a_gt_t  :
    case a_or_t  :
    case a_pow_t :
    case a_not_t :
    case a_and_t :
    case a_le_t  :
    case a_ge_t  :
    case a_eq_t  :
    case a_ne_t  :
    case a_paren_t:
    case a_op_t:
      ast_print_disassemble_expr(out, head);
      break;
    // decl
    case a_decl_t :
    case a_vdecl_t :
      ast_print_disassemble_decl(out, head);
      break;
    // attrg
    case a_attr_t :
    case a_vattr_t :
      ast_print_disassemble_attr(out, head);
      break;
    // facllg
    case a_call_t :
      ast_print_disassemble_call(out, head);
      break;
    // condg
    case a_cond_t :
      ast_print_disassemble_cond(out, head);
      break;
    // whileg
    case a_while_t :
      ast_print_disassemble_while(out, head);
      break;
    // forg
    case a_for_t :
      ast_print_disassemble_for(out, head);
      break;
    // listsg
    case a_plist_t  :
      ast_print_disassemble_plist(out, head);
      break;
    case a_cmdl_t   :
      ast_print_disassemble_cmdl(out, head);
      break;
    case a_csv_t   :
      ast_print_disassemble_csv(out, head);
      break;
    // blocksgno key needed
    case a_block_t :
      ast_print_disassemble_block(out, head);
      break;
    // cmdsgno key needed
    case a_read_t  :
      ast_print_disassemble_read(out, head);
      break;
    case a_print_t :
      ast_print_disassemble_print(out, head);
      break;
    case a_ret_t   :
      ast_print_disassemble_return(out, head);
      break;
    // vector accessgno key needed
    case a_vsym_t  :
      ast_print_disassemble_vsym(out, head);
      break;
    // fsiggno key
    case a_fsig_t  :
      ast_print_disassemble_fsig(out, head);
      break;
    case a_fdecl_t  :
      ast_print_disassemble_fdecl(out, head);
      break;
    // tvar
    case a_tvar_t  :
      ast_print_disassemble_arg(out, head);
      break;
    case a_vlist_t   :
      ast_print_disassemble_vlist(out, head);
      break;
    default:
      fprintf(stderr, "UNKNOWN %s\n", key);
      exit(E_SYNTAX);
  }

}

void
ast_print_disassemble(FILE *out, struct ast_node *head)
{
  if (head == NULL)
    return;

  ast_print_disassemble_node(out, head);
}
