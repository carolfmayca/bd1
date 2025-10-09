CREATE SCHEMA IF NOT EXISTS ecommerce AUTHORIZATION postgres;

CREATE TABLE IF NOT EXISTS Products (
    Id SERIAL UNIQUE NOT NULL,
    ASIN VARCHAR(20) PRIMARY KEY,
    title TEXT,
    "group" VARCHAR(100),
    salesrank INTEGER,
    numSimilar INTEGER,
    numCategories TEXT,
    numReviews INTEGER,
    downloaded INTEGER,
    avg_rating NUMERIC(3, 2)
);

CREATE TABLE IF NOT EXISTS Reviews (
    id SERIAL PRIMARY KEY,
    ASIN VARCHAR(20) REFERENCES Products(ASIN) ON DELETE CASCADE,
    "date" DATE,
    customer VARCHAR(50),
    rating INTEGER,
    votes INTEGER,
    helpful INTEGER
);

CREATE TABLE IF NOT EXISTS Categories (
    id_categorie INT PRIMARY KEY,              
    categorie_name VARCHAR(255),                
    id_father INT REFERENCES Categories(id_categorie)  
);

CREATE TABLE IF NOT EXISTS Categorie_product (
    asin VARCHAR(20) REFERENCES Products(ASIN) ON DELETE CASCADE,
    id_category INT REFERENCES Categories(id_categorie) ON DELETE CASCADE,
    PRIMARY KEY (asin, id_category)
);

CREATE TABLE IF NOT EXISTS "Similar" (
    asin_product VARCHAR(20) REFERENCES Products(ASIN) ON DELETE CASCADE,
    asin_similar VARCHAR(20) REFERENCES Products(ASIN) ON DELETE CASCADE,
    PRIMARY KEY (asin_product, asin_similar)
);




