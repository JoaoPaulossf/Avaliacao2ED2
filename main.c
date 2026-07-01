#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Bplus.h"

//Bloco de estruturas utilizadas para os dados
typedef struct { //Estrutura de Data
    int dia;
    int mes;
    int ano;
} Data;

typedef struct { //Estrutura composta nome + Data
    char nome[5];
    Data data_nascimento;
} ChaveFuncionario;

typedef struct { //Estrutura para o contracheque
    float valor;
    int mes_referencia;
    int ano_referencia;
} Pagamento;

//Estrutura completa que será o dado principal
typedef struct {
    char nome[100];
    Data data_nascimento;
    char nome_mae[100];
    char nome_pai[100];
    char endereco[100];
    char telefone[11];
    Data data_contratacao;
    int ativo; //1 para ativo e 0 para inativo
    Data data_desligamento;
    Pagamento historico_pagamentos[12];
    int qtd_pagamentos;
} Funcionario;

//Bloco de funções callback utilizadas no Bplus.h
int tamanho_chave_rh() {
    return sizeof(ChaveFuncionario);
}

int tamanho_dado_rh() {
    return sizeof(Funcionario);
}

int comparar_chaves_rh(const void *c1, const void *c2) {
    ChaveFuncionario *chave1 = (ChaveFuncionario *)c1;
    ChaveFuncionario *chave2 = (ChaveFuncionario *)c2;
    int cmp_nome = strcmp(chave1->nome, chave2->nome);

    //Primeiro verificamos por ordem alfabetica
    if (cmp_nome != 0) {
        return cmp_nome; 
    }

    //No caso de homonimo comparamos pela data
    if (chave1->data_nascimento.ano != chave2->data_nascimento.ano) {
        return chave1->data_nascimento.ano - chave2->data_nascimento.ano;
    }
    
    if (chave1->data_nascimento.mes != chave2->data_nascimento.mes) {
        return chave1->data_nascimento.mes - chave2->data_nascimento.mes;
    }
    
    if (chave1->data_nascimento.dia != chave2->data_nascimento.dia) {
        return chave1->data_nascimento.dia - chave2->data_nascimento.dia;
    }

    //Se chegou aqui, tudo é absolutamente igual
    return 0; 
}

// Limpa o buffer do teclado e remove o \n do final da string lida
void ler_string(char *destino, int tamanho) {
    fgets(destino, tamanho, stdin);
    destino[strcspn(destino, "\n")] = '\0';
}

void imprimir_ficha_funcionario(Funcionario *func) {
    printf("\n--- FICHA DO FUNCIONARIO ---\n");
    printf("Nome: %s\n", func->nome);
    printf("Nascimento: %02d/%02d/%04d\n", func->data_nascimento.dia, func->data_nascimento.mes, func->data_nascimento.ano);
    printf("Filiacao: %s (Mae) e %s (Pai)\n", func->nome_mae, func->nome_pai);
    printf("Contato: %s | Tel: %s\n", func->endereco, func->telefone);
    printf("Status: %s\n", func->ativo ? "Ativo" : "Inativo");
    printf("Pagamentos Registrados: %d\n", func->qtd_pagamentos);
    printf("----------------------------\n");
}

int main() {
    ArvoreBPlus *arvore_rh = criar_arvore("banco_rh.bin", tamanho_chave_rh, tamanho_dado_rh, comparar_chaves_rh);

    int opcao = 0;

    while (opcao != 6) {
        printf("\n========================================\n");
        printf("      SISTEMA DE GESTAO DE RH\n");
        printf("========================================\n");
        printf("1. Inserir Funcionario\n");
        printf("2. Buscar Funcionario\n");
        printf("3. Excluir Funcionario\n");
        printf("4. Listagem por Intervalo\n");
        printf("5. Exibir Estrutura do Indice\n");
        printf("6. Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &opcao);
        getchar(); 

        switch (opcao) {
            case 1: { 
                printf("\n--- INSERIR NOVO FUNCIONARIO ---\n");
                ChaveFuncionario chave_busca;
                Funcionario func_novo;

                printf("Digite o Nome do funcionario: ");
                ler_string(chave_busca.nome, 50);

                printf("Digite a Data de Nascimento (DD MM AAAA): ");
                scanf("%d %d %d", &chave_busca.data_nascimento.dia, &chave_busca.data_nascimento.mes, &chave_busca.data_nascimento.ano);
                getchar();

                if (buscar_arvore(arvore_rh, &chave_busca, &func_novo, comparar_chaves_rh)) {
                    printf("\n[ATENCAO] Funcionario ja existe no sistema!\n");
                    imprimir_ficha_funcionario(&func_novo);
                    //Caso de atualização de um cadastro existente
                    printf("\nDeseja atualizar este cadastro? (1-Sim / 0-Nao): ");
                    int atualiza;
                    scanf("%d", &atualiza);
                    getchar();
                    
                    if (atualiza) {
                        remover_arvore(arvore_rh, &chave_busca, comparar_chaves_rh);
                        printf("Novo Endereco: "); ler_string(func_novo.endereco, 100);
                        printf("Novo Telefone: "); ler_string(func_novo.telefone, 20);
                        inserir_arvore(arvore_rh, &chave_busca, &func_novo, comparar_chaves_rh);
                        printf("\nFuncionario atualizado com sucesso!\n");
                    }
                } else {
                    //Preenche o novo funcionário do zero
                    strcpy(func_novo.nome, chave_busca.nome);
                    func_novo.data_nascimento = chave_busca.data_nascimento;
                    
                    printf("Nome da Mae: "); ler_string(func_novo.nome_mae, 50);
                    printf("Nome do Pai: "); ler_string(func_novo.nome_pai, 50);
                    printf("Endereco: "); ler_string(func_novo.endereco, 100);
                    printf("Telefone: "); ler_string(func_novo.telefone, 20);
                    
                    func_novo.ativo = 1;
                    func_novo.qtd_pagamentos = 0; 

                    inserir_arvore(arvore_rh, &chave_busca, &func_novo, comparar_chaves_rh);
                    printf("\nFuncionario inserido com sucesso na Arvore B+!\n");
                }
                break;
            }
            case 2: {
                printf("\n--- BUSCAR FUNCIONARIO ---\n");
                ChaveFuncionario chave_busca;
                Funcionario func_encontrado;

                printf("Nome do funcionario: ");
                ler_string(chave_busca.nome, 50);
                
                printf("Data de Nascimento (DD MM AAAA) para formar a Chave Composta: ");
                scanf("%d %d %d", &chave_busca.data_nascimento.dia, &chave_busca.data_nascimento.mes, &chave_busca.data_nascimento.ano);
                getchar();

                if (buscar_arvore(arvore_rh, &chave_busca, &func_encontrado, comparar_chaves_rh)) {
                    imprimir_ficha_funcionario(&func_encontrado);
                } else {
                    printf("\n[ERRO] Funcionario nao encontrado no indice.\n");
                }
                break;
            }
            case 3: {
                printf("\n--- EXCLUIR FUNCIONARIO ---\n");
                ChaveFuncionario chave_busca;
                Funcionario func_encontrado;

                printf("Nome do funcionario a ser excluido: ");
                ler_string(chave_busca.nome, 50);
                
                printf("Data de Nascimento (DD MM AAAA): ");
                scanf("%d %d %d", &chave_busca.data_nascimento.dia, &chave_busca.data_nascimento.mes, &chave_busca.data_nascimento.ano);
                getchar();

                if (buscar_arvore(arvore_rh, &chave_busca, &func_encontrado, comparar_chaves_rh)) {
                    imprimir_ficha_funcionario(&func_encontrado);
                    
                    printf("\nTem CERTEZA que deseja excluir este funcionario fisica e logicamente? (1-Sim / 0-Nao): ");
                    int confirma;
                    scanf("%d", &confirma);
                    getchar();
                    
                    if (confirma) {
                        remover_arvore(arvore_rh, &chave_busca, comparar_chaves_rh);
                        printf("\nFuncionario excluido e Arvore B+ rebalanceada!\n");
                    } else {
                        printf("\nExclusao cancelada.\n");
                    }
                } else {
                    printf("\n[ERRO] Funcionario nao localizado.\n");
                }
                break;
            }
            case 4: { 
                printf("\n--- LISTAGEM DE FUNCIONARIOS POR INTERVALO ---\n");
                printf("[INFO] Para implementar esta funcao, sera necessario criar uma funcao especifica\n");
                printf("no Bplus.c que inicie a busca do 'Nome A', pegue o ponteiro 'proxima_folha'\n");
                printf("e continue lendo os blocos sequencialmente pelo disco ate chegar no 'Nome B'.\n");
                // TODO: Chamar funcao de range no Bplus.c
                break;
            }
            case 5: {
                printf("\n--- ESTRUTURA DO INDICE B+ NO DISCO ---\n");
                printf("[INFO] Requer uma funcao iterativa/recursiva dentro do Bplus.c que varra\n");
                printf("os offsets a partir da raiz, printando nivel por nivel as chaves de controle\n");
                printf("dos nos internos e depois das folhas.\n");
                // TODO: Chamar funcao de exibicao de debug no Bplus.c
                break;
            }
            case 6: {
                printf("\nEncerrando o sistema e garantindo persistencia no disco...\n");
                
                //Fecha a comunicação com o arquivo binário garantindo que buffers do SO sejam salvos
                if (arvore_rh->arquivo_binario != NULL) {
                    fclose(arvore_rh->arquivo_binario);
                }
                
                //Libera a struct de controle principal
                free(arvore_rh);
                
                printf("Sistema de RH finalizado com sucesso. Ate logo!\n");
                break;
            }
            default:
                printf("\nOpcao invalida. Tente novamente.\n");
        }
    }

    return 0;
}