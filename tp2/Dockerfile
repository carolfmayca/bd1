# 1. Imagem Base (conforme especificado no PDF)
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
# (O "." significa "tudo" da pasta atual)
COPY . /app

# 5. Compila o projeto
# Este é o comando-chave: ele executa o alvo "build" do nosso Makefile
RUN make build

# 6. Define o diretório /data como um volume
# É aqui que o CSV de entrada será lido e os arquivos .dat/.idx serão gravados
VOLUME ["/data"]

# 7. Define variáveis de ambiente (conforme o PDF)
# O seu upload_main.cpp modificado vai usar estas variáveis
ENV DATA_DIR=/data/ \
    CSV_PATH=/data/artigo.csv \
    LOG_LEVEL=info

# 8. Comando padrão (ajuda)
# Lista os programas compilados que estão prontos para rodar
CMD ["bash", "-c", "echo 'Imagem construida. Use docker run para executar os programas: /app/bin/upload, /app/bin/findrec, etc.' && ls -l /app/bin"]

