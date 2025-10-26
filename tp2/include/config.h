#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdlib>

// Função para obter variáveis de ambiente com fallback
inline std::string getEnv(const std::string& key, const std::string& defaultValue) {
    const char* value = std::getenv(key.c_str());
    return value ? value : defaultValue;
}

// No include/config.h - CORRIJA PARA:
const std::string DATA_DIR = getEnv("DATA_DIR", "data");
const std::string BIN_DIR = getEnv("BIN_DIR", "bin");
const std::string DB_DIR = getEnv("DB_DIR", DATA_DIR + "/db");

// Caminhos completos - REMOVA /app/
const std::string NOME_ARQUIVO_DADOS = DATA_DIR + "/artigos.dat";
const std::string NOME_ARQUIVO_INDICE_HASH = DB_DIR + "/tabela_hash.idx";
const std::string NOME_ARQUIVO_INDICE_PRIM = DB_DIR + "/prim_index.idx";
const std::string NOME_ARQUIVO_INDICE_SEC = DB_DIR + "/sec_index.idx";
const std::string NOME_CSV_ENTRADA = DATA_DIR + "/artigo.csv";

#endif