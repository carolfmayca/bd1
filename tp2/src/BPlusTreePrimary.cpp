#include "BPlusTree.cpp"

// M = 102 (int = 4 bytes)
template <typename T>
class BPlusTreePrimary : public BPlusTree<T, 102> {
public:
    BPlusTreePrimary(const std::string& filename) 
        : BPlusTree<T, 102>(filename, sizeof(int)) {
    }
};