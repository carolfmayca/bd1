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
string cleanField(string field) {
    if (!field.empty() && field.front() == '"') field.erase(0, 1);
    if (!field.empty() && field.back() == '"') field.pop_back();
    return field;
}

// parser para uma linha do CSV
Article parseCSVLine(const string& line) {
    Article article;
    string field;
    stringstream ss(line);
    vector<string> fields;

    bool insideQuotes = false;
    string current;
    for (char character : line) {
        if (character == '"') {
            insideQuotes = !insideQuotes;
            continue;
        }
        if (character == ';' && !insideQuotes) {
            fields.push_back(current);
            current.clear();
        } else {
            current += character;
        }
    }
    fields.push_back(current); // último campo

    if (fields.size() < 7) {
        cerr << "[ERRO] Linha inválida (campos insuficientes): " << line << endl;
        throw runtime_error("Linha CSV inválida");
    }

    try {
        article.id = stoi(fields[0]);
    } catch (...) {
        cerr << "[ERRO] ID inválido: '" << fields[0] << "' em linha: " << line << endl;
        throw;
    }

    article.title = fields[1];

    try {
        article.year = stoi(fields[2]);
    } catch (...) {
        cerr << "[ERRO] Ano inválido: '" << fields[2] << "' em linha: " << line << endl;
        throw;
    }

    article.authors = fields[3];

    try {
        article.citations = stoi(fields[4]);
    } catch (...) {
        cerr << "[ERRO] Citações inválidas: '" << fields[4] << "' em linha: " << line << endl;
        article.citations = 0;
    }

    article.datetime = fields[5];
    article.snippet = fields[6];
    return article;
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
    string indexPath = dbDir + "/primary_index.dat";

    cout << "[INFO] Lendo arquivo CSV: " << csvPath << endl;
    cout << "[INFO] Criando índice primário em memória..." << endl;

    BPlusTree<long> index;
    string line;
    long offset = 0;
    int count = 0;

    while (getline(file, line)) {
        if (line.empty()) continue;
        Article article = parseCSVLine(line);
        index.insert(article.id, reinterpret_cast<long*>(offset));
        offset += line.size();
        count++;
    }

    cout << "[INFO] Total de registros processados: " << count << endl;
    savePrimaryIndex(index, indexPath);
    cout << "[INFO] Índice salvo em: " << indexPath << endl;
    cout << "[SUCESSO] Upload do índice primário concluído!" << endl;

    return 0;
}
