#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdlib>

// função para obter variáveis de ambiente com fallback
inline std::string getEnv(const std::string& key, const std::string& defaultValue) {
    const char* value = std::getenv(key.c_str());
    return value ? value : defaultValue;
}

const std::string DATA_DIR = getEnv("DATA_DIR", "data");
const std::string BIN_DIR = getEnv("BIN_DIR", "bin");
const std::string DB_DIR = getEnv("DB_DIR", DATA_DIR + "/db");

// caminhos completos 
const std::string ARTIGO_DAT = DATA_DIR + "/artigos.dat";
const std::string TABELA_HASH= DB_DIR + "/tabela_hash.idx";
const std::string PRIM_INDEX= DB_DIR + "/prim_index.idx";
const std::string SEC_INDEX= DB_DIR + "/sec_index.idx";
const std::string ARTIGO_CSV = DATA_DIR + "/artigo.csv";

#endif