#include "BPlusTree.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <vector>
#include <locale>


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

// Função para busca usando B+Tree
bool search_bplus_index(BPlusTree<long>& idx, const std::string& titulo_buscado) {
    std::string norm = normalize(titulo_buscado.c_str());
    if (norm.empty()) {
        std::cout << "Titulo vazio.\n";
        return false;
    }
    int key = static_cast<int>(fnv1a32(norm));

    long leafOffset = idx.search(key);
    
    if (leafOffset == 0) {
        std::cout << "Titulo nao encontrado no indice secundario.\n";
        return false;
    }

    std::vector<long> results = idx.searchAll(key);
    
    if (results.empty()) {
        std::cout << "Titulo nao encontrado no indice secundario.\n";
        return false;
    }
    
    std::cout << "Encontradas " << results.size() << " ocorrencias!\n";
    
    for (size_t idx = 0; idx < results.size(); idx++) {
        long ridOffset = results[idx];
        
        std::cout << "\n--- Resultado " << (idx + 1) << " ---\n";
        
        std::ifstream idxFile("/app/bin/sec_index.idx", std::ios::binary);
        if (!idxFile.is_open()) {
            std::cout << "ERRO: Nao foi possivel abrir arquivo de indice.\n";
            continue;
        }
        
        long actualRID;
        idxFile.seekg(ridOffset);
        if (!idxFile.read(reinterpret_cast<char*>(&actualRID), sizeof(long))) {
            std::cout << "ERRO: Nao foi possivel ler RID do indice (offset=" << ridOffset << ").\n";
            idxFile.close();
            continue;
        }
        idxFile.close();
        
        std::cout << "RID=" << actualRID << std::endl;
        
        std::ifstream dataFile("/data/artigos.dat", std::ios::binary);
        if (dataFile.is_open()) {
            size_t articleIndex = static_cast<size_t>(actualRID);
            size_t blockIndex = articleIndex / REGISTOS_POR_BLOCO;
            size_t positionInBlock = articleIndex % REGISTOS_POR_BLOCO;
            
            Bloco bloco{};
            dataFile.seekg(blockIndex * sizeof(Bloco));
            if (dataFile.read(reinterpret_cast<char*>(&bloco), sizeof(Bloco))) {
                if (positionInBlock < REGISTOS_POR_BLOCO && positionInBlock < bloco.num_registos_usados) {
                    ArticleDisk& art = bloco.artigos[positionInBlock];
                    
                    if (art.ocupado) {
                        std::cout << "ID: " << art.id << std::endl;
                        std::cout << "Titulo: " << art.titulo << std::endl;
                        std::cout << "Ano: " << art.ano << std::endl;
                        std::cout << "Autores: " << art.autores << std::endl;
                        std::cout << "Atualizacao: " << art.atualizacao << std::endl;
                        std::cout << "Citacoes: " << art.citacoes << std::endl;
                        std::cout << "Snippet: " << art.snippet << std::endl;
                    } else {
                        std::cout << "ERRO: Registro nao ocupado (RID=" << actualRID << ").\n";
                    }
                } else {
                    std::cout << "ERRO: Posicao invalida no bloco (posicao=" << positionInBlock << ", registos_usados=" << bloco.num_registos_usados << ").\n";
                }
            } else {
                std::cout << "ERRO: Nao foi possivel ler bloco do arquivo (blockIndex=" << blockIndex << ").\n";
            }
            dataFile.close();
        } else {
            std::cout << "ERRO: Nao foi possivel abrir arquivo de dados.\n";
        }
    }
    
    return true;
}

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <Titulo>\n";
        return 1;
    }
    std::setlocale(LC_ALL, "en_US.UTF-8");


    std::string titulo;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) titulo.push_back(' ');
        titulo.append(argv[i]);
    }
    std::cout << "Buscando titulo: '" << titulo << "'\n";

    BPlusTree<long> idx("/app/bin/sec_index.idx");
    idx.resetStats();
    search_bplus_index(idx, titulo);
    std::cout << "Blocos da árvore lidos: " << idx.getPagesRead() << std::endl;
    return 0;
}