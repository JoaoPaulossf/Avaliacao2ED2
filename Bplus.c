#include <stdio.h>
#include <stdlib.h>

#include <Bplus.h>

//Estrutura interna para gravar no início do ficheiro;
typedef struct{
    long int raiz_offset;
    long int topo_lista_livre;
} cabecalhoArvore;

FILE* inicializar_arquivo(char* nome_arquivo){
    FILE *arquivo = fopen(nome_arquivo, "rb+");
    if (arquivo == NULL) {
        arquivo = fopen(nome_arquivo, "wb+");
        //Se o arquivo não existe ainda, o criamos e implementamos o cabeçalho devido com a Árvore Vazia e sem espaços livres ainda
        cabecalhoArvore cabecalho;
        cabecalho.raiz_offset = -1;
        cabecalho.topo_lista_livre = -1;

        fseek(arquivo, 0, SEEK_SET);
        fwrite(&cabecalho, sizeof(cabecalhoArvore), 1, arquivo);
    } 

    return arquivo;
}
ArvoreBPlus* criar_arvore(char* nome_arquivo, int (*size_chave)(), int (*size_dado)(), int (*comp_chaves)(void*, void*)){
    ArvoreBPlus *arvore = malloc(sizeof(ArvoreBPlus));
    arvore->arquivo_binario = inicializar_arquivo(nome_arquivo); //Abre e inicializa o arquivo criando o cabeçalho;

    cabecalhoArvore cabecalho;
    fseek(arvore->arquivo_binario, 0, SEEK_SET);
    fread(&cabecalho, sizeof(cabecalhoArvore), 1, arvore->arquivo_binario);
    if (fread(&cabecalho, sizeof(cabecalhoArvore), 1, arvore->arquivo_binario) != 1) {
        arvore->raiz_offset = -1;
    } else {
        arvore->raiz_offset = cabecalho.raiz_offset;
    }
    
    arvore->raiz_offset = cabecalho.raiz_offset;

    //Calculo responsável por detrminar a ordem da ávore baseado no tamanho de pagina e de suas variáveis
    int espaco_cabecalho = sizeof(int) * 2 + sizeof(long int);
    int espaco_livre = PAGE_SIZE - espaco_cabecalho;
    arvore->ordem_interna = espaco_livre / (size_chave() + sizeof(long int));
    arvore->ordem_folha = espaco_livre / (size_chave() + size_dado());

    return arvore;
}

void inserir_arvore(ArvoreBPlus *arvore, void *chave, void *dado, int(*comparar)(void*, void*)){

}


int buscar_arvore(ArvoreBPlus *arvore, void *chave, void *dado_retorno, int (*comparar)(void*, void*)) {
    long int offset_atual = arvore->raiz_offset;
    pagina *pagina_atual = malloc(sizeof(pagina));
    
    ler_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
    //Procuramos na estrutura da arvore e descemos até a folha em que esta presente a chave procurada
    while(pagina_atual->eh_folha == 0) {
        int i = 0;
        while (i < pagina_atual->num_chaves && comparar(chave, pagina_atual->chaves[i]) >= 0) {
            i++;
        }
        offset_atual = pagina_atual->filhos[i];
        ler_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
    }
    //Reiniciamos o contador e procuramos entre as chaves da página a chave procurada
    int i = 0;
    while (i < pagina_atual->num_chaves && comparar(chave, pagina_atual->chaves[i]) > 0) {
        i++;
    }
    
    if (i < pagina_atual->num_chaves && comparar(chave, pagina_atual->chaves[i]) == 0) {
        memcpy(dado_retorno, pagina_atual->dados[i], arvore->size_dado);
        free(pagina_atual);
        return 1;
    }
    
    free(pagina_atual);
    return 0;
}

void ler_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado){
    char buffer[4096];
    int posicao_atual = 0;
    //Criamos um pacote, "buffer", para ler pacote por pacote anteriormente salvo no binario, seguindo baseado no cursor posicao_atual
    fseek(arquivo, offset, SEEK_SET);
    fread(buffer, 4096, 1, arquivo);

    memcpy(&(pagina->eh_folha), buffer + posicao_atual, sizeof(int));
    posicao_atual += sizeof(int);

    memcpy(&(pagina->num_chaves), buffer + posicao_atual, sizeof(int));
    posicao_atual += sizeof(int);

    for (int i = 0; i < pagina->num_chaves; i++) {
        pagina->chaves[i] = malloc(size_chave); 
        memcpy(pagina->chaves[i], buffer + posicao_atual, size_chave);
        posicao_atual += size_chave;
    }
    for (int i = 0; i < pagina->num_chaves; i++) {
        memcpy(&(pagina->filhos[i]), buffer + posicao_atual, sizeof(long int));
        posicao_atual += sizeof(long int);
    }
}

void escrever_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado){
    char buffer[4096] = {0};
    int posicao_atual = 0;
    // Criamos um pacote, "buffer", de 4096 bytes e guardamos todos os dados da árvore b+ em um arquivo binário
    // sendo guiados pela variável posição atual.
    memcpy(buffer + posicao_atual, &(pagina->eh_folha), sizeof(int)); //Copia se são folha ou não
    posicao_atual += sizeof(int); 

    memcpy(buffer + posicao_atual, &(pagina->num_chaves), sizeof(int)); //Copia a quantidade de chaves
    posicao_atual += sizeof(int); 

    for(int i = 0; i < pagina->num_chaves; i++){
        memcpy(buffer + posicao_atual, pagina->chaves[i], size_chave); //Copia todas as chaves
        posicao_atual += size_chave;
    }
    for(int i = 0; i < pagina->num_chaves; i++){
        memcpy(buffer + posicao_atual, &(pagina->filhos[i]), sizeof(long int)); //Copia todos os filhos
        posicao_atual += sizeof(long int);
    }

    fseek(arquivo, offset, SEEK_SET); //Salvamos no disco
    fwrite(buffer, 4096, 1, arquivo);
}