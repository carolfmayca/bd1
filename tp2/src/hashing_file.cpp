#include "../include/hashing_file.h"
#include <iostream>
#include <vector>

// Construtor
HashingFile::HashingFile(const std::string& filename, int table_size)
    : nomeArquivo(filename), TAMANHO_TABELA(table_size) {
    
    // Tenta abrir o arquivo já existente para leitura e escrita binária
    arquivo.open(nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);

    // Se o arquivo não existir (falha ao abrir), cria um novo
    if (!arquivo.is_open()) {
        std::cout << "Arquivo de dados '" << nomeArquivo << "' nao encontrado. Criando um novo..." << std::endl;
        criarArquivo();
        // Tenta abrir novamente após a criação
        arquivo.open(nomeArquivo, std::ios::in | std::ios::out | std::ios::binary);
        if (!arquivo.is_open()){
             std::cerr << "Erro fatal: nao foi possivel abrir o arquivo apos a criacao." << std::endl;
        }
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

// Cria o arquivo de dados inicial e o preenche com registros vazios
void HashingFile::criarArquivo() {
    // Abre em modo de escrita, apagando qualquer conteúdo anterior (trunc)
    std::ofstream novoArquivo(nomeArquivo, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!novoArquivo.is_open()) {
        std::cerr << "Erro fatal: nao foi possivel criar o arquivo '" << nomeArquivo << "'." << std::endl;
        return;
    }
    
    Artigo artigoVazio = {}; // Inicializa todos os campos com zero/false
    artigoVazio.ocupado = false;

    // Escreve TAMANHO_TABELA registros vazios no arquivo
    for (int i = 0; i < TAMANHO_TABELA; ++i) {
        novoArquivo.write(reinterpret_cast<const char*>(&artigoVazio), sizeof(Artigo));
    }
    novoArquivo.close();
    std::cout << "Arquivo '" << nomeArquivo << "' criado com " << TAMANHO_TABELA << " posicoes." << std::endl;
}

// Insere um novo artigo usando hashing com sondagem linear
long HashingFile::inserirArtigo(const Artigo& novoArtigo) {
    if (!arquivo.is_open()) {
        std::cerr << "Erro: Arquivo nao esta aberto para insercao." << std::endl;
        return -1;
    }

    // 1. Calcula o endereço inicial usando a função de hash
    int enderecoBase = novoArtigo.id % TAMANHO_TABELA;
    int enderecoAtual = enderecoBase;
    Artigo artigoLido;

    // 2. Inicia a sondagem linear (procura por um balde vazio)
    for (int i = 0; i < TAMANHO_TABELA; ++i) {
        long pos = (long)enderecoAtual * sizeof(Artigo);
        arquivo.seekg(pos);
        arquivo.read(reinterpret_cast<char*>(&artigoLido), sizeof(Artigo));

        // 3. Verifica se o balde está livre
        if (!artigoLido.ocupado) {
            // Encontrou um lugar! Posiciona o cursor para escrita (seekp) e grava
            arquivo.seekp(pos);
            arquivo.write(reinterpret_cast<const char*>(&novoArtigo), sizeof(Artigo));
            
            std::cout << "Artigo ID " << novoArtigo.id << " inserido na posicao " << enderecoAtual << " (offset: " << pos << " bytes)." << std::endl;

            // Retorna a posição em bytes, que será usada pelos arquivos de índice
            return pos; 
        }

        // Se estiver ocupado, vai para o próximo balde (sondagem linear)
        enderecoAtual = (enderecoAtual + 1) % TAMANHO_TABELA;

        // Se deu a volta completa, a tabela está cheia
        if (enderecoAtual == enderecoBase) {
            std::cerr << "Erro: Tabela Hash esta cheia! Nao foi possivel inserir o artigo ID " << novoArtigo.id << std::endl;
            return -1;
        }
    }
    return -1; // Tabela cheia
}


Artigo HashingFile::buscarPorId(int id, int& blocosLidos) {
    blocosLidos = 0;
    if (!arquivo.is_open()) {
        std::cerr << "Erro: Arquivo nao esta aberto para busca." << std::endl;
        return {}; // Retorna um artigo vazio
    }

    // 1. Calcula o endereço inicial usando a mesma função de hash
    int enderecoBase = id % TAMANHO_TABELA;
    int enderecoAtual = enderecoBase;
    Artigo artigoLido;

    // 2. Inicia a sondagem linear para encontrar o ID
    for (int i = 0; i < TAMANHO_TABELA; ++i) {
        long pos = (long)enderecoAtual * sizeof(Artigo);
        arquivo.seekg(pos);
        arquivo.read(reinterpret_cast<char*>(&artigoLido), sizeof(Artigo));
        blocosLidos++;

        // 3. Verifica se o balde está ocupado
        if (artigoLido.ocupado) {
            // Se estiver ocupado, verifica se é o ID que procuramos
            if (artigoLido.id == id) {
                std::cout << "Artigo ID " << id << " encontrado na posicao " << enderecoAtual << "." << std::endl;
                return artigoLido; // Encontrou! Retorna o artigo.
            }
        } else {
            // Se encontrou um balde vazio, significa que o ID não existe no arquivo
            // (pois na inserção, ele teria sido colocado aqui).
            std::cerr << "Artigo ID " << id << " nao encontrado (busca terminou em balde vazio)." << std::endl;
            return {}; // Retorna um artigo vazio
        }

        // Se não era o ID correto, vai para o próximo balde
        enderecoAtual = (enderecoAtual + 1) % TAMANHO_TABELA;
        
        // Se deu a volta completa, o item não existe
        if (enderecoAtual == enderecoBase) {
            break; 
        }
    }

    std::cerr << "Artigo ID " << id << " nao encontrado (tabela inteira verificada)." << std::endl;
    return {}; // Retorna um artigo vazio
}