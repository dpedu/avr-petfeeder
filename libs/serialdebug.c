#include <libs/serialdebug.h>

#if SERIALDEBUG_USE_INTS
#include <avr/interrupt.h>
#endif

void serial_configure(void) {
  // Configure serial output
  SERIALDEBUG_SOFT_TX_PORT |= (1<<SERIALDEBUG_SOFT_TX_BIT);
  SERIALDEBUG_SOFT_TX_DDR |= (1<<SERIALDEBUG_SOFT_TX_BIT);
  fdev_setup_stream(&serialdebug_usartout, usart_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &serialdebug_usartout;
}

int usart_putchar (char c, FILE *stream) {
  #if SERIALDEBUG_USE_INTS
  cli();
  #endif
  // Print 1 char to serial
  uint8_t bit_mask;

  // start bit
  SERIALDEBUG_SOFT_TX_PORT &= ~(1<<SERIALDEBUG_SOFT_TX_BIT);
  _delay_us(SERIALDEBUG_MICROSECONDS_PER_BIT);

  // data bits
  for (bit_mask=0x01; bit_mask; bit_mask<<=1) {
    if (c & bit_mask) {
      SERIALDEBUG_SOFT_TX_PORT |= (1<<SERIALDEBUG_SOFT_TX_BIT);
    }
    else {
      SERIALDEBUG_SOFT_TX_PORT &= ~(1<<SERIALDEBUG_SOFT_TX_BIT);
    }
    _delay_us(SERIALDEBUG_MICROSECONDS_PER_BIT);
  }

  // stop bit(s)
  SERIALDEBUG_SOFT_TX_PORT |= (1<<SERIALDEBUG_SOFT_TX_BIT);
  _delay_us(SERIALDEBUG_MICROSECONDS_PER_BIT * SERIALDEBUG_STOP_BITS);

  #if SERIALDEBUG_USE_INTS
  sei();
  #endif

  return c;

}

void dbg_putstring(char string[]) {
  // Print a string of arbitrary length to serial
  int i=0;
  while (string[i] != '\0') {
    usart_putchar(string[i], &serialdebug_usartout);
    i++;
  }
}

void dbg_putstring_nl(char string[]) {
  if(string) dbg_putstring(string);
  dbg_putstring("\n");
}

void dbg_putint(unsigned int num) {
  // Create string representing int and print to serial
  char c[7];
  itoa(num, c, 10);
  dbg_putstring(c);
}
