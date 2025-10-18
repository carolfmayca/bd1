import sys
import os

arquivo = "data/artigo.csv"

with open(arquivo, "r") as f:
    linhas = f.readlines()

for i, linha in enumerate(linhas):
    if linha.count(";") >= 6: 
        # f.write(linha)
        continue
    else:
        print(f"Id {linha.split(';')[0]} com problema, campos {linha.count(';')} linha {i}")
