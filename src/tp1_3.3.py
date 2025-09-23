import argparse
import logging
import sys
import os
import psycopg2
import pandas as pd

# Configuração básica do logging para exibir mensagens no terminal
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def query_1(cursor, product_asin):
    """
    Dado um produto, listar os 5 comentários mais úteis e com maior avaliação 
    e os 5 comentários mais úteis e com menor avaliação.
    """
    if not product_asin:
        logging.warning("Nenhum ASIN de produto fornecido para a Query 1. Pulando.")
        return

    logging.info(f"Executando Query 1 para o ASIN: {product_asin}")
    
    sql_top_rated = """
        SELECT asin, rating, helpful, votes, customer FROM reviews 
        WHERE asin = %s
        ORDER BY rating DESC, helpful DESC 
        LIMIT 5;
    """ 
    cursor.execute(sql_top_rated, (product_asin,))
    results_top = cursor.fetchall()
    
    print("\n--- Query 1: Top 5 Comentários (Mais Úteis, Maior Avaliação) ---")
    if results_top:
        df_top = pd.DataFrame(results_top, columns=[desc[0] for desc in cursor.description])
        print(df_top)
    else:
        print(f"Nenhum comentário encontrado para o produto com ASIN {product_asin}.")

    sql_low_rated = """
        SELECT asin, rating, helpful, votes, customer FROM reviews 
        WHERE asin = %s
        ORDER BY helpful DESC, rating ASC 
        LIMIT 5;
    """
    cursor.execute(sql_low_rated, (product_asin,))
    results_low = cursor.fetchall()

    print("\n--- Query 1: Top 5 Comentários (Mais Úteis, Menor Avaliação) ---")
    if results_low:
        df_low = pd.DataFrame(results_low, columns=[desc[0] for desc in cursor.description])
        print(df_low)
    else:
        print(f"Nenhum comentário encontrado para o produto com ASIN {product_asin}.")

def query_2(cursor, product_asin):
    """
    Dado um produto, listar os produtos similares com maiores vendas (melhor salesrank) do que ele.
    """
    if not product_asin:
        logging.warning("Nenhum ASIN de produto fornecido para a Query 2. Pulando.")
        return

    logging.info(f"Executando Query 2 para o ASIN: {product_asin}")
    
    sql = """
        SELECT
            p2.asin,
            p2.title,
            p2.salesrank
        FROM
            products AS p1
        JOIN
            "Similar" AS s ON p1.asin = s.asin_product
        JOIN
            products AS p2 ON s.asin_similar = p2.asin
        WHERE
            p1.asin = %s AND p2.salesrank < p1.salesrank
        ORDER BY
            p2.salesrank ASC;
    """
    cursor.execute(sql, (product_asin,))
    results = cursor.fetchall()
    
    print("\n--- Query 2: Produtos Similares com Melhor Salesrank ---")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
    else:
        print(f"Nenhum produto similar com melhor salesrank encontrado para o ASIN {product_asin}.")


# def query_3(cursor, product_asin):
    """
    Dado um produto, mostrar a evolução diária das médias de avaliação ao longo do período coberto no arquivo.
    """
    if not product_asin:
        logging.warning("Nenhum ASIN de produto fornecido para a Query 3. Pulando.")
        return

    logging.info(f"Executando Query 2 para o ASIN: {product_asin}")

    sql = """
        SELECT
            DATE(review_date) AS review_date,
            AVG(rating) AS average_rating
        FROM
            reviews
        WHERE
            asin = %s
        GROUP BY
            DATE(review_date)
        ORDER BY
            DATE(review_date);
    """
    cursor.execute(sql, (product_asin,))
    results = cursor.fetchall()

    print("\n--- Query 3: Evolução Diária das Médias de Avaliação ---")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
    else:
        print(f"Nenhuma avaliação encontrada para o produto com ASIN {product_asin}.")

def query_4(cursor):
    """
    Listar os 10 produtos líderes de venda em cada grupo de produtos.
    """
    logging.info("Executando Query 4 para listar os 10 produtos líderes de venda em cada grupo de produtos.")
    

    sql = """
        WITH ranked_products AS (
            SELECT
                title,
                asin,
                "group",
                salesrank,
                ROW_NUMBER() OVER (PARTITION BY "group" ORDER BY salesrank DESC) AS rn
            FROM
                products
            WHERE
                salesrank >= 0
        )
        SELECT
            *
        FROM
            ranked_products
        WHERE
            rn <= 10;
    """
    cursor.execute(sql)
    results = cursor.fetchall()
    
    print("\n--- Query 4: Top 10 Produtos Líderes de Venda por Grupo ---")
    if results:
        # A filtragem agora é feita no SQL, então o Dataframe já vem pronto.
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
    else:
        print("Nenhum produto encontrado.")

def query_5(cursor):
    """
    Listar os 10 produtos com a maior média de avaliações úteis positivas por produto.
    """
    logging.info("Executando Query 5 para listar os 10 produtos com a maior média de avaliações úteis positivas por produto.")

    sql = """
        SELECT
            p.asin,
            p.title,
            CAST(AVG(r.helpful) AS NUMERIC(10, 2)) AS media_avaliacoes_uteis        
        FROM
            reviews AS r
        JOIN
            products AS p ON r.asin = p.asin
        GROUP BY
            p.asin, p.title
        ORDER BY
            media_avaliacoes_uteis DESC
        LIMIT 10;
    """
    cursor.execute(sql)
    results = cursor.fetchall()

    print("\n--- Query 5: Top 10 Produtos com Maior Média de Avaliações Úteis Positivas ---\n")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
    else:
        print("Nenhum produto encontrado.")

def query_6(cursor):
    """
    Listar as 5 categorias com a maior média de avaliações úteis positivas por produto.
    """

    logging.info("Executando Query 6 para listar as 5 categorias com a maior média de avaliações úteis positivas por produto.")

    sql = """
        SELECT
            c.categorie_name,
            AVG(r.helpful)::NUMERIC(10, 2) AS media_avaliacoes_uteis
        FROM
            reviews AS r
        JOIN
            Categorie_product AS cp ON r.asin = cp.asin
        JOIN
            Categories AS c ON cp.id_category = c.id_categorie
        GROUP BY
            c.id_categorie, c.categorie_name
        ORDER BY
            media_avaliacoes_uteis DESC
        LIMIT 5;
    """
    cursor.execute(sql)
    results = cursor.fetchall()

    print("\n--- Query 6: Top 5 Categorias com Maior Média de Avaliações Úteis Positivas ---\n")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
    else:
        print("Nenhuma categoria encontrada.")



def main():
    """
    Função principal que orquestra a execução do script.
    """
    parser = argparse.ArgumentParser(description="Dashboard de Consultas para o Trabalho Prático de BD")
    parser.add_argument('--db-host', required=True, help="Host do banco de dados")
    parser.add_argument('--db-port', required=True, help="Porta do banco de dados")
    parser.add_argument('--db-name', required=True, help="Nome do banco de dados")
    parser.add_argument('--db-user', required=True, help="Usuário do banco de dados")
    parser.add_argument('--db-pass', required=True, help="Senha do banco de dados")
    # parser.add_argument('--product-asin', help="ASIN do produto")
    parser.add_argument('--output', default='/app/out', help="Diretório para salvar os resultados em CSV")
    
    args = parser.parse_args()
    
    ASIN_PARA_TESTE = '0738700797'

    os.makedirs(args.output, exist_ok=True)
    
    conn = None
    try:
        conn_string = f"host={args.db_host} port={args.db_port} dbname={args.db_name} user={args.db_user} password={args.db_pass}"
        
        logging.info("Conectando ao banco de dados...")
        conn = psycopg2.connect(conn_string)
        cursor = conn.cursor()
        logging.info("Conexão bem-sucedida.")

        # query_1(cursor, ASIN_PARA_TESTE)
        # query_2(cursor, ASIN_PARA_TESTE)
        # query_3(cursor, ASIN_PARA_TESTE)
        # query_4(cursor)
        # query_5(cursor)
        query_6(cursor)

        cursor.close()
    except psycopg2.Error as e:
        logging.error(f"Erro de banco de dados: {e}")
        sys.exit(1)
    finally:
        if conn:
            conn.close()
            logging.info("Conexão com o banco de dados fechada.")

    logging.info("Dashboard executado com sucesso.")
    sys.exit(0)

if __name__ == '__main__':
    main()

