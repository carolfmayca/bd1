#pragma once

#include <string>
#include <fstream>

// representa um registro
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

// artigos que cabem em um único bloco
const int REGISTROS_POR_BLOCO = 2;

// representa um bloco no disco
struct Bloco {
    Artigo artigos[REGISTROS_POR_BLOCO];
    int num_registros_usados;
    long proximo_bloco_offset;
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

