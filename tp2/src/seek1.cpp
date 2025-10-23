#include "BPlusTree.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

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

// Função para buscar artigo por ID usando o índice primário
bool search_primary_index(BPlusTree<long>& idx, int idBuscado) {
    std::cout << "Buscando ID: " << idBuscado << std::endl;

    long leafOffset = idx.search(idBuscado);
    if (leafOffset == 0) {
        std::cout << "ID não encontrado no índice primário.\n";
        return false;
    }

    std::vector<long> results = idx.searchAll(idBuscado);
    if (results.empty()) {
        std::cout << "ID não encontrado no índice primário.\n";
        return false;
    }

    // results[0] é o OFFSET no arquivo de índice — precisamos ler o RID armazenado ali
    long actualRID = -1;
    std::ifstream idxFile("data/db/prim_index.dat", std::ios::binary);
    if (!idxFile.is_open()) {
        std::cout << "Erro ao abrir arquivo de indice primario.\n";
        return false;
    }

    idxFile.seekg(0, std::ios::end);
    std::streamoff idxSize = idxFile.tellg();
    if (results[0] < 0 || static_cast<std::streamoff>(results[0]) + sizeof(long) > idxSize) {
        std::cout << "Offset invalido no arquivo de indice primario.\n";
        idxFile.close();
        return false;
    }

    idxFile.seekg(results[0], std::ios::beg);
    if (!idxFile.read(reinterpret_cast<char*>(&actualRID), sizeof(long))) {
        std::cout << "Erro ao ler RID do indice primario.\n";
        idxFile.close();
        return false;
    }
    idxFile.close();

    // --- DEBUG opcional ---
    std::cout << "[DEBUG] Offset=" << results[0] 
              << " RID=" << actualRID 
              << " (bytes=" << sizeof(long) << ")\n";

    std::ifstream dataFile("data/db/artigos.dat", std::ios::binary);
    if (!dataFile.is_open()) {
        std::cout << "Erro ao abrir data/db/artigos.dat.\n";
        return false;
    }

    size_t articleIndex = static_cast<size_t>(actualRID);
    size_t blockIndex = articleIndex / REGISTOS_POR_BLOCO;
    size_t positionInBlock = articleIndex % REGISTOS_POR_BLOCO;

    dataFile.seekg(0, std::ios::end);
    std::streamoff dataSize = dataFile.tellg();
    size_t totalArticles = (dataSize / sizeof(Bloco)) * REGISTOS_POR_BLOCO;

    if (articleIndex >= totalArticles) {
        std::cout << "RID inválido (fora do tamanho do arquivo de dados).\n";
        dataFile.close();
        return false;
    }

    Bloco bloco{};
    dataFile.seekg(blockIndex * sizeof(Bloco));
    if (dataFile.read(reinterpret_cast<char*>(&bloco), sizeof(Bloco))) {
        if (positionInBlock < bloco.num_registos_usados) {
            ArticleDisk& art = bloco.artigos[positionInBlock];
            if (art.ocupado) {
                std::cout << "\n=== ARTIGO ENCONTRADO ===\n";
                std::cout << "ID: " << art.id << std::endl;
                std::cout << "Título: " << art.titulo << std::endl;
                std::cout << "Ano: " << art.ano << std::endl;
                std::cout << "Autores: " << art.autores << std::endl;
                std::cout << "Atualização: " << art.atualizacao << std::endl;
                std::cout << "Citações: " << art.citacoes << std::endl;
            } else {
                std::cout << "Registro não ocupado.\n";
            }
        } else {
            std::cout << "Posição inválida no bloco.\n";
        }
    } else {
        std::cout << "Erro ao ler bloco do arquivo.\n";
    }

    dataFile.close();
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <ID>\n";
        return 1;
    }

    int id = std::stoi(argv[1]);
    BPlusTree<long> idx("data/db/prim_index.dat");
    idx.resetStats();
    search_primary_index(idx, id);
    std::cout << "Blocos da árvore lidos: " << idx.getPagesRead() << std::endl;
    return 0;
}
