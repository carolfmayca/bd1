# 1. Imagem Base
# Usamos o Debian "bookworm-slim" que é leve
FROM debian:bookworm-slim

# 2. Instala as dependências (compilador C++ e o Make)
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    make \
    && rm -rf /var/lib/apt/lists/*

# 3. Define o diretório de trabalho dentro do contêiner
WORKDIR /app

# 4. Copia TODOS os arquivos do seu projeto para dentro do contêiner
COPY . /app

# 5. Compila o projeto
RUN make build

# 6. Define o diretório /data como um volume
VOLUME ["/data"]

# 7. Define variáveis de ambiente (conforme o PDF)
ENV DATA_DIR=/data/ \
    CSV_PATH=/data/artigo.csv \
    LOG_LEVEL=info

# 8. Comando padrão (ajuda)
# Lista os programas compilados que estão prontos para rodar
CMD ["bash", "-c", "echo 'Imagem construida. Use docker run para executar os programas: /app/bin/upload, /app/bin/findrec, etc.' && ls -l /app/bin"]

