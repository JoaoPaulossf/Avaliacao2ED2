#include <stdio.h>
#include <stdlib.h>

#define MAX_ORDEM 5
#define MAX_CHAVE 100
#define MAX_DADO 256

typedef long int offset;

typedef struct {
    offset raiz;
    offset primeiroLivre;

    int quantidadePaginas;

    int ordemInterna;
    int ordemFolha;

    int tamanhoDado;
    int tamanhoChave;

} cabecalho;

typedef struct {
    int ehFolha; // 1 para folha, 0 para não folha
    int numChaves;
    char chaves[MAX_ORDEM][MAX_CHAVE];
    offset filhos[MAX_ORDEM + 1];
    char dados[MAX_ORDEM][MAX_DADO];
    offset proximaFolha; 
}pagina;

typedef struct {
    FILE *arquivoBinario;
    cabecalho cab;
}ArvoreBPlus;

//Função responsavel por criar o arquivo binário e inicializar o cabeçalho da árvore B+
void criarArquivo(char *nomeArquivo, int ordemInterna, int ordemFolha, int tamanhoChave, int tamanhoDado){
    FILE *arquivo = fopen(nomeArquivo, "wb");
    if (arquivo == NULL) {
        printf("Erro ao criar o arquivo.\n");
        return;
    }

    cabecalho cab;

    cab.raiz = -1; // Inicialmente, não há raiz
    cab.primeiroLivre = -1;   
    cab.quantidadePaginas = 0;

    cab.tamanhoChave = tamanhoChave;
    cab.tamanhoDado = tamanhoDado;

    cab.ordemInterna = ordemInterna;
    cab.ordemFolha = ordemFolha;
    
    fwrite(&cab, sizeof(cabecalho), 1, arquivo);
    fclose(arquivo);
}

ArvoreBPlus* abrirArquivo(char *nomeArquivo) {
    ArvoreBPlus *arvore = (ArvoreBPlus *)malloc(sizeof(ArvoreBPlus));
    if (arvore == NULL) {
        printf("Erro ao alocar memória para a árvore B+.\n");
        return NULL;
    }

    arvore->arquivoBinario = fopen(nomeArquivo, "r+b");

    if (arvore->arquivoBinario == NULL) {
        printf("Erro ao abrir o arquivo.\n");
        free(arvore);
        return NULL;
    }

    fread(&arvore->cab, sizeof(cabecalho), 1, arvore->arquivoBinario);
    return arvore;
}

void fecharArvore(ArvoreBPlus *arvore) {
    fseek(arvore->arquivoBinario, 0, SEEK_SET);
    fwrite(&arvore->cab, sizeof(cabecalho), 1, arvore->arquivoBinario);
    fclose(arvore->arquivoBinario);
    free(arvore);
}

pagina criarPagina(int ehFolha) {
    pagina novaPagina;

    novaPagina.ehFolha = ehFolha;
    
    novaPagina.numChaves = 0;
    
    novaPagina.proximaFolha = -1; // Inicialmente, não há próxima folha

    for(int i = 0; i < MAX_ORDEM + 1; i++) {
        novaPagina.filhos[i] = -1;
    }

    memset(novaPagina.chaves, 0, sizeof(novaPagina.chaves));
    memset(novaPagina.dados, 0, sizeof(novaPagina.dados));

    return novaPagina;
}

void escreverPagina(FILE *fp,offset  pos, pagina *p){
    fseek(fp, pos, SEEK_SET);

    fwrite(p, sizeof(pagina), 1, fp);
    fflush(fp);
}

void lerPagina(FILE *fp, offset pos, pagina *p){
    fseek(fp, pos, SEEK_SET);

    fread(p, sizeof(pagina), 1, fp);
}

offset criarPaginaDisco(ArvoreBPlus *arvore, int ehFolha) {
    pagina novaPagina = criarPagina(ehFolha);

    fseek(arvore->arquivoBinario, 0, SEEK_END);

    offset posicao = ftell(arvore->arquivoBinario);

    fwrite(&novaPagina, sizeof(pagina), 1, arvore->arquivoBinario);
    fflush(arvore->arquivoBinario);
    
    arvore->cab.quantidadePaginas++;

    return posicao;
}

offset obterNovaPagina(ArvoreBPlus *arvore, int ehFolha) {
    if (arvore->cab.primeiroLivre != -1) {
        offset posicao = arvore->cab.primeiroLivre;

        pagina paginaLivre;
        lerPagina(arvore->arquivoBinario, posicao, &paginaLivre);

        arvore->cab.primeiroLivre = paginaLivre.filhos[0]; // quando a pagina está livre, o primeiro filho aponta para a próxima página livre
        salvarCabecalho(arvore);
        pagina nova = criarPagina(ehFolha);
        escreverPagina(arvore->arquivoBinario, posicao, &nova);

        return posicao;
    } else {
        return criarPaginaDisco(arvore, ehFolha);
    }
}