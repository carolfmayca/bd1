#include "../include/hashing_file.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

// --- FUNÇÕES DE PROCESSAMENTO DO CSV ---

// Função auxiliar para remover as aspas do início e do fim de uma string
std::string trimQuotes(const std::string& str) {
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

// FUNÇÃO DE PARSING DO CSV
std::vector<std::string> parseCSVLine(const std::string& linha) {
    std::vector<std::string> campos;
    std::string campo_atual;
    bool dentro_de_aspas = false;

    for (char c : linha) {
        if (c == '"') {
            dentro_de_aspas = !dentro_de_aspas;
        } else if (c == ';' && !dentro_de_aspas) {
            campos.push_back(trimQuotes(campo_atual));
            campo_atual.clear();
        } else {
            campo_atual += c;
        }
    }
    campos.push_back(trimQuotes(campo_atual)); // Adiciona o último campo
    return campos;
}

// Função que lê o CSV e preenche o arquivo de dados com hash
void processarCSV(const std::string& csvPath, HashingFile& hashingFile) {
    std::ifstream csvFile(csvPath);
    if (!csvFile.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo CSV '" << csvPath << "'" << std::endl;
        return;
    }

    std::cout << "\n--- Lendo arquivo " << csvPath << " e inserindo dados ---" << std::endl;
    std::string linha;
    int registrosInseridos = 0;
    int numeroLinha = 0;
    
    while (std::getline(csvFile, linha)) {
        numeroLinha++;
        if (linha.empty()) continue;

        std::vector<std::string> campos = parseCSVLine(linha);

        if (campos.size() == 7) {
            Artigo art = {};
            art.ocupado = true;
            try {
                art.id = std::stoi(campos[0]);
                strncpy(art.titulo, campos[1].c_str(), 300);
                art.ano = std::stoi(campos[2]);
                strncpy(art.autores, campos[3].c_str(), 150);
                art.citacoes = std::stoi(campos[4]);
                strncpy(art.atualizacao, campos[5].c_str(), 19);
                // Trata o caso do snippet ser "NULL"
                if (campos[6] != "NULL") {
                    strncpy(art.snippet, campos[6].c_str(), 1024);
                }
                
                hashingFile.inserirArtigo(art);
                registrosInseridos++;
            } catch (const std::exception& e) {
                // AVISO DE ERRO DE CONVERSÃO
                std::cerr << "--> ERRO DE CONVERSAO na linha " << numeroLinha << ". Verifique os campos numericos. Erro: " << e.what() << std::endl;
            }
        } else {
            // AVISO DE ERRO DE PARSING (NÚMERO DE CAMPOS)
            std::cerr << "--> AVISO: Linha " << numeroLinha << " ignorada. Esperava 7 campos, mas encontrou " << campos.size() << "." << std::endl;
            std::cerr << "    Conteudo da linha: " << linha << std::endl;
        }
    }
    std::cout << "--- Insercao finalizada. " << registrosInseridos << " registros inseridos. ---\n" << std::endl;
    csvFile.close();
}

// --- FUNÇÕES DE TESTE E EXIBIÇÃO ---

void imprimirArtigo(const Artigo& art) {
    if (art.ocupado) {
        std::cout << "  -> ENCONTRADO! ID: " << art.id << ", Titulo: '" << art.titulo << "'" << std::endl;
    } else {
        std::cout << "  -> NAO ENCONTRADO." << std::endl;
    }
}

// --- PROGRAMA PRINCIPAL ---

int main() {
    const std::string NOME_ARQUIVO_DADOS = "artigos.dat";
    const std::string NOME_ARQUIVO_INDICE = "tabela_hash.idx";
    const std::string NOME_CSV_ENTRADA = "../data/artigo.csv";
    const int TAMANHO_TABELA = 2000; 

    // Limpa o ambiente para um teste novo
    remove(NOME_ARQUIVO_DADOS.c_str());
    remove(NOME_ARQUIVO_INDICE.c_str());

    std::cout << "--- INICIANDO TESTE DE CARGA DE DADOS VIA CSV ---" << std::endl;

    // 1. Cria a instância da classe de Hashing
    HashingFile meuArquivoHash(NOME_ARQUIVO_DADOS, TAMANHO_TABELA);

    // 2. Processa o CSV para popular o arquivo de dados
    processarCSV(NOME_CSV_ENTRADA, meuArquivoHash);

    // 3. Testa a busca por TODOS os 30 artigos para ver quais foram inseridos
    std::cout << "--- Verificando a insercao de todos os 30 artigos ---" << std::endl;
    for (int id = 1; id <= 100000; ++id) {
        int blocosLidos = 0; 
        std::cout << "Buscando Artigo ID " << id << "..." << std::endl;
        Artigo res = meuArquivoHash.buscarPorId(id, blocosLidos);
        imprimirArtigo(res);
    }

    std::cout << "\n--- TESTE FINALIZADO ---" << std::endl;

    return 0;
}

