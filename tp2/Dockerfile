# 1. Imagem Base
FROM debian:bookworm-slim

# 2. Instala as dependências
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# 3. Define o diretório de trabalho
WORKDIR /app

# 4. Copia todos os arquivos
COPY . /app

# 5. Compila o projeto
RUN make build

# 6. Define os volumes
VOLUME ["/data", "/bin"]

# 7. Define variáveis de ambiente - MODIFICADO
ENV DATA_DIR=/data \
    BIN_DIR=/bin \
    CSV_PATH=/data/artigo.csv \
    LOG_LEVEL=info

# 8. Comando padrão
CMD ["bash", "-c", "echo 'Imagem construida. Use docker run para executar os programas.' && ls -l /app/bin"]