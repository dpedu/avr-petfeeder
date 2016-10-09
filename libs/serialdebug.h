#pragma once

#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <util/delay.h>
#include <serialsettings.h>

FILE static serialdebug_usartout = {0};

void serial_configure(void);
int usart_putchar (char c, FILE *stream);
int usart_putchar_nl (char c, FILE *stream);
