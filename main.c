#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Bplus.h"
#include "AuxRH.h" // [1]

int main() {
    char *nomeArquivo = "banco_rh.bin";
    
    // 1. Tenta abrir o arquivo. Se não existir, cria um novo passando as funções do RH
    ArvoreBPlus *arvore_rh = abrirArquivo(nomeArquivo);
    if (arvore_rh == NULL) {
        // Exemplo: Ordem 5 para nós internos e folhas. Passamos os callbacks de tamanho [3]
        criarArquivo(nomeArquivo, tamanho_chave_rh(), tamanho_dado_rh());
        arvore_rh = abrirArquivo(nomeArquivo);
    }

    // 2. Injetamos a sua função de desempate de homônimos (ordem alfabética e cronológica) [8]
    arvore_rh->comparar = comparar_chaves_rh; 

    int opcao = 0;
    while (opcao != 6) {
        printf("\n=================================\n");
        printf("    SISTEMA DE GESTAO DE RH\n");
        printf("=================================\n");
        printf("1. Inserir Funcionario\n");
        printf("2. Buscar Funcionario\n");
        printf("3. Excluir Funcionario\n");
        printf("4. Listagem por Intervalo\n");
        printf("5. Exibir Estrutura do Indice\n");
        printf("6. Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &opcao);
        getchar(); // Limpar buffer

        switch (opcao) {
            case 1: {
                printf("\n--- INSERIR FUNCIONARIO ---\n");
                ChaveFuncionario chave;
                Funcionario func;

                // Requisito 3.3.1: Solicitar inicialmente o nome e a data de nascimento [4]
                printf("Nome do Funcionario: ");
                ler_string(chave.nome, 100);
                printf("Data de Nascimento (DD MM AAAA): ");
                scanf("%d %d %d", &chave.data_nascimento.dia, &chave.data_nascimento.mes, &chave.data_nascimento.ano);
                getchar();

                // Busca para verificar se a combinação já existe
                if (buscar(arvore_rh, &chave, &func)) {
                    printf("\n[ATENCAO] Ja existe um funcionario cadastrado com este Nome e Data de Nascimento!\n");
                    imprimir_ficha_funcionario(&func); // [9]
                    
                    printf("\nDeseja realizar uma atualizacao (update)? (1 - Sim / 0 - Nao): ");
                    int opcaoUpdate;
                    scanf("%d", &opcaoUpdate);
                    getchar();

                    // Fluxo de atualização exigido no cenário de colisão [4]
                    if (opcaoUpdate == 1) {
                        printf("\n--- ATUALIZAR DADOS ---\n");
                        printf("Novo Endereco: ");
                        ler_string(func.endereco, 100);
                        printf("Novo Telefone: ");
                        ler_string(func.telefone, 11);
                        printf("Status (1 - Ativo / 0 - Inativo): ");
                        scanf("%d", &func.ativo);
                        getchar();

                        if (func.ativo == 0) {
                            printf("Data de Desligamento (DD MM AAAA): ");
                            scanf("%d %d %d", &func.data_desligamento.dia, &func.data_desligamento.mes, &func.data_desligamento.ano);
                            getchar();
                        }

                        remover(arvore_rh, &chave); // Remove o antigo
                        inserir(arvore_rh, &chave, &func); // Insere o novo mantendo o histórico
                        printf("\nCadastro atualizado com sucesso!\n");
                    } else {
                        printf("Operacao cancelada.\n");
                    }
                } else {
                    // Inserção de novo registro, caso não exista [4]
                    strcpy(func.nome, chave.nome);
                    func.data_nascimento = chave.data_nascimento;

                    printf("Nome da Mae: ");
                    ler_string(func.nome_mae, 100);
                    printf("Nome do Pai: ");
                    ler_string(func.nome_pai, 100);
                    printf("Endereco: ");
                    ler_string(func.endereco, 100);
                    printf("Telefone: ");
                    ler_string(func.telefone, 11);
                    printf("Data de Contratacao (DD MM AAAA): ");
                    scanf("%d %d %d", &func.data_contratacao.dia, &func.data_contratacao.mes, &func.data_contratacao.ano);
                    getchar();

                    func.ativo = 1;
                    func.qtd_pagamentos = 0; // Histórico inicializado vazio [4]

                    inserir(arvore_rh, &chave, &func);
                    printf("\nFuncionario cadastrado com sucesso!\n");
                }
                break;
            }
            case 2: {
                printf("\n--- BUSCAR FUNCIONARIO ---\n");
                ChaveFuncionario chave_busca;
                Funcionario func_retorno;

                printf("Nome do Funcionario: ");
                ler_string(chave_busca.nome, 100);

                // REQUISITO: O sistema deve tratar homônimos listando-os e pedindo a data para desempate.
                // Como a função buscar() da API genérica requer a chave completa (Nome + Data) para a busca O(log n),
                // solicitamos a data de nascimento para formar a chave de desempate exata.
                printf("Data de Nascimento para desempate (DD MM AAAA): ");
                scanf("%d %d %d", &chave_busca.data_nascimento.dia, &chave_busca.data_nascimento.mes, &chave_busca.data_nascimento.ano);
                getchar(); // Limpar buffer do teclado

                // Busca exata utilizando a API genérica do Bplus.h
                if (buscar(arvore_rh, &chave_busca, &func_retorno)) {
                    // Requisito: Exibe a ficha cadastral completa, incluindo o histórico de pagamentos
                    imprimir_ficha_funcionario(&func_retorno);
                } else {
                    printf("\n[AVISO] Nenhum funcionario encontrado com o nome '%s' e esta data.\n", chave_busca.nome);
                }
                break;
            }

            case 3: {
                printf("\n--- EXCLUIR FUNCIONARIO ---\n");
                ChaveFuncionario chave_exclusao;
                Funcionario func_exclusao;

                printf("Nome do Funcionario a ser excluido: ");
                ler_string(chave_exclusao.nome, 100);

                // REQUISITO: Tratamento de homônimos na exclusão (mesma lógica da busca)
                printf("Data de Nascimento para desempate (DD MM AAAA): ");
                scanf("%d %d %d", &chave_exclusao.data_nascimento.dia, &chave_exclusao.data_nascimento.mes, &chave_exclusao.data_nascimento.ano);
                getchar();

                if (buscar(arvore_rh, &chave_exclusao, &func_exclusao)) {
                    // REQUISITO: Exibir dados omitindo o histórico de pagamentos antes de pedir confirmação
                    printf("\n--- CONFIRMACAO DE EXCLUSAO ---\n");
                    printf("Nome: %s\n", func_exclusao.nome);
                    printf("Nascimento: %02d/%02d/%04d\n", func_exclusao.data_nascimento.dia, func_exclusao.data_nascimento.mes, func_exclusao.data_nascimento.ano);
                    printf("Filiacao: %s (Mae) e %s (Pai)\n", func_exclusao.nome_mae, func_exclusao.nome_pai);
                    printf("Status: %s\n", func_exclusao.ativo ? "Ativo" : "Inativo");
                    
                    printf("\nTem certeza que deseja excluir este registro DEFINITIVAMENTE? (1 - Sim / 0 - Nao): ");
                    int confirma;
                    scanf("%d", &confirma);
                    getchar();

                    if (confirma == 1) {
                        // Chama a função genérica de remoção do Bplus.h
                        if (remover(arvore_rh, &chave_exclusao)) {
                            printf("\n[SUCESSO] Funcionario '%s' removido com sucesso!\n", chave_exclusao.nome);
                        } else {
                            printf("\n[ERRO] Falha ao remover o funcionario do disco.\n");
                        }
                    } else {
                        printf("\nOperacao de exclusao cancelada pelo usuario.\n");
                    }
                } else {
                    printf("\n[AVISO] Funcionario nao localizado para exclusao!\n");
                }
                break;
            }
            case 5: {
                printf("\n--- ESTRUTURA DO INDICE (ARVORE B+) ---\n");
                // REQUISITO: Imprimir de forma textual e hierárquica [1].
                // A função imprimirArvoreGen deve ser implementada no Bplus.c, 
                // e nós passamos o ponteiro da função de impressão do RH para ela.
                
                // Assumindo que a equipe criará a função: void imprimirArvoreGen(ArvoreBPlus *arvore, void (*imprimirChave)(void*))
                imprimirArvoreGen(arvore_rh, imprimir_chave_rh);
                
                printf("\n---------------------------------------\n");
                break;
            }
            case 4: {
                printf("\n--- LISTAGEM POR INTERVALO ---\n");
                ChaveFuncionario chave_inicio, chave_fim;

                // REQUISITO: O usuário deve fornecer duas strings delimitadoras (Nome A e Nome B).
                printf("Digite o Nome de inicio do intervalo (Nome A): ");
                ler_string(chave_inicio.nome, 100);

                printf("Digite o Nome de fim do intervalo (Nome B): ");
                ler_string(chave_fim.nome, 100);

                printf("\nBuscando funcionarios no intervalo aberto entre '%s' e '%s'...\n", chave_inicio.nome, chave_fim.nome);

                // Como a árvore é genérica, chamaremos uma função da API do Bplus.h.
                // Assinatura esperada da equipe: 
                // void listar_por_intervalo(ArvoreBPlus *arvore, void *chaveA, void *chaveB, void (*funcao_imprimir)(void*));
                
                // listar_por_intervalo(arvore_rh, &chave_inicio, &chave_fim, (void (*)(void*))imprimir_ficha_funcionario);
                
                printf("\n[AVISO] Funcionalidade dependente da equipe do Bplus.c (Implementar navegacao pela proximaFolha em disco).\n");
                break;
            }
            case 6: {
                printf("\n--- ENCERRANDO O SISTEMA ---\n");
                printf("Salvando os metadados da Arvore B+ e fechando arquivos em disco...\n");
                
                // REQUISITO: Garantir fechamento e integridade dos arquivos em disco
                fecharArvore(arvore_rh);
                
                printf("Sistema encerrado de forma segura. Ate logo!\n");
                break;
            }
            default:
                printf("\n[ERRO] Opcao invalida. Tente novamente.\n");
                break;
        }
    }
    return 0;
}