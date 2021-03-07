%{
#include "hash.h"
#include "parser.tab.h"

int COLUMN = 0;
int RUNNING = 1;

void
initMe(void)
{
  hash_init();
}

int
getLineNumber(void)
{
  return yylineno;
}

int
isRunning(void)
{
  return RUNNING;
}

int
yywrap(void)
{
  RUNNING = 0;
  return 1;
}

#define STORE_ID_LIT(nat)\
  do {\
    struct hash_typeinfo typeinfo = { .nature = (nat), .type = ht_unknown_t }; \
    yylval.symbol = hash_insert(yytext, typeinfo);\
    COLUMN += yyleng;\
  } while(0)

%}

%s COMMENT
abc 		  [A-Za-z_@]
digit     [0-9]
hex       [0-9A-F]
alphanum 	[A-Za-z_0-9@]
integer 	{digit}+{hex}*
real 		  {integer}?"."{integer}
char 		  \'.\'
string 		\"(\\.|[^\\\"\n])*\"
id 			{abc}+{alphanum}*

%%

"/*"  						BEGIN(COMMENT);
<COMMENT>[^*\n]*			//consome qualquer coisa que não seja "*" e \n
<COMMENT>"*"+[^*/\n]*		//consome "*" que não antecede "/"
<COMMENT>\n 				{ yylineno++; } //contabiliza numero de linhas dentro do comentário
<COMMENT>"*"+"/"			BEGIN(INITIAL);

\n 			{ yylineno++; COLUMN = 0; } //contabiliza numero de linhas fora de comentários
"//".* 						//consome comentários
[ \t]+ 						//consome espacos

int 		{
  return KW_INT;
}
float 		{
  return KW_FLOAT;
}
bool 		{
  return KW_BOOL;
}
char 		{
  return KW_CHAR;
}
if 			{
  return KW_IF;
}
then 		{
  return KW_THEN;
}
else 		{
  return KW_ELSE;
}
while 		{
  return KW_WHILE;
}
loop 		{
  return KW_LOOP;
}
read 		{
  return KW_READ;
}
print 		{
  return KW_PRINT;
}
return 		{
  return KW_RETURN;
}

[,;:()\[\]\{\}\+\-\*/<>=\|\^~&\$#] {
  return *yytext;
}

"<=" 		{
  return OPERATOR_LE;
}
">=" 		{
  return OPERATOR_GE;
}
"==" 		{
  return OPERATOR_EQ;
}
"!=" 		{
  return OPERATOR_DIF;
}
FALSE 		{
  STORE_ID_LIT(hn_bool_t);
  return LIT_FALSE;
}
TRUE 		{
  STORE_ID_LIT(hn_bool_t);
  return LIT_TRUE;
}
{integer} 	{
  STORE_ID_LIT(hn_int_t);
  return LIT_INTEGER;
}
{id} 		{
  STORE_ID_LIT(hn_id_t);
  return TK_IDENTIFIER;
}
{real} 		{
  STORE_ID_LIT(hn_float_t);
  return LIT_FLOAT;
}
{char} 		{
  STORE_ID_LIT(hn_char_t);
  return LIT_CHAR;
}
{string} 	{
  STORE_ID_LIT(hn_str_t);
  return LIT_STRING;
}
. 			{
  return TOKEN_ERROR;
}

%%
