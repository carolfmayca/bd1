#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "btree.hpp"

using namespace std;
namespace fs = std::filesystem;

struct ArticleDisk {
    int32_t id;
    char    title[301];      // titulo + '\0'
    int32_t year;
    char    authors[150];
    int32_t citacoes;
    char    atualizacao[20]; // "YYYY-MM-DD HH:MM:SS" + '\0'
    char    snippet[1025]; // resumo + '\0'
};

using RID = int64_t;
struct PostingList {
    std::string title_norm;      // para conferir colisão de hash
    std::vector<RID> rids;       // todos os offsets desse título
};



std::string normalize(const char* t) {
    std::string s(t ? t : "");
    auto notsp = [](unsigned char c){ return !std::isspace(c); };
    auto b = std::find_if(s.begin(), s.end(), notsp);
    auto e = std::find_if(s.rbegin(), s.rend(), notsp).base();
    s = (b < e) ? std::string(b, e) : std::string();
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

uint32_t fnv1a32(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}

int main(){
    const string DATA_FILE = "artigos.dat";
    const string INDEX_FILE = "indice_secundario.dat";

    fs:: remove(INDEX_FILE); // remove índice antigo para teste limpo
    BPlusTree<int> btree; // árvore B+ para índice secundário
    fstream dataFile(DATA_FILE, ios::in | ios::out | ios::binary); // lê arquivo de dados
    if (!dataFile.is_open()) {
        cerr << "Erro: Nao foi possivel abrir o arquivo de dados '" << DATA_FILE << "'" << endl;
        return 1;
    }

    // Carrega todos os artigos do arquivo de dados e insere na árvore B+
    dataFile.seekg(0, ios::end);
    streampos fileSize = dataFile.tellg();
    dataFile.seekg(0, ios::beg);
    const int recordSize = sizeof(ArticleDisk);

    ArticleDisk art;

    for (int i = 0; i < numRecords; i++) {
        dataFile.read(reinterpret_cast<char*>(&art), recordSize);
        if (!dataFile) break;

        if (art.id == 0) continue; // pula vazios
        int64_t rid = int64_t(i) * recordSize; // OFFSET no artigos.dat
        std::string norm = normalize(art.title);
        uint32_t key = fnv1a32(norm);
            
        // cria uma posting list com um único RID
        auto* pl = new PostingList;
        pl->title_norm = norm;
        pl->rids.push_back(rid);
            
        // insere na B+-tree
        btree.insert((int)key, pl);
            
    }

    dataFile.close();
}
