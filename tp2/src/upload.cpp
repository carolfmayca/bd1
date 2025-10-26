#include "hashing_file.h"
#include "BPlusTree.hpp"
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <algorithm>
#include <vector>

// Constantes globais
const std::string NOME_ARQUIVO_DADOS = "/data/artigos.dat";
const std::string NOME_ARQUIVO_INDICE_HASH = "/app/bin/tabela_hash.idx";
const std::string NOME_ARQUIVO_INDICE_PRIM = "/app/bin/prim_index.idx";
const std::string NOME_ARQUIVO_INDICE_SEC = "/app/bin/sec_index.idx";
const std::string NOME_CSV_ENTRADA = "/data/artigo.csv";
const int TAMANHO_TABELA_HASH = 2000000;

// Estrutura para coleta de dados antes da inserção
struct IndexEntry {
    int key;
    long rid;
};

// ====== FUNÇÕES DE APOIO ====
std::string trimQuotes(const std::string& str) {
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.length() - 2);
    }
    return str;
}

// FUNÇÃO DE PARSING DO CSV
std::vector<std::string> parseCSVLine(const std::string& linha) {
    std::vector<std::string> campos;
    std::string campo_atual;
    bool dentro_de_aspas = false;

    for (char c : linha) {
        if (c == '"') {
            dentro_de_aspas = !dentro_de_aspas;
        } else if (c == ';' && !dentro_de_aspas) {
            campos.push_back(trimQuotes(campo_atual));
            campo_atual.clear();
        } else {
            campo_atual += c;
        }
    }
    campos.push_back(trimQuotes(campo_atual)); // Adiciona o último campo
    return campos;
}

// === FUNÇÕES DE NORMALIZAÇÃO ====
static inline std::string trim(const std::string& s) {
    std::size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}


static std::string normalize(const char* tituloRaw) {
    std::string s(tituloRaw ? tituloRaw : "");
    s = trim(s);
    return s;
}

// Hash FNV-1a 32
static uint32_t fnv1a32(const std::string& s) {
    const uint32_t FNV_PRIME = 16777619u;
    uint32_t hash = 2166136261u;
    for (unsigned char c : s) {
        hash ^= c;
        hash *= FNV_PRIME;
    }
    return hash;
}


static bool insereHashing(){
    std::ifstream csvFile(NOME_CSV_ENTRADA);
    if (!csvFile.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo CSV '" << NOME_CSV_ENTRADA << "'" << std::endl;
        return false; // Retorna falso em caso de erro
    }

    std::cout << "\n--- Lendo arquivo " << NOME_CSV_ENTRADA << " e inserindo dados ---" << std::endl;
    std::string linha;
    int registrosInseridos = 0;
    int numeroLinha = 0;
    HashingFile arquivoHash(NOME_ARQUIVO_DADOS, TAMANHO_TABELA_HASH);


    auto start = std::chrono::high_resolution_clock::now();

    while (std::getline(csvFile, linha)) {
        numeroLinha++;
        if (linha.empty()) continue;

        std::vector<std::string> campos = parseCSVLine(linha);

        if (campos.size() == 7) {
            Artigo art = {};
            art.ocupado = true;
            try {
                art.id = std::stoi(campos[0]);
                strncpy(art.titulo, campos[1].c_str(), 300);
                art.ano = std::stoi(campos[2]);
                strncpy(art.autores, campos[3].c_str(), 150);
                art.citacoes = std::stoi(campos[4]);
                strncpy(art.atualizacao, campos[5].c_str(), 19);
                if (campos[6] != "NULL") {
                    strncpy(art.snippet, campos[6].c_str(), 1024);
                }

                arquivoHash.inserirArtigo(art);
                registrosInseridos++;

                if (registrosInseridos > 0 && registrosInseridos % 50000 == 0) {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
                    std::cout << "[LOG] " << registrosInseridos << " registros inseridos no hashing... (" << elapsed << "s)" << std::endl;
                }

            } catch (const std::exception& e) {
                std::cerr << "--> ERRO DE CONVERSAO na linha " << numeroLinha << ". Verifique os campos numericos. Erro: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "--> AVISO: Linha " << numeroLinha << " ignorada. Esperava 7 campos, mas encontrou " << campos.size() << "." << std::endl;
            std::cerr << "    Conteudo da linha: " << linha << std::endl;
        }
    }
    std::cout << "--- Insercao finalizada. " << registrosInseridos << " registros inseridos. ---\n" << std::endl;
    csvFile.close();
    return true;
}


static bool insereIdxPrim(){
    BPlusTree<long> idx(NOME_ARQUIVO_INDICE_PRIM);

    std::ifstream in(NOME_ARQUIVO_DADOS, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[ERRO] Não foi possível abrir " << NOME_ARQUIVO_DADOS << "\n";
        return false;
    }

    const std::size_t blocoSize = sizeof(Bloco);
    Bloco bloco{};
    std::size_t blocoIndex = 0;
    std::size_t totalInseridos = 0;

    std::vector<IndexEntry> entries;
    entries.reserve(1200000);

    std::cout << "[INFO] Coletando registros..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    // coletar o RID e ID
    while (in.read(reinterpret_cast<char*>(&bloco), blocoSize)) {
        for (int i = 0; i < bloco.num_registos_usados; i++) {
            Artigo& art = bloco.artigos[i];
            if (!art.ocupado) continue;

            long rid = static_cast<long>(blocoIndex * REGISTOS_POR_BLOCO + i);
            entries.push_back({art.id, rid});
        }
        blocoIndex++;
    }
    in.close();

    std::cout << "[INFO] Ordenando " << entries.size() << " registros..." << std::endl;
    std::sort(entries.begin(), entries.end(), [](auto& a, auto& b){ return a.key < b.key; });

    std::cout << "[INFO] Inserindo na B+Tree..." << std::endl;

    auto insert_start = std::chrono::high_resolution_clock::now();
    const size_t MEGA_BATCH_SIZE = 100000;

    for (size_t start_idx = 0; start_idx < entries.size(); start_idx += MEGA_BATCH_SIZE) {
        size_t end_idx = std::min(start_idx + MEGA_BATCH_SIZE, entries.size());

        for (size_t j = start_idx; j < end_idx; ++j) {
            idx.insert(entries[j].key, &entries[j].rid);
            totalInseridos++;
        }

        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - insert_start).count();

        std::cout << end_idx << "/" << entries.size()
                  << " (" << elapsed << "s) - "
                  << (end_idx * 100 / entries.size()) << "%" << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsedTotal = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    std::cout << "\n[SUCESSO] Índice primário criado!\n";
    std::cout << "Total de chaves inseridas: " << totalInseridos << std::endl;
    std::cout << "Tempo total: " << elapsedTotal << " segundos\n" << std::endl;

    return true;
}


static bool insereIdxSec(){
    BPlusTree<long> idx(NOME_ARQUIVO_INDICE_SEC);

    std::ifstream in(NOME_ARQUIVO_DADOS, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Erro: nao foi possivel abrir " << NOME_ARQUIVO_DADOS << " para leitura.\n";
        return false;
    }

    const std::size_t blocoSize = sizeof(Bloco);
    Bloco bloco{};
    std::size_t blocoIndex = 0;
    std::size_t chavesInseridas = 0;
    std::size_t registrosVazios = 0;
    std::size_t totalRegistrosProcessados = 0;

    std::vector<IndexEntry> entries;
    entries.reserve(1200000);

    std::cout << "Coletando dados para ordenacao..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    int titulosValidosExibidos = 0;
    
    while (in.read(reinterpret_cast<char*>(&bloco), blocoSize)) {
        for (int i = 0; i < REGISTOS_POR_BLOCO && i < bloco.num_registos_usados; i++) {
            Artigo& art = bloco.artigos[i];
            long rid = static_cast<long>(blocoIndex * REGISTOS_POR_BLOCO + i);
            
            totalRegistrosProcessados++;
            
            // Só processa registros ocupados
            if (!art.ocupado) {
                continue;
            }
            
            std::string norm = normalize(art.titulo);
            
            if (norm.empty()) {
                registrosVazios++;
                continue;
            }
            
            int key = static_cast<int>(fnv1a32(norm));
            entries.push_back({key, rid});
            chavesInseridas++;
        }
        
        blocoIndex++;
        
        if (totalRegistrosProcessados % 200000 == 0) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        }
    }
    in.close();

    std::cout << "Ordenando " << entries.size() << " entradas..." << std::endl;
    
    std::sort(entries.begin(), entries.end(), 
              [](const IndexEntry& a, const IndexEntry& b) {
                  return a.key < b.key;
              });

    auto insert_start = std::chrono::high_resolution_clock::now();
    
    const size_t MEGA_BATCH_SIZE = 100000;
    
    for (size_t start_idx = 0; start_idx < entries.size(); start_idx += MEGA_BATCH_SIZE) {
        size_t end_idx = std::min(start_idx + MEGA_BATCH_SIZE, entries.size());
        
        for (size_t j = start_idx; j < end_idx; ++j) {
            idx.insert(entries[j].key, &entries[j].rid);
        }

        // inserção em lotes
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - insert_start).count();
        std::cout << end_idx << "/" << entries.size() 
                  << " (" << elapsed << "s) - " 
                  << (end_idx * 100 / entries.size()) << "%" << std::endl;
    }

    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::seconds>(total_end - start).count();

    return true;
}

// copiar arquivos de índice para o volume persistente
bool copiarIndicesParaVolume() {
    std::cout << "\n=== INICIANDO COPIA DE INDICES ===\n";
    
    std::vector<std::pair<std::string, std::string>> arquivos = {
        {"/app/bin/tabela_hash.idx", "/bin/tabela_hash.idx"},
        {"/app/bin/prim_index.idx", "/bin/prim_index.idx"},
        {"/app/bin/sec_index.idx", "/bin/sec_index.idx"}
    };
    
    bool tudoOk = true;
    
    for (const auto& par : arquivos) {
        const std::string& origem = par.first;
        const std::string& destino = par.second;
        
        std::cout << "Tentando copiar: " << origem << " -> " << destino << "\n";
        
        std::ifstream src(origem, std::ios::binary);
        if (!src.is_open()) {
            std::cout << "ERRO: Arquivo origem " << origem << " nao encontrado!\n";
            tudoOk = false;
            continue;
        }
        
        src.seekg(0, std::ios::end);
        std::size_t tamanho = src.tellg();
        src.seekg(0, std::ios::beg);
        std::cout << "Arquivo origem tem " << tamanho << " bytes\n";
        
        std::ofstream dst(destino, std::ios::binary);
        if (!dst.is_open()) {
            std::cerr << "ERRO: Nao foi possivel criar arquivo destino " << destino << "\n";
            src.close();
            tudoOk = false;
            continue;
        }
        
        const size_t BUFFER_SIZE = 1024 * 1024; // 1MB buffer
        std::vector<char> buffer(BUFFER_SIZE);
        size_t totalCopiado = 0;
        
        while (src.good() && dst.good()) {
            src.read(buffer.data(), BUFFER_SIZE);
            std::streamsize bytesLidos = src.gcount();
            if (bytesLidos > 0) {
                dst.write(buffer.data(), bytesLidos);
                dst.flush();
                totalCopiado += bytesLidos;
                
                // Log progress for large files
                if (tamanho > 1000000000 && totalCopiado % (100 * 1024 * 1024) == 0) { // Log every 100MB
                    std::cout << "Progresso: " << totalCopiado << "/" << tamanho << " bytes\n";
                }
            }
            if (bytesLidos < BUFFER_SIZE) break;
        }
        src.close();
        dst.close();
        
        std::ifstream verificacao(destino, std::ios::binary);
        if (verificacao.is_open()) {
            verificacao.seekg(0, std::ios::end);
            std::size_t tamanhoDestino = verificacao.tellg();
            verificacao.close();
            
            if (tamanhoDestino == tamanho) {
                std::cout << "SUCESSO: Copiado " << tamanho << " bytes para " << destino << "\n";
            } else {
                std::cout << "ERRO: Tamanhos diferentes! Origem: " << tamanho << ", Destino: " << tamanhoDestino << "\n";
                tudoOk = false;
            }
        } else {
            std::cout << "ERRO: Nao foi possivel verificar arquivo copiado " << destino << "\n";
            tudoOk = false;
        }
    }
    
    std::cout << "=== COPIA " << (tudoOk ? "CONCLUIDA COM SUCESSO" : "FINALIZADA COM ERROS") << " ===\n";
    return tudoOk;
}

int main(){
    // Limpa o ambiente
    remove(NOME_ARQUIVO_DADOS.c_str());
    remove(NOME_ARQUIVO_INDICE_HASH.c_str());
    remove(NOME_ARQUIVO_INDICE_PRIM.c_str());
    remove(NOME_ARQUIVO_INDICE_SEC.c_str());

    std::cout << "--- Inicio de inserção em hash ---\n";
    if (!insereHashing()) {
        std::cerr << "Erro na insercao via hashing. Abortando.\n";
        return 1;
    }
    std::cout << "--- Inserção em hash realizada com sucesso ---\n";
    std::cout << "\n--- Inicio de insercao do indice primario ---\n";
    if (!insereIdxPrim()) {
        std::cerr << "Erro na insercao do indice primario. Abortando.\n";
        return 1;
    }
    std::cout << "--- Inserção do indice primario realizada com sucesso ---\n";
    std::cout << "\n--- Inicio de insercao do indice secundario ---\n";
    if (!insereIdxSec()) {
        std::cerr << "Erro na insercao do indice secundario. Abortando.\n";
        return 1;
    }
    std::cout << "--- Inserção do indice secundario realizada com sucesso ---\n";
    
    if (!copiarIndicesParaVolume()) {
        std::cerr << "AVISO: Erro ao copiar indices para volume persistente.\n";
    }

    return 0;
}