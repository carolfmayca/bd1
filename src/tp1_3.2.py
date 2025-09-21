import argparse
import os
import psycopg2
import re
from psycopg2.extras import execute_batch

def create_schema(conn):
    """
    Cria o esquema do banco de dados executando o script SQL.
    """
    with conn.cursor() as cur:
        with open('./sql/schema.sql', 'r') as f:
            sql_script = f.read()
            cur.execute(sql_script)
    print("Esquema do banco de dados criado com sucesso.")


def find_next_line(iterator, text):
    """
    Avança o iterador até encontrar uma linha que contenha o texto desejado.
    Retorna a linha encontrada ou None se o arquivo terminar antes de encontrar.
    """
    for line in iterator:
        if  text in line:
            return line
    return None

def batch_insert_products(conn, data):
    """Insere uma lista de produtos em lote."""
    sql = """
        INSERT INTO Products (Id, ASIN, title, "group", salesrank, numSimilar, numCategories, numReviews, downloaded, avg_rating)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        ON CONFLICT (ASIN) DO NOTHING;
    """
    with conn.cursor() as cur:
        execute_batch(cur, sql, data)



def find_id(categorie):
    """
    Encontra os IDs das categorias em uma string de categorias.
    Retorna uma lista de tuplas (nome_da_categoria, id_da_categoria)."""
    if categorie is None:
        return None
    pattern = r'([^|\[]*?)\s*\[(\d+)\]'
    categorie = str(categorie)
    match = re.findall(pattern, categorie)
    if match:
        return match
    return None

def batch_insert_general_categories(conn, data):
    """Insere uma lista de categorias gerais em lote."""
    sql = """
        INSERT INTO Categories (id_categorie, categorie_name, id_father)
        VALUES (%s, %s, %s)
        ON CONFLICT (id_categorie) DO NOTHING;
    """
    with conn.cursor() as cur:
        execute_batch(cur, sql, data)

def batch_insert_reviews(conn, data):
    """Insere uma lista de reviews em lote."""
    sql = """
        INSERT INTO Reviews (ASIN, "date", customer, rating, votes, helpful)
        VALUES (%s, %s, %s, %s, %s, %s)
        ON CONFLICT DO NOTHING;
    """
    with conn.cursor() as cur:
        execute_batch(cur, sql, data)

def batch_insert_product_categories(conn, data):
    """Insere uma lista de relações produto-categoria em lote."""
    sql = """
        INSERT INTO Categorie_product (asin, id_category)
        VALUES (%s, %s)
        ON CONFLICT (asin, id_category) DO NOTHING;
    """
    with conn.cursor() as cur:
        execute_batch(cur, sql, data)

def batch_insert_similars(conn, data):
    """Insere uma lista de produtos similares em lote."""
    sql = """
        INSERT INTO "Similar" (asin_product, asin_similar)
        VALUES (%s, %s)
        ON CONFLICT (asin_product, asin_similar) DO NOTHING;
    """
    with conn.cursor() as cur:
        execute_batch(cur, sql, data)
        
def execute_in_chunks(conn, batch_function, data, table_name, chunk_size=10000):
    """Executa uma função de inserção em lote em pedaços menores."""
    if not data:
        return
    
    if isinstance(data, set):
        data = list(data)
    
    total = len(data)
    print(f"Iniciando inserção em lotes para a tabela '{table_name}' ({total} linhas)...")
    
    for i in range(0, total, chunk_size):
        chunk = data[i:i + chunk_size]
        batch_function(conn, chunk)


def insert_data(conn, input_file):
    """
    Função para inserir dados nas tabelas criadas.
    """
    print("Iniciando o parsing do arquivo...")
    with open(input_file, 'r') as f:
        general_categories = set()
        produtos_data = []
        reviews = []
        product_categories_data = set()
        valid_asins = set()
        similars = []
        for line in f:
            if line.startswith('Id'):
                id_ = line.split(':')[1].strip()
                line = find_next_line(f, 'ASIN')
                asin = line.split(':')[1].strip()
                line = next(f)
                title = group = salesrank = similar = None
                num_categories = num_reviews = downloaded = avg_rating = 0
                if not "discontinued product" in line:
                    title = ":".join(line.split(':')[1:]).strip()
                    line = find_next_line(f, 'group')
                    group = line.split(':')[1].strip()
                    line = find_next_line(f, 'salesrank')
                    salesrank = line.split(':')[1].strip()
                    line = find_next_line(f, 'similar')
                    similar = line.split(':')[1].strip()
                    similars.append((asin, similar[1:]) if similar and similar.split()[0] != '0' else ())
                    line = find_next_line(f, 'categories')
                    num_categories = line.split(':')[1].strip()
                    for _ in range(int(num_categories)):
                        line = next(f)
                        general_categories.add(line)
                        categories_id = find_id(line.strip())
                        if categories_id:
                            for cat in categories_id:
                                product_categories_data.add((asin, cat[1]))
                    line = find_next_line(f, 'reviews')
                    line_reviews = line.split(':')
                    num_reviews = line_reviews[2].split()[0]
                    downloaded = line_reviews[3].split()[0]
                    avg_rating = line_reviews[4].split()[0]
                    for _ in range(int(downloaded)):
                        line = next(f)
                        parts = ' '.join(line.split()).split()
                        if len(parts) == 9:
                            reviews.append((asin, parts[0], parts[2], parts[4], parts[6], parts[8]))

                produtos_data.append((id_, asin, title, group, salesrank, similar[0] if similar else None, num_categories, num_reviews, downloaded, avg_rating))
                valid_asins.add(asin)
    print(f"Parsing concluído. {len(produtos_data)} produtos encontrados.")
    print("Inserindo categorias gerais e produtos...")
    
    execute_in_chunks(conn, batch_insert_products, produtos_data, "Products")
    general_categories_data = []
    processed_cat_ids = set()
    for i in general_categories:
        categorie = find_id(i)
        if categorie:
            cat_name = categorie[0][0] if categorie[0][0] != "" else None
            if categorie[0][1] not in processed_cat_ids:
                general_categories_data.append((categorie[0][1], cat_name, None))
                processed_cat_ids.add(categorie[0][1])
            for i in range(1, len(categorie)):
                cat_name = categorie[i][0] if categorie[i][0] != "" else None
                if categorie[i][1] not in processed_cat_ids:
                    general_categories_data.append((categorie[i][1], cat_name, categorie[i-1][1]))
                    processed_cat_ids.add(categorie[i][1])
    execute_in_chunks(conn, batch_insert_general_categories, general_categories_data, "Categories")
    print("Produtos e categorias gerais inseridos.")
    print("Continuando inserções...")
    print("\033[37mwarning: aqui demora um pouco, recomendo ir tomar um café ☕\033[0m")
    print("\033[37mtempo médio: 8 minutos (obviamente depende do pc)\033[0m")
    similars_data = []
    for i in similars:
        if len(i) == 2 and i[1]:
            similar_parts = i[1].split()
            print(i[0])
            for j in similar_parts:
                if j in valid_asins:
                    similars_data.append((i[0], j))
    execute_in_chunks(conn, batch_insert_reviews, reviews, "Reviews")
    execute_in_chunks(conn, batch_insert_product_categories, product_categories_data, "Categorie_product")
    execute_in_chunks(conn, batch_insert_similars, similars_data, "Similar")
    
    print("Dados inseridos com sucesso.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Script para popular o banco de dados de e-commerce.")
    parser.add_argument('--db-host', default=os.environ.get('PGHOST', 'db'), help='Host do banco de dados')
    parser.add_argument('--db-port', default=os.environ.get('PGPORT', 5432), help='Porta do banco de dados')
    parser.add_argument('--db-name', default=os.environ.get('PGDATABASE', 'ecommerce'), help='Nome do banco de dados')
    parser.add_argument('--db-user', default=os.environ.get('PGUSER', 'postgres'), help='Usuário do banco de dados')
    parser.add_argument('--db-pass', default=os.environ.get('PGPASSWORD', 'postgres'), help='Senha do banco de dados')
    parser.add_argument('--input', required=True, help='Caminho para o arquivo de dados de entrada')
    
    args = parser.parse_args()

    conn = None
    try:
        conn = psycopg2.connect(
            host=args.db_host,
            port=args.db_port,
            user=args.db_user,
            password=args.db_pass,
            dbname=args.db_name
        )
        create_schema(conn)
        insert_data(conn, args.input)
        print("Fazendo commit das operações...")
        conn.commit()
        print("Esquema criado e dados inseridos com sucesso.")
    except (Exception, psycopg2.Error) as error:
        print("Ocorreu um erro. Revertendo operações.\nErro:", error)
        if conn:
            conn.rollback()
    
    finally:
        if conn:
            conn.close()
            print("Conexão com o banco de dados fechada.")

