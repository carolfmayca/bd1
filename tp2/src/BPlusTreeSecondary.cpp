#include "BPlusTree.cpp"

// M = 6  (titulo = 300 bytes)
template <typename T>
class BPlusTreeSecondary : public BPlusTree<T, 6> {
public:
    BPlusTreeSecondary(const std::string& filename) 
        : BPlusTree<T, 6>(filename, sizeof(int)) {
    }
};