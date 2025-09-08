CREATE SCHEMA IF NOT EXISTS ecommerce AUTHORIZATION postgres;

CREATE TABLE Produtos (
    Id SERIAL PRIMARY KEY,
    ASIN VARCHAR(20) UNIQUE NOT NULL,
    title TEXT,
    "group" VARCHAR(100),
    salesrank INTEGER,
    similar TEXT,
    numCategories TEXT,
    numReviews INTEGER,
    downloaded INTEGER,
    avg_rating NUMERIC(3, 2)
);

CREATE TABLE Reviews (
    id SERIAL PRIMARY KEY,
    ASIN VARCHAR(20) REFERENCES Produtos(ASIN) ON DELETE CASCADE,
    "date" DATE,
    customer VARCHAR(50),
    rating INTEGER,
    votes INTEGER,
    helpful INTEGER
);

CREATE TABLE Categories (
    id_categorie INT PRIMARY KEY,              
    categorie_name VARCHAR(255) NOT NULL,                
    id_father INT REFERENCES Categories(id_categorie)  
);

CREATE TABLE Categorie_product (
    asin VARCHAR(20) REFERENCES Produtos(ASIN) ON DELETE CASCADE,
    id_category INT REFERENCES Categories(id_categorie) ON DELETE CASCADE,
    PRIMARY KEY (asin, id_category)
);



