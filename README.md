This repository contains a compiler implementation using yacc/bison for a
simple programming language. It was done for a college assignment so the code
is messy and comments/docs are in Portuguese.

Esse respositório contém uma implementação de um compilador para a linguagem
de programação da turma 2020/01 de compiladores da UFRGS (Johann). Abaixo, o
relatório entregue com uma breve explicação do funcionamento.

# Introdução

A implementação se deu conforme instruído em aula: percorremos a hash emitindo
asm para os símbolos, tratando constantes e variáveis como globais, todos tipos
como inteiros 32-bits, e depois percorremos as TACs emitindo asm para as
instruções. Isso está implementado na função pública `asm_print` de `asm.c`:

```C
void
asm_print(struct tac_node *thead, struct hash_node **hhead, size_t hsize)
{
  asm_print_tacs(thead);
  asm_print_hash(hhead, hsize);
  asm_print_fixed_init();
}
```

As funções de interesse são `asm_print_tacs` e `asm_print_hash`, que serão
descritas junto aos testes para cada comando da linguagem, abaixo.

# Testes

O aluno iniciou a implementação pelos comandos de print, return, atribuição, e
read, seguindo o que foi passado em aula, verificando o que `gcc -S teste.c`
emitia e gerando código para emitir o mesmo quando encontrasse essas TACs.

Para o primeiro teste será apresentado o código e asm completos. Nos próximos,
será simplificado para o relatório não ficar muito extenso. Todo o código está
implementado em `asm.c`.

## Atribuição e print

Foi testado o seguinte programa:

```
a = int : 1;
b = int : 2;

main() = int
{
  a = 3
  b = a
  print a, b
};
```

O código da atribuição é gerado desta forma:

```C
static void
asm_print_copy(struct tac_node *thead)
{
  tac_validate_ops(thead, 2, __func__, __LINE__);
  if (LOG_LEVEL == LOG_LEVEL_DEBUG)
    printf("#%s := %s\n", thead->ans->key, thead->op1->key);
  printf("movl "VP"%s(%%rip), %%eax\n", thead->op1->key);
  printf("movl %%eax, "VP"%s(%%rip)\n", thead->ans->key);
}
```

Onde `tac_validate_ops` realiza verificações por `NULL`, sendo seguido da
emissão de comentários quando o programa é compilado em modo debug (vide
Makefile), e depois da emissão de asm em si. Nos próximos exemplos esses
trechos de código auxiliares (verificação e debug) serão omitidos, por
simplicidade. Por fim, `VP` é um macro para uma string que é usada como prefixo
para todas variáveis e constantes, nomeadamente `"ufrgs_var_"`. O macro é
apenas para não precisar digitar a string toda vez.

O seguinte assembly é gerado para o programa acima:

```asm
#Function start
main:
pushq %rbp
movq %rsp, %rbp
#a := 3
movl ufrgs_var_3(%rip), %eax
movl %eax, ufrgs_var_a(%rip)
#b := a
movl ufrgs_var_a(%rip), %eax
movl %eax, ufrgs_var_b(%rip)
#Print b
movl ufrgs_var_b(%rip), %eax
movl %eax, %esi
leaq ufrgs_printf_int(%rip), %rdi
movl $0, %eax
call printf@PLT
movl $0, %eax
#Print a
movl ufrgs_var_a(%rip), %eax
movl %eax, %esi
leaq ufrgs_printf_int(%rip), %rdi
movl $0, %eax
call printf@PLT
movl $0, %eax
#Function end
popq %rbp
ret
#HASH_START
#NODE_START main
.text
.globl main
#NODE_END main
#NODE_START 1
.text
.globl ufrgs_var_1
.data
.size ufrgs_var_1, 4
ufrgs_var_1:
.long 1
#NODE_END 1
#NODE_START 2
.text
.globl ufrgs_var_2
.data
.size ufrgs_var_2, 4
ufrgs_var_2:
.long 2
#NODE_END 2
#NODE_START 3
.text
.globl ufrgs_var_3
.data
.size ufrgs_var_3, 4
ufrgs_var_3:
.long 3
#NODE_END 3
#NODE_START a
.text
.globl ufrgs_var_a
.data
.size ufrgs_var_a, 4
ufrgs_var_a:
.long 1
#NODE_END a
#NODE_START b
.text
.globl ufrgs_var_b
.data
.size ufrgs_var_b, 4
ufrgs_var_b:
.long 2
#NODE_END b
#HASH_END
##INI_START
ufrgs_printf_int:
.string "%d "
ufrgs_scanf_int:
.string "%d"
##INI_END
```

E o resultado da execução do programa é este:

```sh
$ gcc teste.s
$ ./a.out
3 3
```

A única observação notável aqui é que o comando de print tem uma implementação
separada para imprimir inteiros/variáveis, usando a string de formatação do
`printf` pré-inicializada (`ufrgs_printf_int`), e outra implementação para
imprimir strings, usando a string tal qual declarada pelo usuário. Um outro
detalhe é que sanitizamos a string definida pelo usuário na hora de dar um nome
a ela. Por exemplo, a string `"or ok"` é emitida desta forma, removendo o
espaço:

```asm
.data
ufrgs_var_orok:
.string "or ok"
```

E o comando de print fica:

```asm
#Print "or ok"
movl ufrgs_var_orok(%rip), %eax
movl %eax, %esi
leaq ufrgs_var_orok(%rip), %rdi
movl $0, %eax
call printf@PLT
movl $0, %eax
```

Observo que isso poderia ser otimizado trocando `printf` por `puts`, visto que
não há formatação.

## Retorno

A implementação é mover o valor a EAX, seguido de `pop %rdp` e `ret`, em
`ams_print_ret`.

O programa testado foi:

```
main() = int
{
  return 3
  return 2
};
```

Execução e retorno:


```sh
$ gcc teste.s
$ ./a.out
$ echo $?
3
```

## Read

A implementação é uma chamada ao scanf, em `asm_print_read`.

```
main() = int
{
  read a
  print a
  return 0
};
```

```sh
$ gcc teste.s
$ ./a.out
10
10
```

## Expressões e controle de fluxo

O aluno já tinha implementado o controle de fluxo nas TACs, bastava implementar
as os jumps, labels, e as de expressões. As expressões são implementadas em
`asm_print_expr`. Em todos os casos `op1` é movido a `eax` e op2 a `edx`, e o
resultado da expressão é movido de `eax` para `ans`. A implementação de soma,
subtração e multiplicação é trivial. A divisão é implementada da seguinte
forma.

```
  // stackoverflow.com/questions/39658992
  printf("movl %%eax, %%ebx\n");
  printf("movl %%edx, %%ecx\n");
  printf("cdq\n");
  printf("idivl %%ecx\n");
```

O `or` da seguinte forma (baseado no que o gcc faz):

```C
  printf("testl %%eax, %%eax\n");
  printf("jne .true%d\n", or_labels++);
  printf("testl %%edx, %%edx\n");
  printf("je .false%d\n", or_labels++);
  printf(".true%d:\n", or_labels-2);
  printf("movl $1, %%eax\n");
  printf("jmp .finish%d\n", or_labels++);
  printf(".false%d:\n", or_labels-2);
  printf("mov $0, %%eax\n");
  printf(".finish%d:\n", or_labels-1);
```

E o `and`:

```C
  printf("cmpl %%eax, %%edx\n");
  printf("sete %%al\n");
  printf("movzbl %%al, %%eax\n");
```

Onde `movzbl al eax` move o valor do registrador 8-bits `al` para 32-bits
`eax` complementando os bits restantes com zero.

As comparações são implementadas em `asm_print_cmp` e fazem basicamente `cmpl`
seguido de `setl/setg/setge/setle/sete/setne` e `movzbl al eax`.

Finalmente, o jump  e o label são triviais, enquanto o jumpfalse é simplesmente
`testl` seguido de `je`.

Tendo os jumps e as expressões implementadas o controle de fluxo implícito nas
TACs passa a funcionar natualmente. Testes realizados:

```
a = int : 1;

main() = int
{
  loop (a : 1, 10, 1) {
    print a
  }
  while (a > 0) {
    a = a - 1
    print a
  }
  a = a + 10
  if (a > 0) then {
    print a
  }
  if (a < 0) then {
    print "wtf"
  }
  return a
};
```

```sh
$ gcc teste.s
$ ./a.out
1 2 3 4 5 6 7 8 9 9 8 7 6 5 4 3 2 1 0 10
```

Comparações:

```
a = int : 1;

main() = int
{
  if (2 > 1) then {
    a = 1
    print a
  }
  if (1 > 2) then {
    a = 10
    print a
  }
  if (2 < 1 ) then {
    a = 20
    print a
  }
  if (1 < 2) then {
    a = 2
    print a
  }
  if (1 == 1) then {
    a = 3
    print a
  }
  if (1 == 2) then {
    a = 30
    print a
  }
  if (1 != 2) then {
    a = 4
    print a
  }
  if (1 != 1) then {
    a = 40
    print a
  }
  return a
};
```

```sh
$ gcc teste.s
$ ./a.out
1 2 3 4
```

Expressões aritméticas:

```
a = int : 2;

main() = int
{
  print a
  a = a * 2
  print a
  a = a - 4
  print a
  a = a + 2
  print a
  a = a / 2
  print a
  return a - 1
};
```

```sh
$ gcc teste.s
$ ./a.out
2 4 0 2 1
```

And e or:

```
main() = int {
  if (0 | 1) then {
    if (1 | 0) then {
      print "or ok"
    } else {
      print "or nok"
    }
  }
  if (0 | 0) then {
    print "or nok"
  }
  if (1 & 1) then {
    print "and ok"
  }
  if (1 & 0) then {
    print "and nok"
  }
  if (0 & 1) then {
    print "and nok"
  }
};
```

```sh
$ gcc teste.s
$ ./a.out
or okand ok
```

## Call

Para a chamada de função o aluno alterou as TACs de argumento para conter a
função à qual o argumento pertence. A implementação é movendo os valores
passados para as globais dos argumentos. Não foi usada a stack.

```
foo(a = int, b = int, c = int, d = int) = int
{
  return a + b + c + d
};

bar() = int
{
  return 1
};

main() = int
{
  return foo(1, 2, 3, 4) + bar()
};
```


```sh
$ gcc teste.s
$ ./a.out
$ echo $?
11
```

## Vetores

O aluno precisou modificar a hash dos identificadores de natureza "vetor" para
apontar para o nodo correspondente na AST, de forma que, ao percorrer a hash
para emitir asm para os vetores inicializados, saibamos o tamanho deles e os
valores a serem inicializados.

```
vec = int[3] : 1 2 3;

main() = int
{
  vec[2] = 2
  return vec[2]
};
```

Geramos este assembly para isso (simplificado):

```
#NODE_START vec
.text
.globl ufrgs_var_vec
.data
.size ufrgs_var_vec, 12
ufrgs_var_vec:
.long 1
.long 2
.long 3
#NODE_END vec
```

A leitura e escrita é apenas um deslocamento em relação ao endereço da variável:

```
#vec[2] := 2
movl ufrgs_var_2(%rip), %eax
movl %eax, 8+ufrgs_var_vec(%rip)
#dummy0 := vec[2]
movl 8+ufrgs_var_vec(%rip), %eax
movl %eax, ufrgs_var_dummy0(%rip)
movl ufrgs_var_dummy0(%rip), %eax
```

```sh
$ gcc teste.s
$ ./a.out
$ echo $?
2
```
