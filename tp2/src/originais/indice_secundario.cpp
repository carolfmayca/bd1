#include "BPlusTree.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <vector>
#include <chrono>

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

const int REGISTOS_POR_BLOCO = 2;

struct Bloco {
    ArticleDisk artigos[REGISTOS_POR_BLOCO];
    int num_registos_usados;
    long proximo_bloco_offset;
};

static inline std::string trim(const std::string& s) {
    std::size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

static inline std::string tolower_copy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return s;
}

static std::string normalize(const char* tituloRaw) {
    std::string s(tituloRaw ? tituloRaw : "");
    s = trim(s);
    s = tolower_copy(s);
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

// Estrutura para coleta de dados antes da inserção
struct IndexEntry {
    int key;
    long rid;
};

static bool build_secondary_index_bplus(BPlusTree<long>& idx) {
    std::ifstream in("data/db/artigos.dat", std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Erro: nao foi possivel abrir data/db/artigos.dat para leitura.\n";
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
        for (int i = 0; i < REGISTOS_POR_BLOCO && i < bloco.num_registos_usados; i++) {
            ArticleDisk& art = bloco.artigos[i];
            long rid = static_cast<long>(blocoIndex * REGISTOS_POR_BLOCO + i);
            
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
            std::cout << "Coletados: " << totalRegistrosProcessados << " registros em " << elapsed << "s..." << std::endl;
        }
    }
    in.close();

    std::cout << "Ordenando " << entries.size() << " entradas..." << std::endl;
    
    std::sort(entries.begin(), entries.end(), 
              [](const IndexEntry& a, const IndexEntry& b) {
                  return a.key < b.key;
              });

    std::cout << "Inserindo na B+Tree (M=102)..." << std::endl;
    auto insert_start = std::chrono::high_resolution_clock::now();
    
    const size_t MEGA_BATCH_SIZE = 100000; // Lotes de 100k para máxima eficiência
    
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

    std::cout << "=== INDICE B+TREE CRIADO ===" << std::endl;
    std::cout << "Tempo total: " << total_time << " segundos" << std::endl;
    std::cout << "Chaves inseridas: " << chavesInseridas << std::endl;
    std::cout << "Total de registros processados: " << totalRegistrosProcessados << std::endl;
    std::cout << "Registros com titulos vazios: " << registrosVazios << std::endl;
    
    return true;
}

int main(int argc, char* argv[]) {
    BPlusTree<long> idx("data/db/sec_index.dat");
    return build_secondary_index_bplus(idx) ? 0 : 1;
}