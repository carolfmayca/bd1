import os
import psycopg2
import re

def create_schema():
    """
    Conecta ao banco de dados PostgreSQL e executa o script schema.sql
    para criar as tabelas.
    """
    conn = None
    try:
        # Conecta ao banco de dados usando as variáveis de ambiente
        # que foram definidas no docker-compose.yml
        conn = psycopg2.connect(
            host=os.environ.get('PGHOST', 'db'),
            port=os.environ.get('PGPORT', 5432),
            user=os.environ.get('PGUSER', 'postgres'),
            password=os.environ.get('PGPASSWORD', 'postgres'),
            dbname=os.environ.get('PGDATABASE', 'ecommerce')
        )
        
        # Abre uma sessão para executar os comandos
        with conn.cursor() as cur:
            # O caminho para o schema.sql precisa ser relativo
            # à localização de onde o script é executado.
            # Se tp1_3.2.py está em /app/src e schema.sql em /app/sql,
            # o caminho relativo seria '../sql/schema.sql'
            with open('../sql/schema.sql', 'r') as f:
                sql_script = f.read()
                cur.execute(sql_script)
        
        # Confirma as alterações no banco de dados
        conn.commit()
        print("Esquema do banco de dados criado com sucesso.")

    except psycopg2.Error as e:
        print(f"Erro ao conectar ou criar o esquema: {e}")
        if conn:
            conn.rollback() # Reverte as alterações em caso de erro

    finally:
        # Garante que a conexão seja fechada
        if conn:
            conn.close()

def find_next_line(iterator, text):
    """
    Avança o iterador até encontrar uma linha que comece com o texto desejado.
    Retorna a linha encontrada ou None se o arquivo terminar antes de encontrar.
    """
    for line in iterator:
        if  text in line:
            return line
    return None

def insert_products(produto):
    """
    Insere um produto na tabela Products.
    """
    sql = """
        INSERT INTO Products (Id, ASIN, title, "group", salesrank, similar, numCategories, numReviews, downloaded, avg_rating)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        ON CONFLICT (ASIN) DO NOTHING;
        """
    conn = None
    try:
        conn = psycopg2.connect(
            host=os.environ.get('PGHOST', 'db'),
            port=os.environ.get('PGPORT', 5432),
            user=os.environ.get('PGUSER', 'postgres'),
            password=os.environ.get('PGPASSWORD', 'postgres'),
            dbname=os.environ.get('PGDATABASE', 'ecommerce')
        )
        with conn.cursor() as cur:
            cur.execute(sql, (produto['id'], produto['asin'], produto['title'], produto['group'], produto['salesrank'], produto['similar'][0], produto['numCategories'], produto['numReviews'], produto['downloaded'], produto['avg_rating']))
            conn.commit()
    except psycopg2.Error as e:
        print(f"Erro ao inserir produto: {e}")
    finally:
        if conn:
            conn.close()

def insert_reviews(asin,reviews):
    """
    Insere uma avaliação na tabela Reviews.
    """
    if reviews is not None:
        sql = """
            INSERT INTO Reviews (ASIN, "date", customer, rating, votes, helpful)
            VALUES (%s, %s, %s, %s, %s, %s, %s)
            ON CONFLICT (id) DO NOTHING;
            """
        conn = None
        try:
            conn = psycopg2.connect(
                host=os.environ.get('PGHOST', 'db'),
                port=os.environ.get('PGPORT', 5432),
                user=os.environ.get('PGUSER', 'postgres'),
                password=os.environ.get('PGPASSWORD', 'postgres'),
                dbname=os.environ.get('PGDATABASE', 'ecommerce')
            )
            with conn.cursor() as cur:
                cur.execute(sql, (asin, reviews[0], reviews[2], reviews[4], reviews[6], reviews[8]))
                conn.commit()
        except psycopg2.Error as e:
            print(f"Erro ao inserir produto: {e}")
        finally:
            if conn:
                conn.close()

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

def insert_general_categories(categories):
    categorie = find_id(categories)
    if categorie is not None:
        sql = """
            INSERT INTO GeneralCategories (id_category, categorie_name, id_father)
            VALUES (%s, %s, %s)
            ON CONFLICT (id_category) DO NOTHING;
            """
        conn = None
        try:
            conn = psycopg2.connect(
                host=os.environ.get('PGHOST', 'db'),
                port=os.environ.get('PGPORT', 5432),
                user=os.environ.get('PGUSER', 'postgres'),
                password=os.environ.get('PGPASSWORD', 'postgres'),
                dbname=os.environ.get('PGDATABASE', 'ecommerce')
            )
            with conn.cursor() as cur:
                cat_name = categorie[0][0] if categorie[0][0] != "" else None
                cur.execute(sql, (categorie[0][1], cat_name, None))
                for i in range(1,len(categorie)):
                    cur.execute(sql, (categorie[i][1], cat_name, categorie[i-1][1]))
                conn.commit()
        except psycopg2.Error as e:
            print(f"Erro ao inserir categoria: {e}")
            if conn:
                conn.rollback()
        finally:
            if conn:
                conn.close()

def insert_categories(asin,categories):
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
        conn = None
        try:
            conn = psycopg2.connect(
                host=os.environ.get('PGHOST', 'db'),
                port=os.environ.get('PGPORT', 5432),
                user=os.environ.get('PGUSER', 'postgres'),
                password=os.environ.get('PGPASSWORD', 'postgres'),
                dbname=os.environ.get('PGDATABASE', 'ecommerce')
            )
            with conn.cursor() as cur:
                for i in categorie:
                    cur.execute(sql, (asin, i[1]))
        except psycopg2.Error as e:
            print(f"Erro ao inserir categoria: {e}")
            if conn:
                conn.rollback()
        finally:
            if conn:
                conn.close()

def insert_similar(asin, similar):
    """
    Insere produtos similares na tabela Similar.
    """
    if similar is not None:
        if similar[0] != []:
            sql = """
                INSERT INTO Similar (asin_product,id_similar,asin_similar)
                VALUES (%s, %s, %s)
                ON CONFLICT (asin_product, asin_similar) DO NOTHING;
                """
            conn = None
            try:
                conn = psycopg2.connect(
                    host=os.environ.get('PGHOST', 'db'),
                    port=os.environ.get('PGPORT', 5432),
                    user=os.environ.get('PGUSER', 'postgres'),
                    password=os.environ.get('PGPASSWORD', 'postgres'),
                    dbname=os.environ.get('PGDATABASE', 'ecommerce')
                )
                with conn.cursor() as cur:
                    for i in similar[1:]:
                        cur.execute(sql, (asin, i[0], i[1]))
                conn.commit()
            except psycopg2.Error as e:
                print(f"Erro ao inserir similar: {e}")
                if conn:
                    conn.rollback()
            finally:
                if conn:
                    conn.close()

def insert_data():
    """
    Função para inserir dados nas tabelas criadas.
    """
    with open('data/snap_amazon.txt', 'r') as f:
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
                    num_reviews = line.split(':')[2][1].strip()
                    downloaded = line.split(':')[3][1].strip()
                    avg_rating = line.split(':')[4][1].strip()
                    reviews = []
                    for _ in range(int(num_reviews)):
                        line = next(arquivo)
                        reviews.append([line.strip()])
                produtos.append({'id':id_, 'asin':asin, 'title':title, 'group':group, \
                                'salesrank':salesrank, 'similar':similar, \
                                'numCategories':num_categories, 'categories': categories,\
                                 'numReviews':num_reviews, 'reviews':reviews, \
                                 'downloaded':downloaded, 'avg_rating':avg_rating})
    
    for i in general_categories:
        insert_general_categories(i)
    for i in produtos:
        insert_products(i)
        if i['reviews'] is not None:
            for j in i['reviews']:
                insert_reviews(i['asin'],str(j).split())
        if i['categories'] is not None:
            for j in i['categories']:
                insert_categories(i['asin'],j)
        if i['similar'] is not None:
            for j in i['similar'].split():
                insert_similar(i['asin'],j)


if __name__ == "__main__":
    create_schema()
    insert_data()