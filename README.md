# Projeto MIC: Escape Room com Arduino Uno

Projeto final de DAS 5332 (Arquitetura e Programação de Sistemas Microcontrolados, UFSC).
O jogador precisa desarmar uma bomba resolvendo 4 desafios antes que o cronômetro chegue a zero.

## Estrutura

| Arquivo | Conteúdo |
|---|---|
| `escape_room/escape_room.ino` | Sketch integrado, o único que vai para o Arduino |
| `bomba.ino` | Fluxo do jogo original (dificuldade, cronômetro, LCD, display) |
| `binario_decimal1 (2).ino` | Desafio hexadecimal para decimal original |
| `cofre.ino` | Desafio do cofre original |
| `desafio_sequencia.ino` | Desafio da sequência original |
| `fios.ino` | Desafio dos fios original |
| `buzzer.ino` | Teste do buzzer original |
| `clock1ms.ino`, `clock2s.ino` | Testes de timer (não usados na integração) |

Os arquivos originais foram mantidos como referência. No sketch integrado, cada trecho alterado está marcado com o comentário `[INTEGRACAO]`.

Observação: o `bomba.ino` do repositório está com um conflito de merge salvo no arquivo (linhas `=======` e `>>>>>>> main`, com um pedaço do `fios.ino` no meio). A integração usou apenas a parte da bomba.

## Como jogar

1. Ao ligar, o LCD mostra `ESCOLHA O MODO`. Aperte `1` (fácil) ou `2` (difícil) no teclado.
2. O cronômetro começa em 3:00 no display de 7 segmentos.
3. Os 4 desafios aparecem um de cada vez, em ordem sorteada a cada partida. O LCD mostra qual é o desafio (`1/4 COFRE`, por exemplo).
4. Concluir os 4 desafios desarma a bomba (`BOMBA DESARMADA`). Aperte `#` para jogar de novo.

Regras de erro:

| | Fácil | Difícil |
|---|---|---|
| Penalidade por erro | 5 s | 10 s |
| Erros até explodir | 3 | 3 |

Os erros são somados entre todos os desafios. No terceiro erro, ou se o tempo acabar, a bomba explode: o LCD mostra a explosão, o buzzer toca um som grave e, depois de 3 s, o jogo recomeça na escolha de dificuldade.

Nos últimos 30 s o buzzer apita a cada segundo.

### Desafios

| Desafio | Objetivo | O que conta como erro |
|---|---|---|
| Hexadecimal | Converter o número hexadecimal do LCD para decimal. Dígitos digitam, `*` apaga, `#` confirma | Confirmar um valor errado |
| Cofre | Girar os 2 potenciômetros até as faixas certas (A0 entre 600 e 700, A1 entre 300 e 400) e manter por 1 s. Cada LED acende quando o seu potenciômetro está na faixa | Não tem erro |
| Sequência | Repetir nos 2 botões a sequência mostrada pelos 2 LEDs (1, 2, 2, 1, 2) | Apertar o botão errado |
| Fios | Ligar cada fio superior ao inferior da mesma cor e manter por 0,5 s. Ao concluir, o LCD mostra um número sorteado de 1 a 9 | Ligar o vermelho no amarelo |

### Comandos pela serial (9600 baud, 8N1)

| Comando | Efeito |
|---|---|
| `R` ou `r` | Reinicia o jogo a qualquer momento |

O jogo também envia mensagens de acompanhamento (início, desafio atual, erros, leituras do cofre, fim de jogo).

## Pinagem

Os 4 desafios e a bomba, cada um com seus pinos originais, precisariam de mais pinos do que o Uno tem. Por isso os desafios rodam um de cada vez, e alguns pinos são compartilhados entre desafios que nunca estão ativos ao mesmo tempo.

| Pino | Função no sketch integrado |
|---|---|
| D0 / D1 | UART (RX / TX) |
| D2 | Botão 1 da sequência (interrupção externa INT0) |
| D3, D4, D5 | Linhas do teclado (terminais 2, 3 e 4) |
| D6, D7, D8, D9 | Colunas do teclado (terminais 5 a 8, resistor de 10k para 5V em cada) |
| D10 | Botão 2 da sequência (interrupção de mudança de pino PCINT2) |
| D11 | Fio superior amarelo |
| D12 | Fio superior verde, LED 2 da sequência e LED do cofre 2 |
| D13 | Fio superior vermelho, LED 1 da sequência e LED do cofre 1 |
| A0 | Potenciômetro do cofre 1 |
| A1 | Potenciômetro do cofre 2 |
| A2 | Leitura dos fios inferiores |
| A3 | Buzzer |
| A4 / A5 | I2C (SDA / SCL): LCD PCF8574 em 0x20 e display HT16K33 em 0x70 |

### O que mudou de lugar

| Componente | Antes | Agora | Motivo |
|---|---|---|---|
| Teclado, terminal 1 (linha das letras A, B, C, D) | D2 | Desligado | Liberar D2. As letras não eram usadas |
| Teclado próprio do binário (4x3) | D2 a D5 e D8 a D10 | Removido | O binário usa o teclado 4x4 da bomba |
| LED 1 da sequência | D6 | D13 | D6 é coluna do teclado |
| LED 2 da sequência | D4 | D12 | D4 é linha do teclado |
| Botão 2 da sequência | D3 | D10 | D3 é linha do teclado |
| LED do cofre 1 | D5 | D13 | D5 é linha do teclado |
| LED do cofre 2 | D6 | D12 | D6 é coluna do teclado |
| Leitura dos fios inferiores | A1 | A2 | A1 é o potenciômetro do cofre 2 |
| Buzzer | D11 | A3 | D11 é o fio amarelo |

Sem mudança: botão 1 da sequência (D2), colunas e linhas D3 a D9 do teclado, fios superiores (D11 a D13), potenciômetros do cofre (A0 e A1), LCD e display (I2C).

Os LEDs da sequência e do cofre usam os mesmos pinos dos fios vermelho (D13) e verde (D12). Se o circuito dos fios já tem LEDs nesses pinos, eles servem para os 3 desafios. Se não tem, ligue um LED com resistor (220 a 330 ohms) do pino ao GND.

Os botões continuam com resistor pull-down externo (botão ligado ao 5V). Para usar pull-up interno com o botão ligado ao GND, mude `PULLDOWN_EXTERNO` para 0.

## Mudanças de código na integração

- Timer1: cada módulo tinha a própria ISR (1 s, 10 ms ou 1 ms). Agora existe uma ISR de 1 ms que incrementa `ms` (base de tempo de todos os desafios) e decrementa o cronômetro da bomba a cada 1000 ms.
- Timer2: continua gerando o som do buzzer, mas o pino passou a ser invertido pela ISR `TIMER2_COMPA_vect`, porque a saída de hardware (D11) agora é um fio.
- UART: `Serial` da bomba e dos fios foi trocado pelas funções por registradores da sequência, e a UART do cofre foi unificada com elas. O `R` agora reinicia o jogo inteiro.
- I2C: o binário passou a usar a biblioteca Wire, como a bomba, em vez do TWI por registradores, para não ter dois drivers no mesmo barramento. A bomba agora liga a luz de fundo do LCD (bit P3), como o binário já fazia.
- Cada `setup()` e `loop()` de desafio virou `xxx_iniciar()` e `xxx_executar()`. O `executar` faz um passo, volta sem bloquear e devolve 1 quando o desafio termina.
- Erros: os desafios chamam `registrarErro()`, que conta o erro e desconta 5 s ou 10 s do cronômetro.
- Fios: o contador de 5 s no display e o `while(1)` final saíram. O display de 7 segmentos é só do cronômetro, e o número sorteado aparece no LCD.
- Sequência, interrupções externas: o botão 1 (D2) usa INT0 (`EICRA`, `EIMSK`) e o botão 2 (D10) usa a interrupção de mudança de pino PCINT2 (`PCICR`, `PCMSK0`). As ISRs só gravam em `botao_evento` qual botão foi apertado. A borda é de subida com `PULLDOWN_EXTERNO 1` e de descida com `PULLDOWN_EXTERNO 0`.
- Sequência, sem bloqueio: as funções que esperavam dentro de laços viraram estados de uma máquina de estados (tabela abaixo). Os tempos são os mesmos do original (300 ms por LED, 500 ms de pausa, 30 ms de debounce, piscadas de 200 ms, 1 s antes de repetir).
- Funções do Arduino: `delay()` e `delayMicroseconds()` da bomba foram trocadas por `_delay_ms()` e `_delay_us()` da avr-libc. O sketch não usa mais nenhuma função pronta do Arduino para periféricos (só a biblioteca Wire para o LCD e o display).

### Estados da sequência

| Estado | Substitui | O que faz |
|---|---|---|
| `SEQ_APRESENTANDO` | `apresentar_sequencia()` | Pisca os 2 LEDs juntos e depois a sequência, 300 ms aceso e 300 ms apagado |
| `SEQ_PAUSA` | `espera_ms(500)` | Pausa antes da resposta |
| `SEQ_ESPERA_BOTAO` | `ler_botao()` | Espera a flag `botao_evento` das interrupções |
| `SEQ_CONFIRMA_BOTAO` | `ler_botao()` | Depois de 30 ms, lê os botões de novo. Sinal sumiu: descarta. Os dois juntos: ignora |
| `SEQ_SOLTAR` | `aguardar_soltar()` | Espera soltar os botões |
| `SEQ_SOLTAR_REPIQUE` | `aguardar_soltar()` | 30 ms de repique na liberação, depois confere a resposta |
| `SEQ_ERRO` | `indicar_erro()` e `espera_ms(1000)` | 3 piscadas e pausa de 1 s, depois volta à apresentação |
| `SEQ_VITORIA` | `indicar_vitoria()` | LEDs acesos 1 s, apagados 300 ms e acesos de novo |

## Nomes alterados em relação aos códigos originais

Nomes que não aparecem nas tabelas abaixo continuam iguais aos originais.

### Comuns a todos os módulos

| Original | No sketch integrado | Motivo |
|---|---|---|
| `setup()` e `loop()` de cada desafio | `xxx_iniciar()` e `xxx_executar()` | Só pode existir um `setup()` e um `loop()` (os da bomba) |
| `ISR(TIMER1_COMPA_vect)` de cada módulo | Uma única ISR no módulo da bomba | Só pode existir uma ISR por vetor |
| `set_bit`, `clr_bit`, `cpl_bit`, `tst_bit` repetidos em cada arquivo | Definidos uma vez no topo | Evitar repetição |

### bomba.ino

| Original | No sketch integrado |
|---|---|
| `Serial.begin(9600)` | `uart_init()` |
| `Serial.println(texto)` | `uart_enviar_linha(texto)` |
| `Serial.print(texto)` | `uart_enviar_texto(texto)` |
| `Serial.println(tecla)` | `uart_enviar_char(tecla)` seguido de `uart_enviar_texto("\r\n")` |
| `delay(x)` | `_delay_ms(x)` |
| `delayMicroseconds(x)` | `_delay_us(x)` |
| Máscara `0b00111100` das linhas do teclado | `LINHAS_TECLADO` (`0b00111000`, sem o D2) |
| `180` (tempo inicial em `iniciarJogo`) | `TEMPO_TOTAL_S` |
| Trecho do `loop()` que lia o cronômetro e atualizava o display | `servicoBomba()` |
| Fim de jogo dentro do `loop()` (`jogoEncerrado = 1`) | `encerrarJogo(venceu)` e `reiniciarJogo()` |

Novos nomes na bomba: `registrarErro()`, `bip()`, `atualizarBip()`, `telaExplosao()`, `telaVitoria()`, `telaDesafio()`, `sortearOrdem()`, `iniciarDesafioAtual()`, `executarDesafioAtual()`, as variáveis `erros`, `ordemDesafios`, `indiceDesafio`, `desafioIniciado`, `vitoria`, `esperarAte`, `momentoFim`, `teclaJogo`, `contadorSegundo` e as constantes `PENALIDADE_FACIL_S`, `PENALIDADE_DIFICIL_S`, `MAX_ERROS_JOGO`, `TEMPO_AVISO_S`, `PAUSA_DESAFIOS_MS`, `TEMPO_EXPLOSAO_MS`, `NUM_DESAFIOS`, `DESAFIO_BINARIO`, `DESAFIO_COFRE`, `DESAFIO_SEQUENCIA` e `DESAFIO_FIOS`.

### binario_decimal1 (2).ino

| Original | No sketch integrado |
|---|---|
| `setup()` | `binario_iniciar()` |
| `loop()` | `binario_executar()` |
| `static uint8_t erros` (erros do desafio) | `erros` global da bomba (erros do jogo todo) |
| `MAX_ERROS` | `MAX_ERROS_JOGO` (na bomba) |
| `PCF_ENDERECO 0x20` | `PCF_ENDERECO LCD_ADDR` (mesmo valor, definido pela bomba) |
| `teclado_tecla()` lendo o teclado próprio | `teclado_tecla()` devolvendo `teclaJogo` (teclado da bomba) |
| Semente fixa `fase_teclado_inicia(1234)` | Semente vinda do tempo: `ler_tempo_ms() ^ TCNT1` |

Removidos: `CLOCK_HZ`, `TWI_SCL_HZ`, `twi_init()`, `twi_espera()`, `twi_start()`, `twi_stop()`, `twi_envia()`, `lcd_init()`, `LINHAS_DDR`, `LINHAS_PORT`, `COLUNAS_DDR`, `COLUNAS_PORT`, `COLUNAS_PIN`, `BIT_LINHA`, `BIT_COLUNA`, `MAPA_TECLAS`, `teclado_init()`, `teclado_varre()`, `tick_10ms`, `fase_teclado_erros()`, `DESAFIO_FALHOU` e `FASE_FALHOU`.

### cofre.ino

| Original | No sketch integrado |
|---|---|
| `setup()` | `cofre_iniciar()` |
| `loop()` | `cofre_executar()` |
| `desafio_concluido` | `cofre_concluido` (a sequência tinha uma variável com o mesmo nome) |
| `tempo_ms` (16 bits) | `ms` compartilhado (32 bits), lido por `agora_ms()` |
| `LED_COFRE1 PD5` | `LED_COFRE1 PB5` (D13) |
| `LED_COFRE2 PD6` | `LED_COFRE2 PB4` (D12) |
| `PORTD` e `DDRD` dos LEDs | `LED_COFRE_PORT` e `LED_COFRE_DDR` |
| `uart_init()` só de transmissão | `uart_init()` da sequência (transmissão e recepção) |

### desafio_sequencia.ino

| Original | No sketch integrado |
|---|---|
| `setup()` | `sequencia_iniciar()` |
| `loop()` | `sequencia_executar()` |
| `desafio_concluido` | `sequencia_concluida` (o cofre tinha uma variável com o mesmo nome) |
| `ms` (8 bits) | `ms` compartilhado (32 bits) |
| `LED1 PD6` | `LED1 PB5` (D13) |
| `LED2 PD4` | `LED2 PB4` (D12) |
| `BOTAO2 PD3` | `BOTAO2 PB2` (D10) |
| `PORTD`, `DDRD` e `PIND` dos LEDs e botões | `LED_PORT`, `LED_DDR`, `BOTAO1_DDR`, `BOTAO1_PORT`, `BOTAO1_PIN`, `BOTAO2_DDR`, `BOTAO2_PORT`, `BOTAO2_PIN` |
| `PRESSIONADO(pino)` lendo sempre `PIND` | `PRESSIONADO(pino)` lendo o `PIN` da porta de cada botão |
| `uart_enviar_numero(uint8_t)` | `uart_enviar_numero(uint16_t)` do cofre |
| `espera_ms()`, `apresentar_sequencia()`, `aguardar_soltar()`, `ler_botao()`, `indicar_erro()`, `indicar_vitoria()` | Estados `SEQ_*` (ver tabela de estados) |

Novos nomes na sequência: `botao_evento`, `ISR(INT0_vect)`, `ISR(PCINT0_vect)`, `seq_estado`, `seq_passo`, `seq_posicao`, `seq_resposta`, `seq_marca`, `seq_ir_para()`, `seq_decorrido()`, `seq_proximo_passo()`, `seq_conferir_resposta()` e as constantes de tempo `SEQ_TEMPO_LED`, `SEQ_PAUSA_RESPOSTA`, `SEQ_DEBOUNCE`, `SEQ_PISCA_ERRO`, `SEQ_PAUSA_ERRO`, `SEQ_VITORIA_ACESO` e `SEQ_VITORIA_APAGADO`.

`reiniciar_solicitado`, `COMANDO_REINICIO` e `ISR(USART_RX_vect)` continuam com o mesmo nome, mas foram para o módulo compartilhado porque agora reiniciam o jogo inteiro.

### fios.ino

| Original | No sketch integrado |
|---|---|
| `setup()` | `fios_iniciar()` |
| `loop()` | `fios_executar()` |
| `finalizar_jogo()` | `finalizar_fios()` (não encerra mais o jogo, só o desafio) |
| `penalidade_ms += 5000UL` em `aplicar_regra()` | `registrarErro()` |
| Leitura em `PC1` / `MUX0` (A1) | `PC2` / `MUX1` (A2) |
| `Serial.print(...)` | `uart_enviar_texto()` e `uart_enviar_numero()` |

Removidos: `endereco`, `numeros`, `inicio`, `penalidade_ms`, `ultimo`, `encerrado`, `configurar_timer()`, `configurar_display()`, `comando()`, `mostrar()` e `mostrar_resultado()`. `ms` e `ler_tempo_ms()` foram para o módulo compartilhado.

Novos nomes nos fios: `fios_concluido`, `fios_todos_corretos()` e `FIOS_TEMPO_ESTAVEL_MS`.

### buzzer.ino

| Original | No sketch integrado |
|---|---|
| Pino `PB3` (D11, saída OC2A) | `BUZZER_PORT`, `BUZZER_DDR`, `BUZZER_BIT` (`PC3`, A3) |
| `COM2A0` em `meu_tone()` (hardware inverte o pino) | `ISR(TIMER2_COMPA_vect)` inverte o pino |
| `setup()` e `loop()` de teste | Removidos, o buzzer é usado por `bip()` |

`meu_tone()` e `meu_noTone()` mantêm o nome.

## Como compilar

Abra `escape_room/escape_room.ino` na IDE do Arduino (a pasta e o arquivo precisam ter o mesmo nome), selecione Arduino Uno e envie. Única biblioteca usada: `Wire` (já vem com a IDE).

O sketch foi compilado com avr-gcc e o core oficial Arduino AVR 1.8.6, sem avisos: cerca de 8,8 KB de flash e 1,1 KB de RAM. A máquina de estados da sequência foi testada no PC com os registradores simulados (apresentação, resposta errada e resposta certa). O sketch ainda não foi testado no hardware nem no simulador.

## Pendências em relação ao enunciado

- Código final: o enunciado pede que o jogador digite um código final. O número sorteado pelos fios pode ser usado para isso, mas ainda não existe essa etapa.
