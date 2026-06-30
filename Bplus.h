#ifndef BPLUS_H
#define BPLUS_H

#include <stdio.h>

#define PAGE_SIZE 4096

// Struct  de Página da Árvore B+
typedef struct{
    int eh_folha;
    int num_chaves;
    void *chaves;
    long int *filhos;
    void *dados;
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
    int altura;
}ArvoreBPlus;
//1 - Bloco responsável pela manipulação de offsets e do cabeçalho
FILE* inicializar_arquivo(char* nome_arquivo); //Inicialização de um novo cabeçalho

long int buscar_offset_livre(ArvoreBPlus *arvore); //Função de busca de offset

void liberar_offset(ArvoreBPlus *arvore, long int offset_liberado); //Função de esvaziamento de offset

//2 - Bloco de criação e manipulação da arvore B+
ArvoreBPlus* criar_arvore(char* nome_arquivo, int (*size_chave)(), int (*size_dado)(), int (*comp_chaves)(void*, void*)); //Função de criação da arvore

//2.1 Bloco de funções relacionadas a inserção de chaves

void inserir_simples(ArvoreBPlus *arvore, void *chave, void *dado, pagina* pagina_atual, int posicao); //Função auxiliar de inserção simples na arvore

void split_folha(ArvoreBPlus *arvore, pagina *folha_cheia, long int offset_cheia); //Função de cisão de folhas da arvore, ativada no caso de overflow

void* split_interno(ArvoreBPlus *arvore, pagina *pai_cheio, long int offset_cheio, void *chave_inserir, long int offset_filho_direito, long int *offset_novo_pai, int (*comparar)(void*, void*)); //Função de cisão interna da arvore, ativada em caso de overflow de paginas internas

void propagar_insercao_pai(ArvoreBPlus *arvore, void *chave_promovida, long int offset_filho_direito, long int *caminho_pais, int nivel_atual, int (*comparar)(void*, void*)); //Função que propaga as consequências da cisão pela arvore

int inserir_arvore(ArvoreBPlus *arvore, void *chave, void *dado, int (*comparar)(void*, void*)); //Função núcleo de inserção de uma nova chave

//2.2 Função relacionada a busca de chaves dentro da arvore
int buscar_arvore(ArvoreBPlus *arvore, void *chave, void *dado_retorno, int (*comparar)(void*, void*));

//2.3 Bloco de funções relacionadas a remoção de chaves

void underflow_interno(ArvoreBPlus *arvore, pagina *interno, long int offset_interno, long int *caminho_pais, int nivel_atual); //Função que trata o caso de underflow em paginas internas da arvore

void propagar_remocao_pai(ArvoreBPlus *arvore, pagina *pai, long int offset_pai, int pos_chave, int pos_filho, long int *caminho_pais, int nivel_atual); //Função que propaga as consequências da remoção de uma chave pela arvore

void concatenar_folhas(ArvoreBPlus *arvore, pagina *folha_esq, long int offset_esq, pagina *folha_dir, long int offset_dir, pagina *pai, long int offset_pai, int pos_chave_pai, int pos_filho_dir, long int *caminho_pais, int nivel_atual); //Função auxiliar de concatenacao de folhas, ativada no caso de não ser possivel a redistribuição

void underflow_folha(ArvoreBPlus *arvore, pagina *folha, long int offset_folha, long int *caminho_pais, int nivel_atual); //Função que trata o caso de underflow em folhas da arvore

int remover_arvore(ArvoreBPlus *arvore, void *chave, int (*comparar)(void*, void*)); //Função núcleo de remoção de chaves
//3 Bloco de funções relacionadas a escrita e leitura de dados no disco;
void ler_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado); //Função de leitura no disco

void escrever_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado); //Função de escrita no disco

#endif