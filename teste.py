import random

def gerar_carga(quantidade=150):
    with open("carga_150.txt", "w") as f:
        # Alfabeto para gerar nomes variados e forçar splits pela árvore toda
        letras = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        
        for i in range(quantidade):
            # 1. Escolhe a opção Inserir no menu
            f.write("1\n")
            
            # 2. Nome do funcionário (Ex: Funcionario_C_15)
            letra = random.choice(letras)
            f.write(f"Funcionario_{letra}_{i}\n")
            
            # 3. Data de Nascimento (DD MM AAAA)
            dia = random.randint(1, 28)
            mes = random.randint(1, 12)
            ano = random.randint(1970, 2000)
            f.write(f"{dia:02d} {mes:02d} {ano}\n")
            
            # 4. Nome da Mae
            f.write(f"Mae {i}\n")
            
            # 5. Nome do Pai
            f.write(f"Pai {i}\n")
            
            # 6. Endereco
            f.write(f"Rua Teste, {i}\n")
            
            # 7. Telefone
            f.write(f"99999{i:04d}\n")
            
            # 8. Data de Contratacao
            f.write("01 07 2026\n")
            
        # No final das 50 inserções, escolhe a Opção 6 para Sair e salvar no disco
        f.write("6\n")

if __name__ == "__main__":
    gerar_carga(150)
    print("Arquivo 'carga_150.txt' gerado com sucesso!")