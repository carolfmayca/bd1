#include "BPlusTree.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdint>

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

// Função para busca usando B+Tree
bool search_bplus_index(BPlusTree<long>& idx, const std::string& titulo_buscado) {
    std::string norm = normalize(titulo_buscado.c_str());
    if (norm.empty()) {
        std::cout << "Titulo vazio.\n";
        return false;
    }
    int key = static_cast<int>(fnv1a32(norm));

    // Busca na B+Tree - para índice secundário, deveria retornar o RID diretamente
    long leafOffset = idx.search(key);
    
    if (leafOffset == 0) {
        std::cout << "Titulo nao encontrado no indice secundario.\n";
        return false;
    }

    // O search() retorna o offset do nó folha, não o RID
    // Precisamos usar searchAll() ou modificar a busca para retornar o valor correto
    std::vector<long> results = idx.searchAll(key);
    
    if (results.empty()) {
        std::cout << "Titulo nao encontrado no indice secundario.\n";
        return false;
    }
    
    // Para um índice secundário, os resultados são offsets onde os RIDs estão armazenados
    long ridOffset = results[0];
    
    // Ler o RID real do arquivo de índice
    std::ifstream idxFile("../data/db/sec_index.dat", std::ios::binary);
    if (!idxFile.is_open()) {
        std::cout << "Erro ao abrir arquivo de indice.\n";
        return false;
    }
    
    long actualRID;
    idxFile.seekg(ridOffset);
    if (!idxFile.read(reinterpret_cast<char*>(&actualRID), sizeof(long))) {
        std::cout << "Erro ao ler RID do indice.\n";
        idxFile.close();
        return false;
    }
    idxFile.close();
    
    std::cout << "Encontrado! RID=" << actualRID << std::endl;
    
    // Busca o registro no arquivo de dados usando estrutura de blocos
    std::ifstream dataFile("../data/db/artigos.dat", std::ios::binary);
    if (dataFile.is_open()) {
        // RID agora é o índice do artigo
        size_t articleIndex = static_cast<size_t>(actualRID);
        size_t blockIndex = articleIndex / REGISTOS_POR_BLOCO;
        size_t positionInBlock = articleIndex % REGISTOS_POR_BLOCO;
        
        // Ler o bloco correto
        Bloco bloco{};
        dataFile.seekg(blockIndex * sizeof(Bloco));
        if (dataFile.read(reinterpret_cast<char*>(&bloco), sizeof(Bloco))) {
            // Verificar se a posição é válida no bloco
            if (positionInBlock < REGISTOS_POR_BLOCO && positionInBlock < bloco.num_registos_usados) {
                ArticleDisk& art = bloco.artigos[positionInBlock];
                
                if (art.ocupado) {
                    std::cout << "ID: " << art.id << std::endl;
                    std::cout << "Titulo: " << art.titulo << std::endl;
                    std::cout << "Ano: " << art.ano << std::endl;
                    std::cout << "Autores: " << art.autores << std::endl;
                    std::cout << "Atualizacao: " << art.atualizacao << std::endl;
                    std::cout << "Citacoes: " << art.citacoes << std::endl;
                } else {
                    std::cout << "Registro nao ocupado.\n";
                }
            } else {
                std::cout << "Posicao invalida no bloco.\n";
            }
        } else {
            std::cout << "Erro ao ler bloco do arquivo.\n";
        }
        dataFile.close();
    } else {
        std::cout << "Erro ao abrir arquivo de dados.\n";
    }
    
    return true;
}

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <Titulo>\n";
        return 1;
    }

    std::string titulo;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) titulo.push_back(' ');
        titulo.append(argv[i]);
    }
    std::cout << "Buscando titulo: '" << titulo << "'\n";

    BPlusTree<long> idx("../data/db/sec_index.dat");
    search_bplus_index(idx, titulo);
    return 0;
}