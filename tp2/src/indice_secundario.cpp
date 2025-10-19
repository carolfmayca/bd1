#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include "btree.hpp"              // sua B+-tree (sem mudanças)
#include "sec_index_types.hpp"    // ArticleDisk, normalize, fnv1a32, RID

#pragma pack(push,1)
struct SecIdxHeader { uint32_t magic=0x54495853, version=1, blockSize=4096, recSize=12; uint64_t count=0; };
struct SecIdxEntry  { uint32_t key; int64_t rid; };
#pragma pack(pop)

int main() {
    // 1) construir B+-tree em RAM como você já faz (se quiser manter)
    //    e, ao mesmo tempo, acumular os pares (key,rid) num vetor
    std::ifstream in("data/artigos.dat", std::ios::binary);
    if (!in) { std::cerr << "Erro abrindo artigos.dat\n"; return 1; }

    BPlusTree</*seu payload*/int> dummyTree; // se você ainda quiser montar a árvore
    // ^ se a sua B+-tree usa outro T (ex.: PostingList) mantenha como está, não importa aqui

    const size_t recSize = sizeof(ArticleDisk);
    ArticleDisk art{};
    std::vector<SecIdxEntry> entries;
    entries.reserve(10000); // só pra reduzir realocações (opcional)

    size_t i = 0;
    while (in.read(reinterpret_cast<char*>(&art), recSize)) {
        // if (!art.ocupado) { i++; continue; }  // se usar ocupado
        if (art.id != 0) {
            RID rid = static_cast<RID>(i) * static_cast<RID>(recSize);
            std::string norm = normalize(art.titulo);
            uint32_t key = fnv1a32(norm);

            // (opcional) também insere na sua B+-tree em RAM aqui, como já fazia
            // dummyTree.insert((int)key, ...payload...);

            entries.push_back(SecIdxEntry{ key, rid });
        }
        ++i;
    }
    in.close();

    // 2) ordenar por key (para poder fazer busca binária depois)
    std::sort(entries.begin(), entries.end(),
              [](const SecIdxEntry& a, const SecIdxEntry& b){
                  if (a.key != b.key) return a.key < b.key;
                  return a.rid < b.rid;
              });

    // 3) gravar idx_title.dat
    std::ofstream out("idx_title.dat", std::ios::binary | std::ios::trunc);
    if (!out) { std::cerr << "Erro criando idx_title.dat\n"; return 1; }

    SecIdxHeader hdr;
    hdr.count = entries.size();
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(reinterpret_cast<const char*>(entries.data()),
              static_cast<std::streamsize>(entries.size() * sizeof(SecIdxEntry)));
    out.close();

    std::cout << "idx_title.dat criado com " << hdr.count << " entradas.\n";
    return 0;
}
