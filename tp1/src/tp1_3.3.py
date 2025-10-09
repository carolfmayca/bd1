import argparse
import logging
import sys
import os
import psycopg2
import pandas as pd

# Configuração básica do logging para exibir mensagens no terminal
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

def query_1(cursor, product_asin, output_dir):
    """
    Lista os 5 comentários com maior e menor avaliação, salvando em CSV.
    """
    if not product_asin:
        return
    logging.info(f"Executando Query 1 para o ASIN: {product_asin}")
    
    # Maior Avaliação
    sql_top = """
        SELECT asin, rating, helpful, votes, customer 
        FROM reviews 
        WHERE asin = %s ORDER BY rating DESC, helpful DESC LIMIT 5;
    """

    cursor.execute(sql_top, (product_asin,))
    results_top = cursor.fetchall()
    print("\n--- Query 1: Top 5 Comentários (Maior Avaliação, Mais Úteis) ---")

    if results_top:
        df_top = pd.DataFrame(results_top, columns=[desc[0] for desc in cursor.description])
        print(df_top)
        file_path = os.path.join(output_dir, f"q1_maior_avaliacao_{product_asin}.csv")
        df_top.to_csv(file_path, index=False)
        logging.info(f"Resultado salvo em: {file_path}")

    # Menor Avaliação
    sql_low = """
        SELECT asin, rating, helpful, votes, customer 
        FROM reviews 
        WHERE asin = %s ORDER BY rating ASC, helpful DESC LIMIT 5;
    """

    cursor.execute(sql_low, (product_asin,))
    results_low = cursor.fetchall()
    print("\n--- Query 1: Top 5 Comentários (Menor Avaliação, Mais Úteis) ---")
    if results_low:
        df_low = pd.DataFrame(results_low, columns=[desc[0] for desc in cursor.description])
        print(df_low)
        file_path = os.path.join(output_dir, f"q1_menor_avaliacao_{product_asin}.csv")
        df_low.to_csv(file_path, index=False)
        logging.info(f"Resultado salvo em: {file_path}")

def query_2(cursor, product_asin, output_dir):
    if not product_asin:
        return
    logging.info(f"Executando Query 2 para o ASIN: {product_asin}")

    sql = """
        SELECT p2.asin, p2.title, p2.salesrank 
        FROM products AS p1
        JOIN "Similar" AS s ON p1.asin = s.asin_product
        JOIN products AS p2 ON s.asin_similar = p2.asin
        WHERE p1.asin = %s AND p2.salesrank < p1.salesrank AND p2.salesrank > 0
        ORDER BY p2.salesrank ASC;
    """

    cursor.execute(sql, (product_asin,))
    results = cursor.fetchall()
    print("\n--- Query 2: Produtos Similares com Melhor Salesrank ---")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
        file_path = os.path.join(output_dir, f"q2_similares_melhor_venda_{product_asin}.csv")
        df.to_csv(file_path, index=False)
        logging.info(f"Resultado salvo em: {file_path}")

def query_3(cursor, product_asin, output_dir):
    if not product_asin:
        return
    logging.info(f"Executando Query 3 para o ASIN: {product_asin}")

    sql = """
        WITH daily_avg AS (
            SELECT "date", AVG(rating) AS avg_rating 
            FROM reviews
            WHERE asin = %(asin)s GROUP BY "date"
        ), date_series AS (
            SELECT generate_series(
                (SELECT MIN("date") FROM reviews WHERE asin = %(asin)s),
                (SELECT MAX("date") FROM reviews WHERE asin = %(asin)s),
                '1 day'::interval
            )::date AS dia
        )
        SELECT ds.dia, COALESCE(CAST(da.avg_rating AS NUMERIC(10, 2)), 0) AS media_diaria
        FROM date_series ds 
        LEFT JOIN daily_avg da ON ds.dia = da."date"
        ORDER BY ds.dia ASC;
    """

    cursor.execute(sql, {'asin': product_asin})
    results = cursor.fetchall()
    print("\n--- Query 3: Evolução Diária da Média de Avaliação ---")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
        file_path = os.path.join(output_dir, f"q3_evolucao_diaria_{product_asin}.csv")
        df.to_csv(file_path, index=False)
        logging.info(f"Resultado salvo em: {file_path}")

def query_4(cursor, output_dir):
    logging.info("Executando Query 4...")

    sql = """
        WITH ranked_products AS (
            SELECT title, asin, "group", salesrank,
                   ROW_NUMBER() OVER (PARTITION BY "group" ORDER BY salesrank ASC) AS rn
            FROM products WHERE salesrank > 0
        )
        SELECT title, asin, "group", salesrank 
        FROM ranked_products 
        WHERE rn <= 10;
    """

    cursor.execute(sql)
    results = cursor.fetchall()
    print("\n--- Query 4: Top 10 Produtos Líderes de Venda por Grupo ---")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
        file_path = os.path.join(output_dir, "q4_top10_produtos_por_grupo.csv")
        df.to_csv(file_path, index=False)
        logging.info(f"Resultado salvo em: {file_path}")

def query_5(cursor, output_dir):
    logging.info("Executando Query 5...")

    sql = """
        SELECT p.asin, p.title, AVG(r.helpful)::NUMERIC(10, 2) AS media_avaliacoes_uteis
        FROM reviews AS r JOIN products AS p ON r.asin = p.asin
        WHERE r.rating > 3
        GROUP BY p.asin, p.title ORDER BY media_avaliacoes_uteis DESC LIMIT 10;
    """

    cursor.execute(sql)
    results = cursor.fetchall()
    print("\n--- Query 5: Top 10 Produtos com Maior Média de Avaliações Úteis Positivas ---")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
        file_path = os.path.join(output_dir, "q5_top10_produtos_media_util.csv")
        df.to_csv(file_path, index=False)
        logging.info(f"Resultado salvo em: {file_path}")

def query_6(cursor, output_dir):
    logging.info("Executando Query 6...")

    sql = """
        SELECT c.id_categorie, c.categorie_name, AVG(r.helpful)::NUMERIC(10, 2) AS media_avaliacoes_uteis
        FROM reviews AS r JOIN Categorie_product AS cp ON r.asin = cp.asin
        JOIN Categories AS c ON cp.id_category = c.id_categorie
        WHERE r.rating > 3
        GROUP BY c.id_categorie, c.categorie_name ORDER BY media_avaliacoes_uteis DESC LIMIT 5;
    """

    cursor.execute(sql)
    results = cursor.fetchall()
    print("\n--- Query 6: Top 5 Categorias com Maior Média de Avaliações Úteis Positivas ---")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
        file_path = os.path.join(output_dir, "q6_top5_categorias_media_util.csv")
        df.to_csv(file_path, index=False)
        logging.info(f"Resultado salvo em: {file_path}")

def query_7(cursor, output_dir):
    logging.info("Executando Query 7...")

    sql = """
        WITH ranked_customers AS (
            SELECT r.customer, p."group", COUNT(*) as comment_count,
                   ROW_NUMBER() OVER (PARTITION BY p."group" ORDER BY COUNT(*) DESC) as rn
            FROM reviews r JOIN products p ON r.asin = p.asin
            GROUP BY r.customer, p."group"
        )
        SELECT customer, "group", comment_count 
        FROM ranked_customers 
        WHERE rn <= 10;
    """

    cursor.execute(sql)
    results = cursor.fetchall()
    print("\n--- Query 7: Top 10 Clientes com Mais Comentários por Grupo ---")
    if results:
        df = pd.DataFrame(results, columns=[desc[0] for desc in cursor.description])
        print(df)
        file_path = os.path.join(output_dir, "q7_top10_clientes_por_grupo.csv")
        df.to_csv(file_path, index=False)
        logging.info(f"Resultado salvo em: {file_path}")

def main():
    """
    Função principal que define a execução do script.
    """
    parser = argparse.ArgumentParser(description="Dashboard de Consultas para o Trabalho Prático de BD")
    parser.add_argument('--db-host', required=True, help="Host do banco de dados")
    parser.add_argument('--db-port', required=True, help="Porta do banco de dados")
    parser.add_argument('--db-name', required=True, help="Nome do banco de dados")
    parser.add_argument('--db-user', required=True, help="Usuário do banco de dados")
    parser.add_argument('--db-pass', required=True, help="Senha do banco de dados")
    parser.add_argument('--output', default='/app/out', help="Diretório para salvar os resultados em CSV")
    parser.add_argument('--product-asin', help="ASIN do produto para consultas específicas (1, 2, 3). Se não for fornecido, usa um valor padrão.")
    
    args = parser.parse_args()
    
    ASIN_PADRAO = '0807220280'

    if args.product_asin:
        asin_para_teste = args.product_asin
        logging.info(f"Usando ASIN fornecido pelo usuário: {asin_para_teste}")
    else:
        asin_para_teste = ASIN_PADRAO
        logging.info(f"Nenhum ASIN fornecido. Usando ASIN padrão para testes: {asin_para_teste}")

    os.makedirs(args.output, exist_ok=True)
    
    conn = None
    try:
        conn_string = f"host={args.db_host} port={args.db_port} dbname={args.db_name} user={args.db_user} password={args.db_pass}"
        
        logging.info("Conectando ao banco de dados...")
        conn = psycopg2.connect(conn_string)
        cursor = conn.cursor()
        logging.info("Conexão bem-sucedida.")

        # executa todas as queries
        query_1(cursor, asin_para_teste, args.output)
        query_2(cursor, asin_para_teste, args.output)
        query_3(cursor, asin_para_teste, args.output)
        query_4(cursor, args.output)
        query_5(cursor, args.output)
        query_6(cursor, args.output)
        query_7(cursor, args.output)

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

