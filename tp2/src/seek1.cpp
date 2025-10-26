#include "../include/BPlusTree.hpp"
#include "../include/config.h"  // ADICIONAR
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <algorithm>

struct ArticleDisk {
    bool ocupado;
    int id;
    char titulo[301];
    int ano;
    char autores[151];
    int citacoes;
    char atualizacao[20];
    char snippet[1025];
};

const int REGISTROS_POR_BLOCO = 2;

struct Bloco {
    ArticleDisk artigos[REGISTROS_POR_BLOCO];
    int num_registros_usados;
    long proximo_bloco_offset;
};

// Variáveis globais para logging
enum LogLevel { ERROR, WARN, INFO, DEBUG };
LogLevel CURRENT_LOG_LEVEL = INFO;

// determinar o nível de log a partir da variável de ambiente
void setLogLevelFromEnv() {
    const char* log_env = std::getenv("LOG_LEVEL");
    if (log_env != nullptr) {
        std::string level(log_env);
        if (level == "error") CURRENT_LOG_LEVEL = ERROR;
        else if (level == "warn") CURRENT_LOG_LEVEL = WARN;
        else if (level == "info") CURRENT_LOG_LEVEL = INFO;
        else if (level == "debug") CURRENT_LOG_LEVEL = DEBUG;
    }
}

// Funções de logging
void logError(const std::string& message) {
    if (CURRENT_LOG_LEVEL >= ERROR) {
        std::cerr << "[ERROR] " << message << std::endl;
    }
}

void logWarn(const std::string& message) {
    if (CURRENT_LOG_LEVEL >= WARN) {
        std::cout << "[WARN] " << message << std::endl;
    }
}

void logInfo(const std::string& message) {
    if (CURRENT_LOG_LEVEL >= INFO) {
        std::cout << "[INFO] " << message << std::endl;
    }
}

void logDebug(const std::string& message) {
    if (CURRENT_LOG_LEVEL >= DEBUG) {
        std::cout << "[DEBUG] " << message << std::endl;
    }
}

// Função para corrigir encoding
std::string fixEncoding(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        unsigned char c = str[i];
        
        // Corrige caracteres UTF-8 mal interpretados
        if (c == 0xE2 && i + 2 < str.length() && 
            static_cast<unsigned char>(str[i+1]) == 0x80 && 
            static_cast<unsigned char>(str[i+2]) == 0x94) {
            result += "—";
            i += 2;
        }
        else if (c == 0xE2 && i + 2 < str.length() && 
                 static_cast<unsigned char>(str[i+1]) == 0x80 && 
                 static_cast<unsigned char>(str[i+2]) == 0x99) {
            result += "'";
            i += 2;
        }
        else if (c >= 32 && c <= 126) {
            result += c;
        }
        else if (c == '\n' || c == '\t') {
            result += c;
        }
        else {
            result += ' ';
        }
    }
    return result;
}

std::string truncateSnippet(const std::string& snippet, size_t maxLength = 500) {
    if (snippet.length() <= maxLength) {
        return snippet;
    }
    return snippet.substr(0, maxLength) + "... [truncado]";
}

// Estrutura para retornar estatísticas da busca
struct SearchResult {
    bool success;
    int treeBlocksRead;
    int primaryIndexBlocksRead;
    int dataBlocksRead;
    long long durationMs;
};

SearchResult search_primary_index(BPlusTree<long>& idx, int idBuscado) {
    auto startTime = std::chrono::high_resolution_clock::now();
    SearchResult result = {false, 0, 0, 0, 0};
    
    logInfo("Iniciando busca por ID: " + std::to_string(idBuscado));
    logInfo("Caminho do arquivo de dados: " + NOME_ARQUIVO_DADOS);  // MODIFICADO
    logInfo("Caminho do arquivo de índice: " + NOME_ARQUIVO_INDICE_PRIM);  // MODIFICADO

    long leafOffset = idx.search(idBuscado);
    result.treeBlocksRead = idx.getPagesRead();
    
    if (leafOffset == 0) {
        logWarn("ID não encontrado no índice primário: " + std::to_string(idBuscado));
        return result;
    }

    std::vector<long> results = idx.searchAll(idBuscado);
    if (results.empty()) {
        logWarn("ID não encontrado no índice primário: " + std::to_string(idBuscado));
        return result;
    }

    long actualRID = -1;
    std::ifstream idxFile(NOME_ARQUIVO_INDICE_PRIM, std::ios::binary);  // MODIFICADO
    if (!idxFile.is_open()) {
        logError("Erro ao abrir arquivo de índice primário: " + NOME_ARQUIVO_INDICE_PRIM);  // MODIFICADO
        return result;
    }

    idxFile.seekg(0, std::ios::end);
    std::streamoff idxSize = idxFile.tellg();
    if (results[0] < 0 || static_cast<std::streamoff>(results[0]) + sizeof(long) > idxSize) {
        logError("Offset inválido no arquivo de índice primário: " + std::to_string(results[0]));
        idxFile.close();
        return result;
    }

    idxFile.seekg(results[0], std::ios::beg);
    if (!idxFile.read(reinterpret_cast<char*>(&actualRID), sizeof(long))) {
        logError("Erro ao ler RID do índice primário no offset: " + std::to_string(results[0]));
        idxFile.close();
        return result;
    }
    idxFile.close();
    result.primaryIndexBlocksRead = 1;

    // Buscar no arquivo de dados
    std::ifstream dataFile(NOME_ARQUIVO_DADOS, std::ios::binary);  // MODIFICADO
    if (!dataFile.is_open()) {
        logError("Erro ao abrir arquivo de dados: " + NOME_ARQUIVO_DADOS);  // MODIFICADO
        return result;
    }

    size_t articleIndex = static_cast<size_t>(actualRID);
    size_t blockIndex = articleIndex / REGISTROS_POR_BLOCO;
    size_t positionInBlock = articleIndex % REGISTROS_POR_BLOCO;

    dataFile.seekg(0, std::ios::end);
    std::streamoff dataSize = dataFile.tellg();
    size_t totalArticles = (dataSize / sizeof(Bloco)) * REGISTROS_POR_BLOCO;

    if (articleIndex >= totalArticles) {
        logError("RID inválido (fora do tamanho do arquivo de dados): " + std::to_string(actualRID));
        dataFile.close();
        return result;
    }

    Bloco bloco{};
    dataFile.seekg(blockIndex * sizeof(Bloco));
    if (dataFile.read(reinterpret_cast<char*>(&bloco), sizeof(Bloco))) {
        result.dataBlocksRead = 1;
        
        if (positionInBlock < bloco.num_registros_usados) {
            ArticleDisk& art = bloco.artigos[positionInBlock];
            if (art.ocupado) {
                logInfo("Artigo encontrado com sucesso");
                std::cout << "\n=== ARTIGO ENCONTRADO ===" << std::endl;
                std::cout << "ID: " << art.id << std::endl;
                std::cout << "Título: " << art.titulo << std::endl;
                std::cout << "Ano: " << art.ano << std::endl;
                std::cout << "Autores: " << art.autores << std::endl;
                std::cout << "Atualização: " << art.atualizacao << std::endl;
                std::cout << "Citações: " << art.citacoes << std::endl;
                
                std::string cleanedSnippet = fixEncoding(art.snippet);
                std::string finalSnippet = truncateSnippet(cleanedSnippet);
                std::cout << "Snippet: " << finalSnippet << std::endl;
                
                result.success = true;
            } else {
                logWarn("Registro marcado como não ocupado no RID: " + std::to_string(actualRID));
            }
        } else {
            logError("Posição inválida no bloco: " + std::to_string(positionInBlock));
        }
    } else {
        logError("Erro ao ler bloco do arquivo de dados no índice: " + std::to_string(blockIndex));
    }

    dataFile.close();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    
    return result;
}

int main(int argc, char* argv[]) {
    setLogLevelFromEnv();
    
    if (argc < 2) {
        logError("Uso: " + std::string(argv[0]) + " <ID>");
        return 1;
    }

    int id;
    try {
        id = std::stoi(argv[1]);
    } catch (const std::exception& e) {
        logError("ID inválido: " + std::string(argv[1]));
        return 1;
    }

    BPlusTree<long> idx(NOME_ARQUIVO_INDICE_PRIM);  // MODIFICADO
    idx.resetStats();
    
    logDebug("Índice primário carregado");
    
    SearchResult result = search_primary_index(idx, id);
    
    std::cout << "\n=== ESTATÍSTICAS DA BUSCA ===" << std::endl;
    std::cout << "Blocos da árvore lidos: " << result.treeBlocksRead << std::endl;
    std::cout << "Blocos do índice primário lidos: " << result.primaryIndexBlocksRead << std::endl;
    std::cout << "Blocos de dados lidos: " << result.dataBlocksRead << std::endl;
    std::cout << "Tempo total de execução: " << result.durationMs << "ms" << std::endl;
    std::cout << "Total de blocos lidos: " 
              << result.treeBlocksRead + result.primaryIndexBlocksRead + result.dataBlocksRead 
              << std::endl;

    return 0;
}