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
        logging.warning("Query 1 requer um --product-asin. Pulando.")
        return

    logging.info(f"Executando Query 1 para o ASIN: {product_asin}")
    
    sql_top_rated = """
        SELECT asin, rating, helpful, votes, customer FROM reviews 
        WHERE asin = %s
        ORDER BY helpful DESC, rating DESC 
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
        logging.warning("Query 2 requer um --product-asin. Pulando.")
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
    parser.add_argument('--product-asin', help="ASIN do produto para consultas específicas (1, 2, 3)")
    parser.add_argument('--output', default='/app/out', help="Diretório para salvar os resultados em CSV")
    
    args = parser.parse_args()

    # Cria o diretório de output se ele não existir
    os.makedirs(args.output, exist_ok=True)
    
    conn = None
    try:
        # Constrói a string de conexão
        conn_string = f"host={args.db_host} port={args.db_port} dbname={args.db_name} user={args.db_user} password={args.db_pass}"
        
        logging.info("Conectando ao banco de dados...")
        conn = psycopg2.connect(conn_string)
        cursor = conn.cursor()
        logging.info("Conexão bem-sucedida.")

        # --- Execução das Queries ---

        query_2(cursor, args.product_asin)

        cursor.close()
    except psycopg2.Error as e:
        logging.error(f"Erro de banco de dados: {e}")
        sys.exit(1) # Termina com código de erro
    finally:
        if conn:
            conn.close()
            logging.info("Conexão com o banco de dados fechada.")

    logging.info("Dashboard executado com sucesso.")
    sys.exit(0) # Termina com código de sucesso

if __name__ == '__main__':
    main()


