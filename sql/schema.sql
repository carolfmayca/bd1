CREATE SCHEMA IF NOT EXISTS ecommerce AUTHORIZATION postgres;

CREATE TABLE Produtos (
    Id SERIAL PRIMARY KEY,
    ASIN VARCHAR(20) UNIQUE NOT NULL,
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
    ASIN VARCHAR(20) REFERENCES Produtos(ASIN),
    "date" DATE,
    customer VARCHAR(50),
    rating INTEGER,
    votes INTEGER,
    helpful INTEGER
);

CREATE TABLE Categories(
    id SERIAL PRIMARY KEY,
    general_category VARCHAR(100),
    subcategory VARCHAR(100)[],
    specific_category VARCHAR(100)
);

