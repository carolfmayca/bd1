#include <fstream>
#include <iostream>
#include <unordered_set>
#include "sec_index_types.hpp"

#pragma pack(push,1)
struct SecIdxHeader { uint32_t magic, version, blockSize, recSize; uint64_t count; };
struct SecIdxEntry  { uint32_t key; int64_t rid; };
#pragma pack(pop)

static bool readHeader(std::ifstream& f, SecIdxHeader& h) {
    f.seekg(0);
    return static_cast<bool>(f.read(reinterpret_cast<char*>(&h), sizeof(h)));
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: seek2 \"TITULO\"\n";
        return 1;
    }

    const std::string titulo = argv[1];
    const std::string norm   = normalize(titulo.c_str());
    const uint32_t key       = fnv1a32(norm);

    std::ifstream idx("idx_title.dat", std::ios::binary);
    if (!idx) { std::cerr << "Erro abrindo idx_title.dat\n"; return 1; }

    SecIdxHeader hdr{};
    if (!readHeader(idx, hdr) || hdr.recSize != sizeof(SecIdxEntry)) {
        std::cerr << "idx_title.dat inválido\n"; return 1;
    }

    const uint64_t N = hdr.count;
    const uint64_t base = sizeof(SecIdxHeader);
    const uint32_t B = hdr.blockSize ? hdr.blockSize : 4096;

    // binary search do primeiro registro >= key
    int64_t lo = 0, hi = static_cast<int64_t>(N) - 1, first = -1;
    std::unordered_set<uint64_t> blocksTouched; // p/ contar blocos lidos

    auto readEntry = [&](uint64_t i, SecIdxEntry& e) {
        const uint64_t off = base + i * sizeof(SecIdxEntry);
        idx.seekg(off);
        idx.read(reinterpret_cast<char*>(&e), sizeof(e));
        // bloco indexado por off/B
        blocksTouched.insert(off / B);
        return static_cast<bool>(idx);
    };

    SecIdxEntry midE{};
    while (lo <= hi) {
        int64_t mid = (lo + hi) >> 1;
        if (!readEntry(mid, midE)) break;
        if (midE.key < key) lo = mid + 1;
        else { first = mid; hi = mid - 1; }
    }
    if (first == -1) { std::cout << "Nenhum artigo com esse titulo.\n"; return 0; }

    // varre para frente enquanto key igual
    std::vector<RID> rids;
    for (uint64_t i = first; i < N; ++i) {
        SecIdxEntry e{};
        if (!readEntry(i, e)) break;
        if (e.key != key) break;
        rids.push_back(e.rid);
    }

    // confirma títulos no arquivo de dados
    std::ifstream data("artigos.dat", std::ios::binary);
    if (!data) { std::cerr << "Erro abrindo artigos.dat\n"; return 1; }
    const size_t recSize = sizeof(ArticleDisk);
    ArticleDisk art{};
    int encontrados = 0;

    for (RID rid : rids) {
        data.seekg(rid);
        if (!data.read(reinterpret_cast<char*>(&art), recSize)) continue;
        if (normalize(art.titulo) == norm) {
            std::cout << "[OK] offset=" << rid
                      << " id=" << art.id
                      << " titulo=" << art.titulo << "\n";
            encontrados++;
        }
    }

    std::cout << "Blocos lidos do indice: " << blocksTouched.size()
              << " / Total de blocos do indice: "
              << ((base + N*sizeof(SecIdxEntry) + B - 1) / B) << "\n";

    if (encontrados == 0)
        std::cout << "Nenhum artigo com esse titulo (apos confirmacao).\n";
    return 0;
}
