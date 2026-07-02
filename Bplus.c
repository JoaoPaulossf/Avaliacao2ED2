#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int (*compara)(const void*, const void*);
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

void salvarCabecalho(ArvoreBPlus *arvore) {
    fseek(arvore->arquivoBinario, 0, SEEK_SET);
    fwrite(&arvore->cab, sizeof(cabecalho), 1, arvore->arquivoBinario);
    fflush(arvore->arquivoBinario);
}

int buscar(ArvoreBPlus *arvore, void *chaveBusca, void *dadoRetorno) {
    if (arvore->cab.raiz == -1) return 0; // Árvore vazia

    offset atual = arvore->cab.raiz;
    pagina p;

    while (1) {
        lerPagina(arvore->arquivoBinario, atual, &p);

        int i = 0;
        while (i < p.numChaves && arvore->compara(chaveBusca, p.chaves[i]) > 0) {
            i++;
        }

        if (p.ehFolha) {
            if (i < p.numChaves && arvore->compara(chaveBusca, p.chaves[i]) == 0) {
                if (dadoRetorno != NULL) {
                    // Retorna o dado genérico copiando os bytes exatos
                    memcpy(dadoRetorno, p.dados[i], arvore->cab.tamanhoDado);
                }
                return 1; 
            }
            return 0; // Chave não encontrada
        } else {
            if (i < p.numChaves && arvore->compara(chaveBusca, p.chaves[i]) == 0) {
                atual = p.filhos[i + 1];
            } else {
                atual = p.filhos[i];
            }
        }
    }
}

int remover(ArvoreBPlus *arvore, void *chave) {
    if (arvore->cab.raiz == -1) return 0;

    offset atual = arvore->cab.raiz;
    pagina p;

    while (1) {
        lerPagina(arvore->arquivoBinario, atual, &p);

        int i = 0;
        while (i < p.numChaves && arvore->compara(chave, p.chaves[i]) > 0) {
            i++;
        }

        if (p.ehFolha) {
            if (i < p.numChaves && arvore->compara(chave, p.chaves[i]) == 0) {
                // Removendo genéricos
                for (int j = i; j < p.numChaves - 1; j++) {
                    memcpy(p.chaves[j], p.chaves[j+1], arvore->cab.tamanhoChave);
                    memcpy(p.dados[j], p.dados[j+1], arvore->cab.tamanhoDado);
                }
                p.numChaves--;
                escreverPagina(arvore->arquivoBinario, atual, &p);
                return 1; 
            }
            return 0; 
        } else {
            if (i < p.numChaves && arvore->compara(chave, p.chaves[i]) == 0) {
                atual = p.filhos[i + 1];
            } else {
                atual = p.filhos[i];
            }
        }
    }
}

//Retorna 1 se ocorrer split
int inserirAux(ArvoreBPlus *arvore, offset atualOff, void *chave, void *dado, void *chavePromovida, offset *filhoPromovido) {
    pagina p;
    lerPagina(arvore->arquivoBinario, atualOff, &p);

    if (p.ehFolha) {
        int i = 0;
        while (i < p.numChaves && arvore->compara(chave, p.chaves[i]) > 0) i++;
        if (i < p.numChaves && arvore->compara(chave, p.chaves[i]) == 0) return 0; // Chave duplicada

        // Deslocamento de memória de blocos genéricos
        for (int j = p.numChaves; j > i; j--) {
            memcpy(p.chaves[j], p.chaves[j-1], arvore->cab.tamanhoChave);
            memcpy(p.dados[j], p.dados[j-1], arvore->cab.tamanhoDado);
        }
        memcpy(p.chaves[i], chave, arvore->cab.tamanhoChave);
        memcpy(p.dados[i], dado, arvore->cab.tamanhoDado);
        p.numChaves++;

        // Checa ordem da folha
        if (p.numChaves <= arvore->cab.ordemFolha) {
            escreverPagina(arvore->arquivoBinario, atualOff, &p);
            return 0;
        } else {
            // Split na folha
            pagina nova = criarPagina(1);
            int splitIndex = p.numChaves / 2;
            nova.numChaves = p.numChaves - splitIndex;
            p.numChaves = splitIndex;

            for (int j = 0; j < nova.numChaves; j++) {
                memcpy(nova.chaves[j], p.chaves[splitIndex + j], arvore->cab.tamanhoChave);
                memcpy(nova.dados[j], p.dados[splitIndex + j], arvore->cab.tamanhoDado);
            }
            
            nova.proximaFolha = p.proximaFolha;
            offset novaOff = obterNovaPagina(arvore, 1);
            p.proximaFolha = novaOff;

            // Sobe a menor chave do nó direito
            memcpy(chavePromovida, nova.chaves[0], arvore->cab.tamanhoChave);
            *filhoPromovido = novaOff;

            escreverPagina(arvore->arquivoBinario, atualOff, &p);
            escreverPagina(arvore->arquivoBinario, novaOff, &nova);
            return 1; 
        }
    //não é folha
    } else {
        int i = 0;
        while (i < p.numChaves && arvore->compara(chave, p.chaves[i]) >= 0) i++;

        void *chaveSubPromovida = malloc(arvore->cab.tamanhoChave);
        offset filhoSubPromovido = -1;

        int promoveu = inserirAux(arvore, p.filhos[i], chave, dado, chaveSubPromovida, &filhoSubPromovido);

        if (promoveu) {
            int pos = 0;
            while (pos < p.numChaves && arvore->compara(chaveSubPromovida, p.chaves[pos]) > 0) pos++;

            for (int j = p.numChaves; j > pos; j--) {
                memcpy(p.chaves[j], p.chaves[j-1], arvore->cab.tamanhoChave);
                p.filhos[j+1] = p.filhos[j];
            }
            memcpy(p.chaves[pos], chaveSubPromovida, arvore->cab.tamanhoChave);
            p.filhos[pos+1] = filhoSubPromovido;
            p.numChaves++;

            if (p.numChaves <= arvore->cab.ordemInterna) {
                escreverPagina(arvore->arquivoBinario, atualOff, &p);
                free(chaveSubPromovida);
                return 0;
            } else {
                // Split de Nó Interno
                pagina nova = criarPagina(0);
                int splitIndex = p.numChaves / 2;
                
                // Em nós internos, a chave do meio sobe e não fica no nó direito
                memcpy(chavePromovida, p.chaves[splitIndex], arvore->cab.tamanhoChave);
                
                nova.numChaves = p.numChaves - splitIndex - 1;
                p.numChaves = splitIndex;

                for (int j = 0; j < nova.numChaves; j++) {
                    memcpy(nova.chaves[j], p.chaves[splitIndex + 1 + j], arvore->cab.tamanhoChave);
                    nova.filhos[j] = p.filhos[splitIndex + 1 + j];
                }
                nova.filhos[nova.numChaves] = p.filhos[splitIndex + 1 + nova.numChaves];
                
                offset novaOff = obterNovaPagina(arvore, 0);
                *filhoPromovido = novaOff;
                
                escreverPagina(arvore->arquivoBinario, atualOff, &p);
                escreverPagina(arvore->arquivoBinario, novaOff, &nova);
                free(chaveSubPromovida);
                return 1;
            }
        }
        free(chaveSubPromovida);
        return 0;
    }
}

void inserir(ArvoreBPlus *arvore, void *chave, void *dado) {
    if (arvore->cab.raiz == -1) {
        offset raizOff = obterNovaPagina(arvore, 1);
        pagina p = criarPagina(1);
        memcpy(p.chaves[0], chave, arvore->cab.tamanhoChave);
        memcpy(p.dados[0], dado, arvore->cab.tamanhoDado);
        p.numChaves = 1;
        escreverPagina(arvore->arquivoBinario, raizOff, &p);
        
        arvore->cab.raiz = raizOff;
        salvarCabecalho(arvore);
        return;
    }
    
    void *chavePromovida = malloc(arvore->cab.tamanhoChave);
    offset filhoPromovido = -1;
    
    int promoveu = inserirAux(arvore, arvore->cab.raiz, chave, dado, chavePromovida, &filhoPromovido);
    
    // Tratamento de split na raiz da árvore
    if (promoveu) {
        offset novaRaizOff = obterNovaPagina(arvore, 0);
        pagina novaRaiz = criarPagina(0);
        novaRaiz.numChaves = 1;
        memcpy(novaRaiz.chaves[0], chavePromovida, arvore->cab.tamanhoChave);
        novaRaiz.filhos[0] = arvore->cab.raiz;
        novaRaiz.filhos[1] = filhoPromovido;
        escreverPagina(arvore->arquivoBinario, novaRaizOff, &novaRaiz);
        
        arvore->cab.raiz = novaRaizOff;
        salvarCabecalho(arvore);
    }
    free(chavePromovida);
}