#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include "logging.h"
#include "asm.h"
#include "tac.h"
#include "ast.h"
#include "hash.h"
#include "errors.h"
#include "parser.tab.h"
#include "semantic.h"
//extern int yylex_destroy(void);
extern int isRunning(void);
extern void initMe(void);
extern FILE *yyin;

int
main(int argc, char **argv)
{
  int ans = E_SUCCESS;

  if (argc < 3) {
    fprintf(stderr, "Número de argumentos insuficiente. Sintaxe: ./etapa3 INPUT OUTPUT\n");
    ans = E_ARGS;
    goto gc_none;
  }
  yyin = fopen(argv[1], "r");
  if (yyin == NULL) {
    fprintf(stderr, "Não foi possível abrir o arquivo %s para leitura: %s\n", argv[1], strerror(errno));
    ans = E_IO;
    goto gc_none;
  }
  FILE *out = fopen(argv[2], "w");
  if (out == NULL) {
    fprintf(stderr, "Não foi possível abrir o arquivo %s para escrita: %s\n", argv[2], strerror(errno));
    ans = E_IO;
    goto gc_in;
  }
  initMe();
  int err = yyparse();
  if (err == 0) {
    // Etapa 3
    //ast_print(AST_HEAD, 0);
    //ast_print_disassemble(out, AST_HEAD);
    // Etapa 4
    int nerr;
    nerr = semantic_analyze(AST_HEAD);
    if (nerr > 0) {
      fprintf(stderr, "There were %d semantic errors.\n", nerr);
      if (nerr > 9000)
        fprintf(stderr, "It's over 9000!\n");
      ans = E_SEMANTIC;
    }
    // Etapa 5
    struct tac_node *tactail = tac_gencode(AST_HEAD);
    //tac_print(tac_get_head(tactail));
    // Etapa 6
    asm_print(out, tac_get_head(tactail), HASH_TABLE, HASH_SIZE);
    // TODO ast free
  } else {
    fprintf(stderr, "bison parsing error=%d\n", ans);
    ans = E_SYNTAX;
    //goto gc_hash;
  }
  //printf("Success. Running=%d\n", isRunning());
//  hash_print();
//gc_parser:
  //yylex_destroy();
//gc_hash:
  hash_free();
//gc_out:
  fclose(out);
gc_in:
  fclose(yyin);
gc_none:
  return ans;
}
