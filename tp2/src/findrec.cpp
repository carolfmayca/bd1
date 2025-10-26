//fernando

#include "../include/hashing_file.h"
#include <iostream>
#include <string>
#include <vector>

// Função para imprimir os detalhes de um artigo de forma legível
void imprimirArtigoCompleto(const Artigo& art) {
    if (art.ocupado) {
        std::cout << "--- Registro Encontrado ---" << std::endl;
        std::cout << "ID: " << art.id << std::endl;
        std::cout << "Titulo: " << art.titulo << std::endl;
        std::cout << "Ano: " << art.ano << std::endl;
        std::cout << "Autores: " << art.autores << std::endl;
        std::cout << "Citacoes: " << art.citacoes << std::endl;
        std::cout << "Atualizacao: " << art.atualizacao << std::endl;
        std::cout << "Snippet: " << art.snippet << std::endl;
    } else {
        std::cout << "Registro com o ID informado nao foi encontrado." << std::endl;
    }
}

// --- PROGRAMA PRINCIPAL ---

int main(int argc, char* argv[]) {
    // Verifica se o utilizador passou o ID como argumento na linha de comando
    if (argc != 2) {
        std::cerr << "Uso: ./findrec <ID_do_artigo>" << std::endl;
        return 1; // Termina o programa com um código de erro
    }

    // Nomes dos arquivos e parâmetros
    const std::string NOME_ARQUIVO_DADOS = "/data/artigos.dat";
    const int TAMANHO_TABELA = 2000000; // IMPORTANTE: Use o mesmo tamanho de tabela do upload!
    
    try {
        // Converte o argumento da linha de comando de texto para número
        int id_para_buscar = std::stoi(argv[1]);
        
        // Cria uma instância da classe de Hashing para interagir com os arquivos
        HashingFile arquivoHash(NOME_ARQUIVO_DADOS, TAMANHO_TABELA);
        
        int blocosLidos = 0;
        Artigo resultado = arquivoHash.buscarPorId(id_para_buscar, blocosLidos);
        
        imprimirArtigoCompleto(resultado);

        // Imprime as estatísticas pedidas no trabalho
        std::cout << "\n----------------------------------------" << std::endl;
        std::cout << "Blocos lidos para encontrar o registro: " << blocosLidos << std::endl;
        std::cout << "Quantidade total de blocos do arquivo de dados: " << arquivoHash.getTotalBlocos() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Erro: O ID informado nao e um numero valido ou o arquivo de dados nao existe." << std::endl;
        std::cerr << "Certifique-se de executar o programa de carga primeiro." << std::endl;
        return 1;
    }

    return 0;
}

