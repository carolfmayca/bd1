#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring> // Para strncpy
#include "../include/hashing_file.h"

// Função auxiliar para remover as aspas do início e do fim de uma string
std::string trimQuotes(const std::string& str) {
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

// Função principal que lê o CSV e preenche o arquivo de dados
void processarCSV(const std::string& csvPath, HashingFile& hashingFile) {
    std::ifstream csvFile(csvPath);
    if (!csvFile.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo CSV '" << csvPath << "'" << std::endl;
        return;
    }

    std::string linha;
    int contador = 0;
    
    // Lê o arquivo CSV linha por linha
    while (std::getline(csvFile, linha)) {
        std::stringstream ss(linha);
        std::string campo;
        std::vector<std::string> campos;

        // Separa a linha pelos ponto-e-vírgulas (;)
        while (std::getline(ss, campo, ';')) {
            campos.push_back(trimQuotes(campo));
        }

        if (campos.size() == 7) {
            Artigo art = {}; // Zera a struct
            art.ocupado = true;

            // Converte e copia os campos para o struct Artigo
            try {
                art.id = std::stoi(campos[0]);
                strncpy(art.titulo, campos[1].c_str(), 300);
                art.ano = std::stoi(campos[2]);
                strncpy(art.autores, campos[3].c_str(), 150);
                art.citacoes = std::stoi(campos[4]);
                strncpy(art.atualizacao, campos[5].c_str(), 19);
                strncpy(art.snippet, campos[6].c_str(), 1024);

                // Insere o artigo lido no arquivo de hash
                hashingFile.inserirArtigo(art);
                contador++;

            } catch (const std::invalid_argument& e) {
                std::cerr << "Aviso: Nao foi possivel converter um campo na linha: " << linha << std::endl;
            }
        }
    }
    std::cout << "\nProcessamento do CSV finalizado. " << contador << " registros inseridos." << std::endl;
    csvFile.close();
}

int main() {
    const std::string NOME_HASH_FILE = "artigos.dat";
    const std::string NOME_CSV_FILE = "teste_artigos.csv";
    const int TAMANHO_TABELA = 23; // Um número primo maior que a quantidade de dados

    // Garante um teste limpo removendo o arquivo de dados antigo
    remove(NOME_HASH_FILE.c_str());

    std::cout << "--- Iniciando Carga de Dados do CSV ---" << std::endl;

    // 1. Cria a instância do arquivo de Hashing
    HashingFile meuArquivoHash(NOME_HASH_FILE, TAMANHO_TABELA);

    // 2. Chama a função para ler o CSV e popular o arquivo de hash
    processarCSV(NOME_CSV_FILE, meuArquivoHash);

    std::cout << "\n--- Carga Finalizada ---" << std::endl;

    return 0;
}