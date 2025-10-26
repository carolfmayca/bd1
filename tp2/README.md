# Docker
**COMANDOS DEVEM SER EXECUTADOS A PARTIR DE `tp2`**

criar imagem do docker: `docker build -t tp2-bd .`

upload: `docker run --rm -v "$(pwd)/data:/data" -v "$(pwd)/bin:/bin" tp2-bd /app/upload`

findrec: `docker run --rm -v "$(pwd)/data:/data" -v "$(pwd)/bin:/bin" tp2-bd /app/findrec <id do artigo>`

seek1: `docker run --rm -v "$(pwd)/data:/data" -v "$(pwd)/bin:/bin" tp2-bd /app/seek1 <ID_DO_ARTIGO>`

seek2: `docker run --rm -v "$(pwd)/data:/data" -v "$(pwd)/bin:/bin" tp2-bd /app/seek2 <TITULO_DO_ARTIGO>`


# Local
**COMANDOS DEVEM SER EXECUTADOS A PARTIR DE `tp2`**
upload: `g++ -std=c++17 -Wall -Wextra -I./include -O2 src/upload.cpp -o upload`

findrec: `g++ -std=c++17 -Wall -Wextra -I./include -O2 src/findrec.cpp -o findrec`

seek1: `g++ -std=c++17 -Wall -Wextra -I./include -O2 src/seek1.cpp -o seek1`

seek2: `g++ -std=c++17 -Wall -Wextra -I./include -O2 src/seek2.cpp -o seek2`
