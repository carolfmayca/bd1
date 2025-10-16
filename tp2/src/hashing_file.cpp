#include "../include/hashing_file.h"
#include <iostream>
#include <vector>

// Construtor
HashingFile::HashingFile(const std::string& filename, int table_size)
    : nomeArquivo(filename), TAMANHO_TABELA(table_size) {
    arquivo.open(nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
    if (!arquivo.is_open()) {
        std::cout << "Arquivo de dados '" << nomeArquivo << "' nao encontrado. Criando novo conjunto de arquivos..." << std::endl;
        criarArquivos();
    } else {
        std::cout << "Arquivo de dados '" << nomeArquivo << "' aberto com sucesso." << std::endl;
    }
}

// Destrutor
HashingFile::~HashingFile() {
    if (arquivo.is_open()) {
        arquivo.close();
    }
}

// Cria os dois arquivos necessários: o de dados e o de índice (tabela hash)
void HashingFile::criarArquivos() {
    arquivo.open(nomeArquivo, std::ios::out | std::ios::binary);
    if (arquivo.is_open()) {
        std::cout << "Arquivo '" << nomeArquivo << "' criado." << std::endl;
        arquivo.close();
    }

    std::string nomeTabela = "tabela_hash.idx";
    std::ofstream tabela(nomeTabela, std::ios::out | std::ios::binary);
    if (tabela.is_open()) {
        long offset_vazio = -1;
        for (int i = 0; i < TAMANHO_TABELA; ++i) {
            tabela.write(reinterpret_cast<const char*>(&offset_vazio), sizeof(long));
        }
        tabela.close();
        std::cout << "Arquivo de indice '" << nomeTabela << "' criado com " << TAMANHO_TABELA << " posicoes." << std::endl;
    }
    
    arquivo.open(nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
}

// Insere um novo artigo usando encadeamento
long HashingFile::inserirArtigo(Artigo& novoArtigo) {
    if (!arquivo.is_open()) {
        std::cerr << "Erro: Arquivo de dados nao esta aberto." << std::endl;
        return -1;
    }

    novoArtigo.proximo_offset = -1;

    int endereco = novoArtigo.id % TAMANHO_TABELA;
    std::string nomeTabela = "tabela_hash.idx";

    std::fstream tabela(nomeTabela, std::ios::in | std::ios::out | std::ios::binary);
    if (!tabela.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo de indice." << std::endl;
        return -1;
    }

    long offset_inicio_cadeia;
    tabela.seekg(endereco * sizeof(long));
    tabela.read(reinterpret_cast<char*>(&offset_inicio_cadeia), sizeof(long));

    arquivo.seekg(0, std::ios::end);
    long nova_posicao = arquivo.tellg();

    if (offset_inicio_cadeia == -1) {
        tabela.seekp(endereco * sizeof(long));
        tabela.write(reinterpret_cast<const char*>(&nova_posicao), sizeof(long));
    } else {
        long offset_atual = offset_inicio_cadeia;
        Artigo artigo_temp;
        while (true) {
            arquivo.seekg(offset_atual);
            arquivo.read(reinterpret_cast<char*>(&artigo_temp), sizeof(Artigo));
            if (artigo_temp.proximo_offset == -1) {
                artigo_temp.proximo_offset = nova_posicao;
                arquivo.seekp(offset_atual);
                arquivo.write(reinterpret_cast<const char*>(&artigo_temp), sizeof(Artigo));
                break;
            }
            offset_atual = artigo_temp.proximo_offset;
        }
    }

    arquivo.seekp(nova_posicao);
    arquivo.write(reinterpret_cast<const char*>(&novoArtigo), sizeof(Artigo));
    

    tabela.close();
    return nova_posicao;
}

// IMPLEMENTAÇÃO DA BUSCA
Artigo HashingFile::buscarPorId(int id, int& blocosLidos) {
    blocosLidos = 0;
    Artigo artigo_vazio = {};
    artigo_vazio.ocupado = false;

    if (!arquivo.is_open()) {
        std::cerr << "Erro: Arquivo de dados nao esta aberto para busca." << std::endl;
        return artigo_vazio;
    }

    // 1. Calcula o hash para encontrar o bucket de início
    int endereco = id % TAMANHO_TABELA;
    std::string nomeTabela = "tabela_hash.idx";

    std::ifstream tabela(nomeTabela, std::ios::in | std::ios::binary);
    if (!tabela.is_open()) {
        std::cerr << "Erro: Nao foi possivel abrir o arquivo de indice para busca." << std::endl;
        return artigo_vazio;
    }

    // 2. Lê o ponteiro para o início da cadeia de colisão
    long offset_atual;
    tabela.seekg(endereco * sizeof(long));
    tabela.read(reinterpret_cast<char*>(&offset_atual), sizeof(long));
    tabela.close();

    // 3. Percorre a cadeia (lista encadeada) no arquivo de dados
    while (offset_atual != -1) {
        arquivo.seekg(offset_atual);
        Artigo artigo_temp;
        arquivo.read(reinterpret_cast<char*>(&artigo_temp), sizeof(Artigo));
        blocosLidos++;

        if (artigo_temp.id == id) {
            // Encontrou!
            return artigo_temp;
        }
        // Se não for o ID certo, pula para o próximo da cadeia
        offset_atual = artigo_temp.proximo_offset;
    }

    // Se o loop terminar, o ID não foi encontrado na cadeia
    return artigo_vazio;
}
