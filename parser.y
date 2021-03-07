%{
#include <stdio.h>
#include <stdlib.h>
#include "hash.h"
#include "ast.h"
int yylex(void);
void yyerror(char const *s);
%}

%union
{
  struct hash_node *symbol;
  struct ast_node *node;
}

%token<symbol> KW_CHAR
%token<symbol> KW_INT
%token<symbol> KW_FLOAT
%token<symbol> KW_BOOL

%token KW_IF
%token KW_THEN
%token KW_ELSE
%token KW_WHILE
%token KW_LOOP
%token KW_READ
%token KW_PRINT
%token KW_RETURN

%token OPERATOR_LE
%token OPERATOR_GE
%token OPERATOR_EQ
%token OPERATOR_DIF
%token<symbol> TK_IDENTIFIER

%token<symbol> LIT_INTEGER
%token<symbol> LIT_FLOAT
%token<symbol> LIT_TRUE
%token<symbol> LIT_FALSE
%token<symbol> LIT_CHAR
%token<symbol> LIT_STRING

%token TOKEN_ERROR

%type<node> expr fcall_arg fcall_arglist fcall_arglistresto fcall lit operando id attr bloco cmd cmdlist flow ifelse elseopt whiledo loop print return read parglist parg string type fsig arg arglist arglistresto gsimple gvector global gvectoropt vlist programa root func

%define parse.error verbose

%left '|' '&'
%left '<' '>' OPERATOR_LE OPERATOR_GE OPERATOR_EQ OPERATOR_DIF
%left '+' '-'
%left '*' '/'
%left '^'
%left '~'

%%

root:
  programa { $$ = $1; AST_HEAD = $1; }

programa:
  global programa { $$ = ast_create(a_plist_t, NULL, 2, $1, $2); }|
  func programa   { $$ = ast_create(a_plist_t, NULL, 2, $1, $2); }|
                  { $$ = NULL; };

/*
 * Globais
 */

global:
  gsimple ';' { $$ = $1; }|
  gvector ';' { $$ = $1; };

gsimple:
  id '=' type ':' lit { $$ = ast_create(a_decl_t, NULL, 3, $1, $3, $5); };

gvector:
  id '=' type '[' LIT_INTEGER ']' gvectoropt { $$ = ast_create(a_vdecl_t, $5, 3, $1, $3, $7); };

gvectoropt:
  ':' vlist { $$ = $2; }|
            { $$ = NULL; };

/*
 * Funções
 */

func:
  fsig bloco ';' { $$ = ast_create(a_fdecl_t, NULL, 2, $1, $2); };

fsig:
  id '(' arglist ')' '=' type { $$ = ast_create(a_fsig_t, NULL, 3, $1, $3, $6); };

arglist:
  arg arglistresto  { $$ = ast_create(a_csv_t, NULL, 2, $1, $2); }|
                    { $$ = NULL; };

arglistresto:
  ',' arg arglistresto { $$ = ast_create(a_csv_t, NULL, 2, $2, $3); }|
                       { $$ = NULL; };

arg:
  id '=' type { $$ = ast_create(a_tvar_t, NULL, 2, $1, $3); };

/* Expressões */

bloco:
  '{' cmdlist '}' { $$ = ast_create(a_block_t, NULL, 1, $2); };

cmdlist:
  cmd         { $$ = $1; }|
  cmd cmdlist { $$ = ast_create(a_cmdl_t, NULL, 2, $1, $2); };

/*
 * Vai gerar shift/reduce, professor disse que é ok. Remover o vazio para
 * debugar outros conflitos
 */
cmd:
  bloco  { $$ = $1; }|
  attr   { $$ = $1; }|
  flow   { $$ = $1; }|
  read   { $$ = $1; }|
  print  { $$ = $1; }|
  return { $$ = $1; }|
         { $$ = NULL; };

attr:
  id '=' expr              { $$ = ast_create(a_attr_t, NULL, 2, $1, $3); } |
  id '[' expr ']' '=' expr { $$ = ast_create(a_vattr_t, NULL, 3, $1, $3, $6); };

read:
  KW_READ id { $$ = ast_create(a_read_t, NULL, 1, $2); };

print:
  KW_PRINT parglist { $$ = ast_create(a_print_t, NULL, 1, $2); };

parglist:
  parg              { $$ = ast_create(a_csv_t, NULL, 1, $1); ; }|
  parg ',' parglist { $$ = ast_create(a_csv_t, NULL, 2, $1, $3); };

parg:
  string { $$ = $1; }|
  expr   { $$ = $1; };

string:
  LIT_STRING { $$ = ast_create(a_sym_t, $1, 0); };

return:
  KW_RETURN expr { $$ = ast_create(a_ret_t, NULL, 1, $2); };

/* Não simplificar com expr operador expr, %left não funciona daí */
expr:
  '(' expr ')'                { $$ = ast_create(a_paren_t, NULL, 1, $2); } |
  operando                    { $$ = ast_create(a_op_t,  NULL, 1, $1); } |
  expr '+' expr               { $$ = ast_create(a_add_t, NULL, 2, $1, $3); } |
  expr '-' expr               { $$ = ast_create(a_sub_t, NULL, 2, $1, $3); } |
  expr '*' expr               { $$ = ast_create(a_mul_t, NULL, 2, $1, $3); } |
  expr '/' expr               { $$ = ast_create(a_div_t, NULL, 2, $1, $3); } |
  expr '<' expr               { $$ = ast_create(a_lt_t,  NULL, 2, $1, $3); } |
  expr '>' expr               { $$ = ast_create(a_gt_t,  NULL, 2, $1, $3); } |
  expr '|' expr               { $$ = ast_create(a_or_t,  NULL, 2, $1, $3); } |
  expr '^' expr               { $$ = ast_create(a_pow_t, NULL, 2, $1, $3); } |
  expr '~' expr               { $$ = ast_create(a_not_t, NULL, 2, $1, $3); } |
  expr '&' expr               { $$ = ast_create(a_and_t, NULL, 2, $1, $3); } |
  expr OPERATOR_LE expr       { $$ = ast_create(a_le_t,  NULL, 2, $1, $3); } |
  expr OPERATOR_GE expr       { $$ = ast_create(a_ge_t,  NULL, 2, $1, $3); } |
  expr OPERATOR_EQ expr       { $$ = ast_create(a_eq_t,  NULL, 2, $1, $3); } |
  expr OPERATOR_DIF expr      { $$ = ast_create(a_ne_t,  NULL, 2, $1, $3); };

id:
  TK_IDENTIFIER { $$ = ast_create(a_sym_t, $1, 0); };

operando:
  lit             { $$ = $1; }|
  fcall           { $$ = $1; }|
  id              { $$ = $1; }|
  id '[' expr ']' { $$ = ast_create(a_vsym_t, NULL, 2, $1, $3); };

fcall:
  id '(' fcall_arglist ')' { $$ = ast_create(a_call_t, NULL, 2, $1, $3);};

fcall_arglist:
  fcall_arg fcall_arglistresto        { $$ = ast_create(a_csv_t, NULL, 2, $1, $2);} |
                                      { $$ = NULL; };
fcall_arglistresto:
  ',' fcall_arg fcall_arglistresto    { $$ = ast_create(a_csv_t, NULL, 2, $2, $3);} |
                                      { $$ = NULL; };
fcall_arg:
  expr { $$ = $1; };

flow:
  ifelse    { $$ = $1; }|
  whiledo   { $$ = $1; }|
  loop      { $$ = $1; };

ifelse:
  KW_IF '(' expr ')' KW_THEN cmd elseopt { $$ = ast_create(a_cond_t, NULL, 3, $3, $6, $7); };
/*
 * Vai gerar um shift/reduce mesmo, professor disse que é ok. Remover o vazio
 * toranando o else obrigatório para debugar conflitos.
 */
elseopt:
  KW_ELSE cmd { $$ = $2; }|
              { $$ = NULL; };

whiledo:
  KW_WHILE '(' expr ')' cmd { $$ = ast_create(a_while_t, NULL, 2, $3, $5); };

loop:
  KW_LOOP '(' id ':' expr ',' expr ',' expr ')' cmd { $$ = ast_create(a_for_t, NULL, 5, $3, $5, $7, $9, $11); };


/*
 * Tipos e valores
 */

type:
  KW_CHAR  { $$ = ast_create(a_kwc_t, NULL, 0); }|
  KW_INT   { $$ = ast_create(a_kwi_t, NULL, 0); }|
  KW_FLOAT { $$ = ast_create(a_kwf_t, NULL, 0); }|
  KW_BOOL  { $$ = ast_create(a_kwb_t, NULL, 0); };

lit:
 LIT_INTEGER { $$ = ast_create(a_sym_t, $1, 0); }|
 LIT_FLOAT   { $$ = ast_create(a_sym_t, $1, 0); }|
 LIT_TRUE    { $$ = ast_create(a_sym_t, $1, 0); }|
 LIT_FALSE   { $$ = ast_create(a_sym_t, $1, 0); }|
 LIT_CHAR    { $$ = ast_create(a_sym_t, $1, 0); };

vlist:
  lit        { $$ = $1; }|
  lit vlist  { $$ = ast_create(a_vlist_t, NULL, 2, $1, $2); };


%%

extern int yylineno;

void
yyerror(char const *s)
{
  fprintf(stderr, "%s at line %d\n", s, yylineno);
  exit(3);
}
