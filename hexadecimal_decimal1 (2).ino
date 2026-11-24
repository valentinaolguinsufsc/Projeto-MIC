#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#define set_bit(reg, bit) ((reg) |= (1 << (bit)))
#define clr_bit(reg, bit) ((reg) &= ~(1 << (bit)))
#define cpl_bit(reg, bit) ((reg) ^= (1 << (bit)))
#define tst_bit(reg, bit) ((reg) & (1 << (bit)))

#define PCF_ENDERECO      0x20   // PCF8574 com A0, A1 e A2 no GND
#define CLOCK_HZ          16000000UL   // clock do Arduino Uno
#define TWI_SCL_HZ        100000UL     // velocidade do I2C

// bits do PCF8574 ligados ao LCD
#define LCD_RS            0x01   // P0
#define LCD_E             0x04   // P2
#define LCD_LUZ           0x08   // P3 (luz de fundo, se estiver ligada)
// P4 a P7 levam os dados D4 a D7 do LCD

#define FASE_BITS         8      // número sorteado de 8 bits (0x00 a 0xFF)
#define FASE_NUM_MIN      1      // menor número sorteado
#define FASE_MAX_DIGITOS  3      // máximo de dígitos decimais digitados
#define MAX_ERROS         3      // no terceiro erro o desafio falha

// estados do desafio
#define DESAFIO_ANDAMENTO 0
#define DESAFIO_CONCLUIDO 1
#define DESAFIO_FALHOU    2

// resultados devolvidos por fase_teclado_atualiza()
#define FASE_EM_ANDAMENTO 0
#define FASE_ACERTOU      1
#define FASE_ERROU        2
#define FASE_FALHOU       3      // terceiro erro: o desafio termina sem passar de fase

static uint8_t estado_desafio = DESAFIO_ANDAMENTO;
static uint8_t erros = 0;   // contagem de erros do desafio

// Configura o I2C de hardware (TWI) a 100 kHz, nos pinos A4 (SDA) e A5 (SCL)
static void twi_init(void) {
  TWSR = 0;
  TWBR = ((CLOCK_HZ / TWI_SCL_HZ) - 16) / 2;
  set_bit(PORTC, PC4);
  set_bit(PORTC, PC5);
  TWCR = (1 << TWEN);
}

// espera o fim da operação do TWI. devolve 0 se estourar o tempo
static uint8_t twi_espera(void) {
  uint16_t tentativas = 0xFFFF;
  while (!tst_bit(TWCR, TWINT)) {
    if (--tentativas == 0) return 0;
  }
  return 1;
}

// envia a condição de START e confere o status (0x08 ou 0x10)
static uint8_t twi_start(void) {
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
  if (!twi_espera()) return 0;
  uint8_t status = TWSR & 0xF8;
  return (status == 0x08 || status == 0x10);
}

// envia a condição de STOP
static void twi_stop(void) {
  TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}


static uint8_t twi_envia(uint8_t dado, uint8_t status_esperado) {
  TWDR = dado;
  TWCR = (1 << TWINT) | (1 << TWEN);
  if (!twi_espera()) return 0;
  return (TWSR & 0xF8) == status_esperado;
}

static void pcf_escreve_dois(uint8_t primeiro, uint8_t segundo) {
  if (twi_start()) {
    if (twi_envia((PCF_ENDERECO << 1) | 0, 0x18)) {
      if (twi_envia(primeiro, 0x28)) {
        twi_envia(segundo, 0x28);
      }
    }
  }
  twi_stop();
}


static void lcd_nibble(uint8_t nibble, uint8_t rs) {
  uint8_t dado = (nibble << 4) | LCD_LUZ | rs;
  pcf_escreve_dois(dado | LCD_E, dado & ~LCD_E);
}


static void lcd_byte(uint8_t valor, uint8_t rs) {
  lcd_nibble(valor >> 4, rs);
  lcd_nibble(valor & 0x0F, rs);
}

// envia um comando ao LCD. limpar e voltar ao início levam cerca de 2 ms
static void lcd_comando(uint8_t comando) {
  lcd_byte(comando, 0);
  if (comando <= 0x03) _delay_ms(2);
}

// escreve um caractere na posição atual do cursor
static void lcd_caractere(char c) {
  lcd_byte(c, LCD_RS);
}

// escreve um texto a partir da posição atual do cursor
static void lcd_texto(const char *texto) {
  while (*texto) lcd_caractere(*texto++);
}

// escreve um número decimal sem sinal (até 5 dígitos)
static void lcd_numero(uint16_t numero) {
  char invertido[5];
  uint8_t n = 0;
  if (numero == 0) {
    lcd_caractere('0');
    return;
  }
  while (numero > 0) {
    invertido[n++] = '0' + (numero % 10);
    numero /= 10;
  }
  while (n > 0) lcd_caractere(invertido[--n]);
}

// move o cursor para a coluna (0 a 15) e a linha (0 ou 1)
static void lcd_posicao(uint8_t coluna, uint8_t linha) {
  lcd_comando(0x80 | (coluna + (linha ? 0x40 : 0x00)));
}

//escreve um texto na linha indicada, completando com espaços até 16 colunas
static void lcd_linha(uint8_t linha, const char *texto) {
  lcd_posicao(0, linha);
  uint8_t n = 0;
  while (texto[n] && n < 16) {
    lcd_caractere(texto[n]);
    n++;
  }
  while (n < 16) {
    lcd_caractere(' ');
    n++;
  }
}

//sequência de inicialização do LCD HD44780 em modo de 4 bits, 2 linhas
static void lcd_init(void) {
  _delay_ms(50);
  lcd_nibble(0x03, 0);
  _delay_ms(5);
  lcd_nibble(0x03, 0);
  _delay_us(150);
  lcd_nibble(0x03, 0);
  _delay_us(150);
  lcd_nibble(0x02, 0);   // passa para o modo de 4 bits
  lcd_comando(0x28);     // 4 bits, 2 linhas, fonte 5x8
  lcd_comando(0x0C);     // display ligado, cursor apagado
  lcd_comando(0x06);     // cursor avança para a direita
  lcd_comando(0x01);     // limpa a tela
}

//mostra na linha 1 o número que o jogador deve converter (ex.: "Hex:0x2F")
void display_mostra_enunciado(const char *texto) {
  lcd_posicao(0, 0);
  lcd_texto("Hex:");
  lcd_texto(texto);
  for (uint8_t i = 8; i < 16; i++) lcd_caractere(' ');
}

// mostra na linha 2 o valor decimal digitado até agora (ex.: "Dec:47")
// completa o campo com espaços e apaga a mensagem "Errou!" da linha 1
void display_mostra_digitado(uint16_t valor, uint8_t qtd_digitos) {
  uint8_t n = 0;
  if (qtd_digitos > 0) n = (valor < 10) ? 1 : (valor < 100) ? 2 : 3;
  lcd_posicao(0, 1);
  lcd_texto("Dec:");
  if (qtd_digitos > 0) lcd_numero(valor);
  for (uint8_t i = 4 + n; i < 7; i++) lcd_caractere(' ');
  lcd_posicao(9, 0);
  for (uint8_t i = 9; i < 16; i++) lcd_caractere(' ');
}

// mostra a quantidade de erros no canto direito da linha 2 (ex.: "Erros:1")
static void mostra_erros(void) {
  lcd_posicao(7, 1);
  lcd_texto("Erros:");
  lcd_numero(erros);
}

// linhas do teclado em D2 a D5 (saídas), colunas em D8 a D10 (entradas com pull-up)
#define LINHAS_DDR   DDRD
#define LINHAS_PORT  PORTD
#define COLUNAS_DDR  DDRB
#define COLUNAS_PORT PORTB
#define COLUNAS_PIN  PINB
static const uint8_t BIT_LINHA[4]  = {PD2, PD3, PD4, PD5};
static const uint8_t BIT_COLUNA[3] = {PB0, PB1, PB2};

// tecla correspondente a cada cruzamento linha x coluna
static const char MAPA_TECLAS[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

//configura linhas em alta impedância e colunas como entrada com pull-up
static void teclado_init(void) {
  for (uint8_t i = 0; i < 4; i++) {
    clr_bit(LINHAS_DDR, BIT_LINHA[i]);
    clr_bit(LINHAS_PORT, BIT_LINHA[i]);
  }
  for (uint8_t i = 0; i < 3; i++) {
    clr_bit(COLUNAS_DDR, BIT_COLUNA[i]);
    set_bit(COLUNAS_PORT, BIT_COLUNA[i]);
  }
}

// Ativa uma linha por vez (nível 0) e lê as colunas.
// Devolve a tecla pressionada no momento, ou 0 se nenhuma.
static char teclado_varre(void) {
  for (uint8_t linha = 0; linha < 4; linha++) {
    set_bit(LINHAS_DDR, BIT_LINHA[linha]);
    _delay_us(5);
    uint8_t leitura = COLUNAS_PIN;
    clr_bit(LINHAS_DDR, BIT_LINHA[linha]);
    for (uint8_t col = 0; col < 3; col++) {
      if (!(leitura & (1 << BIT_COLUNA[col]))) return MAPA_TECLAS[linha][col];
    }
  }
  return 0;
}

// debounce: deve ser chamada a cada 10 ms. devolve a tecla uma única vez
// por aperto, depois de 4 leituras iguais seguidas (40 ms)
static char teclado_tecla(void) {
  static char candidata = 0, estavel = 0;
  static uint8_t contador = 0;

  char lida = teclado_varre();
  if (lida == candidata) {
    if (contador < 3) contador++;
  } else {
    candidata = lida;
    contador = 0;
  }
  if (contador == 3 && candidata != estavel) {
    estavel = candidata;
    if (estavel) return estavel;
  }
  return 0;
}

static uint8_t  numero_alvo = 0;
static uint16_t valor_digitado = 0;
static uint8_t  qtd_digitos = 0;
static uint16_t estado_aleatorio = 1;


static uint16_t proximo_aleatorio(void) {
  estado_aleatorio ^= estado_aleatorio << 7;
  estado_aleatorio ^= estado_aleatorio >> 9;
  estado_aleatorio ^= estado_aleatorio << 8;
  return estado_aleatorio;
}

// Converte um número em texto hexadecimal

void hex_para_texto(uint8_t numero, uint8_t bits, char *saida) {
  uint8_t k = 0;
  saida[k++] = '0';
  saida[k++] = 'x';
  for (int8_t i = ((bits + 3) / 4) - 1; i >= 0; i--) {
    uint8_t nibble = (numero >> (4 * i)) & 0x0F;
    saida[k++] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
  }
  saida[k] = '\0';
}

// sorteia um número, mostra em hexadecimal e zera o que foi digitado
void fase_teclado_inicia(uint16_t semente) {
  uint8_t maximo = (uint8_t)((1u << FASE_BITS) - 1);
  char texto[5];

  estado_aleatorio = semente | 1;
  numero_alvo = FASE_NUM_MIN + (proximo_aleatorio() % (uint16_t)(maximo - FASE_NUM_MIN + 1));
  valor_digitado = 0;
  qtd_digitos = 0;
  erros = 0;

  hex_para_texto(numero_alvo, FASE_BITS, texto);
  display_mostra_enunciado(texto);
  display_mostra_digitado(0, 0);
  mostra_erros();
}

// devolve a quantidade de erros do desafio (para o jogo geral)
uint8_t fase_teclado_erros(void) {
  return erros;
}

// verifica a tecla apertada (se houver): dígitos montam o valor decimal,
// '*' apaga e '#' confirma. Devolve ACERTOU, ERROU ou FALHOU (terceiro erro)
// uma única vez a cada confirmação, e EM_ANDAMENTO nos demais casos
uint8_t fase_teclado_atualiza(void) {
  char tecla = teclado_tecla();
  if (!tecla) return FASE_EM_ANDAMENTO;

  if (tecla >= '0' && tecla <= '9') {
    if (qtd_digitos < FASE_MAX_DIGITOS) {
      valor_digitado = valor_digitado * 10 + (tecla - '0');
      qtd_digitos++;
      display_mostra_digitado(valor_digitado, qtd_digitos);
    }
  } else if (tecla == '*') {
    valor_digitado = 0;
    qtd_digitos = 0;
    display_mostra_digitado(0, 0);
  } else if (tecla == '#') {
    if (qtd_digitos == 0) return FASE_EM_ANDAMENTO;
    uint8_t acertou = (valor_digitado == numero_alvo);
    valor_digitado = 0;
    qtd_digitos = 0;
    display_mostra_digitado(0, 0);
    if (acertou) return FASE_ACERTOU;
    if (erros < 255) erros++;
    mostra_erros();
    return (erros >= MAX_ERROS) ? FASE_FALHOU : FASE_ERROU;
  }
  return FASE_EM_ANDAMENTO;
}

volatile uint8_t tick_10ms = 0;

// Interrupção do Timer1: a cada 10 ms levanta a flag usada no loop.
ISR(TIMER1_COMPA_vect) {
  tick_10ms = 1;
}

// Inicializa o I2C, o LCD, o Timer1 (CTC, 10 ms), o teclado e o desafio
void setup() {
  twi_init();
  lcd_init();

  cli();
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);   // CTC, prescaler 64
  OCR1A = 2499;                                        // 2500 passos de 4 us = 10 ms
  TIMSK1 = (1 << OCIE1A);
  sei();

  teclado_init();
  fase_teclado_inicia(1234);
}

// a cada 10 ms: lê o teclado e ve se acertou ou errou
// acertar conclui o desafio e libera a próxima fase; o terceiro erro encerra o
// desafio sem liberar a próxima fase; nos outros erros só limpa os dígitos e avisa
// (a penalidade de tempo é tratada no jogo geral)
void loop() {
  if (!tick_10ms || estado_desafio != DESAFIO_ANDAMENTO) return;
  tick_10ms = 0;

  uint8_t r = fase_teclado_atualiza();
  if (r == FASE_ACERTOU) {
    estado_desafio = DESAFIO_CONCLUIDO;
    lcd_linha(0, "Parabens, passou");
    lcd_linha(1, "p/ proxima fase");
  } else if (r == FASE_FALHOU) {
    estado_desafio = DESAFIO_FALHOU;
    lcd_linha(0, "3 ERROS");
    lcd_linha(1, "VOCE FALHOU!");
  } else if (r == FASE_ERROU) {
    lcd_posicao(9, 0);
    lcd_texto("Errou!");
  }
}
