#include <stdio.h>
#include <stdlib.h>

#include <Bplus.h>

//Estrutura interna para gravar no início do ficheiro;
typedef struct{
    long int raiz_offset;
    long int topo_lista_livre;
    int altura;
} cabecalhoArvore;

void liberar_offset(ArvoreBPlus *arvore, long int offset_liberado) {
    //Funcao responsavel por liberar um offset e evitar que o arquivo cresca desnecessariamente
    cabecalhoArvore cabecalho;
    
    //Lê o cabeçalho atual para descobrir quem é o topo da lista livre
    fseek(arvore->arquivo_binario, 0, SEEK_SET);
    fread(&cabecalho, sizeof(cabecalhoArvore), 1, arvore->arquivo_binario);

    //Grava o endereço do antigo topo nos primeiros bytes da página que estamos liberando
    fseek(arvore->arquivo_binario, offset_liberado, SEEK_SET);
    fwrite(&(cabecalho.topo_lista_livre), sizeof(long int), 1, arvore->arquivo_binario);

    //Atualiza o cabeçalho para apontar para esta página recém-liberada
    cabecalho.topo_lista_livre = offset_liberado;
    fseek(arvore->arquivo_binario, 0, SEEK_SET);
    fwrite(&cabecalho, sizeof(cabecalhoArvore), 1, arvore->arquivo_binario);
}

long int buscar_offset_livre(ArvoreBPlus *arvore) {
    //Funcao responsável por buscar um offset livre durante a criação de uma nova pagina
    long int offset_retorno;
    cabecalhoArvore cabecalho;
    
    //Lê o cabeçalho atual para descobrir quem é o topo da lista livre
    fseek(arvore->arquivo_binario, 0, SEEK_SET);
    fread(&cabecalho, sizeof(cabecalho), 1, arvore->arquivo_binario);
    
    if (cabecalho.topo_lista_livre != -1) {
        //No caso de nao ter um livre se aloca um novo
        offset_retorno = cabecalho.topo_lista_livre;
        
        pagina *p = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_retorno, p, arvore->size_chave, arvore->size_dado);
        
        cabecalho.topo_lista_livre = p->proxima_folha; 
        free(p);
    } else {
        //No caso de ter, determina quem é
        fseek(arvore->arquivo_binario, 0, SEEK_END);
        offset_retorno = ftell(arvore->arquivo_binario);
    }
    //Atualiza o cabeçalho e retorna
    fseek(arvore->arquivo_binario, 0, SEEK_SET);
    fwrite(&cabecalho, sizeof(cabecalho), 1, arvore->arquivo_binario);
    
    return offset_retorno;
}

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

void* split_interno(ArvoreBPlus *arvore, pagina *pai_cheio, long int offset_cheio, void *chave_inserir, long int offset_filho_direito, long int *offset_novo_pai, int (*comparar)(void*, void*)) {
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

void propagar_insercao_pai(ArvoreBPlus *arvore, void *chave_promovida, long int offset_filho_direito, long int *caminho_pais, int nivel_atual, int (*comparar)(void*, void*)) {
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

int inserir_arvore(ArvoreBPlus *arvore, void *chave, void *dado, int (*comparar)(void*, void*)) {
    //Função responsável por inserir novas chaves na arvore e tratar possíveis casos de overflow 
    long int *caminho_pais = malloc((arvore->altura + 1) * sizeof(long int)); //Será usado na propagação
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


int buscar_arvore(ArvoreBPlus *arvore, void *chave, void *dado_retorno, int (*comparar)(void*, void*)) {
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

void underflow_interno(ArvoreBPlus *arvore, pagina *interno, long int offset_interno, long int *caminho_pais, int nivel_atual) {
    //Funcao responsavel por tratar casos de um underflow interno;
    if (nivel_atual == 0) return;

    //Carrega-se o pai para descobrir os irmãos
    long int offset_pai = caminho_pais[nivel_atual - 1];
    pagina *pai = malloc(sizeof(pagina));
    ler_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);

    //Descobre a posição do nosso nó interno
    int pos_pai = 0;
    while (pos_pai <= pai->num_chaves && pai->filhos[pos_pai] != offset_interno) {
        pos_pai++;
    }

    int minimo_chaves = arvore->ordem_interna / 2;

    //Primeiramente tentamos a redistribuição a esquerda
    if (pos_pai > 0) {
        long int offset_esq = pai->filhos[pos_pai - 1];
        pagina *irmao_esq = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_esq, irmao_esq, arvore->size_chave, arvore->size_dado);

        if (irmao_esq->num_chaves > minimo_chaves) {
            //Reorganiza a pagina interna
            memmove((char*)interno->chaves + arvore->size_chave, interno->chaves, interno->num_chaves * arvore->size_chave);
            memmove(interno->filhos + 1, interno->filhos, (interno->num_chaves + 1) * sizeof(long int));

            //A chave do pai desce para a nossa pagina
            memcpy(interno->chaves, (char*)pai->chaves + ((pos_pai - 1) * arvore->size_chave), arvore->size_chave);
            
            //O último filho do irmão esquerdo passa a ser o primeiro filho
            interno->filhos = irmao_esq->filhos[irmao_esq->num_chaves];

            //A última chave do irmão esquerdo soba para substituir a do pai
            memcpy((char*)pai->chaves + ((pos_pai - 1) * arvore->size_chave), (char*)irmao_esq->chaves + ((irmao_esq->num_chaves - 1) * arvore->size_chave), arvore->size_chave);

            irmao_esq->num_chaves--;
            interno->num_chaves++;

            //Escreve-se no disco as alterações
            escrever_pagina(arvore->arquivo_binario, offset_esq, irmao_esq, arvore->size_chave, arvore->size_dado);
            escrever_pagina(arvore->arquivo_binario, offset_interno, interno, arvore->size_chave, arvore->size_dado);
            escrever_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);

            free(irmao_esq->chaves); free(irmao_esq->filhos); free(irmao_esq);
            free(pai->chaves); free(pai->filhos); free(pai);
            return;
        }
        free(irmao_esq->chaves); free(irmao_esq->filhos); free(irmao_esq);
    }

    //Tentamos enfima  redistribuição a direita
    if (pos_pai < pai->num_chaves) {
        long int offset_dir = pai->filhos[pos_pai + 1];
        pagina *irmao_dir = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_dir, irmao_dir, arvore->size_chave, arvore->size_dado);

        if (irmao_dir->num_chaves > minimo_chaves) {
            //A chave do pai desce para ser a última da pagina
            memcpy((char*)interno->chaves + (interno->num_chaves * arvore->size_chave), (char*)pai->chaves + (pos_pai * arvore->size_chave), arvore->size_chave);
            
            //O primeiro filho do irmão direito passa a ser o último filho
            interno->filhos[interno->num_chaves + 1] = irmao_dir->filhos;

            //A primeira chave do irmão direito sobe para substituir a do pai
            memcpy((char*)pai->chaves + (pos_pai * arvore->size_chave), irmao_dir->chaves, arvore->size_chave);

            //Reorganiza a pagina
            memmove(irmao_dir->chaves, (char*)irmao_dir->chaves + arvore->size_chave, (irmao_dir->num_chaves - 1) * arvore->size_chave);
            memmove(irmao_dir->filhos, irmao_dir->filhos + 1, irmao_dir->num_chaves * sizeof(long int));

            irmao_dir->num_chaves--;
            interno->num_chaves++;

            //Escreve-se as alteraçẽos no disco
            escrever_pagina(arvore->arquivo_binario, offset_dir, irmao_dir, arvore->size_chave, arvore->size_dado);
            escrever_pagina(arvore->arquivo_binario, offset_interno, interno, arvore->size_chave, arvore->size_dado);
            escrever_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);

            free(irmao_dir->chaves); free(irmao_dir->filhos); free(irmao_dir);
            free(pai->chaves); free(pai->filhos); free(pai);
            return;
        }
        free(irmao_dir->chaves); free(irmao_dir->filhos); free(irmao_dir);
    }

    //Iniciasse a concatenação no caso de nenhuma pagina vizinha puder redistribuir
    if (pos_pai > 0) {
        //Irmão esquerdo absorve a pagina
        long int offset_esq = pai->filhos[pos_pai - 1];
        pagina *irmao_esq = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_esq, irmao_esq, arvore->size_chave, arvore->size_dado);

        //Reorganiza inserindo a chave do pai
        memcpy((char*)irmao_esq->chaves + (irmao_esq->num_chaves * arvore->size_chave), (char*)pai->chaves + ((pos_pai - 1) * arvore->size_chave), arvore->size_chave);
        irmao_esq->num_chaves++;

        //Copiamos chaves e filhos da pagina para o final do irmao esquerdo
        memcpy((char*)irmao_esq->chaves + (irmao_esq->num_chaves * arvore->size_chave), interno->chaves, interno->num_chaves * arvore->size_chave);
        memcpy(irmao_esq->filhos + irmao_esq->num_chaves, interno->filhos, (interno->num_chaves + 1) * sizeof(long int));
        irmao_esq->num_chaves += interno->num_chaves;

        escrever_pagina(arvore->arquivo_binario, offset_esq, irmao_esq, arvore->size_chave, arvore->size_dado);

        remover_entrada_pai(arvore, pai, offset_pai, pos_pai - 1, pos_pai, caminho_pais, nivel_atual);
        liberar_offset(arvore, offset_interno);

        free(irmao_esq->chaves); free(irmao_esq->filhos); free(irmao_esq);

    } else {
        //Pagina absorve o irmao direito
        long int offset_dir = pai->filhos[pos_pai + 1];
        pagina *irmao_dir = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_dir, irmao_dir, arvore->size_chave, arvore->size_dado);

        //Reorganizamos a pagina com a chave do pai
        memcpy((char*)interno->chaves + (interno->num_chaves * arvore->size_chave), (char*)pai->chaves + (pos_pai * arvore->size_chave), arvore->size_chave);
        interno->num_chaves++;

        //Copiamos chaves e filhos do irmao direito para o final da pagina
        memcpy((char*)interno->chaves + (interno->num_chaves * arvore->size_chave), irmao_dir->chaves, irmao_dir->num_chaves * arvore->size_chave);
        memcpy(interno->filhos + interno->num_chaves, irmao_dir->filhos, (irmao_dir->num_chaves + 1) * sizeof(long int));
        interno->num_chaves += irmao_dir->num_chaves;

        escrever_pagina(arvore->arquivo_binario, offset_interno, interno, arvore->size_chave, arvore->size_dado);

        remover_entrada_pai(arvore, pai, offset_pai, pos_pai, pos_pai + 1, caminho_pais, nivel_atual);
        liberar_offset(arvore, offset_dir);

        free(irmao_dir->chaves); free(irmao_dir->filhos); free(irmao_dir);
    }

    free(pai->chaves); free(pai->filhos); free(pai);
}

void propagar_remocao_pai(ArvoreBPlus *arvore, pagina *pai, long int offset_pai, int pos_chave, int pos_filho, long int *caminho_pais, int nivel_atual) {
    //Funcao responsável por propagar os resultados da remocao aos níveis superiores da arvore
    //Desloca as chaves para a esquerda
    if (pos_chave < pai->num_chaves - 1) {
        void *dest_chaves = (char*)pai->chaves + (pos_chave * arvore->size_chave);
        void *src_chaves = (char*)pai->chaves + ((pos_chave + 1) * arvore->size_chave);
        memmove(dest_chaves, src_chaves, (pai->num_chaves - pos_chave - 1) * arvore->size_chave);
    }

    //Desloca os filhos para a esquerda
    if (pos_filho < pai->num_chaves) {
        long int *dest_filhos = pai->filhos + pos_filho;
        long int *src_filhos = pai->filhos + (pos_filho + 1);
        memmove(dest_filhos, src_filhos, (pai->num_chaves - pos_filho) * sizeof(long int));
    }

    pai->num_chaves--;

    //Tratamento no caso da raiz, afetando a altura
    if (offset_pai == arvore->raiz_offset) {
        if (pai->num_chaves == 0) {
            arvore->raiz_offset = pai->filhos;
            arvore->altura--; 

            //Atualiza o cabeçalho
            cabecalhoArvore cabecalho;
            fseek(arvore->arquivo_binario, 0, SEEK_SET);
            fread(&cabecalho, sizeof(cabecalhoArvore), 1, arvore->arquivo_binario);
            cabecalho.raiz_offset = arvore->raiz_offset;
            cabecalho.altura = arvore->altura;
            fseek(arvore->arquivo_binario, 0, SEEK_SET);
            fwrite(&cabecalho, sizeof(cabecalhoArvore), 1, arvore->arquivo_binario);
            liberar_offset(arvore, offset_pai);
        } else {
            //A raiz apenas perdeu uma chave, mas continua válida.
            escrever_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);
        }
        return; 
    }
    escrever_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);
    
    //Verificacao de um overflow interno;
    int minimo_chaves_interno = arvore->ordem_interna / 2;
    if (pai->num_chaves < minimo_chaves_interno) {
        underflow_interno(arvore, pai, offset_pai, caminho_pais, nivel_atual - 1);
    }
}

void concatenar_folhas(ArvoreBPlus *arvore, pagina *folha_esq, long int offset_esq, pagina *folha_dir, long int offset_dir, pagina *pai, long int offset_pai, int pos_chave_pai, int pos_filho_dir, long int *caminho_pais, int nivel_atual) {
    //Funcao responsável pela concatenação de páginas, ocorre durante o underflow da remocao e nenhuma folha vizinha pode redistribuir
    
    //A página da esquerda absorve todas as chaves e dados da página da direita
    memcpy((char*)folha_esq->chaves + (folha_esq->num_chaves * arvore->size_chave), folha_dir->chaves, folha_dir->num_chaves * arvore->size_chave);
    memcpy((char*)folha_esq->dados + (folha_esq->num_chaves * arvore->size_dado), folha_dir->dados, folha_dir->num_chaves * arvore->size_dado);

    folha_esq->num_chaves += folha_dir->num_chaves;
    
    //Refaz a lista encadeada (O irmão esquerdo pula a folha absorvida e aponta para o próximo)
    folha_esq->proxima_folha = folha_dir->proxima_folha; 

    //Grava a folha esquerda atualizada no disco
    escrever_pagina(arvore->arquivo_binario, offset_esq, folha_esq, arvore->size_chave, arvore->size_dado);

    //Remove a "placa de sinalização" da página absorvida no nó pai
    propagar_remocao_pai(arvore, pai, offset_pai, pos_chave_pai, pos_filho_dir, caminho_pais, nivel_atual);

    //Libera o offset para evitar uma fragmentacao do arquivo;
    liberar_offset(arvore, offset_dir);
}

void underflow_folha(ArvoreBPlus *arvore, pagina *folha, long int offset_folha, long int *caminho_pais, int nivel_atual) {
    //Funcao responsael por tratar o caso de underflow durante a remocao
    long int offset_pai = caminho_pais[nivel_atual - 1];
    pagina *pai = malloc(sizeof(pagina));
    ler_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);

    //Encontra a posição da nossa folha no vetor de filhos do pai
    int pos_pai = 0;
    while (pos_pai <= pai->num_chaves && pai->filhos[pos_pai] != offset_folha) {
        pos_pai++;
    }

    int minimo_chaves = arvore->ordem_folha / 2;

    //Primeiro tentamos realizar uma redistribuição, pela esquerda
    if (pos_pai > 0) {
        long int offset_esq = pai->filhos[pos_pai - 1];
        pagina *irmao_esq = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_esq, irmao_esq, arvore->size_chave, arvore->size_dado);

        if (irmao_esq->num_chaves > minimo_chaves) {
            //Abre espaço na nossa folha empurrando tudo para a direita
            memmove((char*)folha->chaves + arvore->size_chave, folha->chaves, folha->num_chaves * arvore->size_chave);
            memmove((char*)folha->dados + arvore->size_dado, folha->dados, folha->num_chaves * arvore->size_dado);

            //Copia a ultima chave e dado do irmão esquerdo para a primeira posição da nossa folha
            int ultimo_esq = irmao_esq->num_chaves - 1;
            memcpy(folha->chaves, (char*)irmao_esq->chaves + (ultimo_esq * arvore->size_chave), arvore->size_chave);
            memcpy(folha->dados, (char*)irmao_esq->dados + (ultimo_esq * arvore->size_dado), arvore->size_dado);

            irmao_esq->num_chaves--;
            folha->num_chaves++;

            memcpy((char*)pai->chaves + ((pos_pai - 1) * arvore->size_chave), folha->chaves, arvore->size_chave);

            //Escreve no disco as alterações
            escrever_pagina(arvore->arquivo_binario, offset_esq, irmao_esq, arvore->size_chave, arvore->size_dado);
            escrever_pagina(arvore->arquivo_binario, offset_folha, folha, arvore->size_chave, arvore->size_dado);
            escrever_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);

            free(irmao_esq->chaves); free(irmao_esq->dados); free(irmao_esq);
            free(pai->chaves); free(pai->filhos); free(pai);
            return;
        }
        free(irmao_esq->chaves); free(irmao_esq->dados); free(irmao_esq);
    }

    //Então tentamos uma redistribuição pela esquerda
    if (pos_pai < pai->num_chaves) {
        long int offset_dir = pai->filhos[pos_pai + 1];
        pagina *irmao_dir = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_dir, irmao_dir, arvore->size_chave, arvore->size_dado);

        if (irmao_dir->num_chaves > minimo_chaves) {
            //Copia a primeira chave e dado do irmão direito para o final da nossa folha
            memcpy((char*)folha->chaves + (folha->num_chaves * arvore->size_chave), irmao_dir->chaves, arvore->size_chave);
            memcpy((char*)folha->dados + (folha->num_chaves * arvore->size_dado), irmao_dir->dados, arvore->size_dado);

            //Reorganiza as chaves
            memmove(irmao_dir->chaves, (char*)irmao_dir->chaves + arvore->size_chave, (irmao_dir->num_chaves - 1) * arvore->size_chave);
            memmove(irmao_dir->dados, (char*)irmao_dir->dados + arvore->size_dado, (irmao_dir->num_chaves - 1) * arvore->size_dado);

            irmao_dir->num_chaves--;
            folha->num_chaves++;

            memcpy((char*)pai->chaves + (pos_pai * arvore->size_chave), irmao_dir->chaves, arvore->size_chave);

            //Escreve no disco as alterações
            escrever_pagina(arvore->arquivo_binario, offset_dir, irmao_dir, arvore->size_chave, arvore->size_dado);
            escrever_pagina(arvore->arquivo_binario, offset_folha, folha, arvore->size_chave, arvore->size_dado);
            escrever_pagina(arvore->arquivo_binario, offset_pai, pai, arvore->size_chave, arvore->size_dado);

            free(irmao_dir->chaves); free(irmao_dir->dados); free(irmao_dir);
            free(pai->chaves); free(pai->filhos); free(pai);
            return;
        }
        free(irmao_dir->chaves); free(irmao_dir->dados); free(irmao_dir);
    }

    //Neste ponto a redistribuição não foi possível então precisaemos concatenar paginas
    if (pos_pai > 0) {
        //Irmão da esquerda absorve a nossa folha
        long int offset_esq = pai->filhos[pos_pai - 1];
        pagina *irmao_esq = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_esq, irmao_esq, arvore->size_chave, arvore->size_dado);

        concatenar_folhas(arvore, irmao_esq, offset_esq, folha, offset_folha, pai, offset_pai, pos_pai - 1, pos_pai, caminho_pais, nivel_atual);

        free(irmao_esq->chaves); free(irmao_esq->dados); free(irmao_esq);
        
    } else {
        //A folha absorve o irmão da direita
        long int offset_dir = pai->filhos[pos_pai + 1];
        pagina *irmao_dir = malloc(sizeof(pagina));
        ler_pagina(arvore->arquivo_binario, offset_dir, irmao_dir, arvore->size_chave, arvore->size_dado);

        concatenar_folhas(arvore, folha, offset_folha, irmao_dir, offset_dir, pai, offset_pai, pos_pai, pos_pai + 1, caminho_pais, nivel_atual);

        free(irmao_dir->chaves); free(irmao_dir->dados); free(irmao_dir);
    }
    
    free(pai->chaves); free(pai->filhos); free(pai);
}

int remover_arvore(ArvoreBPlus *arvore, void *chave, int (*comparar)(void*, void*)) {
    //Funcao responsável por procurar e remover uma chave de uma pagina, além de tratar casos de underflow
    long int offset_atual = arvore->raiz_offset;
    if (offset_atual == -1) return 0; //Árvore vazia, nada a remover

    //Alocamos um vetor para marcar o caminho até a chave no caso de underflow
    long int *caminho_pais = malloc((arvore->altura + 1) * sizeof(long int));
    int nivel_atual = 0;
    
    pagina *pagina_atual = malloc(sizeof(pagina));
    
    //Descida Rastreada até a folha
    while(1) {
        ler_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
        caminho_pais[nivel_atual] = offset_atual;
        nivel_atual++;
        
        if (pagina_atual->eh_folha == 1) break;

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

    //Busca a chave na folha
    int i = 0;
    int encontrou = 0;
    while (i < pagina_atual->num_chaves) {
        void *chave_i = (char *)pagina_atual->chaves + (i * arvore->size_chave);
        int cmp = comparar(chave, chave_i);
        
        if (cmp == 0) {
            encontrou = 1;
            break;
        } else if (cmp < 0) {
            break;
        }
        i++;
    }

    if (encontrou) {
        // Se a chave não for a última, puxamos as chaves da direita para a esquerda
        if (i < pagina_atual->num_chaves - 1) {
            void *dest_chaves = (char *)pagina_atual->chaves + (i * arvore->size_chave);
            void *src_chaves = (char *)pagina_atual->chaves + ((i + 1) * arvore->size_chave);
            memmove(dest_chaves, src_chaves, (pagina_atual->num_chaves - i - 1) * arvore->size_chave);

            void *dest_dados = (char *)pagina_atual->dados + (i * arvore->size_dado);
            void *src_dados = (char *)pagina_atual->dados + ((i + 1) * arvore->size_dado);
            memmove(dest_dados, src_dados, (pagina_atual->num_chaves - i - 1) * arvore->size_dado);
        }

        pagina_atual->num_chaves--; // Reduzimos o contador

        //Verificação de undeflow
        int minimo_chaves = arvore->ordem_folha / 2;
        
        if (pagina_atual->num_chaves >= minimo_chaves || offset_atual == arvore->raiz_offset) {
            escrever_pagina(arvore->arquivo_binario, offset_atual, pagina_atual, arvore->size_chave, arvore->size_dado);
        } else {
            underflow_folha(arvore, pagina_atual, offset_atual, caminho_pais, nivel_atual);
        }
    }

    // Limpeza de memória
    free(pagina_atual->chaves);
    free(pagina_atual->dados);
    free(pagina_atual);
    free(caminho_pais);

    return encontrou; // 1 se excluiu, 0 se a chave não existia
}

void ler_pagina(FILE *arquivo, long int offset, pagina *pagina, int size_chave, int size_dado) {
    //Função responsável por se ler no disco os dados já escritos e transferi-los para a memória RAM em blocos de 4096bytes;
    char buffer[PAGE_SIZE] = {0};
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
    char buffer[PAGE_SIZE] = {0};
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