#ifndef AUXRH_H
#define AUXRH_H

#include <stdio.h>

//Bloco de estruturas utilizadas para os dados
typedef struct { //Estrutura de Data
    int dia;
    int mes;
    int ano;
} Data;

typedef struct { //Estrutura composta nome + Data
    char nome[100];
    Data data_nascimento;
} ChaveFuncionario;

typedef struct { //Estrutura para o contracheque
    float valor;
    int mes_referencia;
    int ano_referencia;
} Pagamento;

//Estrutura completa que será o dado principal
typedef struct {
    char nome[100];
    Data data_nascimento;
    char nome_mae[100];
    char nome_pai[100];
    char endereco[100];
    char telefone[11];
    Data data_contratacao;
    int ativo; //1 para ativo e 0 para inativo
    Data data_desligamento;
    Pagamento historico_pagamentos[12];
    int qtd_pagamentos;
} Funcionario;

//Bloco de funções responsaveis por serem auxiliares na chamada de funções para com o banco de dados;
int tamanho_chave_rh(); //Retorna o tamanho das chaves;

int tamanho_dado_rh(); //Retorna o tamanho dos dados;

int comparar_chaves_rh(void *c1, void *c2); //Função callback que retorna qual das duas tem prioridade baseada na ordem alfabetica e cronologica
int comparar_nome_rh(void *c1, void *c2); //Função auxiliar de comparação
int comparar_data_rh(Data d1, Data d2); //Função auxiliar de comparação


void ler_string(char *destino, int tamanho); //Função auxiliar para leitura de strings;

void imprimir_ficha_funcionario(Funcionario *func); //Função auxiliar para exibição de ficha de funcionarios;

void imprimir_chave_rh(void *chave); //Função auxiliar para a impressão da árvore

#endif