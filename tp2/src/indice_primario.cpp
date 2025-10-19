#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "btree.hpp"

using namespace std;
namespace fs = std::filesystem;

struct Article {
    int id;
    string title;
    int year;
    string authors;
    int citations;
    string datetime;
    string snippet;
};

// remover aspas e espaços
string cleanField(string f) {
    if (!f.empty() && f.front() == '"') f.erase(0, 1);
    if (!f.empty() && f.back() == '"') f.pop_back();
    return f;
}

// parser para uma linha do CSV
Article parseCSVLine(const string& line) {
    Article a;
    string field;
    stringstream ss(line);
    vector<string> fields;

    bool insideQuotes = false;
    string current;
    for (char c : line) {
        if (c == '"') {
            insideQuotes = !insideQuotes;
            continue;
        }
        if (c == ';' && !insideQuotes) {
            fields.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(current); // último campo

    if (fields.size() < 7) {
        cerr << "[ERRO] Linha inválida (campos insuficientes): " << line << endl;
        throw runtime_error("Linha CSV inválida");
    }

    try {
        a.id = stoi(fields[0]);
    } catch (...) {
        cerr << "[ERRO] ID inválido: '" << fields[0] << "' em linha: " << line << endl;
        throw;
    }

    a.title = fields[1];

    try {
        a.year = stoi(fields[2]);
    } catch (...) {
        cerr << "[ERRO] Ano inválido: '" << fields[2] << "' em linha: " << line << endl;
        throw;
    }

    a.authors = fields[3];

    try {
        a.citations = stoi(fields[4]);
    } catch (...) {
        cerr << "[ERRO] Citações inválidas: '" << fields[4] << "' em linha: " << line << endl;
        a.citations = 0;
    }

    a.datetime = fields[5];
    a.snippet = fields[6];
    return a;
}

void dumpLeafKeys(std::ofstream& out, typename BPlusTree<long>::BPlusTreeNode* node) {
    if (!node) return;

    if (node->isLeaf) {
        for (int i = 0; i < node->numKeys; i++) {
            int key = node->keys[i];
            long offset = reinterpret_cast<long>(node->children[i]);
            out.write(reinterpret_cast<const char*>(&key), sizeof(int));
            out.write(reinterpret_cast<const char*>(&offset), sizeof(long));
        }
    } else {
        for (int i = 0; i <= node->numKeys; i++) {
            dumpLeafKeys(out, static_cast<typename BPlusTree<long>::BPlusTreeNode*>(node->children[i]));
        }
    }
}

// salva índice primário em binário
void savePrimaryIndex(BPlusTree<long>& index, const string& filename) {
    ofstream out(filename, ios::binary);
    if (!out.is_open()) {
        cerr << "Erro ao criar arquivo de índice primário: " << filename << endl;
        return;
    }

    cout << "[DEBUG] Gravando chaves no arquivo de índice..." << endl;
    dumpLeafKeys(out, reinterpret_cast<BPlusTree<long>::BPlusTreeNode*>(*(void**)&index));
    out.close();
}



int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso: upload <arquivo.csv>" << endl;
        return 1;
    }

    string csvPath = argv[1];
    ifstream file(csvPath);
    if (!file.is_open()) {
        cerr << "Erro ao abrir CSV: " << csvPath << endl;
        return 1;
    }

    // cria diretório de dados se não existir
    string dbDir = "data/db";
    fs::create_directories(dbDir);
    string indexPath = dbDir + "/indice_primario.dat";

    cout << "[INFO] Lendo arquivo CSV: " << csvPath << endl;
    cout << "[INFO] Criando índice primário em memória..." << endl;

    BPlusTree<long> index;
    string line;
    long offset = 0;
    int count = 0;

    while (true) {
        long offset = file.tellg();  
        string line;
        if (!getline(file, line)) break;

        if (line.empty()) continue;

        Article a = parseCSVLine(line);
        index.insert(a.id, reinterpret_cast<long*>(offset));
        count++;
    }


    cout << "[INFO] Total de registros processados: " << count << endl;
    savePrimaryIndex(index, indexPath);
    cout << "[INFO] Índice salvo em: " << indexPath << endl;
    cout << "[SUCESSO] Upload do índice primário concluído!" << endl;

    return 0;
}
