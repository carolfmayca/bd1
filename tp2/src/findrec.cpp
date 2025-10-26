#include "../include/hashing_file.h"
#include "../include/config.h" 
#include <iostream>
#include <string>
#include <vector>

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


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: ./findrec <ID_do_artigo>" << std::endl;
        return 1;
    }

    const int TAMANHO_TABELA = 100000; // mesmo tamanho de tabela do upload
    
    try {
        int id_para_buscar = std::stoi(argv[1]);
        
        HashingFile arquivoHash(ARTIGO_DAT, TAMANHO_TABELA);  
        
        int blocosLidos = 0;
        Artigo resultado = arquivoHash.buscarPorId(id_para_buscar, blocosLidos);
        
        imprimirArtigoCompleto(resultado);

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