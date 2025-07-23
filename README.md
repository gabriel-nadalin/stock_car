
# Stock Car – Jogo para microcontrolador 8051 com GLCD

Este projeto é um jogo de corrida, inspirado no Stock Car de ZX81, desenvolvido para microcontroladores 8051, exibido em um GLCD KS0108 (128x64 pixels).

O jogo foi criado utilizando o compilador Keil para desenvolvimento em C e testado no simulador Proteus, permitindo validar o funcionamento do código antes de rodar no hardware real.

## Funcionalidades

- Estrada gerada proceduralmente com segmentos aleatórios
- Obstáculos dinâmicos que surgem na pista
- HUD exibindo distância, velocidade e vidas restantes
- Detecção de colisões com bordas da pista e obstáculos
- Tela de título, Game Over e Win
- Controle por serial (ou teclado matricial opcional)
- **Túnel em desenvolvimento**

## Hardware Utilizado
- Microcontrolador AT89S52 (ou qualquer 8051 compatível)
- GLCD KS0108 ou compatível (128x64 pixels)
- Comunicação serial para entrada de comandos
- (Opcional) Teclado matricial 4x3 como alternativa de controle

Os pinos do GLCD são mapeados conforme o código:

| GLCD              | Pinos do 8051 |
|-------------------|---------------|
| DataBus D0–D7 | P2.0 – P2.7   |
| RS            | P1.0          |
| EN            | P1.1          |
| CS1 / CS2     | P1.2 / P1.3   |
| RW            | P1.4          |
| RST           | P1.5          |

## Ferramentas de Desenvolvimento
- Compilador Keil uVision para compilar o código em C para 8051
- Proteus para simulação do GLCD, microcontrolador e entradas

## Testes e Validação

O jogo foi testado em dois ambientes diferentes:

1. Simulação no Proteus  
   - Validou o comportamento do GLCD, geração de estradas, colisões e HUD  
   - Permitiu depuração e ajustes antes de rodar em hardware real  

2. Execução em uma placa de hardware real com AT89S52 e GLCD KS0108  
   - O desempenho foi satisfatório, sem travamentos  
   - Os tempos de resposta, atualização do HUD e detecção de colisões funcionaram corretamente  
   - A jogabilidade apresentou o mesmo resultado do ambiente simulado  

Os resultados foram consistentes entre simulação e hardware real, garantindo a confiabilidade do projeto.

## Estrutura do Código

O código é organizado em módulos lógicos:
- GLCD – Funções de inicialização, escrita de caracteres, strings e gráficos
- HUD – Exibe distância, velocidade e vidas
- Player – Movimento, entrada de controles, desenho do carro
- Estrada (Road)
    - Pontos de controle aleatórios para gerar curvas
    - Interpolação para renderizar a pista
- Obstáculos – Gerados em posições aleatórias sobre a pista
- Renderização – Desenha quadro a quadro estrada, carro e obstáculos

## Controles

Os comandos são enviados pela serial (ou teclado matricial, opcional):

| Tecla | Função |
|-------|--------|
| 7 | Move o carro para a esquerda |
| 9 | Move o carro para a direita |
| 1 | Velocidade 1 |
| 2 | Velocidade 2 |
| 3 | Velocidade 3 |
| 4 | Velocidade 4 |
| 5 | Velocidade 5 |

## Geração e Renderização da Estrada

A pista é definida por pontos de controle armazenados em `road_points_x` e `road_points_y`.

- Cada ponto representa a posição central da pista em uma coordenada Y  
- Quando o jogador avança e um ponto sai da tela, um novo ponto é gerado aleatoriamente  
- Para desenhar a pista suavemente entre dois pontos, é utilizada interpolação linear (*lerp*)  
- A largura da pista é constante (`road_radius`), desenhando bordas esquerda e direita  

Essa técnica permite curvas suaves e imprevisíveis, criando a sensação de estrada infinita.

## HUD e Fontes

O HUD é exibido na lateral direita do GLCD com informações em tempo real:

```
DIST. 00000
SPEED 3
LIVES 9
```

As fontes utilizadas são 5x7 pixels com espaçamento de 1 coluna, armazenadas em matrizes no arquivo `font.h`.
Existem caracteres para dígitos, letras maiúsculas e símbolos básicos.

## Objetivo do Jogo
- Percorrer 15.000 unidades de distância para vencer.
- Evitar obstáculos e não bater nas bordas da pista.
- Cada colisão reduz as vidas até chegar ao Game Over.

## Túnel (Em Desenvolvimento)

No jogo original, após uma certa distância, o jogador entra em um túnel que limita sua visibilidade apenas a uma área iluminada à frente do carro. Essa funcionalidade ainda está parcialmente implementada.

### Como funciona

- Uma máscara de luz em formato de cone é calculada utilizando equações implícitas de linha 
- Para cada pixel renderizado, o código testa se ele está dentro do cone de iluminação
- Apenas os pixels dentro do cone são visíveis; o restante da tela fica escuro
- Esse efeito simula os faróis do carro, aumentando a dificuldade

Atualmente, a lógica das máscaras e linhas está parcialmente implementada (`get_mask_xy` e `byte_mask`), mas o túnel ainda não está totalmente integrado ao loop principal, sendo considerado incompleto.
