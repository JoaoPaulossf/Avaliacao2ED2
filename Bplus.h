#ifndef BPLUS_H
#define BPLUS_H

#include <stdio.h>

#define PAGE_SIZE 4096

// Struct  de Página da Árvore B+
typedef struct{
    int eh_folha;
    int num_chaves;
    void **chaves;
    long int *filhos;
    void **dados;
    long int proxima_folha;
}pagina;

//Struct da Árvore B+
typedef struct{
    FILE *arquivo_binario;
    long int raiz_offset;
    int ordem_interna;
    int ordem_folha;
    int size_chave;
    int size_dado;
}ArvoreBPlus;

FILE* inicializar_arquivo(char* nome_arquivo);

ArvoreBPlus* criar_arvore(char* nome_arquivo, int (*size_chave)(), int (*size_dado)(), int (*comp_chaves)(void*, void*));

void inserir_arvore(ArvoreBPlus *arvore, void *chave, void *dado, int(*comparar)(void*, void*));

int buscar_arvore(ArvoreBPlus *arvore, void *chave, void *dado_retorno, int (*comparar)(void*, void*));

void ler_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado);

void escrever_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado);

#endif