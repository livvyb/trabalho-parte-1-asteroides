### Experimento A: Variacao de N e de k (base lista_enc, resultado lista_ord)

| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |
|---|---|---|---|---|---|---|---|---|---|---|
| 1000 | lista_enc | lista_ord | 10 | 427 | 1.833 | 0.078 | 0.186 | 0.097 | 2.193 | 0.365 |
| 5000 | lista_enc | lista_ord | 50 | 2079 | 7.638 | 0.341 | 3.376 | 0.105 | 11.461 | 3.782 |
| 10000 | lista_enc | lista_ord | 100 | 4143 | 13.145 | 0.690 | 13.154 | 0.103 | 27.092 | 3.679 |
| 25000 | lista_enc | lista_ord | 500 | 9709 | 26.778 | 1.812 | 87.111 | 0.238 | 115.938 | 3.032 |
| 50000 | lista_enc | lista_ord | 1000 | 12818 | 49.700 | 3.344 | 182.371 | 0.365 | 235.780 | 1.677 |
| toda | lista_enc | lista_ord | todos | 18881 | 96.081 | 6.014 | 465.441 | 5.328 | 572.863 | 11.222 |

### Experimento B: Estruturas como BASE (resultado lista_enc, N=toda a base, k=100)

| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |
|---|---|---|---|---|---|---|---|---|---|---|
| toda | pilha | lista_enc | 100 | 18881 | 91.828 | 4.244 | 0.607 | 0.112 | 96.790 | 3.499 |
| toda | fila | lista_enc | 100 | 18881 | 94.047 | 4.345 | 0.630 | 0.101 | 99.123 | 0.496 |
| toda | lista_seq | lista_enc | 100 | 18881 | 90.076 | 3.285 | 0.602 | 0.099 | 94.062 | 2.481 |
| toda | lista_enc | lista_enc | 100 | 18881 | 90.287 | 4.213 | 0.600 | 0.092 | 95.192 | 5.002 |
| toda | lista_cabeca | lista_enc | 100 | 18881 | 90.891 | 4.139 | 0.585 | 0.093 | 95.708 | 3.290 |
| toda | lista_ord | lista_enc | 100 | 18881 | 48312.879 | 7.423 | 0.586 | 0.116 | 48321.004 | 137.271 |
| toda | lista_gen | lista_enc | 100 | 18881 | 117.932 | 7.547 | 0.599 | 0.102 | 126.180 | 6.848 |
| toda | lista_cruz | lista_enc | 100 | 18881 | 118.429 | 7.077 | 0.605 | 0.095 | 126.207 | 3.473 |

### Experimento C: Estruturas como RESULTADO (base lista_enc, N=toda a base, k=100)

| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |
|---|---|---|---|---|---|---|---|---|---|---|
| toda | lista_enc | pilha | 100 | 18881 | 88.788 | 4.189 | 0.591 | 0.092 | 93.660 | 3.002 |
| toda | lista_enc | fila | 100 | 18881 | 88.180 | 4.282 | 0.603 | 0.095 | 93.161 | 2.142 |
| toda | lista_enc | lista_seq | 100 | 18881 | 89.733 | 4.347 | 0.424 | 0.094 | 94.598 | 3.288 |
| toda | lista_enc | lista_enc | 100 | 18881 | 89.441 | 4.293 | 0.608 | 0.100 | 94.442 | 1.551 |
| toda | lista_enc | lista_cabeca | 100 | 18881 | 93.201 | 4.778 | 0.645 | 0.107 | 98.731 | 0.534 |
| toda | lista_enc | lista_ord | 100 | 18881 | 98.331 | 6.181 | 487.126 | 0.119 | 591.756 | 14.789 |
| toda | lista_enc | lista_gen | 100 | 18881 | 93.044 | 4.329 | 4.387 | 0.099 | 101.859 | 1.884 |
| toda | lista_enc | lista_cruz | 100 | 18881 | 91.520 | 4.143 | 4.426 | 0.099 | 100.188 | 0.533 |

### Experimento D: Variacao de k (base lista_enc, resultado lista_ord, N=toda a base)

| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |
|---|---|---|---|---|---|---|---|---|---|---|
| toda | lista_enc | lista_ord | 10 | 18881 | 97.322 | 6.459 | 462.343 | 0.096 | 566.220 | 20.727 |
| toda | lista_enc | lista_ord | 50 | 18881 | 94.752 | 6.312 | 461.197 | 0.126 | 562.387 | 15.748 |
| toda | lista_enc | lista_ord | 100 | 18881 | 96.084 | 6.347 | 504.425 | 0.137 | 606.994 | 32.423 |
| toda | lista_enc | lista_ord | 500 | 18881 | 99.815 | 6.288 | 506.413 | 0.260 | 612.776 | 17.667 |
| toda | lista_enc | lista_ord | 1000 | 18881 | 100.158 | 6.668 | 488.562 | 0.417 | 595.806 | 12.673 |
| toda | lista_enc | lista_ord | todos | 18881 | 97.126 | 6.534 | 481.766 | 5.394 | 590.821 | 23.794 |
