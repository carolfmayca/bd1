#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <algorithm>
#include "BPlusTree.cpp"

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

// Estrutura auxiliar para inserção
struct IndexEntry {
    int key;
    long rid;
};

// Função que constrói o índice primário
static bool build_primary_index_bplus(BPlusTree<long>& idx) {
    std::ifstream in("data/db/artigos.dat", std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[ERRO] Não foi possível abrir artigos.dat.\n";
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

    while (in.read(reinterpret_cast<char*>(&bloco), blocoSize)) {
        for (int i = 0; i < bloco.num_registos_usados; i++) {
            ArticleDisk& art = bloco.artigos[i];
            if (!art.ocupado) continue;

            long rid = static_cast<long>(blocoIndex * REGISTOS_POR_BLOCO + i);
            entries.push_back({art.id, rid});
        }
        blocoIndex++;
    }
    in.close();

    std::cout << "[INFO] Ordenando " << entries.size() << " registros..." << std::endl;
    std::sort(entries.begin(), entries.end(), [](auto& a, auto& b){ return a.key < b.key; });

    std::cout << "[INFO] Inserindo na B+Tree..." << std::endl;
    for (auto& e : entries) {
        idx.insert(e.key, &e.rid);
        totalInseridos++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    std::cout << "[SUCESSO] Índice primário criado!" << std::endl;
    std::cout << "Total de chaves inseridas: " << totalInseridos << std::endl;
    std::cout << "Tempo total: " << elapsed << " segundos" << std::endl;

    return true;
}

int main() {
    BPlusTree<long> idx("data/db/prim_index.dat");
    return build_primary_index_bplus(idx) ? 0 : 1;
}
