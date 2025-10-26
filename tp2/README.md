# Docker
**COMANDOS DEVEM SER EXECUTADOS A PARTIR DE `tp2`**

compila binarios: `make build`

criar imagem do docker: `make docker-build`

upload: `make docker-run-upload`

findrec: `make docker-run-findrec ID=<ID_DO_ARTIGO>`

seek1: `make docker-run-seek1 ID=<ID_DO_ARTIGO>`

seek2: `make docker-run-seek2 TITLE="<TÍTULO_DO_ARTIGO>"`


# Local
**COMANDOS DEVEM SER EXECUTADOS A PARTIR DE `tp2`**
upload: `g++ -std=c++17 -Wall -Wextra -I./include -O2 src/upload.cpp -o upload`

findrec: `g++ -std=c++17 -Wall -Wextra -I./include -O2 src/findrec.cpp -o findrec`

seek1: `g++ -std=c++17 -Wall -Wextra -I./include -O2 src/seek1.cpp -o seek1`

seek2: `g++ -std=c++17 -Wall -Wextra -I./include -O2 src/seek2.cpp -o seek2`
