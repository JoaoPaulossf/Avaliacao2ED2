#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "AuxRH.h"

//Bloco de funções callback utilizadas no Bplus.h
int tamanho_chave_rh() {
    //Função callback de tamanho da chave;
    return sizeof(ChaveFuncionario);
}

int tamanho_dado_rh() {
    //Função callback de tamanho do dado
    return sizeof(Funcionario);
}

int comparar_data_rh(Data d1, Data d2) {
    //Função auxiliar de comparação de ordem cronologica
    if (d1.ano != d2.ano) return d1.ano - d2.ano;
    if (d1.mes != d2.mes) return d1.mes - d2.mes;
    return d1.dia - d2.dia;
}

int comparar_nome_rh(void *c1, void *c2) {
    //Função auxiliar de comparação por ordem alfabetica
    ChaveFuncionario *chave1 = (ChaveFuncionario *)c1;
    ChaveFuncionario *chave2 = (ChaveFuncionario *)c2;
    return strcmp(chave1->nome, chave2->nome);
}

int comparar_chaves_rh(void *c1, void *c2) {
    //Função principal de callback para comparação de chaves por ordem alfabetica e em caso de empate cronologica
    int cmp_nome = comparar_nome_rh(c1, c2);
    if (cmp_nome != 0) {
        return cmp_nome; 
    }

    ChaveFuncionario *chave1 = (ChaveFuncionario *)c1;
    ChaveFuncionario *chave2 = (ChaveFuncionario *)c2;
    return comparar_data_rh(chave1->data_nascimento, chave2->data_nascimento);
}

void ler_string(char *destino, int tamanho) {
    //Função auxiliar de leitura de string
    fgets(destino, tamanho, stdin);
    destino[strcspn(destino, "\n")] = '\0';
}

void imprimir_ficha_funcionario(Funcionario *func) {
    //Funcao responsavel por imprimir a ficha de funcionario
    printf("\n--- FICHA DO FUNCIONARIO ---\n");
    printf("Nome: %s\n", func->nome);
    printf("Nascimento: %02d/%02d/%04d\n", func->data_nascimento.dia, func->data_nascimento.mes, func->data_nascimento.ano);
    printf("Filiacao: %s (Mae) e %s (Pai)\n", func->nome_mae, func->nome_pai);
    printf("Contato: %s | Tel: %s\n", func->endereco, func->telefone);
    printf("Status: %s\n", func->ativo ? "Ativo" : "Inativo");
    int qtd_adaptado = func->qtd_pagamentos - 12*(func->qtd_pagamentos/12);
    printf("Historico de Pagamentos:\n");
    for(int i = 0; i < qtd_adaptado; i++){
        printf("%d/%d: %.2f\n", func->historico_pagamentos[i].mes_referencia, func->historico_pagamentos[i].ano_referencia, func->historico_pagamentos[i].valor);
    }
    printf("Pagamentos Registrados: %d\n", func->qtd_pagamentos);
    printf("----------------------------\n");
}

void imprimir_chave_rh(void *chave) {
    //Função auxiliar para a impressão da árvore;
    ChaveFuncionario *c = (ChaveFuncionario *)chave;
    printf("[%s - %02d/%02d/%04d]", c->nome, c->data_nascimento.dia, c->data_nascimento.mes, c->data_nascimento.ano);
}