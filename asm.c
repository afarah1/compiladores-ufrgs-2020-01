#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include "logging.h"
#include "asm.h"
#include "tac.h"
#include "hash.h"
#include "dry.h"

#define VP "ufrgs_var_" // VAR PREFIX
#define VL ".ufrgs_label_" // label PREFIX

static long int
asm_strtol(char * const str)
{
  char *endptr = NULL;
  long int ans = strtol(str, &endptr, 16);
  if (((ans == LONG_MIN) || (ans == LONG_MAX)) && (errno == ERANGE)) {
    LOG_WARNING("Overflow on long conversion: %ld\n", ans);
    ans = 0;
  } else if ((endptr != NULL) && (*endptr != '\0')) {
    LOG_WARNING("Unconverted chars on long conversion: %ld\n", ans);
    ans = 0;
  }
  return ans;
}

static void
asm_print_cmp(FILE *out, enum ttype_t ttype)
{
  fprintf(out, "cmpl %%edx, %%eax\n");
  switch (ttype) {
    case t_lt_t:
      fprintf(out, "setl %%al\n");
      break;
    case t_gt_t:
      fprintf(out, "setg %%al\n");
      break;
    case t_ge_t:
      fprintf(out, "setge %%al\n");
      break;
    case t_le_t:
      fprintf(out, "setle %%al\n");
      break;
    case t_eq_t:
      fprintf(out, "sete %%al\n");
      break;
    case t_ne_t:
      fprintf(out, "setne %%al\n");
      break;
    default:
      LOG_AND_EXIT("Not a comparison: %d", ttype);
  }
  fprintf(out, "movzbl %%al, %%eax\n");
}

static void
asm_print_expr(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 3, __func__, __LINE__);
  char *ans = thead->ans->key,
       *op1 = thead->op1->key,
       *op2 = thead->op2->key;
  enum ttype_t ttype = thead->ttype;
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#EXPR_START\n");
  static int or_labels = 0;
  fprintf(out, "movl "VP"%s(%%rip), %%eax\n", op1);
  fprintf(out, "movl "VP"%s(%%rip), %%edx\n", op2);
  switch (ttype) {
    case t_lt_t:
    case t_le_t:
    case t_gt_t:
    case t_ge_t:
    case t_eq_t:
    case t_ne_t:
      asm_print_cmp(out, ttype);
      break;
    case t_add_t:
      fprintf(out, "addl %%edx, %%eax\n");
      break;
    case t_sub_t:
      fprintf(out, "subl %%edx, %%eax\n");
      break;
    case t_mul_t:
      fprintf(out, "imull %%edx, %%eax\n");
      break;
    case t_div_t:
      // stackoverflow.com/questions/39658992
      fprintf(out, "movl %%eax, %%ebx\n");
      fprintf(out, "movl %%edx, %%ecx\n");
      fprintf(out, "cdq\n");
      fprintf(out, "idivl %%ecx\n");
      break;
    case t_or_t:
      if (LOG_LEVEL == LOG_LEVEL_DEBUG)
        fprintf(out, "#%s := %s or %s\n", ans, op1, op2);
      fprintf(out, "testl %%eax, %%eax\n");
      fprintf(out, "jne .true%d\n", or_labels++);
      fprintf(out, "testl %%edx, %%edx\n");
      fprintf(out, "je .false%d\n", or_labels++);
      fprintf(out, ".true%d:\n", or_labels-2);
      fprintf(out, "movl $1, %%eax\n");
      fprintf(out, "jmp .finish%d\n", or_labels++);
      fprintf(out, ".false%d:\n", or_labels-2);
      fprintf(out, "mov $0, %%eax\n");
      fprintf(out, ".finish%d:\n", or_labels-1);
      break;
    case t_and_t:
      if (LOG_LEVEL == LOG_LEVEL_DEBUG)
        fprintf(out, "#%s := %s and %s\n", ans, op1, op2);
      fprintf(out, "cmpl %%eax, %%edx\n");
      fprintf(out, "sete %%al\n");
      fprintf(out, "movzbl %%al, %%eax\n");
      break;
    default:
      LOG_AND_EXIT("Not an expression: %d\n", ttype);
  }
  fprintf(out, "movl %%eax, "VP"%s(%%rip)\n", ans);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#EXPR_END\n");
}

static char *
asm_get_str_wo_quotes(char * const str)
{
  size_t len = strlen(str); // for debbuging
  char *ans = calloc(len - 1, 1);
  strncpy(ans, str + 1, len - 2);
  return ans;
}

static char *
asm_get_str_sanitized(char * const str)
{
  char *ans = strdup(str);
  size_t j = 0;
  for (size_t i = 0; i < strlen(str); i++) {
    if (isalnum(str[i]))
      ans[j++] = str[i];
  }
  ans[j] = '\0';
  return realloc(ans, strlen(ans) + 1);
}

static char *
asm_get_argname(struct ast_node *anode, int argc)
{
  if (anode == NULL)
    LOG_AND_EXIT("Argc mismatch\n");
  // The arglist is backwards, go to the last arg then count back argc
  struct ast_node *argv[MAX_ARGC];
  int i = 0;
  do {
    argv[i] = anode;
    anode = anode->children[1];
    i++;
  } while ((anode != NULL) && (i < MAX_ARGC));
  if (i >= MAX_ARGC)
    LOG_AND_EXIT("Max argc of %d exceeded\n", MAX_ARGC);
  if (argc >= i)
    LOG_AND_EXIT("Argc mismatch %d:%d\n", argc, i);
  anode = argv[i - (argc + 1)];
  ast_validate_children(anode, 1, __func__, __LINE__);
  ast_validate_symbol(anode->children[0], 1, __func__, __LINE__);
  return anode->children[0]->children[0]->symbol->key;
}

static void
asm_print_label(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 1, __func__, __LINE__);
  fprintf(out, VL"%s:\n", thead->ans->key);
}

static void
asm_print_vread(FILE *out, struct tac_node *thead)
{
  // 0 - dummy
  // 1 - id
  // 2 - index
  tac_validate_ops(thead, 3, __func__, __LINE__);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#%s := %s[%s]\n", thead->ans->key, thead->op1->key, thead->op2->key);
  long int index = asm_strtol(thead->op2->key);
  fprintf(out, "movl %ld+"VP"%s(%%rip), %%eax\n", index * 4, thead->op1->key); // TODO sizes based on type
  fprintf(out, "movl %%eax, "VP"%s(%%rip)\n", thead->ans->key);
}

static void
asm_print_vcopy(FILE *out, struct tac_node *thead)
{
  // 0 - id
  // 1 - index
  // 2 - value
  tac_validate_ops(thead, 3, __func__, __LINE__);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#%s[%s] := %s\n", thead->ans->key, thead->op1->key, thead->op2->key);
  long int index = asm_strtol(thead->op1->key);
  fprintf(out, "movl "VP"%s(%%rip), %%eax\n", thead->op2->key);
  fprintf(out, "movl %%eax, %ld+"VP"%s(%%rip)\n", index * 4, thead->ans->key); // TODO sizes based on type
}

static void
asm_print_jmpf(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 2, __func__, __LINE__);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#jmpf %s, %s\n", thead->ans->key, thead->op1->key);
  fprintf(out, "movl "VP"%s(%%rip), %%eax\n", thead->op1->key);
  fprintf(out, "testl %%eax, %%eax\n");
  fprintf(out, "je "VL"%s\n", thead->ans->key);
}

static void
asm_print_jmp(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 1, __func__, __LINE__);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#jmp %s\n", thead->ans->key);
  fprintf(out, "jmp "VL"%s\n", thead->ans->key);
}

static void
asm_print_fstart(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 1, __func__, __LINE__);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#Function start\n");
  fprintf(out, "%s:\n", thead->ans->key);
  fprintf(out, "pushq %%rbp\n");
  fprintf(out, "movq %%rsp, %%rbp\n");
}

static void
asm_print_fend(FILE *out)
{
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#Function end\n");
  fprintf(out, "popq %%rbp\n");
  fprintf(out, "ret\n");
}

static void
asm_print_copy(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 2, __func__, __LINE__);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#%s := %s\n", thead->ans->key, thead->op1->key);
  fprintf(out, "movl "VP"%s(%%rip), %%eax\n", thead->op1->key);
  fprintf(out, "movl %%eax, "VP"%s(%%rip)\n", thead->ans->key);
}

static void
asm_print_print(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 1, __func__, __LINE__);
  char *copy = NULL,
       *sanitized = NULL;

  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#Print %s\n", thead->ans->key);

  if (hash_is_str(thead->ans)) {
    copy = asm_get_str_wo_quotes(thead->ans->key);
    sanitized = asm_get_str_sanitized(copy);
  } else {
    copy = strdup(thead->ans->key);
    sanitized = strdup(copy);
  }

  fprintf(out, "movl "VP"%s(%%rip), %%eax\n", sanitized);
  fprintf(out, "movl %%eax, %%esi\n");

  if (hash_is_str(thead->ans)) {
    fprintf(out, "leaq "VP"%s(%%rip), %%rdi\n", sanitized);
  } else { // TODO treat all as integer?
    fprintf(out, "leaq ufrgs_printf_int(%%rip), %%rdi\n");
  }

  fprintf(out, "movl $0, %%eax\n");
  fprintf(out, "call printf@PLT\n");
  fprintf(out, "movl $0, %%eax\n");

  free(copy);
  free(sanitized);
}

static void
asm_print_read(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 1, __func__, __LINE__);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#Read %s\n", thead->ans->key);
  fprintf(out, "leaq "VP"%s(%%rip), %%rsi\n", thead->ans->key);
  fprintf(out, "leaq ufrgs_scanf_int(%%rip), %%rdi\n");
  fprintf(out, "movl $0, %%eax\n");
  fprintf(out, "call __isoc99_scanf@PLT\n");
}

static void
asm_print_arg(FILE *out, struct tac_node *thead, int argc)
{
  tac_validate_ops(thead, 2, __func__, __LINE__);
  char *copy = asm_get_argname(thead->op1->astinfo, argc);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#Arg %d (%s) = %s\n", argc, copy, thead->ans->key);
  fprintf(out, "movl "VP"%s(%%rip), %%eax\n", thead->ans->key);
  fprintf(out, "movl %%eax, "VP"%s(%%rip)\n", copy);
}

static void
asm_print_call(FILE *out, struct tac_node *thead)
{
  tac_validate_ops(thead, 2, __func__, __LINE__);
  fprintf(out, "call %s\n", thead->op1->key);
  fprintf(out, "movl %%eax, "VP"%s(%%rip)\n", thead->ans->key);
}

static void
asm_print_tac_node(FILE *out, struct tac_node *thead)
{
  // Assumes thread <> NULL
  static int _argc = 0;
  switch (thead->ttype) {
    case t_sym_t:
      // Already printed on hash print
      break;
    case t_label_t:
      asm_print_label(out, thead);
      break;
    case t_vread_t:
      asm_print_vread(out, thead);
      break;
    case t_vcopy_t:
      asm_print_vcopy(out, thead);
      break;
    case t_add_t:
    case t_sub_t:
    case t_mul_t:
    case t_div_t:
    case t_lt_t:
    case t_gt_t:
    case t_le_t:
    case t_ge_t:
    case t_eq_t:
    case t_ne_t:
    case t_or_t:
    case t_and_t:
      asm_print_expr(out, thead);
      break;
    case t_jmpf_t:
      asm_print_jmpf(out, thead);
      break;
    case t_jmp_t:
      asm_print_jmp(out, thead);
      break;
    case t_fstart_t:
      asm_print_fstart(out, thead);
      break;
    case t_ret_t:
      tac_validate_ops(thead, 1, __func__, __LINE__);
      fprintf(out, "movl "VP"%s(%%rip), %%eax\n", thead->ans->key);
      // fall through
    case t_fend_t:
      asm_print_fend(out);
      break;
    case t_copy_t:
      asm_print_copy(out, thead);
      break;
    case t_print_t:
      asm_print_print(out, thead);
      break;
    case t_read_t:
      asm_print_read(out, thead);
      break;
    case t_arg_t:
      asm_print_arg(out, thead, _argc);
      _argc++;
      break;
    case t_call_t:
      asm_print_call(out, thead);
      _argc = 0;
      break;
    case t_pow_t:
      LOG_ERROR("Expression a^b (power) not implemented\n");
      break;
    case t_not_t:
      LOG_ERROR("Expression ~a (not) not implemented\n");
      break;
    case t_unk_t:
      LOG_ERROR("Unknown\n");
      break;
  }
}

static void
asm_print_tacs(FILE *out, struct tac_node *thead)
{
  while (thead) {
    asm_print_tac_node(out, thead);
    thead = thead->next;
  }
}

static void
asm_print_hash_node(FILE *out, struct hash_node *hnode)
{
  // Assumes node != NULL
  // TODO modularize
  char *endptr = NULL,
       *sanitized = NULL;
  long int inival = 0,
           size = 0;
  struct ast_node *vlist = NULL;
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#NODE_START %s\n", hnode->key);
  switch (hnode->typeinfo.nature) {
    case hn_vec_t:
      if ((hnode->astinfo == NULL) || (hnode->astinfo->symbol == NULL))
        LOG_AND_EXIT("No size information for vector %s\n", hnode->key);
      size = asm_strtol(hnode->astinfo->symbol->key);
      if (size == 0)
        LOG_AND_EXIT("Invalid vector size for %s\n", hnode->key);
      fprintf(out, ".text\n");
      fprintf(out, ".globl "VP"%s\n", hnode->key);
      fprintf(out, ".data\n");
      fprintf(out, ".size "VP"%s, %ld\n", hnode->key, size * 4); // TODO sizes based on type
      fprintf(out, VP"%s:\n", hnode->key);
      vlist = hnode->astinfo->children[2];
      // TODO ugh...
      while (size > 0) {
        if (vlist != NULL) {
          if (vlist->children[0] != NULL) {
            ast_validate_children(vlist, 1, __func__, __LINE__);
            if (vlist->children[0]->symbol != NULL)
              inival = asm_strtol(vlist->children[0]->symbol->key);
            else
              LOG_ERROR("?\n");
          } else if (vlist->symbol != NULL) {
            inival = asm_strtol(vlist->symbol->key);
          } else {
            LOG_ERROR("?\n");
          }
          fprintf(out, ".long %ld\n", inival);
          vlist = vlist->children[1];
        } else {
          fprintf(out, ".long 0\n");
        }
        size--;
      }
      break;
    case hn_int_t:
      fprintf(out, ".text\n");
      fprintf(out, ".globl "VP"%s\n", hnode->key);
      fprintf(out, ".data\n");
      fprintf(out, ".size "VP"%s, 4\n", hnode->key); // TODO sizes based on type
      fprintf(out, VP"%s:\n", hnode->key);
      fprintf(out, ".long %s\n", hnode->key);
      break;
    case hn_str_t:
      // remove double quotes
      endptr = asm_get_str_wo_quotes(hnode->key);
      sanitized = asm_get_str_sanitized(endptr);
      fprintf(out, ".data\n");
      fprintf(out, VP"%s:\n", sanitized);
      fprintf(out, ".string %s\n", hnode->key);
      free(endptr);
      free(sanitized);
      break;
    case hn_id_t:
    case hn_var_t:
    case hn_arg_t:
      fprintf(out, ".text\n");
      fprintf(out, ".globl "VP"%s\n", hnode->key);
      fprintf(out, ".data\n");
      fprintf(out, ".size "VP"%s, 4\n", hnode->key); // TODO sizes based on type
      fprintf(out, VP"%s:\n", hnode->key);
      if ((hnode->astinfo != NULL) && (hnode->astinfo->symbol != NULL)) {
        inival = asm_strtol(hnode->astinfo->symbol->key);
      }
      fprintf(out, ".long %ld\n", inival);
      break;
    case hn_func_t:
      fprintf(out, ".text\n");
      fprintf(out, ".globl %s\n", hnode->key);
      break;
    default:
      LOG_INFO("Unmapped nature %d\n", hnode->typeinfo.nature);
  }
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    fprintf(out, "#NODE_END %s\n", hnode->key);
}

static void
asm_print_hash(FILE *out, struct hash_node **hhead, size_t hsize)
{
  fprintf(out, "#HASH_START\n");
  struct hash_node *node = NULL;
  for (size_t i = 0; i < hsize; i++) {
    node = hhead[i];
    while (node != NULL) {
      asm_print_hash_node(out, node);
      node = node->next;
    }
  }
  fprintf(out, "#HASH_END\n");
}

static void
asm_print_fixed_init(FILE *out)
{
  fprintf(out, "##INI_START\n");
  fprintf(out, "ufrgs_printf_int:\n");
  fprintf(out, ".string \"%%d \"\n");
  fprintf(out, "ufrgs_scanf_int:\n");
  fprintf(out, ".string \"%%d\"\n");
  fprintf(out, "##INI_END\n");
}

void
asm_print(FILE *out, struct tac_node *thead, struct hash_node **hhead, size_t hsize)
{
  asm_print_tacs(out, thead);
  asm_print_hash(out, hhead, hsize);
  asm_print_fixed_init(out);
}
