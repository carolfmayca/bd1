# --- Compilador e Flags ---
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -O2

# --- Diretórios ---
SRC_DIR = src
BIN_DIR = bin
DATA_DIR = data

# --- Arquivos Fonte ---
HASH_SRC = $(SRC_DIR)/hashing_file.cpp

# --- Alvos (Executáveis) ---
UPLOAD_EXEC = upload
FINDREC_EXEC = findrec
SEEK1_EXEC = seek1
SEEK2_EXEC = seek2

EXECUTABLES = $(UPLOAD_EXEC) $(FINDREC_EXEC) $(SEEK1_EXEC) $(SEEK2_EXEC)

.PHONY: build clean all

# --- Alvo Principal ---
build: $(BIN_DIR) $(DATA_DIR) $(EXECUTABLES)

all: build

# --- Regras para criar os diretórios ---
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

# --- Regras de Compilação ---
$(UPLOAD_EXEC): $(SRC_DIR)/upload.cpp $(HASH_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(FINDREC_EXEC): $(SRC_DIR)/findrec.cpp $(HASH_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SEEK1_EXEC): $(SRC_DIR)/seek1.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SEEK2_EXEC): $(SRC_DIR)/seek2.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# --- Alvo para Limpeza ---
clean:
	rm -rf $(EXECUTABLES) $(BIN_DIR)/* $(DATA_DIR)/artigos.dat