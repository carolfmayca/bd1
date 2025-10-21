#pragma once

#include <fstream>
#include <iostream>
#include <cstring>
#include <string>

#define BLOCK_SIZE 4096 // padrão SO

// Forward declaration
template <typename T, int M_VALUE> class BPlusTree;

class FileManager {
private:
    std::fstream file;
    long nextFreeOffset; // Próximo offset livre no arquivo

    // Estrutura de cabeçalho do arquivo
    struct FileHeader {
        long rootOffset;
        long nextFreeOffset;
        int m;
    } header;

public:
    FileManager(const std::string& filename, int treeM) : header({0, sizeof(FileHeader), treeM}) {
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        if (!file.is_open()) {
            // Se o arquivo não existe, cria um novo
            file.clear();
            file.open(filename, std::ios::out | std::ios::binary);
            file.close();
            
            // Reabre para leitura/escrita
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
            
            // Escreve o cabeçalho inicial
            writeHeader();
        } else {
            // Se existe, lê o cabeçalho
            readHeader();
        }
        nextFreeOffset = header.nextFreeOffset;
    }

    ~FileManager() {
        writeHeader(); // Salva o cabeçalho atualizado ao fechar
        file.close();
    }
    // Nova função para atualizar o offset da raiz (rootOffset não é mais estático)
    void updateRootOffset(long newRootOffset) {
        header.rootOffset = newRootOffset;
        writeHeader(); // Salva imediatamente no disco
    }
    
    // Novo método para acessar o cabeçalho
    const FileHeader& getHeader() const {
        return header;
    }

    void readHeader() {
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
        nextFreeOffset = header.nextFreeOffset;
    }

    void writeHeader() {
        header.nextFreeOffset = nextFreeOffset;
        file.seekp(0, std::ios::beg);
        file.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));
    }
    
    // Aloca espaço e retorna o offset
    long getNewOffset() {
        long offset = nextFreeOffset;
        nextFreeOffset += BLOCK_SIZE;
        return offset;
    }

    // Leitura de um nó do arquivo
    template <typename T, int M_VALUE>
    bool readNode(long offset, typename BPlusTree<T, M_VALUE>::BPlusTreeNode& node) {
        if (offset == 0) return false;
        
        file.seekg(offset, std::ios::beg);
        // Usar o tamanho real da estrutura 
        file.read(reinterpret_cast<char*>(&node), sizeof(typename BPlusTree<T, M_VALUE>::BPlusTreeNode)); 
        return file.good();
    }

    // Escrita de um nó no arquivo
    template <typename T, int M_VALUE>
    void writeNode(long offset, const typename BPlusTree<T, M_VALUE>::BPlusTreeNode& node) {
        file.seekp(offset, std::ios::beg);
        // Usar o tamanho real da estrutura
        file.write(reinterpret_cast<const char*>(&node), sizeof(typename BPlusTree<T, M_VALUE>::BPlusTreeNode));
        file.flush(); // Garantir que está escrito no disco
    }
    
    // Método para escrever dados arbitrários no arquivo
    template <typename T>
    void writeData(long offset, const T* data) {
        file.seekp(offset, std::ios::beg);
        file.write(reinterpret_cast<const char*>(data), sizeof(T));
        file.flush();
    }
};

template <typename T, int M_VALUE = 100>
class BPlusTree {
public:
    struct BPlusTreeNode {
        int keys [2 * M_VALUE]; // máximo de chaves
        int numKeys;
        bool isLeaf;
        long childrenOffsets[2 * M_VALUE + 1]; // Para 2*M chaves, precisa de 2*M+1 ponteiros
        long nextLeafOffset;
    };

protected:
    static constexpr int m = M_VALUE; // ordem da árvore
    
public:
    BPlusTree(const std::string& filename);
    ~BPlusTree();

    void insert(int key, T *data);
    long search(int k); 
    int getM() const { return m; }
    void resetStats() { fileManager->resetStats(); }
    std::size_t getPagesRead() const { return fileManager->getPagesRead(); }


private:
    long rootOffset;
    FileManager *fileManager;

    void insert(int key, long dataOffset, long nodeOffset); // recursiva

    // utilidades
    long newNode(bool leaf);
    static int upperBound(const int *arr, int n, int key);
    static int lowerBound(const int *arr, int n, int key);
    void splitLeaf(long nodeOffset, int key, long dataOffset);
    void splitInternal(long parentOffset, int promoteKey, long rightChildOffset);
    void insertInternal(int key, long parentOffset, long childOffset);
    long findParent(long subrootOffset, long childOffset);
};