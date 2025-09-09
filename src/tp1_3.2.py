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
                cur.execute(sql, asin, reviews[0], reviews[2], reviews[4], reviews[6], reviews[8])
                conn.commit()
        except psycopg2.Error as e:
            print(f"Erro ao inserir produto: {e}")
        finally:
            if conn:
                conn.close()


def insert_data():
    """
    Função para inserir dados de exemplo nas tabelas criadas.
    """
    with open('data/snap_amazon.txt', 'r') as f:
            arquivo = (line for line in f.read().splitlines() if line.strip())
            produtos = []
            generalCategories = set()
            for line in arquivo:
                if line.startswith('Id'):
                    id_ = line.split(':')[1].strip()
                    line = find_next_line(arquivo, 'ASIN')
                    asin = line.split(':')[1].strip()
                    line = next(arquivo)
                    if "discontinued product" in line:
                        title = group = salesrank = similar =categories =reviews =numCategories = numReviews = downloaded = avg_rating = None
                    else:
                        title = ":".join(line.split(':')[1:]).strip()  # pois o título pode ter ':' dentro
                        line = find_next_line(arquivo, 'group')
                        group = line.split(':')[1].strip()
                        line = find_next_line(arquivo, 'salesrank')
                        salesrank = line.split(':')[1].strip()
                        line = find_next_line(arquivo, 'similar')
                        similar = line.split(':')[1].strip()
                        line = find_next_line(arquivo, 'categories')
                        numCategories = line.split(':')[1].strip()
                        categories = []
                        for _ in range(int(numCategories)):
                            line = next(arquivo)
                            generalCategories.update(line.split('|'))
                            categories.append([line.strip()])
                        line = find_next_line(arquivo, 'reviews')
                        numReviews = line.split(':')[2][1].strip()
                        downloaded = line.split(':')[3][1].strip()
                        avg_rating = line.split(':')[4][1].strip()
                        reviews = []
                        for _ in range(int(numReviews)):
                            line = next(arquivo)
                            reviews.append([line.strip()])
                    produtos.append({'id':id_, 'asin':asin, 'title':title, 'group':group, 'salesrank':salesrank,\
                                     'similar':similar, 'numCategories':numCategories,'categories': categories,\
                                     'numReviews':numReviews, 'reviews':reviews, 'downloaded':downloaded, \
                                     'avg_rating':avg_rating})
    for i in generalCategories:
        print(i)
    # for i in produtos:
        # insert_products(i)
        # for j in i['reviews']:
            # insert_reviews(i['id'],str(j).split())
        # for j in i['categories']:
            # insert_categories(i['id'],str(j).split())

                


if __name__ == "__main__":
    # create_schema()
    insert_data()

