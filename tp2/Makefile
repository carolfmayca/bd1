# --- Compilador e Flags ---
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -O2

# --- Diretórios ---
SRC_DIR  = src
BIN_DIR  = bin
DATA_DIR = data

# --- Docker ---
PWD_ABS          := $(shell pwd)
DOCKER_IMAGE     := tp2-bd
# Só montamos /data (nunca /bin). Variáveis/flags do docker run ficam aqui:
DOCKER_RUN_OPTS  := docker run --rm -v "$(PWD_ABS)/data:/data"

# --- Fontes ---
HASH_SRC = $(SRC_DIR)/hashing_file.cpp

# --- Executáveis (no host) ---
UPLOAD_EXEC  = $(BIN_DIR)/upload
FINDREC_EXEC = $(BIN_DIR)/findrec
SEEK1_EXEC   = $(BIN_DIR)/seek1
SEEK2_EXEC   = $(BIN_DIR)/seek2
EXECUTABLES  = $(UPLOAD_EXEC) $(FINDREC_EXEC) $(SEEK1_EXEC) $(SEEK2_EXEC)

# Permite usar TITULO=... como alias de TITLE=...
TITLE ?= $(TITULO)

# --- PHONY ---
.PHONY: all build clean docker-build docker-prep docker-run-upload docker-run-findrec docker-run-seek1 docker-run-seek2 index-local

# --- Alvo Principal ---
all: build

# Compila localmente e garante diretórios
build: $(BIN_DIR) $(DATA_DIR) $(DATA_DIR)/db $(EXECUTABLES)

# Diretórios
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(DATA_DIR):
	mkdir -p $(DATA_DIR)

$(DATA_DIR)/db:
	mkdir -p $(DATA_DIR)/db

# --- Regras de Compilação ---
$(UPLOAD_EXEC): $(SRC_DIR)/upload.cpp $(HASH_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(FINDREC_EXEC): $(SRC_DIR)/findrec.cpp $(HASH_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SEEK1_EXEC): $(SRC_DIR)/seek1.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SEEK2_EXEC): $(SRC_DIR)/seek2.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# --- Docker ---
docker-build:
	docker build -t $(DOCKER_IMAGE) .

# Garante pastas antes dos runs
docker-prep: | $(BIN_DIR) $(DATA_DIR) $(DATA_DIR)/db

# Nos runs via docker, os programas devem respeitar OUT_DIR (gravando .idx em /data/db)
docker-run-upload: docker-prep
	$(DOCKER_RUN_OPTS) -e DATA_DIR=/data -e DB_DIR=/data/db $(DOCKER_IMAGE) /app/bin/upload

docker-run-findrec: docker-prep
	@test -n "$(ID)" || (echo "Uso: make docker-run-findrec ID=<ID_DO_ARTIGO>"; exit 1)
	$(DOCKER_RUN_OPTS) -e DATA_DIR=/data -e DB_DIR=/data/db $(DOCKER_IMAGE) /app/bin/findrec $(ID)

docker-run-seek1: docker-prep
	@test -n "$(ID)" || (echo "Uso: make docker-run-seek1 ID=<ID_DO_ARTIGO>"; exit 1)
	$(DOCKER_RUN_OPTS) -e DATA_DIR=/data -e DB_DIR=/data/db $(DOCKER_IMAGE) /app/bin/seek1 $(ID)

docker-run-seek2: docker-prep
	@test -n "$(TITLE)" || (echo 'Uso: make docker-run-seek2 TITLE="TITULO_DO_ARTIGO"'; exit 1)
	$(DOCKER_RUN_OPTS) -e DATA_DIR=/data -e DB_DIR=/data/db $(DOCKER_IMAGE) /app/bin/seek2 "$(TITLE)"


# --- (Opcional) gerar índices localmente usando os binários compilados ---
index-local: build
	OUT_DIR=$(DATA_DIR)/db ./$(UPLOAD_EXEC)

# --- Limpeza ---
clean:
	rm -rf $(BIN_DIR)/ $(DATA_DIR)/artigos.dat $(DATA_DIR)/db/
