# Patrimônio Paulista: site interativo

Interface para apresentar o trabalho: mapa do estado de São Paulo com os 527 bens
de `dados/patrimonio.csv`, uma busca que gera o arquivo de consulta e a visualização
das 8 estruturas de dados funcionando.

## Como abrir

Online: **https://livvyb.github.io/trabalho-parte-1-asteroides/**

Ou abra `site/index.html` no navegador (Chrome, Firefox ou Edge atualizados).
**Não precisa de internet nem de servidor**: os dados e o mapa já estão nos arquivos `.js`.

O workflow `.github/workflows/pages.yml` republica o site sozinho a cada push na
`main` que mexa em `site/`.

## As quatro abas

1. **Explorar o mapa**: monte a busca no painel da esquerda (município, região, tipo,
   livro, faixa de anos, “parecido com…”). O mapa mostra uma bolha por cidade; clicar
   numa cidade filtra por ela. O painel mostra o arquivo de consulta (`FILTER`, `SIM`,
   `ACCEPT`, `ORDER`, `GROUP`, `CROSS`) e o comando para rodar a mesma busca no programa em C.
2. **Por dentro do programa**: animação passo a passo de carga → busca → escrita,
   desenhando a estrutura base e a de resultado a cada inserção.
3. **Corrida**: quantos passos cada estrutura gasta para guardar o resultado da busca
   atual, mais os tempos medidos nos experimentos (`resultados_p1/` e `resultados_p2/`).
4. **Desafios**: 7 perguntas com dica, “montar a busca” e resposta.

Dicas para a feira: `A+` aumenta o texto para o projetor; o botão de tema alterna
claro/escuro; na aba 2, “Usar só 15 bens” deixa a animação fácil de acompanhar.

## Fidelidade ao programa em C

`motor.js` é uma tradução do leitor, da consulta e das 8 estruturas (`reader.c`,
`query.c`, `pilha.c` … `lista_cruz.c`, `main.c`). Para conferir que a saída é
idêntica à do binário:

```sh
make
node site/ferramentas/verificar.js ./trabalho
```

O único recurso que não existe no C é a busca “Nome contém” (marcada como
“extra do site”); ela aparece como comentário no arquivo de consulta.

## Arquivos

| Arquivo | O que é |
|---|---|
| `index.html`, `estilo.css` | página e estilos |
| `app.js` | interface (busca, mapa, lista, animação, corrida, desafios) |
| `viz.js` | desenho de cada estrutura |
| `motor.js` | lógica do programa em C, em JavaScript |
| `dados.js` | o CSV embutido (gerado) |
| `mapa.js` | contorno dos 645 municípios, da malha do IBGE (gerado) |
| `codigo.js` | trechos do código C mostrados na aba 2 (gerado) |
| `experimentos.js` | médias de `resultados_p*/medias.csv` (gerado) |
| `ferramentas/gerar_dados.py` | gera os quatro arquivos acima |
| `ferramentas/verificar.js` | compara o motor com o binário em C |
| `ferramentas/ibge/` | malha e nomes dos municípios baixados do IBGE |

Se o CSV, o código C ou os experimentos mudarem, rode
`python3 site/ferramentas/gerar_dados.py` a partir da pasta do trabalho.
