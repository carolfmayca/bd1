#include "BPlusTree.hpp"
#include "config.h"  // ADICIONAR
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <vector>
#include <locale>

// Variáveis globais para logging
enum LogLevel { ERROR, WARN, INFO, DEBUG };
LogLevel CURRENT_LOG_LEVEL = INFO;

// Função para determinar o nível de log a partir da variável de ambiente
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

// Funções de normalização (copiadas do indice_secundario)
static inline std::string trim(const std::string& s) {
    std::size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}


static std::string normalize(const char* tituloRaw) {
    std::string s(tituloRaw ? tituloRaw : "");
    s = trim(s);
    return s;
}

// Hash FNV-1a 32
static uint32_t fnv1a32(const std::string& s) {
    const uint32_t FNV_PRIME = 16777619u;
    uint32_t hash = 2166136261u;
    for (unsigned char c : s) {
        hash ^= c;
        hash *= FNV_PRIME;
    }
    return hash;
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

// Função para busca usando B+Tree
bool search_bplus_index(BPlusTree<long>& idx, const std::string& titulo_buscado) {
    std::string norm = normalize(titulo_buscado.c_str());
    if (norm.empty()) {
        logWarn("Titulo vazio.");
        return false;
    }
    int key = static_cast<int>(fnv1a32(norm));

    long leafOffset = idx.search(key);
    if (leafOffset == 0) {
        logWarn("Titulo nao encontrado no indice secundario.");
        return false;
    }

    std::vector<long> results = idx.searchAll(key);
    if (results.empty()) {
        logWarn("Titulo nao encontrado no indice secundario.");
        return false;
    }

    logInfo("Encontradas " + std::to_string(results.size()) + " ocorrencias!");

    for (size_t idxRes = 0; idxRes < results.size(); idxRes++) {
        long ridOffset = results[idxRes];
        std::cout << "\n--- Resultado " << (idxRes + 1) << " ---\n";

        std::ifstream idxFile(NOME_ARQUIVO_INDICE_SEC, std::ios::binary);  // MODIFICADO
        if (!idxFile.is_open()) {
            logError("Nao foi possivel abrir arquivo de indice.");
            continue;
        }

        long actualRID;
        idxFile.seekg(ridOffset);
        if (!idxFile.read(reinterpret_cast<char*>(&actualRID), sizeof(long))) {
            logError("Nao foi possivel ler RID do indice (offset=" + std::to_string(ridOffset) + ").");
            idxFile.close();
            continue;
        }
        idxFile.close();

        std::cout << "RID=" << actualRID << std::endl;

        std::ifstream dataFile(NOME_ARQUIVO_DADOS, std::ios::binary);  // MODIFICADO
        if (dataFile.is_open()) {
            size_t articleIndex = static_cast<size_t>(actualRID);
            size_t blockIndex = articleIndex / REGISTROS_POR_BLOCO;
            size_t positionInBlock = articleIndex % REGISTROS_POR_BLOCO;

            Bloco bloco{};
            dataFile.seekg(blockIndex * sizeof(Bloco));
            if (dataFile.read(reinterpret_cast<char*>(&bloco), sizeof(Bloco))) {
                if (positionInBlock < REGISTROS_POR_BLOCO && positionInBlock < bloco.num_registros_usados) {
                    ArticleDisk& art = bloco.artigos[positionInBlock];

                    if (art.ocupado) {
                        std::cout << "ID: " << art.id << std::endl;
                        std::cout << "Titulo: " << fixEncoding(art.titulo) << std::endl;
                        std::cout << "Ano: " << art.ano << std::endl;
                        std::cout << "Autores: " << fixEncoding(art.autores) << std::endl;
                        std::cout << "Atualizacao: " << art.atualizacao << std::endl;
                        std::cout << "Citacoes: " << art.citacoes << std::endl;
                        std::cout << "Snippet: " << fixEncoding(art.snippet) << std::endl;
                    } else {
                        logWarn("Registro nao ocupado (RID=" + std::to_string(actualRID) + ").");
                    }
                } else {
                    logError("Posicao invalida no bloco (posicao=" + std::to_string(positionInBlock) + ", registros_usados=" + std::to_string(bloco.num_registros_usados) + ").");
                }
            } else {
                logError("Nao foi possivel ler bloco do arquivo (blockIndex=" + std::to_string(blockIndex) + ").");
            }
            dataFile.close();
        } else {
            logError("Nao foi possivel abrir arquivo de dados.");
        }
    }

    return true;
}

int main(int argc, char* argv[]){
    setLogLevelFromEnv();
    if (argc < 2) {
        logError("Uso: " + std::string(argv[0]) + " <Titulo>");
        return 1;
    }
    std::setlocale(LC_ALL, "en_US.UTF-8");

    std::string titulo;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) titulo.push_back(' ');
        titulo.append(argv[i]);
    }
    logInfo("Buscando titulo: '" + titulo + "'");

    BPlusTree<long> idx(NOME_ARQUIVO_INDICE_SEC);  // MODIFICADO
    idx.resetStats();
    search_bplus_index(idx, titulo);
    std::cout << "Blocos da árvore lidos: " << idx.getPagesRead() << std::endl;
    return 0;
}