#include <stdio.h>
#include <stdlib.h>

#include <Bplus.h>

//Estrutura interna para gravar no início do ficheiro;
typedef struct{
    long int raiz_offset;
    long int topo_lista_livre;
    int altura;
} cabecalhoArvore;

FILE* inicializar_arquivo(char* nome_arquivo){
    FILE *arquivo = fopen(nome_arquivo, "rb+");
    if (arquivo == NULL) {
        arquivo = fopen(nome_arquivo, "wb+");
        //Se o arquivo não existe ainda, o criamos e implementamos o cabeçalho devido com a Árvore Vazia e sem espaços livres ainda
        cabecalhoArvore cabecalho;
        cabecalho.raiz_offset = -1;
        cabecalho.topo_lista_livre = -1;
        cabecalho.altura = 0;

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
    arvore->altura = cabecalho.altura;

    //Calculo responsável por detrminar a ordem da ávore baseado no tamanho de pagina e de suas variáveis
    int espaco_cabecalho = sizeof(int) * 2 + sizeof(long int);
    int espaco_livre = PAGE_SIZE - espaco_cabecalho;
    arvore->ordem_interna = espaco_livre / (size_chave() + sizeof(long int));
    arvore->ordem_folha = espaco_livre / (size_chave() + size_dado());

    return arvore;
}

long int buscar_offset_livre(ArvoreBPlus *arvore) {
    //Funcao responsável por buscar um offset livre durante a criação de uma nova pagina
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

void inserir_simples(ArvoreBPlus *arvore, void *chave, void *dado, pagina* pagina_atual, int posicao){
    //Função responsável pela inserçãos simples da chave em uma página, basicamente inserdindoa  chave nova e movendo as demais para manter a ordem
    if (posicao < pagina_atual->num_chaves) {
        void *dest_chaves = (char *)pagina_atual->chaves + ((posicao + 1) * arvore->size_chave);
        void *src_chaves = (char *)pagina_atual->chaves + (posicao * arvore->size_chave);
        memmove(dest_chaves, src_chaves, (pagina_atual->num_chaves - posicao) * arvore->size_chave);

        void *dest_dados = (char *)pagina_atual->dados + ((posicao + 1) * arvore->size_dado);
        void *src_dados = (char *)pagina_atual->dados + (posicao * arvore->size_dado);
        memmove(dest_dados, src_dados, (pagina_atual->num_chaves - posicao) * arvore->size_dado);
    }

    memcpy((char *)pagina_atual->chaves + (posicao * arvore->size_chave), chave, arvore->size_chave);
    memcpy((char *)pagina_atual->dados + (posicao * arvore->size_dado), dado, arvore->size_dado);

    pagina_atual->num_chaves++;
}

void* split_folha(ArvoreBPlus *arvore, pagina *folha_cheia, long int offset_cheia) {
    //Função responsável pela cisão nas folhas da árvore
    pagina *nova_folha = malloc(sizeof(pagina));
    nova_folha->eh_folha = 1;
    
    //Passasse metade das chaves para a nova folha criada
    int metade = folha_cheia->num_chaves / 2;
    int qtd_movida = folha_cheia->num_chaves - metade;
    nova_folha->num_chaves = qtd_movida;

    nova_folha->chaves = malloc(arvore->ordem_folha * arvore->size_chave);
    nova_folha->dados = malloc(arvore->ordem_folha * arvore->size_dado);

    void *origem_chaves = (char *)folha_cheia->chaves + (metade * arvore->size_chave);
    void *origem_dados = (char *)folha_cheia->dados + (metade * arvore->size_dado);

    memcpy(nova_folha->chaves, origem_chaves, qtd_movida * arvore->size_chave);
    memcpy(nova_folha->dados, origem_dados, qtd_movida * arvore->size_dado);

    //Atualiza-se a folha antiga
    folha_cheia->num_chaves = metade;

    long int novo_offset = buscar_offset_livre(arvore);
    nova_folha->proxima_folha = folha_cheia->proxima_folha;
    folha_cheia->proxima_folha = novo_offset;

    //Escreve-se no disco as alterações;
    escrever_pagina(arvore->arquivo_binario, novo_offset, nova_folha, arvore->size_chave, arvore->size_dado);
    escrever_pagina(arvore->arquivo_binario, offset_cheia, folha_cheia, arvore->size_chave, arvore->size_dado);

    void *chave_promovida = malloc(arvore->size_chave);
    memcpy(chave_promovida, nova_folha->chaves, arvore->size_chave);

    free(nova_folha->chaves);
    free(nova_folha->dados);
    free(nova_folha);

    return chave_promovida; 
}

void* split_interno(ArvoreBPlus *arvore, pagina *pai_cheio, long int offset_cheio, void *chave_inserir, long int offset_filho_direito, long int *offset_novo_pai, int (*comparar)(const void*, const void*)) {
    //Função responsável pela cisao interna da arvore ocasionados durante a propagação após a insercao de uma nova chave
    //Arrays temporários apenas para a manipulação interna
    void *temp_chaves = malloc(arvore->ordem_interna * arvore->size_chave);
    long int *temp_filhos = malloc((arvore->ordem_interna + 1) * sizeof(long int));

    //Copia os dados atuais para os temporários
    memcpy(temp_chaves, pai_cheio->chaves, pai_cheio->num_chaves * arvore->size_chave);
    memcpy(temp_filhos, pai_cheio->filhos, (pai_cheio->num_chaves + 1) * sizeof(long int));

    //Procura e insere no array temporário na posição correta
    int i = 0;
    while (i < pai_cheio->num_chaves) {
        void *chave_i = (char *)temp_chaves + (i * arvore->size_chave);
        if (comparar(chave_inserir, chave_i) < 0) break;
        i++;
    }

    if (i < pai_cheio->num_chaves) {
        void *dest_chaves = (char *)temp_chaves + ((i + 1) * arvore->size_chave);
        void *src_chaves = (char *)temp_chaves + (i * arvore->size_chave);
        memmove(dest_chaves, src_chaves, (pai_cheio->num_chaves - i) * arvore->size_chave);

        long int *dest_filhos = temp_filhos + (i + 2);
        long int *src_filhos = temp_filhos + (i + 1);
        memmove(dest_filhos, src_filhos, (pai_cheio->num_chaves - i) * sizeof(long int));
    }
    memcpy((char *)temp_chaves + (i * arvore->size_chave), chave_inserir, arvore->size_chave);
    temp_filhos[i + 1] = offset_filho_direito;

    int total_chaves_temp = pai_cheio->num_chaves + 1;

    //Começasse a cisão interna
    int metade = total_chaves_temp / 2;

    //A chave do meio sobe e abandona esse nível!
    void *nova_chave_promovida = malloc(arvore->size_chave);
    memcpy(nova_chave_promovida, (char *)temp_chaves + (metade * arvore->size_chave), arvore->size_chave);

    //Configura a nova página interna
    pagina *novo_pai = malloc(sizeof(pagina));
    novo_pai->eh_folha = 0;
    novo_pai->proxima_folha = -1;
    
    novo_pai->num_chaves = total_chaves_temp - metade - 1; 
    
    novo_pai->chaves = malloc(arvore->ordem_interna * arvore->size_chave);
    novo_pai->filhos = malloc((arvore->ordem_interna + 1) * sizeof(long int));

    memcpy(novo_pai->chaves, (char *)temp_chaves + ((metade + 1) * arvore->size_chave), novo_pai->num_chaves * arvore->size_chave);
    memcpy(novo_pai->filhos, temp_filhos + (metade + 1), (novo_pai->num_chaves + 1) * sizeof(long int));

    //Atualiza o pai antigo (que fica com a metade esquerda)
    pai_cheio->num_chaves = metade;
    memcpy(pai_cheio->chaves, temp_chaves, pai_cheio->num_chaves * arvore->size_chave);
    memcpy(pai_cheio->filhos, temp_filhos, (pai_cheio->num_chaves + 1) * sizeof(long int));

    *offset_novo_pai = buscar_offset_livre(arvore);
    //Escreve-se no disco as alterações;
    escrever_pagina(arvore->arquivo_binario, offset_cheio, pai_cheio, arvore->size_chave, arvore->size_dado);
    escrever_pagina(arvore->arquivo_binario, *offset_novo_pai, novo_pai, arvore->size_chave, arvore->size_dado);

    free(temp_chaves); free(temp_filhos);
    free(novo_pai->chaves); free(novo_pai->filhos); free(novo_pai);

    return nova_chave_promovida;
};

void propagar_insercao_pai(ArvoreBPlus *arvore, void *chave_promovida, long int offset_filho_direito, long int *caminho_pais, int nivel_atual, int (*comparar)(const void*, const void*)) {
    //Função responsável por propagar casos excepcionais da inserção de uma nova chave, normalmente ocasionadas pela cisao
    //Caso base = Criar nova raiz
    if (nivel_atual == 0) {
        //Se inicia o processo de se criar uma nova págin que terá como filhos apontados as chaves
        pagina *nova_raiz = malloc(sizeof(pagina));
        nova_raiz->eh_folha = 0; 
        nova_raiz->num_chaves = 1;
        nova_raiz->proxima_folha = -1; 
        nova_raiz->chaves = malloc(arvore->ordem_interna * arvore->size_chave);
        nova_raiz->filhos = malloc((arvore->ordem_interna + 1) * sizeof(long int));

        //Copia da chave promovida
        memcpy(nova_raiz->chaves, chave_promovida, arvore->size_chave);

        //Conecta os ponteiros (filho esquerdo e filho direito)
        nova_raiz->filhos = arvore->raiz_offset; 
        nova_raiz->filhos[5] = offset_filho_direito; 

        //Escreve-se no disco as alterações;
        long int novo_offset = buscar_offset_livre(arvore); 
        escrever_pagina(arvore->arquivo_binario, novo_offset, nova_raiz, arvore->size_chave, arvore->size_dado);

        //Atualiza os dados da arvore
        arvore->raiz_offset = novo_offset;
        arvore->altura++;

        cabecalhoArvore cabecalho;
        cabecalho.raiz_offset = arvore->raiz_offset;
        cabecalho.topo_lista_livre = -1;
        cabecalho.altura = arvore->altura;

        fseek(arvore->arquivo_binario, 0, SEEK_SET);
        fwrite(&cabecalho, sizeof(cabecalhoArvore), 1, arvore->arquivo_binario);

        free(nova_raiz->chaves);
        free(nova_raiz->filhos);
        free(nova_raiz);
        
        return; 
    } else {
        long int offset_pai = caminho_pais[nivel_atual - 1]; 
        
        pagina *pai = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);

        //Busca a posição 'i' correta para inserir a chave promovida no pai
        int i = 0;
        while (i < pai->num_chaves) {
            void *chave_i = (char *)pai->chaves + (i * arvore->size_chave);
            if (comparar(chave_promovida, chave_i) < 0) {
                break;
            }
            i++;
        }

        //Inserção Simples no Pai
        if (pai->num_chaves < arvore->ordem_interna - 1) {
            if (i < pai->num_chaves) {
                void *dest_chaves = (char *)pai->chaves + ((i + 1) * arvore->size_chave);
                void *src_chaves = (char *)pai->chaves + (i * arvore->size_chave);
                memmove(dest_chaves, src_chaves, (pai->num_chaves - i) * arvore->size_chave);

                long int *dest_filhos = pai->filhos + (i + 2);
                long int *src_filhos = pai->filhos + (i + 1);
                memmove(dest_filhos, src_filhos, (pai->num_chaves - i) * sizeof(long int));
            }

            //Insere a chave promovida e o ponteiro para a nova folha
            memcpy((char *)pai->chaves + (i * arvore->size_chave), chave_promovida, arvore->size_chave);
            pai->filhos[i + 1] = offset_filho_direito;

            pai->num_chaves++;
            //Escreve-se no disco as alterações;
            escrever_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);
            
            free(pai->chaves);
            free(pai->filhos);
            free(pai);

            return;
        } 
        //No caso de overflow no nó interno precisamos tratar
        else {
            long int offset_novo_pai;
            //Fazemos o split interno e pegamos a nova chave que deve subir para o avô
            void *nova_chave_promovida = split_interno(arvore, pai, offset_pai, chave_promovida, offset_filho_direito, &offset_novo_pai, comparar);
            propagar_insercao_pai(arvore, nova_chave_promovida, offset_novo_pai, caminho_pais, nivel_atual - 1, comparar);

            free(nova_chave_promovida);
            
            free(pai->chaves);
            free(pai->filhos);
            free(pai);
        }
    }
}

int inserir_arvore(ArvoreBPlus *arvore, void *chave, void *dado, int (*comparar)(const void*, const void*)) {
    //Função responsável por inserir novas chaves na arvore e tratar possíveis casos de overflow
    long int *caminho_pais = malloc(arvore->altura * sizeof(long int)); //Será usado na propagação
    int nivel_atual = 0;
    long int offset_atual = arvore->raiz_offset;

    if (offset_atual == -1) { //No caso da arvore estar vazia, realizamos a primeira inserção;
        pagina *nova_raiz = malloc(sizeof(pagina));
        nova_raiz->eh_folha = 1;
        nova_raiz->num_chaves = 1;
        nova_raiz->proxima_folha = -1;

        nova_raiz->chaves = malloc(arvore->ordem_folha * arvore->size_chave);
        nova_raiz->dados = malloc(arvore->ordem_folha * arvore->size_dado);

        // Copia a primeira chave e o primeiro dado para o início dos blocos (índice 0)
        memcpy(nova_raiz->chaves, chave, arvore->size_chave);
        memcpy(nova_raiz->dados, dado, arvore->size_dado);

        // Obtém um novo endereço no arquivo para gravar esta página
        long int novo_offset = buscar_offset_livre(arvore);
        arvore->raiz_offset = novo_offset; 
        arvore->altura = 1;
        //Escreve-se no disco as alterações;
        escrever_pagina(arvore->arquivo_binario, novo_offset, nova_raiz, arvore->size_chave, arvore->size_dado);

        cabecalhoArvore cabecalho_atualizado;
        cabecalho_atualizado.raiz_offset = arvore->raiz_offset;
        cabecalho_atualizado.topo_lista_livre = -1; 
        cabecalho_atualizado.altura = arvore->altura;

        fseek(arvore->arquivo_binario, 0, SEEK_SET);
        fwrite(&cabecalho_atualizado, sizeof(cabecalhoArvore), 1, arvore->arquivo_binario);

        free(nova_raiz->chaves); free(nova_raiz->dados); free(nova_raiz);
        return 0; 
    }

    pagina *pagina_atual = malloc(sizeof(pagina));

    while(1) {
        ler_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
        caminho_pais[nivel_atual] = offset_atual;
        nivel_atual++;
        
        if (pagina_atual->eh_folha == 1) break; // Chegou na folha

        int i = 0;
        while (i < pagina_atual->num_chaves) {
            void *chave_i = (char *)pagina_atual->chaves + (i * arvore->size_chave);
            if (comparar(chave, chave_i) < 0) break;
            i++;
        }
        offset_atual = pagina_atual->filhos[i];
        
        free(pagina_atual->chaves);
        free(pagina_atual->filhos);
    }

    //Procurar a posição correta de inserção da chave
    int i = 0;
    while (i < pagina_atual->num_chaves) {
        void *chave_i = (char *)pagina_atual->chaves + (i * arvore->size_chave);
        if (comparar(chave, chave_i) < 0) {
            break; 
        }
        i++;
    }

    //Inserção Simples
    if (pagina_atual->num_chaves < arvore->ordem_folha - 1) {
        inserir_simples(arvore, chave, dado, pagina_atual, i);
        //Escreve-se no disco as alterações;
        escrever_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
        free(pagina_atual->chaves);
        free(pagina_atual->dados);
        free(pagina_atual);
        
        return 0; 
    } 
    else { //No caso de OVERFLOW realizamos a cisão
        void *chave_promovida = split_folha(arvore, pagina_atual, offset_atual);
        long int offset_nova_folha = pagina_atual->proxima_folha; 
        nivel_atual--; 
        
        propagar_insercao_pai(arvore, chave_promovida, offset_nova_folha, caminho_pais, nivel_atual, comparar);
        free(caminho_pais);
        free(chave_promovida);
        return 1; 
    }
}


int buscar_arvore(ArvoreBPlus *arvore, void *chave, void *dado_retorno, int (*comparar)(const void*, const void*)) {
    //Função responsável por descer a arvore buscando o dado da chave procurada;
    long int offset_atual = arvore->raiz_offset;
    if (offset_atual == -1) return 0;

    pagina *pagina_atual = malloc(sizeof(pagina));
    
    // Desce até a folha
    while(1) {
        ler_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
        
        if (pagina_atual->eh_folha == 1) break; 
        int i = 0;
        while (i < pagina_atual->num_chaves) {
            void *chave_i = (char *)pagina_atual->chaves + (i * arvore->size_chave);
            if (comparar(chave, chave_i) < 0) {
                break;
            }
            i++;
        }
        offset_atual = pagina_atual->filhos[i];
        
        free(pagina_atual->filhos);
    }

    int i = 0;
    int encontrado = 0;
    while (i < pagina_atual->num_chaves) {
        void *chave_i = (char *)pagina_atual->chaves + (i * arvore->size_chave);
        int cmp = comparar(chave, chave_i);
        
        if (cmp == 0) {
            void *dado_i = (char *)pagina_atual->dados + (i * arvore->size_dado);
            memcpy(dado_retorno, dado_i, arvore->size_dado);
            encontrado = 1;
            break;
        } else if (cmp < 0) {
            break; 
        }
        i++;
    }

    free(pagina_atual->chaves);
    free(pagina_atual->dados);
    free(pagina_atual);

    return encontrado;
}

void ler_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado) {
    //Função responsável por se ler no disco os dados já escritos e transferi-los para a memória RAM em blocos de 4096bytes;
    char buffer;
    int posicao_atual = 0;

    fseek(arquivo, offset, SEEK_SET);
    fread(buffer, 4096, 1, arquivo);
    //Se copia os dados do aquivo para a a memória
    memcpy(&(pagina->eh_folha), buffer + posicao_atual, sizeof(int));
    posicao_atual += sizeof(int);
    
    memcpy(&(pagina->num_chaves), buffer + posicao_atual, sizeof(int));
    posicao_atual += sizeof(int);

    pagina->chaves = malloc(pagina->num_chaves * size_chave);
    memcpy(pagina->chaves, buffer + posicao_atual, pagina->num_chaves * size_chave);
    posicao_atual += (pagina->num_chaves * size_chave);

    if (pagina->eh_folha == 1) {
        pagina->dados = malloc(pagina->num_chaves * size_dado);
        memcpy(pagina->dados, buffer + posicao_atual, pagina->num_chaves * size_dado);
        posicao_atual += (pagina->num_chaves * size_dado);
        memcpy(&(pagina->proxima_folha), buffer + posicao_atual, sizeof(long int));
    } else {
        pagina->filhos = malloc((pagina->num_chaves + 1) * sizeof(long int));
        memcpy(pagina->filhos, buffer + posicao_atual, (pagina->num_chaves + 1) * sizeof(long int));
    }
}

void escrever_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado) {
    //Função responsável por escrever no disco as alterações realizadas em blocos de 4096bytes, além de armazenar os dados de forma física para não se perderem.
    char buffer = {0}; 
    int posicao_atual = 0;
    //Se copia os dados da memória para o arquivo
    memcpy(buffer + posicao_atual, &(pagina->eh_folha), sizeof(int));
    posicao_atual += sizeof(int);

    memcpy(buffer + posicao_atual, &(pagina->num_chaves), sizeof(int));
    posicao_atual += sizeof(int);

    memcpy(buffer + posicao_atual, pagina->chaves, pagina->num_chaves * size_chave);
    posicao_atual += (pagina->num_chaves * size_chave);

    if (pagina->eh_folha == 1) {
        memcpy(buffer + posicao_atual, pagina->dados, pagina->num_chaves * size_dado);
        posicao_atual += (pagina->num_chaves * size_dado);
        
        memcpy(buffer + posicao_atual, &(pagina->proxima_folha), sizeof(long int));
    } else {
        memcpy(buffer + posicao_atual, pagina->filhos, (pagina->num_chaves + 1) * sizeof(long int));
    }

    fseek(arquivo, offset, SEEK_SET);
    fwrite(buffer, 4096, 1, arquivo);
}