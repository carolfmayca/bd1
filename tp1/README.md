## Trabalho Prático I - Bancos de Dados (2025/02)
Para executar o projeto, siga os passos abaixo. É necessário ter o Docker e o Docker Compose instalados.
### Construir e Iniciar os Serviços
Este comando irá construir a imagem da aplicação Python e iniciar os contêineres do banco de dados e da aplicação em segundo plano.

```docker compose up -d --build```


### (Opcional) Verificar a Saúde do Banco de Dados
Você pode usar este comando para garantir que o contêiner do PostgreSQL (db) está rodando e saudável.
```docker compose ps```


### Criar o Esquema e Carregar os Dados
Este comando executa o script tp1_3.2.py, que irá criar as tabelas e popular o banco de dados com os dados do arquivo snap_amazon.txt. Este processo pode levar alguns minutos.

```docker compose run --rm app python src/tp1_3.2.py --db-host db --db-port 5432 --db-name ecommerce --db-user postgres --db-pass postgres --input data/snap_amazon.txt```


### Executar o Dashboard
Este comando executa o script tp1_3.3.py, que irá rodar todas as 7 consultas do dashboard. Os resultados serão exibidos no terminal e também salvos em arquivos CSV na pasta out/.
Para rodar com o ASIN padrão:

```docker compose run --rm app python src/tp1_3.3.py --db-host db --db-port 5432 --db-name ecommerce --db-user postgres --db-pass postgres --output /app/out  ```


- Para rodar com um ASIN específico (opcional):
Para as consultas 1, 2 e 3, você pode especificar um produto diferente usando o argumento --product-asin.
```docker compose run --rm app python src/tp1_3.3.py --db-host db --db-port 5432 --db-name ecommerce --db-user postgres --db-pass postgres --output /app/out --product-asin 0801983258```


