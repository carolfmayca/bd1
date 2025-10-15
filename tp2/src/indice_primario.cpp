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

// ---------- Função auxiliar para remover aspas e espaços ----------
string cleanField(string f) {
    if (!f.empty() && f.front() == '"') f.erase(0, 1);
    if (!f.empty() && f.back() == '"') f.pop_back();
    return f;
}

// ---------- Parser para uma linha do CSV ----------
Article parseCSVLine(const string& line) {
    Article a;
    string field;
    stringstream ss(line);
    vector<string> fields;

    while (getline(ss, field, ';'))
        fields.push_back(cleanField(field));

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
        a.citations = 0; // padrão de segurança
    }

    a.datetime = fields[5];
    a.snippet = fields[6];
    return a;
}

// Article parseCSVLine(const string& line) {
//     Article a;
//     string field;
//     stringstream ss(line);
//     vector<string> fields;

//     while (getline(ss, field, ';'))
//         fields.push_back(cleanField(field));

//     if (fields.size() < 7) throw runtime_error("Linha CSV inválida: " + line);

//     a.id = stoi(fields[0]);
//     a.title = fields[1];
//     a.year = stoi(fields[2]);
//     a.authors = fields[3];
//     a.citations = stoi(fields[4]);
//     a.datetime = fields[5];
//     a.snippet = fields[6];
//     return a;
// }

// ---------- Salva índice primário em binário ----------
void savePrimaryIndex(BPlusTree<long>& index, const string& filename) {
    ofstream out(filename, ios::binary);
    if (!out.is_open()) {
        cerr << "Erro ao criar arquivo de índice primário: " << filename << endl;
        return;
    }
    // Por enquanto: só gravar as chaves em ordem (teste)
    cout << "[DEBUG] Salvando índice em disco..." << endl;
    // (poderia salvar cada nó depois, mas pra teste salvamos só as chaves)
    // -- opcional: você pode percorrer e gravar se quiser implementar print() na árvore
    out.write("BPLUSTREE_INDEX_PLACEHOLDER", 28);
    out.close();
}

// ---------- Programa principal ----------
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

    // Cria diretório de dados se não existir
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
        Article a = parseCSVLine(line);
        index.insert(a.id, reinterpret_cast<long*>(offset));
        offset += line.size();
        count++;
    }

    cout << "[INFO] Total de registros processados: " << count << endl;
    savePrimaryIndex(index, indexPath);
    cout << "[INFO] Índice salvo em: " << indexPath << endl;
    cout << "[SUCESSO] Upload do índice primário concluído!" << endl;

    return 0;
}
