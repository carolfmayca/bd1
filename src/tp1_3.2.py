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

def insert_products(conn, produto):
    """
    Insere um produto na tabela Products.
    """
    if produto['title'] is None:
        return

    sql = """
        INSERT INTO Products (Id, ASIN, title, "group", salesrank, numSimilar, numCategories, numReviews, downloaded, avg_rating)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        ON CONFLICT (ASIN) DO NOTHING;
        """
    with conn.cursor() as cur:

        num_similar = 0
        if produto['similar'] is not None:
            num_similar = produto['similar'].split()[0]

        cur.execute(sql, (
            produto['id'],
            produto['asin'],
            produto['title'],
            produto['group'],
            produto['salesrank'],
            num_similar, 
            produto['numCategories'],
            produto['numReviews'],
            produto['downloaded'],
            produto['avg_rating']
        ))


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

def insert_general_categories(conn,categories):
    categorie = find_id(categories)
    if categorie is not None:
        sql = """
            INSERT INTO Categories (id_categorie, categorie_name, id_father)
            VALUES (%s, %s, %s)
            ON CONFLICT (id_categorie) DO NOTHING;
            """
        with conn.cursor() as cur:
            cat_name = categorie[0][0] if categorie[0][0] != "" else None
            cur.execute(sql, (categorie[0][1], cat_name, None))
            for i in range(1,len(categorie)):
                cat_name = categorie[i][0] if categorie[i][0] != "" else None
                cur.execute(sql, (categorie[i][1], cat_name, categorie[i-1][1]))

def batch_insert_reviews(conn, data):
    """Insere uma lista de reviews em lote."""
    sql = """
        INSERT INTO Reviews (ASIN, "date", customer, rating, votes, helpful)
        VALUES (%s, %s, %s, %s, %s, %s)
        ON CONFLICT (id) DO NOTHING;
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

def insert_data(conn):
    """
    Função para inserir dados nas tabelas criadas.
    """
    print("Iniciando o parsing do arquivo...")
    with open('./data/snap_amazon.txt', 'r') as f:
        arquivo = (line for line in f.read().splitlines() if line.strip())
        produtos = []
        general_categories = set()
        for line in arquivo:
            if line.startswith('Id'):
                id_ = line.split(':')[1].strip()
                line = find_next_line(arquivo, 'ASIN')
                asin = line.split(':')[1].strip()
                line = next(arquivo)
                title = group = salesrank = similar = None
                categories = reviews = num_categories = num_reviews = downloaded = avg_rating = None
                if not "discontinued product" in line:
                    title = ":".join(line.split(':')[1:]).strip()
                    line = find_next_line(arquivo, 'group')
                    group = line.split(':')[1].strip()
                    line = find_next_line(arquivo, 'salesrank')
                    salesrank = line.split(':')[1].strip()
                    line = find_next_line(arquivo, 'similar')
                    similar = line.split(':')[1].strip()
                    line = find_next_line(arquivo, 'categories')
                    num_categories = line.split(':')[1].strip()
                    categories = []
                    for _ in range(int(num_categories)):
                        line = next(arquivo)
                        general_categories.add(line)
                        categories.append([line.strip()])
                    line = find_next_line(arquivo, 'reviews')
                    line_reviews = line.split(':')
                    num_reviews = line_reviews[2][1].strip()
                    downloaded = line_reviews[3][1].strip()
                    avg_rating = line_reviews[4][1].strip()
                    reviews = []
                    for _ in range(int(downloaded)):
                        line = next(arquivo)
                        reviews.append([line.strip()])
                produtos.append({'id':id_, 'asin':asin, 'title':title, 'group':group, \
                                'salesrank':salesrank,'similar':similar, \
                                'numCategories':num_categories, 'categories': categories,\
                                 'numReviews':num_reviews, 'reviews':reviews, \
                                 'downloaded':downloaded, 'avg_rating':avg_rating})
    print(f"Parsing concluído. {len(produtos)} produtos encontrados.")
    print("Inserindo categorias gerais e produtos...")
    for i in general_categories:
        insert_general_categories(conn,i)
    valid_asins = set()
    for i in produtos:
        if i['title'] is not None:
            insert_products(conn,i)
            valid_asins.add(i['asin'])
    
    print("Produtos e categorias gerais inseridos.")
    print("Inserindo avaliações, categorias específicas e produtos similares...")
    print("\033[37mwarning: aqui demora um pouco, recomendo ir tomar um café ☕\033[0m")
    print("\033[37mtempo médio: 8 minutos (obviamente depende do pc\033[0m)")
    reviews_data = []
    product_categories_data = []
    similars_data = []
    meio = len(produtos)//2
    for i in produtos:
        if i['title'] is None:
            continue
        if i['reviews'] is not None:
            for j in i['reviews']:
                parts = ' '.join(j[0].split()).split()
                reviews_data.append((i['asin'], parts[0], parts[2], parts[4], parts[6], parts[8]))
        if i['categories'] is not None:
            for j in i['categories']:
                categories_id = find_id(j)
                if categories_id:
                    for cat in categories_id:
                        product_categories_data.append((i['asin'], cat[1]))
        if i['similar'] is not None and i['similar'].split()[0] != '0':
            similar_parts = i['similar'].split()
            for j in similar_parts[1:]:
                if j in valid_asins:
                    similars_data.append((i['asin'], j))
    batch_insert_reviews(conn, reviews_data)
    batch_insert_product_categories(conn, product_categories_data)
    batch_insert_similars(conn, similars_data)         
    print("Dados inseridos com sucesso.")

if __name__ == "__main__":
    conn = None
    try:
        conn = psycopg2.connect(
            host=os.environ.get('PGHOST', 'db'),
            port=os.environ.get('PGPORT', 5432),
            user=os.environ.get('PGUSER', 'postgres'),
            password=os.environ.get('PGPASSWORD', 'postgres'),
            dbname=os.environ.get('PGDATABASE', 'ecommerce')
        )
        create_schema(conn)
        insert_data(conn)
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

