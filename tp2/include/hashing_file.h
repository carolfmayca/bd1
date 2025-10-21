#pragma once

#include <string>
#include <fstream>

// Estrutura que representa um registo. Já não precisa do ponteiro de encadeamento.
struct Artigo {
    bool ocupado;
    int id;
    char titulo[301];
    int ano;
    char autores[151];
    int citacoes;
    char atualizacao[20];
    char snippet[1025];
};

// Define quantos artigos cabem em um único bloco.
const int REGISTOS_POR_BLOCO = 2;

// Estrutura que representa um bloco no disco.
// Este é o contentor que armazena múltiplos artigos.
struct Bloco {
    Artigo artigos[REGISTOS_POR_BLOCO];
    int num_registos_usados;   // Contador de quantos artigos estão neste bloco.
    long proximo_bloco_offset; // Ponteiro para o próximo bloco na cadeia de colisão.
};


class HashingFile {
public:
    HashingFile(const std::string& filename, int table_size);
    ~HashingFile();

    long inserirArtigo(Artigo& novoArtigo);
    Artigo buscarPorId(int id, int& blocosLidos);
    long getTotalBlocos();

private:
    void criarArquivos();

    std::string nomeArquivo;
    int TAMANHO_TABELA;
    std::fstream arquivo;
};

