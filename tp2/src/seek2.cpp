//carole
#include <iostream>
#include <fstream>
#include "btree.hpp"

// Assumindo que você já tem:
//  - normalize(const char*)
//  - fnv1a32(const std::string&)
//  - struct ArticleDisk (layout “POD” igual ao gravado)
//  - using RID = int64_t;
//  - struct PostingList { std::string title_norm; std::vector<RID> rids; };

void seek2(BPlusTree<PostingList>& idxSec, const char* tituloBuscado) {
    std::string norm = normalize(tituloBuscado);
    int key = static_cast<int>(fnv1a32(norm));

    PostingList* pl = idxSec.searchPayload(key);
    if (!pl) {
        std::cout << "Nenhum artigo com esse titulo.\n";
        return;
    }

    std::ifstream in("artigos.dat", std::ios::binary);
    if (!in) {
        std::cerr << "Erro abrindo artigos.dat\n";
        return;
    }

    const std::size_t recSize = sizeof(ArticleDisk);
    ArticleDisk art;
    int encontrados = 0;

    for (RID rid : pl->rids) {
        in.seekg(rid);
        if (!in.read(reinterpret_cast<char*>(&art), recSize)) continue;

        // confirma o título (resolve colisões raras do hash)
        if (normalize(art.titulo) == norm) {
            std::cout << "[OK] offset=" << rid
                      << " id=" << art.id
                      << " titulo=" << art.titulo << "\n";
            encontrados++;
        }
    }

    if (encontrados == 0) {
        std::cout << "Nenhum artigo com esse titulo (apos confirmacao).\n";
    }
}

int main() {
    // 1) abrir dados
    std::ifstream in("artigos.dat", std::ios::binary);
    if (!in) { std::cerr << "sem artigos.dat\n"; return 1; }

    // 2) criar B+-tree em memória
    BPlusTree<PostingList> idxSec;

    // 3) varrer e inserir: título -> RID
    const std::size_t recSize = sizeof(ArticleDisk);
    ArticleDisk art;
    std::size_t i = 0;

    while (in.read(reinterpret_cast<char*>(&art), recSize)) {
        // se você ainda usa 'ocupado', pule se vazio:
        // if (!art.ocupado) { i++; continue; }

        if (art.id != 0) {
            RID rid = static_cast<RID>(i) * static_cast<RID>(recSize);
            std::string norm = normalize(art.titulo);
            int key = static_cast<int>(fnv1a32(norm));

            auto* pl = new PostingList;
            pl->title_norm = norm;
            pl->rids.push_back(rid);

            idxSec.insert(key, pl); // API pública (2 parâmetros)
        }
        i++;
    }
    in.close();

    std::cout << "Indice secundario construido.\n";

    // 4) teste de busca
    seek2(idxSec, "Digite aqui um titulo que exista no seu CSV");
    return 0;
}
