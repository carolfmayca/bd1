# --- Compilador e Flags ---
CXX = g++
# -I./include avisa o compilador para procurar os .h na pasta 'include'
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -O2

# --- Diretórios ---
SRC_DIR = src
BIN_DIR = bin
DATA_DIR = data

# --- Arquivos Fonte ---
HASH_SRC = $(SRC_DIR)/hashing_file.cpp

# --- Alvos (Executáveis) ---
UPLOAD_EXEC = $(BIN_DIR)/upload
FINDREC_EXEC = $(BIN_DIR)/findrec
SEEK1_EXEC = $(BIN_DIR)/seek1
SEEK2_EXEC = $(BIN_DIR)/seek2

EXECUTABLES = $(UPLOAD_EXEC) $(FINDREC_EXEC) $(SEEK1_EXEC) $(SEEK2_EXEC)

# O alvo .PHONY diz ao Make que 'build' e 'clean' não são ficheiros reais
.PHONY: build clean all

# --- Alvo Principal ---
# Garante que os diretórios existem e compila tudo
build: $(BIN_DIR) $(DATA_DIR) $(EXECUTABLES)

# Alias para 'build'
all: build

# --- Regras para criar os diretórios (se não existirem) ---
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

# --- Regras de Compilação ---

# Regra para compilar o 'upload'
# O upload depende do seu main (upload.cpp) e do hashing
$(UPLOAD_EXEC): $(SRC_DIR)/upload.cpp $(HASH_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regra para compilar o 'findrec'
$(FINDREC_EXEC): $(SRC_DIR)/findrec.cpp $(HASH_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regra para compilar o 'seek1' (B-Tree Primária)
$(SEEK1_EXEC): $(SRC_DIR)/seek1.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regra para compilar o 'seek2' (B-Tree Secundária)
$(SEEK2_EXEC): $(SRC_DIR)/seek2.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# --- Alvo para Limpeza ---
# Remove os executáveis e diretórios criados
clean:
	rm -rf $(BIN_DIR)/* $(DATA_DIR)/artigos.dat

# Explicação das variáveis especiais:
# $@ : O nome do alvo da regra (ex: bin/upload)
# $^ : Os nomes de todas as dependências (ex: src/upload.cpp src/hashing_file.cpp)

