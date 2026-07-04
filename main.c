#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Bplus.h"
#include "AuxRH.h"

int main() {
    ArvoreBPlus *arvore_rh = criar_arvore("banco_rh.bin", tamanho_chave_rh, tamanho_dado_rh);

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
                memset(&chave_busca, 0, sizeof(ChaveFuncionario)); // Zera todo o lixo da RAM
                memset(&func_novo, 0, sizeof(Funcionario));

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
                        printf("Insira o pagamento (Valor MM AAAA): ");
                        int qtd_adaptado = func_novo.qtd_pagamentos - 12*(func_novo.qtd_pagamentos/12);
                        scanf("%f %d %d", &func_novo.historico_pagamentos[qtd_adaptado].valor, &func_novo.historico_pagamentos[qtd_adaptado].mes_referencia, &func_novo.historico_pagamentos[qtd_adaptado].ano_referencia);
                        func_novo.qtd_pagamentos++;
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
                    printf("Digite a Data de Contratação (DD MM AAAA): ");
                    scanf("%d %d %d", &func_novo.data_contratacao.dia, &func_novo.data_contratacao.mes, &func_novo.data_contratacao.ano);
                    
                    func_novo.ativo = 1;
                    func_novo.qtd_pagamentos = 0; 

                    inserir_arvore(arvore_rh, &chave_busca, &func_novo, comparar_chaves_rh);
                    printf("\nFuncionario inserido com sucesso na Arvore B+!\n");
                    imprimir_ficha_funcionario(&func_novo);
                }
                break;
            }
            case 2: {
                printf("\n--- BUSCAR FUNCIONARIO ---\n");
                
                //Preparação de variáveis
                ChaveFuncionario chave_busca;
                memset(&chave_busca, 0, sizeof(ChaveFuncionario));
                
                //Coleta inicial de chave primária
                printf("Nome do funcionario: ");
                ler_string(chave_busca.nome, 100); 
                
                int quantidade = 0;
                //Faz a busca inicial no disco considerando apenas o nome
                Funcionario *lista_homonimos = (Funcionario *)buscar_multiplos(arvore_rh, &chave_busca, &quantidade, comparar_nome_rh);
                
                if (quantidade == 0) {
                    printf("\n[ERRO] Nenhum funcionario com o nome '%s' encontrado no indice.\n", chave_busca.nome);
                } 
                else if (quantidade == 1) {
                    //Imprime direto o único resultado encontrado.
                    imprimir_ficha_funcionario(lista_homonimos);
                } 
                else {
                    //Homônimos, Lista as opções de desempate
                    printf("\n[INFO] Encontramos %d funcionarios com o nome '%s' (Homonimos)!\n", quantidade, chave_busca.nome);
                    for (int i = 0; i < quantidade; i++) {
                        printf("%d) Nascido em: %02d/%02d/%04d\n", i + 1, 
                               lista_homonimos[i].data_nascimento.dia,
                               lista_homonimos[i].data_nascimento.mes,
                               lista_homonimos[i].data_nascimento.ano);
                    }
                    
                    //Solicita a data para montar a chave composta exata
                    printf("\nDigite a Data de Nascimento para desempatar (DD MM AAAA): ");
                    scanf("%d %d %d", &chave_busca.data_nascimento.dia, &chave_busca.data_nascimento.mes, &chave_busca.data_nascimento.ano);
                    getchar(); 
                    
                    Funcionario func_exato;
                    memset(&func_exato, 0, sizeof(Funcionario));
                    
                    //Usa o motor da Árvore B+ original para trazer o registro exato
                    if (buscar_arvore(arvore_rh, &chave_busca, &func_exato, comparar_chaves_rh)) {
                        imprimir_ficha_funcionario(&func_exato);
                    } else {
                        printf("\n[ERRO] A data informada nao corresponde a nenhum dos homonimos listados.\n");
                    }
                }
                
                if (lista_homonimos != NULL) free(lista_homonimos);
                
                break;
            }
            case 3: {
                printf("\n--- EXCLUIR FUNCIONARIO ---\n");
                
                ChaveFuncionario chave_busca;
                memset(&chave_busca, 0, sizeof(ChaveFuncionario));
                
                printf("Nome do funcionario a ser excluido: ");
                ler_string(chave_busca.nome, 100);
                
                int quantidade = 0;
                Funcionario *lista_homonimos = (Funcionario *)buscar_multiplos(arvore_rh, &chave_busca, &quantidade, comparar_nome_rh);
                
                if (quantidade == 0) {
                    printf("\n[ERRO] Nenhum funcionario com o nome '%s' encontrado.\n", chave_busca.nome);
                } 
                else {
                    //Se achou apenas 1, já sabemos a data de nascimento dele e preenchemos automaticamente a chave.
                    if (quantidade == 1) {
                        chave_busca.data_nascimento = lista_homonimos->data_nascimento;
                    } 
                    //Se achou mais de 1, listamos e pedimos para o usuário desempatar.
                    else {
                        printf("\n[ATENCAO] Encontramos %d homonimos para exclusao!\n", quantidade);
                        for (int i = 0; i < quantidade; i++) {
                            printf("%d) Nascido em: %02d/%02d/%04d\n", i + 1, 
                                   lista_homonimos[i].data_nascimento.dia,
                                   lista_homonimos[i].data_nascimento.mes,
                                   lista_homonimos[i].data_nascimento.ano);
                        }
                        
                        printf("\nDigite a Data de Nascimento do funcionario que deseja EXCLUIR (DD MM AAAA): ");
                        scanf("%d %d %d", &chave_busca.data_nascimento.dia, &chave_busca.data_nascimento.mes, &chave_busca.data_nascimento.ano);
                        getchar();
                    }
                    
                    //Com a chave composta completa (nome + data), validamos e excluímos
                    Funcionario func_exato;
                    memset(&func_exato, 0, sizeof(Funcionario));
                    
                    //Trazemos a ficha exata do disco para confirmar a exclusão visualmente
                    if (buscar_arvore(arvore_rh, &chave_busca, &func_exato, comparar_chaves_rh)) {
                        imprimir_ficha_funcionario(&func_exato);
                        
                        printf("\nTem CERTEZA que deseja excluir este funcionario fisica e logicamente? (1-Sim / 0-Nao): ");
                        int confirma;
                        scanf("%d", &confirma);
                        getchar();
                        
                        if (confirma == 1) {
                            //Chama a remoção genérica passando a chave completa e o callback principal
                            remover_arvore(arvore_rh, &chave_busca, comparar_chaves_rh);
                            printf("\nFuncionario excluido e Arvore B+ rebalanceada com sucesso!\n");
                        } else {
                            printf("\nExclusao cancelada pelo usuario.\n");
                        }
                    } else {
                        printf("\n[ERRO] O funcionario exato para exclusao nao foi localizado.\n");
                    }
                }
                
                if (lista_homonimos != NULL) free(lista_homonimos);
                
                break;
            }
            case 4: {
                printf("\n--- LISTAGEM POR INTERVALO ---\n");
                ChaveFuncionario chave_a, chave_b;
                memset(&chave_a, 0, sizeof(ChaveFuncionario));
                memset(&chave_b, 0, sizeof(ChaveFuncionario));
                
                printf("Digite o Nome A (Limite Inferior): ");
                ler_string(chave_a.nome, 100);
                
                printf("Digite o Nome B (Limite Superior): ");
                ler_string(chave_b.nome, 100);
                
                //Impede que o usuário digite a ordem invertida (Ex: Z a A)
                if (strcmp(chave_a.nome, chave_b.nome) >= 0) {
                    printf("\n[ERRO] O Nome A deve vir alfabeticamente antes do Nome B.\n");
                    break;
                }
                
                int quantidade = 0;
                //A árvore desce até o "Nome A" e varre o disco pra direita usando o seu callback de nomes!
                Funcionario *lista_intervalo = (Funcionario *)buscar_intervalo(arvore_rh, &chave_a, &chave_b, &quantidade, comparar_nome_rh);
                
                if (quantidade == 0) {
                    printf("\nNenhum funcionario encontrado no intervalo [%s, %s].\n", chave_a.nome, chave_b.nome);
                } else {
                    printf("\n[INFO] Foram encontrados %d funcionarios no intervalo!\n", quantidade);
                    for (int i = 0; i < quantidade; i++) {
                        imprimir_ficha_funcionario(&lista_intervalo[i]); // Reutiliza a sua função de impressão
                    }
                }
                if (lista_intervalo != NULL) free(lista_intervalo);
                
                break;
            }
            case 5: { 
                printf("\n--- EXIBIR ESTRUTURA DO INDICE ---\n");
                //Passamos o callback responsável por desmascarar a chave
                exibir_arvore(arvore_rh, imprimir_chave_rh);
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