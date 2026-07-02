#ifndef BPLUS_H
#define BPLUS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definição do tipo offset para navegação no arquivo binário
typedef long int offset;

// Cabeçalho da árvore: armazena metadados estruturais
typedef struct {
    offset raiz;
    offset primeiroLivre;
    // Adicionados atributos obrigatórios para a árvore genérica funcionar
    int ordemInterna;
    int ordemFolha;
    int tamanhoChave;
    int tamanhoDado;
} cabecalho;

// Página genérica da Árvore (nó interno ou folha)
typedef struct {
    int ehFolha;            
    int numChaves;          
    void *chaves;          
    offset *filhos;         
    void *dados;            
    offset proximaFolha;   
} pagina;

// Estrutura principal em RAM que gerencia o arquivo e os callbacks
typedef struct {
    FILE *arquivoBinario;
    cabecalho cab;
    // Callback corrigido: Recebe uma função que compara duas chaves genéricas
    int (*comparar)(const void*, const void*); 
} ArvoreBPlus;

// Inicialização e Gerenciamento do Arquivo 
void criarArquivo(char *nomeArquivo, int ordemInterna, int ordemFolha, int tamanhoChave, int tamanhoDado);
ArvoreBPlus* abrirArquivo(char *nomeArquivo);
void fecharArvore(ArvoreBPlus *arvore);

// Operações Principais (CRUD Genérico)
void inserir(ArvoreBPlus *arvore, void *chave, void *dado);
int buscar(ArvoreBPlus *arvore, void *chaveBusca, void *dadoRetorno);
int remover(ArvoreBPlus *arvore, void *chave);

// Manipulação e navegação das páginas em disco 
pagina criarPagina(int ehFolha);
offset criarPaginaDisco(ArvoreBPlus *arvore, int ehFolha);
offset obterNovaPagina(ArvoreBPlus *arvore, int ehFolha);
void escreverPagina(FILE *fp, offset pos, pagina *p);
void lerPagina(FILE *fp, offset pos, pagina *p);
void salvarCabecalho(ArvoreBPlus *arvore);

// Lógica de particionamento (Split) 
int inserirAux(ArvoreBPlus *arvore, offset atualOff, void *chave, void *dado, void *chavePromovida, offset *filhoPromovido);

#endif 