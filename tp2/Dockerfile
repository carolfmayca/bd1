# ====== Build ======
FROM gcc:13 AS build
WORKDIR /app
COPY . .
RUN make clean || true && rm -rf bin && make -B build

# ====== Runtime ======
FROM gcc:13
WORKDIR /app
COPY --from=build /app/bin /app/bin
ENV DATA_DIR=/data \
    DB_DIR=/data/db \
    BIN_DIR=/app/bin \
    CSV_PATH=/data/artigo.csv \
    LOG_LEVEL=info
VOLUME ["/data"]
CMD ["bash", "-lc", "echo 'Imagem pronta'; ls -l /app/bin && strings /usr/lib/x86_64-linux-gnu/libstdc++.so.6 | grep GLIBCXX_3.4.32 | head -n1 || true"]
