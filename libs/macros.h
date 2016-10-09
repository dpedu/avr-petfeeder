#pragma once

#define SET_BIT(byte, bit) ((byte) |= _BV(bit))
#define CLEAR_BIT(byte,bit) ((byte) &= ~_BV(bit))
#define IS_SET(byte,bit) (((byte) & (1UL << (bit))) >> (bit))
