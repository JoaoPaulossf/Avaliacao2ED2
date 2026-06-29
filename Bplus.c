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

long int buscar_offset_livre(ArvoreBPlus *arvore) {
    long int offset_retorno;
    cabecalhoArvore cabecalho;
    
    fseek(arvore->arquivo_binario, 0, SEEK_SET);
    fread(&cabecalho, sizeof(cabecalho), 1, arvore->arquivo_binario);
    
    if (cabecalho.topo_lista_livre != -1) {
        offset_retorno = cabecalho.topo_lista_livre;
        
        pagina *p = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_retorno, p, arvore->size_chave, arvore->size_dado);
        
        cabecalho.topo_lista_livre = p->proxima_folha; 
        free(p);
    } else {
        fseek(arvore->arquivo_binario, 0, SEEK_END);
        offset_retorno = ftell(arvore->arquivo_binario);
    }
    
    fseek(arvore->arquivo_binario, 0, SEEK_SET);
    fwrite(&cabecalho, sizeof(cabecalho), 1, arvore->arquivo_binario);
    
    return offset_retorno;
}

void* split_folha(ArvoreBPlus *arvore, pagina *folha_cheia, long int offset_cheia) {
    //Função responsável por criar uma nova folha quando a anterior esta cheia
    pagina *nova_folha = malloc(sizeof(pagina));
    nova_folha->eh_folha = 1;
    
    nova_folha->chaves = malloc(arvore->ordem_folha * sizeof(void*));
    nova_folha->dados = malloc(arvore->ordem_folha * arvore->size_dado);
    
    //Com a nova folha criada, se passa metade dos dados da cheia para a nova;
    int total_chaves = folha_cheia->num_chaves;
    int metade = total_chaves / 2;
    int j = 0;

    for (int i = metade; i < total_chaves; i++) {
        nova_folha->chaves[j] = malloc(arvore->size_chave);
        memcpy(nova_folha->chaves[j], folha_cheia->chaves[i], arvore->size_chave);
        
        memcpy(&nova_folha->dados[j * arvore->size_dado], &folha_cheia->dados[i * arvore->size_dado], arvore->size_dado);
        j++;
        free(folha_cheia->chaves[i]); 
        folha_cheia->chaves[i] = NULL;
    }

    nova_folha->num_chaves = j;
    folha_cheia->num_chaves = metade; 

    nova_folha->proxima_folha = folha_cheia->proxima_folha;
    folha_cheia->proxima_folha = buscar_offset_livre(arvore); 

    escrever_pagina(arvore->arquivo_binario, offset_cheia, folha_cheia, arvore->size_chave, arvore->size_dado);
    escrever_pagina(arvore->arquivo_binario, folha_cheia->proxima_folha, nova_folha, arvore->size_chave, arvore->size_dado);
    
    free(nova_folha); 
}

int inserir_arvore(ArvoreBPlus *arvore, void *chave, void *dado, int(*comparar)(void*, void*)){
    long int offset_atual = arvore->raiz_offset;
    
    long int caminho_pais[10]; 
    int nivel_atual = 0;

    pagina *pagina_atual = malloc(sizeof(pagina));
    ler_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
    
    while (pagina_atual->eh_folha == 0) {
        caminho_pais[nivel_atual] = offset_atual;
        nivel_atual++;

        int i = 0;
        while (i < pagina_atual->num_chaves && comparar(chave, pagina_atual->chaves[i]) >= 0) {
            i++;
        }
        offset_atual = pagina_atual->filhos[i];
        ler_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
    }

    //Procura a posição dentro da pagina em que se deve inserir a chave e desloca o resto para encaixar.
    if(pagina_atual->num_chaves < arvore->ordem_folha -1){
        int i = 0;
        while (i < pagina_atual->num_chaves && comparar(chave, pagina_atual->chaves[i]) > 0) {
            i++;
        }
        memmove(&pagina_atual->chaves[i + 1], &pagina_atual->chaves[i], (pagina_atual->num_chaves - i) * sizeof(void*));
        memmove(&pagina_atual->dados[i + 1], &pagina_atual->dados[i], (pagina_atual->num_chaves - i) * arvore->size_dado);

        pagina_atual->chaves[i] = chave;
        pagina_atual->dados[i] = dado;
        pagina_atual->num_chaves++;

        escrever_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
        free(pagina_atual);
        return 0;
    } else {
        void *chave_promovida = split_folha(arvore, pagina_atual, offset_atual);
        
        long int offset_nova_folha = pagina_atual->proxima_folha; 
        
        propagar_insercao_pai(arvore, chave_promovida, offset_nova_folha, caminho_pais, nivel_atual);

        free(pagina_atual);
        return 1;
    }
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

    if (pagina->eh_folha == 0) {
        for (int i = 0; i <= pagina->num_chaves; i++) { // B+ tem num_chaves + 1 filhos [cite: 217]
            memcpy(&(pagina->filhos[i]), buffer + posicao_atual, sizeof(long int));
            posicao_atual += sizeof(long int);
        }
    } else {
        for (int i = 0; i < pagina->num_chaves; i++) {
            pagina->dados[i] = malloc(size_dado);
            memcpy(pagina->dados[i], buffer + posicao_atual, size_dado);
            posicao_atual += size_dado;
        }
        memcpy(&(pagina->proxima_folha), buffer + posicao_atual, sizeof(long int));
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