#ifndef BTREE_HPP
#define BTREE_HPP

#include <iostream>
#include <cstring>
/*
(2m + 1)* POINTER_SIZE + 2m * POINTER_SIZE + 2m * KEY_SIZE <= BLOCK_SIZE
(2m + 1)* 8 + 2m * 8 + 2m * 4 <= 4096
m ≃ 102,2 ==> m = 100 
*/

#define BLOCK_SIZE 4096
#define KEY_SIZE 4
#define POINTER_SIZE 8
#define M 100

template <typename T>
class BPlusTree {
public:
    struct BPlusTreeNode {
        int *keys;              // até 2m chaves
        int numKeys;            // número de chaves usadas
        bool isLeaf;            // folha ou interno
        // se é folha, children são ponteiros para os dados; 
        // se não é folha, children são ponteiros para outros nós
        void **children;        // tamanho alocado: 2m+1
    };
    
public:
    BPlusTree();

    void insert(int key, T *data) { insert(key, data, root); }
    void insert(int k, T *data, BPlusTreeNode *node); // recursiva
    bool search(int k);

private:
    BPlusTreeNode *root;
    int m; // árvore deve ter no mínimo m chaves e no máximo 2m chaves  

    // utilidades
    BPlusTreeNode* newNode(bool leaf);
    static int upperBound(const int *arr, int n, int key);
    static int lowerBound(const int *arr, int n, int key);

    // divisão & promoção
    void splitLeaf(BPlusTreeNode *node, int key, T *data);
    void splitInternal(BPlusTreeNode *parent, int promoteKey, BPlusTreeNode *rightChild);

    // função auxiliar para inserção em nós internos
    void insertInternal(int key, BPlusTreeNode *parent, BPlusTreeNode *child);

    // função auxiliar para encontrar o pai de um nó
    BPlusTreeNode* findParent(BPlusTreeNode *subroot, BPlusTreeNode *child);
};

/*
Construção da árvore B+
*/
template <typename T>
BPlusTree<T>::BPlusTree() {
    this->m = M;  // define o valor de m
    this->root = newNode(true); // nó raiz folha vazio
}

// Cria um novo nó com arrays alocados dinamicamente e inicializados
template <typename T>
typename BPlusTree<T>::BPlusTreeNode* BPlusTree<T>::newNode(bool leaf) {
    auto* n = new BPlusTreeNode;
    n->isLeaf = leaf;
    n->numKeys = 0;
    n->keys = new int[2 * m];
    n->children = new void*[2 * m + 1];
    std::memset(n->children, 0, sizeof(void*) * (2 * m + 1)); // inicializa com nullptr
    return n;
}

/*
Função de inserção em uma árvore B+
Parâmetros:
- key: chave a ser inserida
- data: ponteiro para os dados a serem armazenados (na folha)
- node: ponteiro para o nó atual (chamada recursiva; normalmente iniciar por 'root')
*/
template <typename T>
void BPlusTree<T>::insert(int key, T *data, BPlusTreeNode *node) {
    if (node->isLeaf) {
        if (node->numKeys < 2 * m) {
            // Nó não está cheio: insere de forma ordenada (in-place)
            int i = node->numKeys - 1;
            // Desloca chaves maiores para a direita
            while (i >= 0 && node->keys[i] > key) {
                node->keys[i + 1] = node->keys[i];
                node->children[i + 1] = node->children[i];
                --i;
            }
            // Insere nova chave na posição correta
            node->keys[i + 1] = key;
            node->children[i + 1] = static_cast<void*>(data);
            node->numKeys++;
        } else {
            // Nó está cheio - dividir folha e promover separador
            splitLeaf(node, key, data);
        }
        return;
    }

    // Nó interno: escolher filho apropriado e descer
    int i = upperBound(node->keys, node->numKeys, key); // encontra primeiro child > key
    auto* child = static_cast<BPlusTreeNode*>(node->children[i]);
    insert(key, data, child);

    // Caso 'child' tenha sido dividido, a promoção é tratada dentro de split/insertInternal
}

/*
Divide nó folha cheio e promove a menor chave do novo nó (separator key).
*/
template <typename T>
void BPlusTree<T>::splitLeaf(BPlusTreeNode *node, int key, T *data) {
    // Arrays temporários para organizar todas as chaves e dados
    int *tmpKeys = new int[2 * m + 1];
    void **tmpChildren = new void*[2 * m + 1];

    // Copiar chaves e dados existentes para arrays temporários, inserindo a nova chave ordenadamente
    int i = 0, j = 0;
    bool inserted = false;

    // Merge ordenado das chaves existentes com a nova chave
    for (i = 0; i < node->numKeys; i++) {
        if (!inserted && key < node->keys[i]) {
            tmpKeys[j] = key;
            tmpChildren[j] = static_cast<void*>(data);
            j++;
            inserted = true;
        }
        tmpKeys[j] = node->keys[i];
        tmpChildren[j] = node->children[i];
        j++;
    }

    // Se ainda não inseriu (chave é a maior)
    if (!inserted) {
        tmpKeys[j] = key;
        tmpChildren[j] = static_cast<void*>(data);
        j++;
    }

    int totalKeys = j;
    int splitPoint = totalKeys / 2; // metade inferior fica à esquerda; metade superior vai à direita

    // Reorganizar nó original (primeira metade)
    node->numKeys = splitPoint;
    for (i = 0; i < splitPoint; i++) {
        node->keys[i] = tmpKeys[i];
        node->children[i] = tmpChildren[i];
    }

    // Criar novo nó folha (direita) e copiar segunda metade
    BPlusTreeNode *newLeaf = newNode(true);
    int k = 0;
    for (i = splitPoint; i < totalKeys; i++, k++) {
        newLeaf->keys[k] = tmpKeys[i];
        newLeaf->children[k] = tmpChildren[i];
        newLeaf->numKeys++;
    }

    // Chave a promover para o pai é a menor chave do novo nó
    int promoteKey = newLeaf->keys[0];

    // Se node era a raiz (não tem pai explícito), cria nova raiz interna
    if (node == root) {
        BPlusTreeNode *newRoot = newNode(false);
        newRoot->keys[0] = promoteKey;
        newRoot->children[0] = static_cast<void*>(node);
        newRoot->children[1] = static_cast<void*>(newLeaf);
        newRoot->numKeys = 1;
        root = newRoot;
    } else {
        // Inserir chave promocional no pai
        insertInternal(promoteKey, findParent(root, node), newLeaf);
    }

    // Limpar arrays temporários
    delete[] tmpKeys;
    delete[] tmpChildren;
}

/*
Inserção em nó interno (semelhante à de folha, mas deslocando também ponteiros de filhos).
Pode disparar divisão de nó interno.
*/
template <typename T>
void BPlusTree<T>::insertInternal(int key, BPlusTreeNode *parent, BPlusTreeNode *child) {
    if (!parent) {
        // Caso raro: pai não encontrado – cria uma nova raiz.
        BPlusTreeNode *newRoot = newNode(false);
        newRoot->keys[0] = key;
        newRoot->children[0] = static_cast<void*>(root);
        newRoot->children[1] = static_cast<void*>(child);
        newRoot->numKeys = 1;
        root = newRoot;
        return;
    }

    if (parent->numKeys < 2 * m) {
        // Pai não está cheio
        int i = parent->numKeys - 1;
        // Desloca chaves e ponteiros para abrir espaço
        while (i >= 0 && parent->keys[i] > key) {
            parent->keys[i + 1] = parent->keys[i];
            parent->children[i + 2] = parent->children[i + 1];
            --i;
        }
        // Insere nova chave e ponteiro
        parent->keys[i + 1] = key;
        parent->children[i + 2] = static_cast<void*>(child);
        parent->numKeys++;
        return;
    }

    // Pai está cheio - dividir nó interno
    splitInternal(parent, key, child);
}

/*
Divisão de nó interno: insere (key, rightChild), e então promove a chave do meio.
*/
template <typename T>
void BPlusTree<T>::splitInternal(BPlusTreeNode *parent, int promoteKey, BPlusTreeNode *rightChild) {
    // Temporários com espaço para +1 chave e +1 ponteiro
    int *tmpKeys = new int[2 * m + 1];
    void **tmpChildren = new void*[2 * m + 2];

    // Merge ordenado de (parent) com a nova (promoteKey, rightChild)
    int i, j, pos = 0;
    // encontrar posição onde a promoteKey deve entrar
    while (pos < parent->numKeys && parent->keys[pos] < promoteKey) pos++;

    // copiar [0..pos-1] - chaves e ponteiros antes da inserção
    for (i = 0; i < pos; ++i) {
        tmpKeys[i] = parent->keys[i];
        tmpChildren[i] = parent->children[i];
    }
    tmpChildren[pos] = parent->children[pos];

    // inserir promoteKey e ponteiro à direita
    tmpKeys[pos] = promoteKey;
    tmpChildren[pos + 1] = static_cast<void*>(rightChild);

    // copiar restante [pos..] - chaves e ponteiros após a inserção
    for (j = pos; j < parent->numKeys; ++j) {
        tmpKeys[j + 1] = parent->keys[j];
        tmpChildren[j + 2] = parent->children[j + 1];
    }

    int totalKeys = parent->numKeys + 1;
    int mid = totalKeys / 2;               // chave do meio que sobe
    int upKey = tmpKeys[mid];

    // Reescrever nó esquerdo (parent) com a metade esquerda
    parent->numKeys = mid;
    for (i = 0; i < mid; ++i) {
        parent->keys[i] = tmpKeys[i];
        parent->children[i] = tmpChildren[i];
    }
    parent->children[i] = tmpChildren[i]; // último ponteiro à esquerda

    // Criar nó direito com a metade direita (após mid)
    BPlusTreeNode *right = newNode(false);
    int k = 0;
    for (i = mid + 1; i < totalKeys; ++i, ++k) {
        right->keys[k] = tmpKeys[i];
        right->children[k] = tmpChildren[i];
        right->numKeys++;
    }
    right->children[k] = tmpChildren[totalKeys]; // último ponteiro à direita

    // Ligar no pai do 'parent'
    if (parent == root) {
        // Cria nova raiz se parent era a raiz
        BPlusTreeNode *newRoot = newNode(false);
        newRoot->keys[0] = upKey;
        newRoot->children[0] = static_cast<void*>(parent);
        newRoot->children[1] = static_cast<void*>(right);
        newRoot->numKeys = 1;
        root = newRoot;
    } else {
        // Propaga divisão para cima
        insertInternal(upKey, findParent(root, parent), right);
    }

    delete[] tmpKeys;
    delete[] tmpChildren;
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
bool BPlusTree<T>::search(int k) {
    BPlusTreeNode *node = root;
    // Desce até folha seguindo os ponteiros corretos
    while (node && !node->isLeaf) {
        int i = upperBound(node->keys, node->numKeys, k);
        node = static_cast<BPlusTreeNode*>(node->children[i]);
    }
    if (!node) return false;
    // Busca binária na folha
    int pos = lowerBound(node->keys, node->numKeys, k);
    return (pos < node->numKeys && node->keys[pos] == k);
}

template <typename T>
typename BPlusTree<T>::BPlusTreeNode* 
BPlusTree<T>::findParent(BPlusTreeNode* subroot, BPlusTreeNode* child) {
    if (subroot == nullptr || subroot->isLeaf) 
        return nullptr;

    // percorre todos os filhos do subroot
    for (int i = 0; i <= subroot->numKeys; i++) {
        auto* c = static_cast<BPlusTreeNode*>(subroot->children[i]);
        if (c == child) {
            return subroot;
        }

        auto* p = findParent(c, child);
        if (p != nullptr) return p;
    }

    return nullptr;
}
#endif
