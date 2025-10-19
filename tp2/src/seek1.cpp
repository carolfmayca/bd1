#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstring>
using namespace std;
namespace fs = std::filesystem;

const int BLOCK_SIZE = 4096;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Uso: seek1 <arquivo.csv> <ID>" << endl;
        return 1;
    }

    string csvPath = argv[1];
    int targetID = stoi(argv[2]);

    string indexPath = "data/db/indice_primario.dat";
    ifstream idx(indexPath, ios::binary);
    if (!idx.is_open()) {
        cerr << "Erro ao abrir índice: " << indexPath << endl;
        return 1;
    }

    // tamanho total do arquivo
    idx.seekg(0, ios::end);
    long fileSize = idx.tellg();
    idx.seekg(0, ios::beg);
    int totalBlocks = (fileSize + BLOCK_SIZE - 1) / BLOCK_SIZE;

    int blocksRead = 0;
    bool found = false;
    long foundOffset = -1;

    // leitura sequencial
    while (!idx.eof()) {
        char buffer[BLOCK_SIZE];
        idx.read(buffer, BLOCK_SIZE);
        streamsize bytesRead = idx.gcount();
        blocksRead++;

        for (int pos = 0; pos + 12 <= bytesRead; pos += 12) {
            int id;
            long offset;
            memcpy(&id, buffer + pos, sizeof(int));
            memcpy(&offset, buffer + pos + sizeof(int), sizeof(long));

            if (id == targetID) {
                found = true;
                foundOffset = offset;
                break;
            }
        }
        if (found) break;
    }

    idx.close();

    if (!found) {
        cout << "[ERRO] ID " << targetID << " não encontrado." << endl;
        cout << "Blocos lidos: " << blocksRead << " / " << totalBlocks << endl;
        return 0;
    }

    // lê registro do CSV no offset encontrado
    ifstream csv(csvPath);
    if (!csv.is_open()) {
        cerr << "Erro ao abrir CSV: " << csvPath << endl;
        return 1;
    }

    csv.seekg(foundOffset);
    string line;
    getline(csv, line);

    cout << "[SUCESSO] Registro encontrado!" << endl;
    cout << "Linha: " << line << endl;
    cout << "Offset: " << foundOffset << endl;
    cout << "Blocos lidos: " << blocksRead << " / " << totalBlocks << endl;

    return 0;
}
