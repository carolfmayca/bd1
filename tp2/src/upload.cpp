#include "hashing_file.h"
#include "BPlusTree.hpp"
#include "config.h"  // NOVO: inclui configurações
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <algorithm>
#include <vector>

const int TAMANHO_TABELA_HASH = 10000;

// Estrutura para coleta de dados antes da inserção
struct IndexEntry {
    int key;
    long rid;
};

// ====== FUNÇÕES DE APOIO ====
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
    campos.push_back(trimQuotes(campo_atual));
    return campos;
}

// === FUNÇÕES DE NORMALIZAÇÃO ====
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

static bool insereHashing(){
    std::ifstream csvFile(ARTIGO_CSV);
    if (!csvFile.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo CSV '" << ARTIGO_CSV << "'" << std::endl;
        return false;
    }

    std::cout << "\n--- Lendo arquivo " << ARTIGO_CSV << " e inserindo dados ---" << std::endl;
    std::string linha;
    int registrosInseridos = 0;
    int numeroLinha = 0;
    HashingFile arquivoHash(ARTIGO_DAT, TAMANHO_TABELA_HASH);

    auto start = std::chrono::high_resolution_clock::now();

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
                if (campos[6] != "NULL") {
                    strncpy(art.snippet, campos[6].c_str(), 1024);
                }

                arquivoHash.inserirArtigo(art);
                registrosInseridos++;

                if (registrosInseridos > 0 && registrosInseridos % 50000 == 0) {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
                    std::cout << "[LOG] " << registrosInseridos << " registros inseridos no hashing... (" << elapsed << "s)" << std::endl;
                }

            } catch (const std::exception& e) {
                std::cerr << "--> ERRO DE CONVERSAO na linha " << numeroLinha << ". Verifique os campos numericos. Erro: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "--> AVISO: Linha " << numeroLinha << " ignorada. Esperava 7 campos, mas encontrou " << campos.size() << "." << std::endl;
            std::cerr << "    Conteudo da linha: " << linha << std::endl;
        }
    }
    std::cout << "--- Insercao finalizada. " << registrosInseridos << " registros inseridos. ---\n" << std::endl;
    csvFile.close();
    return true;
}

static bool insereIdxPrim(){
    BPlusTree<long> idx(PRIM_INDEX);

    std::ifstream in(ARTIGO_DAT, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[ERRO] Não foi possível abrir " << ARTIGO_DAT << "\n";
        return false;
    }

    const std::size_t blocoSize = sizeof(Bloco);
    Bloco bloco{};
    std::size_t blocoIndex = 0;
    std::size_t totalInseridos = 0;

    std::vector<IndexEntry> entries;
    entries.reserve(1200000);

    std::cout << "[INFO] Coletando registros..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    // coletar o RID e ID
    while (in.read(reinterpret_cast<char*>(&bloco), blocoSize)) {
        for (int i = 0; i < bloco.num_registros_usados; i++) {
            Artigo& art = bloco.artigos[i];
            if (!art.ocupado) continue;

            long rid = static_cast<long>(blocoIndex * REGISTROS_POR_BLOCO + i);
            entries.push_back({art.id, rid});
        }
        blocoIndex++;
    }
    in.close();

    std::cout << "[INFO] Ordenando " << entries.size() << " registros..." << std::endl;
    std::sort(entries.begin(), entries.end(), [](auto& a, auto& b){ return a.key < b.key; });

    std::cout << "[INFO] Inserindo na B+Tree..." << std::endl;

    auto insert_start = std::chrono::high_resolution_clock::now();
    const size_t MEGA_BATCH_SIZE = 100000;

    for (size_t start_idx = 0; start_idx < entries.size(); start_idx += MEGA_BATCH_SIZE) {
        size_t end_idx = std::min(start_idx + MEGA_BATCH_SIZE, entries.size());

        for (size_t j = start_idx; j < end_idx; ++j) {
            idx.insert(entries[j].key, &entries[j].rid);
            totalInseridos++;
        }

        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - insert_start).count();

        std::cout << end_idx << "/" << entries.size()
                  << " (" << elapsed << "s) - "
                  << (end_idx * 100 / entries.size()) << "%" << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsedTotal = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    std::cout << "\n[SUCESSO] Índice primário criado!\n";
    std::cout << "Total de chaves inseridas: " << totalInseridos << std::endl;
    std::cout << "Tempo total: " << elapsedTotal << " segundos\n" << std::endl;

    return true;
}


static bool insereIdxSec(){
    BPlusTree<long> idx(SEC_INDEX);

    std::ifstream in(ARTIGO_DAT, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Erro: nao foi possivel abrir " << ARTIGO_DAT << " para leitura.\n";
        return false;
    }

    const std::size_t blocoSize = sizeof(Bloco);
    Bloco bloco{};
    std::size_t blocoIndex = 0;
    std::size_t chavesInseridas = 0;
    std::size_t registrosVazios = 0;
    std::size_t totalRegistrosProcessados = 0;

    std::vector<IndexEntry> entries;
    entries.reserve(1200000);

    std::cout << "Coletando dados para ordenacao..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    int titulosValidosExibidos = 0;
    
    while (in.read(reinterpret_cast<char*>(&bloco), blocoSize)) {
        for (int i = 0; i < REGISTROS_POR_BLOCO && i < bloco.num_registros_usados; i++) {
            Artigo& art = bloco.artigos[i];
            long rid = static_cast<long>(blocoIndex * REGISTROS_POR_BLOCO + i);
            
            totalRegistrosProcessados++;
            
            // Só processa registros ocupados
            if (!art.ocupado) {
                continue;
            }
            
            std::string norm = normalize(art.titulo);
            
            if (norm.empty()) {
                registrosVazios++;
                continue;
            }
            
            int key = static_cast<int>(fnv1a32(norm));
            entries.push_back({key, rid});
            chavesInseridas++;
        }
        
        blocoIndex++;
        
        if (totalRegistrosProcessados % 200000 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        }
    }
    in.close();

    std::cout << "Ordenando " << entries.size() << " entradas..." << std::endl;
    
    std::sort(entries.begin(), entries.end(), 
              [](const IndexEntry& a, const IndexEntry& b) {
                  return a.key < b.key;
              });

    auto insert_start = std::chrono::high_resolution_clock::now();
    
    const size_t MEGA_BATCH_SIZE = 100000;
    
    for (size_t start_idx = 0; start_idx < entries.size(); start_idx += MEGA_BATCH_SIZE) {
        size_t end_idx = std::min(start_idx + MEGA_BATCH_SIZE, entries.size());
        
        for (size_t j = start_idx; j < end_idx; ++j) {
            idx.insert(entries[j].key, &entries[j].rid);
        }

        // inserção em lotes
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - insert_start).count();
        std::cout << end_idx << "/" << entries.size() 
                  << " (" << elapsed << "s) - " 
                  << (end_idx * 100 / entries.size()) << "%" << std::endl;
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(total_end - start).count();

    return true;
}

int main(){
    // Limpa o ambiente
    remove(ARTIGO_DAT.c_str());
    remove(TABELA_HASH.c_str());
    remove(PRIM_INDEX.c_str());
    remove(SEC_INDEX.c_str());

    std::cout << "DATA_DIR: " << DATA_DIR << std::endl;
    std::cout << "BIN_DIR: " << BIN_DIR << std::endl;
    std::cout << "CSV: " << ARTIGO_CSV << std::endl;

    std::cout << "--- Inicio de inserção em hash ---\n";
    if (!insereHashing()) {
        std::cerr << "Erro na insercao via hashing. Abortando.\n";
        return 1;
    }
    std::cout << "--- Inserção em hash realizada com sucesso ---\n";
    std::cout << "\n--- Inicio de insercao do indice primario ---\n";
    if (!insereIdxPrim()) {
        std::cerr << "Erro na insercao do indice primario. Abortando.\n";
        return 1;
    }
    std::cout << "--- Inserção do indice primario realizada com sucesso ---\n";
    std::cout << "\n--- Inicio de insercao do indice secundario ---\n";
    if (!insereIdxSec()) {
        std::cerr << "Erro na insercao do indice secundario. Abortando.\n";
        return 1;
    }
    std::cout << "--- Inserção do indice secundario realizada com sucesso ---\n";

    return 0;
}