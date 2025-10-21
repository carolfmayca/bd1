#include <cstdint>
#include <iostream>
constexpr size_t BLOCK_SIZE    = 4096;                  // 4 KB
constexpr size_t KEY_SIZE      = 4;//= sizeof(int32_t);       // chave int (4 bytes)
constexpr size_t POINTER_SIZE  = 8;//= sizeof(uintptr_t);     // 8 bytes em 64-bit

int calcM() {
    return static_cast<int>(
        (BLOCK_SIZE - POINTER_SIZE) / (2 * (KEY_SIZE + 2*POINTER_SIZE))
    );
}

int main(){
    std::cout << "Valor de m calculado: " << calcM() << std::endl;
}