#pragma once

#include "ast.h"
#include "hash.h"

enum se_t { se_redcl_t, se_iop_t, se_badargs_t, se_badargt_t };

/*
 * Imprime erros sintáticos e retorna a quantidade
 */
int
semantic_analyze(struct ast_node *head);
