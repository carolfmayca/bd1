import os
import psycopg2
import re

def create_schema(conn):
    """
    Cria o esquema do banco de dados executando o script SQL.
    """
    # Abre uma sessão para executar os comandos
    with conn.cursor() as cur:
        # O caminho para o schema.sql precisa ser relativo
        # à localização de onde o script é executado.
        # Se tp1_3.2.py está em /app/src e schema.sql em /app/sql,
        # o caminho relativo seria '../sql/schema.sql'
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
    # Se o produto for 'discontinued', o título será None. Não há nada a inserir.
    if produto['title'] is None:
        return

    sql = """
        INSERT INTO Products (Id, ASIN, title, "group", salesrank, numSimilar, numCategories, numReviews, downloaded, avg_rating)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        ON CONFLICT (ASIN) DO NOTHING;
        """
    with conn.cursor() as cur:
        # O primeiro item da string 'similar' é a contagem.
        # Se 'similar' for None, a contagem é 0.
        num_similar = 0
        if produto['similar'] is not None:
            num_similar = produto['similar'].split()[0]

        cur.execute(sql, (
            produto['id'],
            produto['asin'],
            produto['title'],
            produto['group'],
            produto['salesrank'],
            num_similar,  # Usamos a contagem extraída
            produto['numCategories'],
            produto['numReviews'],
            produto['downloaded'],
            produto['avg_rating']
        ))

def insert_reviews(conn,asin,reviews):
    """
    Insere uma avaliação na tabela Reviews.
    """
    if reviews is not None:
        sql = """
            INSERT INTO Reviews (ASIN, "date", customer, rating, votes, helpful)
            VALUES ( %s, %s, %s, %s, %s, %s)
            ON CONFLICT (id) DO NOTHING;
            """
        with conn.cursor() as cur:
            # A linha de review tem um formato fixo, o split pode quebrar.
            # Ex: '2000-7-28  cutomer: A2JW67OY8U6HHK  rating: 5  votes:  10  helpful:   9'
            parts = ' '.join(reviews).split()
            if len(parts) >= 9:
                 cur.execute(sql, (asin, parts[0], parts[2], parts[4], parts[6], parts[8]))


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

def insert_categories(conn,asin,categories):
    """
    Insere categorias na tabela Categorie_product.
    """
    categorie = find_id(categories)
    if categorie is not None:
        sql = """
            INSERT INTO Categorie_product (asin, id_category)
            VALUES (%s, %s)
            ON CONFLICT (asin, id_category) DO NOTHING;
            """
        with conn.cursor() as cur:
            for i in categorie:
                cur.execute(sql, (asin, i[1]))


def insert_similar(conn, asin, similar, valid_asins):
    """
    Insere produtos similares na tabela Similar.
    Verifica se o ASIN similar existe no conjunto de ASINs válidos antes de inserir.
    """
    # Garante que 'similar' não é None e tem mais de um item (contagem > 0)
    if similar is None or len(similar) <= 1:
        return
    sql = """
        INSERT INTO "Similar" (asin_product,asin_similar)
        VALUES (%s, %s)
        ON CONFLICT (asin_product, asin_similar) DO NOTHING;
        """
    with conn.cursor() as cur:
        for i in similar[1:]:
            if i in valid_asins:
                cur.execute(sql, (asin, i))

def insert_data(conn):
    """
    Função para inserir dados nas tabelas criadas.
    """
    print("Iniciando o parsing do arquivo de dados...")
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
    for i in produtos:
        insert_products(conn,i)
    
    print("Produtos e categorias inseridos.")
    print("Inserindo relações (reviews, categorias, similares)...")
    with conn.cursor() as cur:
        cur.execute("SELECT asin FROM Products")
        valid_asins = {row[0] for row in cur.fetchall()}
    for i in produtos:
        if i['reviews'] is not None:
            for j in i['reviews']:
                insert_reviews(conn,i['asin'],j[0].split())
        if i['categories'] is not None:
            for j in i['categories']:
                insert_categories(conn,i['asin'],j)
        if i['similar'] is not None and i['similar'].split()[0] != '0':
            insert_similar(conn, i['asin'], i['similar'].split(), valid_asins)
            
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

