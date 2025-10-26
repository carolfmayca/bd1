#include "../include/hashing_file.h"
#include <iostream>
#include <vector>

HashingFile::HashingFile(const std::string& filename, int table_size)
    : nomeArquivo(filename), TAMANHO_TABELA(table_size) {
    arquivo.open(nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
    if (!arquivo.is_open()) {
        std::cerr << "AVISO: Arquivo de dados '" << nomeArquivo << "' nao encontrado." << std::endl;
    }
}

HashingFile::~HashingFile() {
    if (arquivo.is_open()) {
        arquivo.close();
    }
}

void HashingFile::criarArquivos() {
    std::ofstream dataFile(nomeArquivo, std::ios::out | std::ios::binary);
    dataFile.close();
    std::cout << "Arquivo '" << nomeArquivo << "' criado." << std::endl;

    std::string nomeTabela = "/app/bin/tabela_hash.idx";
    std::ofstream tabela(nomeTabela, std::ios::out | std::ios::binary);
    if (tabela.is_open()) {
        long offset_vazio = -1;
        for (int i = 0; i < TAMANHO_TABELA; ++i) {
            tabela.write(reinterpret_cast<const char*>(&offset_vazio), sizeof(long));
        }
        tabela.close();
        std::cout << "Arquivo de indice '" << nomeTabela << "' criado com " << TAMANHO_TABELA << " posicoes." << std::endl;
    }
}

long HashingFile::inserirArtigo(Artigo& novoArtigo) {
    if (!arquivo.is_open()) {
        criarArquivos();
        arquivo.open(nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
        if(!arquivo.is_open()){
             std::cerr << "Erro: Nao foi possivel abrir o arquivo de dados para insercao." << std::endl;
             return -1;
        }
    }

    int endereco = novoArtigo.id % TAMANHO_TABELA;
    std::string nomeTabela = "/app/bin/tabela_hash.idx";
    std::fstream tabela(nomeTabela, std::ios::in | std::ios::out | std::ios::binary);
    if (!tabela.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo de indice '" << nomeTabela << "'." << std::endl;
        return -1;
    }

    long offset_inicio_cadeia;
    tabela.seekg(endereco * sizeof(long));
    tabela.read(reinterpret_cast<char*>(&offset_inicio_cadeia), sizeof(long));

    if (offset_inicio_cadeia == -1) {
        // cadeia vazia, criamos um novo bloco
        Bloco novo_bloco = {};
        novo_bloco.artigos[0] = novoArtigo;
        novo_bloco.num_registos_usados = 1;
        novo_bloco.proximo_bloco_offset = -1;

        arquivo.seekg(0, std::ios::end);
        long nova_posicao_bloco = arquivo.tellg();
        arquivo.write(reinterpret_cast<const char*>(&novo_bloco), sizeof(Bloco));

        // atualiza tabela
        tabela.seekp(endereco * sizeof(long));
        tabela.write(reinterpret_cast<const char*>(&nova_posicao_bloco), sizeof(long));
    } else {
        // cadeia já existe, procura por um espaço livre
        long offset_bloco_atual = offset_inicio_cadeia;
        Bloco bloco_temp;
        while (true) {
            arquivo.seekg(offset_bloco_atual);
            arquivo.read(reinterpret_cast<char*>(&bloco_temp), sizeof(Bloco));

            if (bloco_temp.num_registos_usados < REGISTOS_POR_BLOCO) {
                bloco_temp.artigos[bloco_temp.num_registos_usados] = novoArtigo;
                bloco_temp.num_registos_usados++;
                arquivo.seekp(offset_bloco_atual);
                arquivo.write(reinterpret_cast<const char*>(&bloco_temp), sizeof(Bloco));
                break;
            } else if (bloco_temp.proximo_bloco_offset == -1) {
                // bloco está cheio e é o último da cadeia, cria um novo bloco de overflow
                Bloco novo_bloco_overflow = {};
                novo_bloco_overflow.artigos[0] = novoArtigo;
                novo_bloco_overflow.num_registos_usados = 1;
                novo_bloco_overflow.proximo_bloco_offset = -1;

                arquivo.seekg(0, std::ios::end);
                long nova_posicao_bloco_overflow = arquivo.tellg();
                arquivo.write(reinterpret_cast<const char*>(&novo_bloco_overflow), sizeof(Bloco));

                // atualizamos o bloco anterior
                bloco_temp.proximo_bloco_offset = nova_posicao_bloco_overflow;
                arquivo.seekp(offset_bloco_atual);
                arquivo.write(reinterpret_cast<const char*>(&bloco_temp), sizeof(Bloco));
                break;
            } else {
                // bloco cheio
                offset_bloco_atual = bloco_temp.proximo_bloco_offset;
            }
        }
    }
    tabela.close();
    return 0;
}

Artigo HashingFile::buscarPorId(int id, int& blocosLidos) {
    blocosLidos = 0;
    if (!arquivo.is_open()) return {};

    int endereco = id % TAMANHO_TABELA;
    std::string nomeTabela = "/bin/tabela_hash.idx";
    std::fstream tabela(nomeTabela, std::ios::in | std::ios::binary);
    if (!tabela.is_open()) return {};

    long offset_bloco_atual;
    tabela.seekg(endereco * sizeof(long));
    tabela.read(reinterpret_cast<char*>(&offset_bloco_atual), sizeof(long));
    tabela.close();

    while (offset_bloco_atual != -1) {
        Bloco bloco_temp;
        arquivo.seekg(offset_bloco_atual);
        arquivo.read(reinterpret_cast<char*>(&bloco_temp), sizeof(Bloco));
        blocosLidos++;

        for (int i = 0; i < bloco_temp.num_registos_usados; ++i) {
            if (bloco_temp.artigos[i].id == id) {
                return bloco_temp.artigos[i];
            }
        }
        offset_bloco_atual = bloco_temp.proximo_bloco_offset;
    }

    return {};
}

long HashingFile::getTotalBlocos() {
    if (!arquivo.is_open() || sizeof(Bloco) == 0) {
        return 0;
    }
    arquivo.seekg(0, std::ios::end);
    long tamanho_total_bytes = arquivo.tellg();
    return tamanho_total_bytes / sizeof(Bloco);
}

