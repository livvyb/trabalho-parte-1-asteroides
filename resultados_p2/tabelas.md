### Experimento A: Variacao de N e de k (base lista_enc, resultado lista_ord)

| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |
|---|---|---|---|---|---|---|---|---|---|---|
| 50 | lista_enc | lista_ord | 5 | 50 | 0.082 | 0.002 | 0.005 | 0.055 | 0.143 | 0.031 |
| 100 | lista_enc | lista_ord | 10 | 100 | 0.250 | 0.007 | 0.017 | 0.084 | 0.358 | 0.100 |
| 200 | lista_enc | lista_ord | 20 | 200 | 0.400 | 0.010 | 0.047 | 0.078 | 0.535 | 0.120 |
| 400 | lista_enc | lista_ord | 50 | 400 | 0.904 | 0.029 | 0.163 | 0.113 | 1.208 | 0.382 |
| toda | lista_enc | lista_ord | todos | 527 | 0.941 | 0.026 | 0.187 | 0.291 | 1.445 | 0.298 |

### Experimento B: Estruturas como BASE (resultado lista_enc, N=toda a base, k=100)

| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |
|---|---|---|---|---|---|---|---|---|---|---|
| toda | pilha | lista_enc | 100 | 527 | 0.775 | 0.034 | 0.038 | 0.125 | 0.972 | 0.232 |
| toda | fila | lista_enc | 100 | 527 | 0.596 | 0.023 | 0.017 | 0.101 | 0.738 | 0.068 |
| toda | lista_seq | lista_enc | 100 | 527 | 0.795 | 0.029 | 0.024 | 0.126 | 0.974 | 0.179 |
| toda | lista_enc | lista_enc | 100 | 527 | 0.800 | 0.028 | 0.022 | 0.124 | 0.974 | 0.236 |
| toda | lista_cabeca | lista_enc | 100 | 527 | 0.847 | 0.029 | 0.022 | 0.103 | 1.001 | 0.070 |
| toda | lista_ord | lista_enc | 100 | 527 | 1.123 | 0.025 | 0.022 | 0.104 | 1.274 | 0.265 |
| toda | lista_gen | lista_enc | 100 | 527 | 0.969 | 0.029 | 0.023 | 0.095 | 1.116 | 0.198 |
| toda | lista_cruz | lista_enc | 100 | 527 | 1.597 | 0.038 | 0.033 | 0.130 | 1.798 | 0.595 |

### Experimento C: Estruturas como RESULTADO (base lista_enc, N=toda a base, k=100)

| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |
|---|---|---|---|---|---|---|---|---|---|---|
| toda | lista_enc | pilha | 100 | 527 | 1.718 | 0.049 | 0.045 | 0.177 | 1.989 | 0.433 |
| toda | lista_enc | fila | 100 | 527 | 2.452 | 0.075 | 0.075 | 0.286 | 2.887 | 0.361 |
| toda | lista_enc | lista_seq | 100 | 527 | 2.451 | 0.074 | 0.048 | 0.282 | 2.855 | 0.334 |
| toda | lista_enc | lista_enc | 100 | 527 | 2.599 | 0.072 | 0.084 | 0.244 | 2.999 | 0.236 |
| toda | lista_enc | lista_cabeca | 100 | 527 | 2.588 | 0.077 | 0.074 | 0.300 | 3.039 | 0.425 |
| toda | lista_enc | lista_ord | 100 | 527 | 2.259 | 0.070 | 0.475 | 0.234 | 3.039 | 0.587 |
| toda | lista_enc | lista_gen | 100 | 527 | 2.601 | 0.083 | 0.403 | 0.294 | 3.381 | 0.865 |
| toda | lista_enc | lista_cruz | 100 | 527 | 2.326 | 0.081 | 0.547 | 0.266 | 3.219 | 0.456 |

### Experimento D: Variacao de k (base lista_enc, resultado lista_ord, N=toda a base)

| N | Base | Resultado | k | Casados | Carga (ms) | Busca (ms) | Insercao res. (ms) | Saida (ms) | Total (ms) | Desvio total |
|---|---|---|---|---|---|---|---|---|---|---|
| toda | lista_enc | lista_ord | 5 | 527 | 2.488 | 0.069 | 0.526 | 0.170 | 3.253 | 0.505 |
| toda | lista_enc | lista_ord | 10 | 527 | 2.051 | 0.066 | 0.415 | 0.131 | 2.663 | 0.778 |
| toda | lista_enc | lista_ord | 20 | 527 | 1.662 | 0.048 | 0.332 | 0.118 | 2.160 | 0.557 |
| toda | lista_enc | lista_ord | 50 | 527 | 1.500 | 0.046 | 0.298 | 0.127 | 1.971 | 0.631 |
| toda | lista_enc | lista_ord | todos | 527 | 1.984 | 0.062 | 0.413 | 0.517 | 2.977 | 0.500 |
