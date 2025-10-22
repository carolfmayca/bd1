#include <fstream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector> // NOVO: Para armazenar múltiplos offsets

#define BLOCK_SIZE 4096 // padrão SO
#define M 102 // Ordem da árvore B+: mínimo M, máximo 2*M chaves

// Forward declaration
template <typename T> class BPlusTree;

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
    mutable std::size_t pagesRead = 0;

public:
    FileManager(const std::string& filename, int treeM) : header({0, 4096, treeM}) {
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

        if (!file.is_open()) {
            // Se o arquivo não existe, cria um novo
            file.clear();
            file.open(filename, std::ios::out | std::ios::binary);
            file.close();
            
            // Reabre para leitura/escrita
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
            
            // Inicializa nextFreeOffset antes de escrever o header
            nextFreeOffset = header.nextFreeOffset;
            // Escreve o cabeçalho inicial
            writeHeader();
        } else {
            // Se existe, lê o cabeçalho
            readHeader();
            nextFreeOffset = header.nextFreeOffset;
        }
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
    template <typename T>
    bool readNode(long offset, typename BPlusTree<T>::BPlusTreeNode& node) {
        if (offset == 0) return false;
        
        file.seekg(offset, std::ios::beg);
        // Usar o tamanho real da estrutura
        file.read(reinterpret_cast<char*>(&node), sizeof(typename BPlusTree<T>::BPlusTreeNode)); 
        pagesRead++; // Incrementa contador de blocos lidos
        return file.good();
    }

    // Escrita de um nó no arquivo
    template <typename T>
    void writeNode(long offset, const typename BPlusTree<T>::BPlusTreeNode& node) {
        file.seekp(offset, std::ios::beg);
        // Usar o tamanho real da estrutura em vez de BLOCK_SIZE
        file.write(reinterpret_cast<const char*>(&node), sizeof(typename BPlusTree<T>::BPlusTreeNode));
        file.flush(); // Garantir que está escrito no disco
    }
    
    // Método para escrever dados arbitrários no arquivo
    template <typename T>
    void writeData(long offset, const T* data) {
        file.seekp(offset, std::ios::beg);
        file.write(reinterpret_cast<const char*>(data), sizeof(T));
        file.flush();
    }
    
    void resetStats() { pagesRead = 0; }
    std::size_t getPagesRead() const { return pagesRead; }

};

template <typename T>
class BPlusTree {
public:
    struct BPlusTreeNode {
        int keys [2 * M]; // máximo de chaves
        int numKeys;
        bool isLeaf;
        // Offsets de arquivo (long)
        long childrenOffsets[2 * M + 1];
        // O `nextLeaf` é crucial para B+ Tree e deve ser um offset
        long nextLeafOffset;
    };

protected:
    static constexpr int m = M; // ordem da árvore
    
public:
    BPlusTree(const std::string& filename);
    ~BPlusTree();

    void insert(int key, T *data);
    // Retorna o OFFSET do nó folha que contém a chave, ou 0 se não encontrar
    long search(int k); 
    // NOVO: Retorna todos os offsets de dados associados a uma chave
    std::vector<long> searchAll(int k);
    int getM() const { return m; }
    
    // Métodos para estatísticas de I/O
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

    // divisão & promoção
    void splitLeaf(long nodeOffset, int key, long dataOffset);
    void splitInternal(long parentOffset, int promoteKey, long rightChildOffset);

    // função auxiliar para inserção em nós internos
    void insertInternal(int key, long parentOffset, long childOffset);

    // função auxiliar para encontrar o pai de um nó
    long findParent(long subrootOffset, long childOffset);
};



// Construtor: Inicializa FileManager e carrega a raiz (offset)
template <typename T>
BPlusTree<T>::BPlusTree(const std::string& filename) {
    fileManager = new FileManager(filename, m);
    
    // Lê o cabeçalho do arquivo
    auto header = fileManager->getHeader();
    rootOffset = header.rootOffset;

    if (rootOffset == 0) {
        // Se o arquivo é novo, cria o nó raiz e salva seu offset
        rootOffset = newNode(true);
        fileManager->updateRootOffset(rootOffset);
    }
}
// Destrutor
template <typename T>
BPlusTree<T>::~BPlusTree() {
    fileManager->updateRootOffset(rootOffset);
    delete fileManager;
}



template <typename T>
long BPlusTree<T>::newNode(bool leaf) {
    long offset = fileManager->getNewOffset();
    
    typename BPlusTree<T>::BPlusTreeNode tempNode; 
    
    // Inicialização do nó em memória
    tempNode.isLeaf = leaf;
    tempNode.numKeys = 0;
    tempNode.nextLeafOffset = 0;
    // Zera os arrays de chaves e ponteiros
    std::memset(tempNode.keys, 0, sizeof(tempNode.keys));
    std::memset(tempNode.childrenOffsets, 0, sizeof(tempNode.childrenOffsets)); 

    // Escreve o nó inicializado no disco
    fileManager->writeNode<T>(offset, tempNode);
    return offset;
}

template <typename T>
void BPlusTree<T>::insert(int key, T *data) {
    // Armazena o dado no arquivo e obtém o offset
    long dataOffset = fileManager->getNewOffset();
    
    // Escreve o dado no arquivo no offset alocado
    fileManager->writeData(dataOffset, data);

    if (rootOffset == 0) {
        rootOffset = newNode(true);
        fileManager->updateRootOffset(rootOffset);
    }
    
    // Chamada à função recursiva.
    insert(key, dataOffset, rootOffset);
}
/*
Função de inserção em uma árvore B+
Parâmetros:
- key: chave a ser inserida
- data: ponteiro para os dados a serem armazenados (na folha)
- node: ponteiro para o nó atual (chamada recursiva; normalmente iniciar por 'root')
*/
template <typename T>
void BPlusTree<T>::insert(int key, long dataOffset, long nodeOffset) {
    typename BPlusTree<T>::BPlusTreeNode node;
    bool readSuccess = fileManager->readNode<T>(nodeOffset, node);
    
    if (!readSuccess) {
        return;
    }

    if (node.isLeaf) {
        if (node.numKeys < 2 * m) {
            // Nó não está cheio: insere de forma ordenada (in-place)
            int i = node.numKeys - 1;
            // Desloca chaves maiores para a direita
            while (i >= 0 && node.keys[i] > key) {
                node.keys[i + 1] = node.keys[i];
                node.childrenOffsets[i + 1] = node.childrenOffsets[i];
                --i;
            }
            // Insere nova chave na posição correta
            node.keys[i + 1] = key;
            node.childrenOffsets[i + 1] = dataOffset;
            node.numKeys++;

            fileManager->writeNode<T>(nodeOffset, node);
        } else {
            // Nó está cheio - dividir folha e promover separador
            splitLeaf(nodeOffset, key, dataOffset);
        }
        return;
    }

    // Nó interno: escolher filho apropriado e descer
    int i = upperBound(node.keys, node.numKeys, key); // encontra primeiro child > key
    long childOffset = node.childrenOffsets[i]; // Usa o offset
    insert(key, dataOffset, childOffset);

    // Caso 'child' tenha sido dividido, a promoção é tratada dentro de split/insertInternal
}

/*
Divide nó folha cheio e promove a menor chave do novo nó (separator key).
*/
// CORREÇÃO: A assinatura deve receber long para o dado
template <typename T>
void BPlusTree<T>::splitLeaf(long nodeOffset, int key, long dataOffset) { 
    // 1. LER O NÓ ORIGINAL DO DISCO
    typename BPlusTree<T>::BPlusTreeNode node;
    fileManager->readNode<T>(nodeOffset, node);

    // CORREÇÃO: Não usar new/delete. Usar arrays de tamanho fixo na pilha.
    int tmpKeys[2 * M + 1];
    long tmpChildrenOffsets[2 * M + 1]; 

    // Lógica de merge e cópia para arrays temporários (CORRIGIDO PARA OFFSETS)
    int i = 0, j = 0;
    bool inserted = false;
    for (i = 0; i < node.numKeys; i++) {
        if (!inserted && key < node.keys[i]) {
            tmpKeys[j] = key;
            tmpChildrenOffsets[j] = dataOffset;
            j++;
            inserted = true;
        }
        tmpKeys[j] = node.keys[i];
        tmpChildrenOffsets[j] = node.childrenOffsets[i]; // CORREÇÃO: childrenOffsets
        j++;
    }
    if (!inserted) {
        tmpKeys[j] = key;
        tmpChildrenOffsets[j] = dataOffset;
        j++;
    }

    int totalKeys = j;
    int splitPoint = totalKeys / 2;

    // 2. REESCREVER NÓ ORIGINAL (ESQUERDA)
    node.numKeys = splitPoint;
    for (i = 0; i < splitPoint; i++) {
        node.keys[i] = tmpKeys[i];
        node.childrenOffsets[i] = tmpChildrenOffsets[i];
    }
    // Zera os slots restantes para o novo nó
    std::memset(&node.keys[splitPoint], 0, (2*M - splitPoint) * sizeof(int));

    // 3. CRIAR NOVO NÓ FOLHA (DIREITA)
    long newLeafOffset = newNode(true);
    typename BPlusTree<T>::BPlusTreeNode newLeaf;
    // LER o nó inicializado (embora newNode já o tenha inicializado com zeros)
    fileManager->readNode<T>(newLeafOffset, newLeaf); 

    int k = 0;
    for (i = splitPoint; i < totalKeys; i++, k++) {
        newLeaf.keys[k] = tmpKeys[i];
        newLeaf.childrenOffsets[k] = tmpChildrenOffsets[i];
        newLeaf.numKeys++;
    }

    // 4. ATUALIZAR LISTA LIGADA (nextLeafOffset)
    newLeaf.nextLeafOffset = node.nextLeafOffset;
    node.nextLeafOffset = newLeafOffset;

    // 5. CHAVE A PROMOVER (menor chave do novo nó)
    int promoteKey = newLeaf.keys[0];

    // 6. TRATAMENTO DO PAI
    // CORREÇÃO: Usar rootOffset
    long parentOffset = findParent(rootOffset, nodeOffset); 

    if (nodeOffset == rootOffset) {
        // CRIA NOVA RAIZ INTERNA
        long newRootOffset = newNode(false);
        typename BPlusTree<T>::BPlusTreeNode newRoot;
        fileManager->readNode<T>(newRootOffset, newRoot); // LER O NOVO NÓ
        
        newRoot.keys[0] = promoteKey;
        newRoot.childrenOffsets[0] = nodeOffset;
        newRoot.childrenOffsets[1] = newLeafOffset;
        newRoot.numKeys = 1;
        
        fileManager->writeNode<T>(newRootOffset, newRoot); // ESCREVER A NOVA RAIZ
        
        rootOffset = newRootOffset; // ATUALIZA O OFFSET RAIZ GLOBAL
        fileManager->updateRootOffset(newRootOffset); // Salva no cabeçalho
    } else {
        // Propaga para o pai usando offsets
        insertInternal(promoteKey, parentOffset, newLeafOffset);
    }
    
    // 7. ESCREVER NÓS ALTERADOS NO DISCO (RMW completo)
    fileManager->writeNode<T>(nodeOffset, node);
    fileManager->writeNode<T>(newLeafOffset, newLeaf);
}

/*
Inserção em nó interno (semelhante à de folha, mas deslocando também ponteiros de filhos).
Pode disparar divisão de nó interno.
*/
template <typename T>
void BPlusTree<T>::insertInternal(int key, long parentOffset, long childOffset) {
    if (parentOffset == 0) {
        // Se parentOffset for 0, significa que o nó original foi dividido
        // e a nova raiz precisa ser criada.
        long newRootOffset = newNode(false);
        typename BPlusTree<T>::BPlusTreeNode newRoot;
        fileManager->readNode<T>(newRootOffset, newRoot); // R: LER O NOVO NÓ

        newRoot.keys[0] = key;
        newRoot.childrenOffsets[0] = rootOffset; // rootOffset ainda aponta para a raiz antiga/esquerda
        newRoot.childrenOffsets[1] = childOffset;
        newRoot.numKeys = 1;
        
        fileManager->writeNode<T>(newRootOffset, newRoot); // W: ESCREVER
        
        rootOffset = newRootOffset;
        fileManager->updateRootOffset(newRootOffset);
        return;
    }

    typename BPlusTree<T>::BPlusTreeNode parentNode;
    fileManager->readNode<T>(parentOffset, parentNode); // R: LER O NÓ PAI

    if (parentNode.numKeys < 2 * m) {
        // Pai não está cheio: insere de forma ordenada (RMW)
        int i = parentNode.numKeys - 1;
        // Desloca chaves e ponteiros para abrir espaço
        while (i >= 0 && parentNode.keys[i] > key) {
            parentNode.keys[i + 1] = parentNode.keys[i];
            parentNode.childrenOffsets[i + 2] = parentNode.childrenOffsets[i + 1];
            --i;
        }
        // Insere nova chave e ponteiro (childOffset)
        parentNode.keys[i + 1] = key;
        parentNode.childrenOffsets[i + 2] = childOffset;
        parentNode.numKeys++;

        fileManager->writeNode<T>(parentOffset, parentNode); // W: ESCREVER
        return;
    }

    // Pai está cheio - dividir nó interno (RMW)
    splitInternal(parentOffset, key, childOffset);
}

/*
Divisão de nó interno: insere (key, rightChild), e então promove a chave do meio.
*/
template <typename T>
void BPlusTree<T>::splitInternal(long parentOffset, int promoteKey, long rightChildOffset) {
    // 1. LER O NÓ ORIGINAL DO DISCO
    typename BPlusTree<T>::BPlusTreeNode parentNode;
    fileManager->readNode<T>(parentOffset, parentNode);

    // 2. Usar arrays de tamanho fixo na pilha (substitui new/delete)
    int tmpKeys[2 * M + 1];
    long tmpChildrenOffsets[2 * M + 2]; 

    // Lógica de merge ordenado (copiar chaves e offsets do nó original + chave promovida/ponteiro)
    int i, j, pos = 0;
    while (pos < parentNode.numKeys && parentNode.keys[pos] < promoteKey) pos++;

    for (i = 0; i < pos; ++i) {
        tmpKeys[i] = parentNode.keys[i];
        tmpChildrenOffsets[i] = parentNode.childrenOffsets[i];
    }
    tmpChildrenOffsets[pos] = parentNode.childrenOffsets[pos];

    tmpKeys[pos] = promoteKey;
    tmpChildrenOffsets[pos + 1] = rightChildOffset;

    for (j = pos; j < parentNode.numKeys; ++j) {
        tmpKeys[j + 1] = parentNode.keys[j];
        tmpChildrenOffsets[j + 2] = parentNode.childrenOffsets[j + 1];
    }

    int totalKeys = parentNode.numKeys + 1;
    int mid = totalKeys / 2;               
    int upKey = tmpKeys[mid]; // Chave que sobe

    // 3. Reescrever nó esquerdo (parentNode) com a metade esquerda
    parentNode.numKeys = mid;
    for (i = 0; i < mid; ++i) {
        parentNode.keys[i] = tmpKeys[i];
        parentNode.childrenOffsets[i] = tmpChildrenOffsets[i];
    }
    parentNode.childrenOffsets[i] = tmpChildrenOffsets[i]; // último ponteiro à esquerda
    
    // 4. Criar nó direito com a metade direita (após mid)
    long newRightOffset = newNode(false);
    typename BPlusTree<T>::BPlusTreeNode rightNode;
    fileManager->readNode<T>(newRightOffset, rightNode); // R: LER O NOVO NÓ

    int k = 0;
    for (i = mid + 1; i < totalKeys; ++i, ++k) {
        rightNode.keys[k] = tmpKeys[i];
        rightNode.childrenOffsets[k] = tmpChildrenOffsets[i];
        rightNode.numKeys++;
    }
    rightNode.childrenOffsets[k] = tmpChildrenOffsets[totalKeys]; // último ponteiro à direita

    // 5. TRATAMENTO DO PAI
    if (parentOffset == rootOffset) {
        // Cria nova raiz
        long newRootOffset = newNode(false);
        typename BPlusTree<T>::BPlusTreeNode newRoot;
        fileManager->readNode<T>(newRootOffset, newRoot); // R: LER O NOVO NÓ
        
        newRoot.keys[0] = upKey;
        newRoot.childrenOffsets[0] = parentOffset;
        newRoot.childrenOffsets[1] = newRightOffset;
        newRoot.numKeys = 1;

        fileManager->writeNode<T>(newRootOffset, newRoot); // W: ESCREVER NOVA RAIZ
        rootOffset = newRootOffset;
        fileManager->updateRootOffset(newRootOffset);
    } else {
        // Propaga divisão para cima (usando offsets)
        long grandparentOffset = findParent(rootOffset, parentOffset);
        insertInternal(upKey, grandparentOffset, newRightOffset);
    }

    // 6. ESCREVER NÓS ALTERADOS NO DISCO (RMW completo)
    fileManager->writeNode<T>(parentOffset, parentNode);
    fileManager->writeNode<T>(newRightOffset, rightNode);
}

// ---------- buscas e utilidades ----------

// Retorna primeiro índice onde arr[i] > key (busca binária)
template <typename T>
int BPlusTree<T>::upperBound(const int *arr, int n, int key) {
    int l = 0, r = n;
    while (l < r) {
        int mid = (l + r) / 2;
        if (arr[mid] <= key) l = mid + 1;
        else r = mid;
    }
    return l; // primeiro índice com arr[i] > key
}

// Retorna primeiro índice onde arr[i] >= key (busca binária)
template <typename T>
int BPlusTree<T>::lowerBound(const int *arr, int n, int key) {
    int l = 0, r = n;
    while (l < r) {
        int mid = (l + r) / 2;
        if (arr[mid] < key) l = mid + 1;
        else r = mid;
    }
    return l; // primeiro índice com arr[i] >= key
}

// Busca uma chave na árvore B+
template <typename T>
long BPlusTree<T>::search(int k) {
    long currentOffset = rootOffset;
    BPlusTreeNode currentNode;

    if (currentOffset == 0) {
        return 0; // Árvore vazia
    }

    // Desce até folha
    while (true) {
        // 1. Lê o nó do disco para a variável temporária 'currentNode'
        fileManager->readNode<T>(currentOffset, currentNode); 
        
        // 2. Verifica se é a folha
        if (currentNode.isLeaf) {
            // Na folha, faz a busca final.
            int pos = lowerBound(currentNode.keys, currentNode.numKeys, k);
            
            if (pos < currentNode.numKeys && currentNode.keys[pos] == k) {
                // Chave encontrada! Retorna o offset do nó folha que a contém.
                return currentOffset; 
            } else {
                // Chave não encontrada
                return 0; 
            }
        }

        // 3. Nó interno: encontra o próximo offset
        int i = upperBound(currentNode.keys, currentNode.numKeys, k);
        currentOffset = currentNode.childrenOffsets[i];

        if (currentOffset == 0) {
            // Se um ponteiro nulo é encontrado, a chave não existe na subárvore
            return 0;
        }
    }
}

// NOVO: Busca todas as ocorrências de uma chave e retorna os offsets correspondentes
template <typename T>
std::vector<long> BPlusTree<T>::searchAll(int k) {
    std::vector<long> results;
    long leafOffset = search(k);
    
    if (leafOffset == 0) return results; // Chave não encontrada
    
    BPlusTreeNode leafNode;
    fileManager->readNode<T>(leafOffset, leafNode);
    
    // Encontra todas as ocorrências da chave neste nó
    for (int i = 0; i < leafNode.numKeys; i++) {
        if (leafNode.keys[i] == k) {
            results.push_back(leafNode.childrenOffsets[i]);
        }
    }
    
    return results;
}

template <typename T>
long BPlusTree<T>::findParent(long subrootOffset, long childOffset) {
    // 0 é usado como offset NULO
    if (subrootOffset == childOffset || subrootOffset == 0) return 0; 

    typename BPlusTree<T>::BPlusTreeNode parentNode;
    // R: LER O NÓ PAI (subroot)
    fileManager->readNode<T>(subrootOffset, parentNode); 

    // Verifica se o filho é um dos ponteiros diretos
    for (int i = 0; i <= parentNode.numKeys; ++i) {
        if (parentNode.childrenOffsets[i] == childOffset) {
            return subrootOffset; // Encontrou o pai: retorna o offset da sub-raiz
        }
    }

    // Se o nó atual não é o pai, desce recursivamente nos filhos
    if (!parentNode.isLeaf) {
        for (int i = 0; i <= parentNode.numKeys; ++i) {
            long nextOffset = parentNode.childrenOffsets[i];
            if (nextOffset != 0) {
                // Chama findParent para o próximo nível
                long result = findParent(nextOffset, childOffset);
                if (result != 0) return result;
            }
        }
    }
    return 0; // Pai não encontrado
}
// Instanciação explícita para o índice secundário
template class BPlusTree<long>;
