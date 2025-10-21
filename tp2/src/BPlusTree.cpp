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
        // Implementar a desserialização aqui. 
        // Simplificação: se o nó é menor ou igual a BLOCK_SIZE, pode ser lido de uma vez.
        file.read(reinterpret_cast<char*>(&node), BLOCK_SIZE); 
        return file.good();
    }

    // Escrita de um nó no arquivo
    template <typename T, int M_VALUE>
    void writeNode(long offset, const typename BPlusTree<T, M_VALUE>::BPlusTreeNode& node) {
        file.seekp(offset, std::ios::beg);
        // Implementar a serialização aqui.
        file.write(reinterpret_cast<const char*>(&node), BLOCK_SIZE);
        file.flush(); // Garantir que está escrito no disco
    }
};

template <typename T, int M_VALUE = 100>
class BPlusTree {
public:
    struct BPlusTreeNode {
        int keys [2 * M_VALUE]; // máximo de chaves
        int numKeys;
        bool isLeaf;
        // Offsets de arquivo (long)
        long childrenOffsets[2 * M_VALUE + 1];
        // O `nextLeaf` é crucial para B+ Tree e deve ser um offset
        long nextLeafOffset;
    };

protected:
    static constexpr int m = M_VALUE; // ordem da árvore
    
public:
    BPlusTree(const std::string& filename);
    ~BPlusTree();

    void insert(int key, T *data);
    // Retorna o OFFSET do nó folha que contém a chave, ou 0 se não encontrar
    long search(int k); 
    int getM() const { return m; }

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
template <typename T, int M_VALUE>
BPlusTree<T, M_VALUE>::BPlusTree(const std::string& filename) {
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
template <typename T, int M_VALUE>
BPlusTree<T, M_VALUE>::~BPlusTree() {
    fileManager->updateRootOffset(rootOffset);
    delete fileManager;
}



template <typename T, int M_VALUE>
long BPlusTree<T, M_VALUE>::newNode(bool leaf) {
    long offset = fileManager->getNewOffset();
    typename BPlusTree<T, M_VALUE>::BPlusTreeNode tempNode; 
    
    // Inicialização do nó em memória
    tempNode.isLeaf = leaf;
    tempNode.numKeys = 0;
    tempNode.nextLeafOffset = 0;
    // Zera os arrays de chaves e ponteiros
    std::memset(tempNode.keys, 0, sizeof(tempNode.keys));
    std::memset(tempNode.childrenOffsets, 0, sizeof(tempNode.childrenOffsets)); 

    // Escreve o nó inicializado no disco
    fileManager->writeNode<T, M_VALUE>(offset, tempNode);
    return offset;
}

template <typename T, int M_VALUE>
void BPlusTree<T, M_VALUE>::insert(int key, T *data) {
    // 1. O T *data é tipicamente o offset do registro de dados no arquivo.
    // Usamos reinterpret_cast para simular que o T* passado pelo usuário é, na verdade,
    // o offset (long) para onde o dado T está ou será armazenado.
    long dataOffset = reinterpret_cast<long>(data); 

    if (rootOffset == 0) {
        rootOffset = newNode(true);
        fileManager->updateRootOffset(rootOffset);
    }
    
    // Chamada à função recursiva.
    // A função recursiva agora deve aceitar o long dataOffset.
    insert(key, dataOffset, rootOffset);
}
/*
Função de inserção em uma árvore B+
Parâmetros:
- key: chave a ser inserida
- data: ponteiro para os dados a serem armazenados (na folha)
- node: ponteiro para o nó atual (chamada recursiva; normalmente iniciar por 'root')
*/
template <typename T, int M_VALUE>
void BPlusTree<T, M_VALUE>::insert(int key, long dataOffset, long nodeOffset) {
    typename BPlusTree<T, M_VALUE>::BPlusTreeNode node;
    fileManager->readNode<T, M_VALUE>(nodeOffset, node);

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

            fileManager->writeNode<T, M_VALUE>(nodeOffset, node);
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
template <typename T, int M_VALUE>
// CORREÇÃO: A assinatura deve receber long para o dado
void BPlusTree<T, M_VALUE>::splitLeaf(long nodeOffset, int key, long dataOffset) { 
    // 1. LER O NÓ ORIGINAL DO DISCO
    typename BPlusTree<T, M_VALUE>::BPlusTreeNode node;
    fileManager->readNode<T, M_VALUE>(nodeOffset, node);

    // CORREÇÃO: Não usar new/delete. Usar arrays de tamanho fixo na pilha.
    int tmpKeys[2 * M_VALUE + 1];
    long tmpChildrenOffsets[2 * M_VALUE + 1]; 

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
    std::memset(&node.keys[splitPoint], 0, (2*M_VALUE - splitPoint) * sizeof(int));

    // 3. CRIAR NOVO NÓ FOLHA (DIREITA)
    long newLeafOffset = newNode(true);
    typename BPlusTree<T, M_VALUE>::BPlusTreeNode newLeaf;
    // LER o nó inicializado (embora newNode já o tenha inicializado com zeros)
    fileManager->readNode<T, M_VALUE>(newLeafOffset, newLeaf); 

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
        typename BPlusTree<T, M_VALUE>::BPlusTreeNode newRoot;
        fileManager->readNode<T, M_VALUE>(newRootOffset, newRoot); // LER O NOVO NÓ
        
        newRoot.keys[0] = promoteKey;
        newRoot.childrenOffsets[0] = nodeOffset;
        newRoot.childrenOffsets[1] = newLeafOffset;
        newRoot.numKeys = 1;
        
        fileManager->writeNode<T, M_VALUE>(newRootOffset, newRoot); // ESCREVER A NOVA RAIZ
        
        rootOffset = newRootOffset; // ATUALIZA O OFFSET RAIZ GLOBAL
        fileManager->updateRootOffset(newRootOffset); // Salva no cabeçalho
    } else {
        // Propaga para o pai usando offsets
        insertInternal(promoteKey, parentOffset, newLeafOffset);
    }
    
    // 7. ESCREVER NÓS ALTERADOS NO DISCO (RMW completo)
    fileManager->writeNode<T, M_VALUE>(nodeOffset, node);
    fileManager->writeNode<T, M_VALUE>(newLeafOffset, newLeaf);
}

/*
Inserção em nó interno (semelhante à de folha, mas deslocando também ponteiros de filhos).
Pode disparar divisão de nó interno.
*/
template <typename T, int M_VALUE>
void BPlusTree<T, M_VALUE>::insertInternal(int key, long parentOffset, long childOffset) {
    if (parentOffset == 0) {
        // Se parentOffset for 0, significa que o nó original foi dividido
        // e a nova raiz precisa ser criada.
        long newRootOffset = newNode(false);
        typename BPlusTree<T, M_VALUE>::BPlusTreeNode newRoot;
        fileManager->readNode<T, M_VALUE>(newRootOffset, newRoot); // R: LER O NOVO NÓ

        newRoot.keys[0] = key;
        newRoot.childrenOffsets[0] = rootOffset; // rootOffset ainda aponta para a raiz antiga/esquerda
        newRoot.childrenOffsets[1] = childOffset;
        newRoot.numKeys = 1;
        
        fileManager->writeNode<T, M_VALUE>(newRootOffset, newRoot); // W: ESCREVER
        
        rootOffset = newRootOffset;
        fileManager->updateRootOffset(newRootOffset);
        return;
    }

    typename BPlusTree<T, M_VALUE>::BPlusTreeNode parentNode;
    fileManager->readNode<T, M_VALUE>(parentOffset, parentNode); // R: LER O NÓ PAI

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

        fileManager->writeNode<T, M_VALUE>(parentOffset, parentNode); // W: ESCREVER
        return;
    }

    // Pai está cheio - dividir nó interno (RMW)
    splitInternal(parentOffset, key, childOffset);
}

/*
Divisão de nó interno: insere (key, rightChild), e então promove a chave do meio.
*/
template <typename T, int M_VALUE>
void BPlusTree<T, M_VALUE>::splitInternal(long parentOffset, int promoteKey, long rightChildOffset) {
    // 1. LER O NÓ ORIGINAL DO DISCO
    typename BPlusTree<T, M_VALUE>::BPlusTreeNode parentNode;
    fileManager->readNode<T, M_VALUE>(parentOffset, parentNode);

    // 2. Usar arrays de tamanho fixo na pilha (substitui new/delete)
    int tmpKeys[2 * M_VALUE + 1];
    long tmpChildrenOffsets[2 * M_VALUE + 2]; 

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
    typename BPlusTree<T, M_VALUE>::BPlusTreeNode rightNode;
    fileManager->readNode<T, M_VALUE>(newRightOffset, rightNode); // R: LER O NOVO NÓ

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
        typename BPlusTree<T, M_VALUE>::BPlusTreeNode newRoot;
        fileManager->readNode<T, M_VALUE>(newRootOffset, newRoot); // R: LER O NOVO NÓ
        
        newRoot.keys[0] = upKey;
        newRoot.childrenOffsets[0] = parentOffset;
        newRoot.childrenOffsets[1] = newRightOffset;
        newRoot.numKeys = 1;

        fileManager->writeNode<T, M_VALUE>(newRootOffset, newRoot); // W: ESCREVER NOVA RAIZ
        rootOffset = newRootOffset;
        fileManager->updateRootOffset(newRootOffset);
    } else {
        // Propaga divisão para cima (usando offsets)
        long grandparentOffset = findParent(rootOffset, parentOffset);
        insertInternal(upKey, grandparentOffset, newRightOffset);
    }

    // 6. ESCREVER NÓS ALTERADOS NO DISCO (RMW completo)
    fileManager->writeNode<T, M_VALUE>(parentOffset, parentNode);
    fileManager->writeNode<T, M_VALUE>(newRightOffset, rightNode);
}

// ---------- buscas e utilidades ----------

// Retorna primeiro índice onde arr[i] > key (busca binária)
template <typename T, int M_VALUE>
int BPlusTree<T, M_VALUE>::upperBound(const int *arr, int n, int key) {
    int l = 0, r = n;
    while (l < r) {
        int mid = (l + r) / 2;
        if (arr[mid] <= key) l = mid + 1;
        else r = mid;
    }
    return l; // primeiro índice com arr[i] > key
}

// Retorna primeiro índice onde arr[i] >= key (busca binária)
template <typename T, int M_VALUE>
int BPlusTree<T, M_VALUE>::lowerBound(const int *arr, int n, int key) {
    int l = 0, r = n;
    while (l < r) {
        int mid = (l + r) / 2;
        if (arr[mid] < key) l = mid + 1;
        else r = mid;
    }
    return l; // primeiro índice com arr[i] >= key
}

// Busca uma chave na árvore B+
template <typename T, int M_VALUE>
long BPlusTree<T, M_VALUE>::search(int k) {
    long currentOffset = rootOffset;
    BPlusTreeNode currentNode;

    if (currentOffset == 0) return 0; // Árvore vazia

    // Desce até folha
    while (true) {
        // 1. Lê o nó do disco para a variável temporária 'currentNode'
        fileManager->readNode<T, M_VALUE>(currentOffset, currentNode); 
        
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

template <typename T, int M_VALUE>
long BPlusTree<T, M_VALUE>::findParent(long subrootOffset, long childOffset) {
    // 0 é usado como offset NULO
    if (subrootOffset == childOffset || subrootOffset == 0) return 0; 

    typename BPlusTree<T, M_VALUE>::BPlusTreeNode parentNode;
    // R: LER O NÓ PAI (subroot)
    fileManager->readNode<T, M_VALUE>(subrootOffset, parentNode); 

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