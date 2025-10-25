**COMANDOS DEVEM SER EXECUTADOS A PARTIR DE `tp2`**

criar imagem do docker: `docker build -t tp2-bd .`

upload: `docker run --rm -v "$(pwd)/data:/data" tp2-bd /app/bin/upload`

findrec: `docker run --rm -v "$(pwd)/data:/data" tp2-bd /app/bin/findrec <id do artigo>`

seek1: `docker run --rm -v "$(pwd)/data:/data" tp2-bd /app/bin/seek1 <ID_DO_ARTIGO>`

seek2: `docker run --rm -v "$(pwd)/data:/data" tp2-bd /app/bin/seek2 <TITULO_DO_ARTIGO>`'