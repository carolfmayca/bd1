#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

using RID = int64_t; // offset em artigos.dat

// Igual (ou compatível) com o que você grava no hashing_file
struct ArticleDisk {
    int32_t id;
    char    titulo[300];
    int32_t ano;
    char    autores[150];
    int32_t citacoes;
    char    atualizacao[20];
    char    snippet[1024];
    int64_t proximo_offset; // se no seu arquivo é 'long', use 'long' aqui também
    uint8_t ocupado;        // remova se não usa nos dois lados
};

// Normalização simples
inline std::string normalize(const char* t) {
    std::string s(t ? t : "");
    auto notsp = [](unsigned char c){ return !std::isspace(c); };
    auto b = std::find_if(s.begin(), s.end(), notsp);
    auto e = std::find_if(s.rbegin(), s.rend(), notsp).base();
    s = (b < e) ? std::string(b, e) : std::string();
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

// Hash 32-bit (cabe em int)
inline uint32_t fnv1a32(const std::string& s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}
