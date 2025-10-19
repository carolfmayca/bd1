#pragma once // Evita que o arquivo seja incluído mais de uma vez

#include <string>
#include <fstream>

// Estrutura que representa um registro no arquivo de dados
struct Artigo {
    bool ocupado;
    int id;
    char titulo[301];     // 300 + 1 para o char nulo '\0'
    int ano;
    char autores[151];    // 150 + 1 para o char nulo '\0'
    int citacoes;
    char atualizacao[20];
    char snippet[1025];   // 1024 + 1 para o char nulo '\0'
    long proximo_offset;  // Guarda a posição em bytes do próximo artigo na cadeia, ou -1 se for o último
};

class HashingFile {
public:
    // Construtor: recebe o nome do arquivo e o tamanho da tabela
    HashingFile(const std::string& filename, int table_size);
    // Destrutor: fecha o arquivo se estiver aberto
    ~HashingFile();

    // Insere um novo artigo no arquivo
    // Retorna a posição (em bytes) onde o artigo foi inserido ou -1 se deu erro.
    // O 'const' foi removido para permitir a modificação do proximo_offset.
    long inserirArtigo(Artigo& novoArtigo);

    // Busca um artigo pelo ID
    // Retorna o artigo encontrado e o número de blocos lidos
    Artigo buscarPorId(int id, int& blocosLidos);

private:
    // Cria o arquivo de dados e o arquivo de índice (tabela hash)
    void criarArquivos();

    std::string nomeArquivo;
    int TAMANHO_TABELA;
    std::fstream arquivo; // Para ler e escrever no arquivo de dados
};
