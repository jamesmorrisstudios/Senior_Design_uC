/* Arduino Sd2Card Library
 * Copyright (C) 2012 by William Greiman
 *
 * This file is part of the Arduino Sd2Card Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino Sd2Card Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 *
 * Substantial portions of this code was taken from the Arduino Sd2Card library
 * as it was the only native SPI implementation on the Teensy 3.0 so far.
 *
 *
 *
 *
 */

#include <nativeSPI.h>
//==============================================================================
//#elif USE_NATIVE_MK20DX128_SPI
// Teensy 3.0 functions
#include <mk20dx128.h>
// use 16-bit frame if SPI_USE_8BIT_FRAME is zero
#define SPI_USE_8BIT_FRAME 0
// Limit initial fifo to three entries to avoid fifo overrun
#define SPI_INITIAL_FIFO_DEPTH 3
// define some symbols that are not in mk20dx128.h
#ifndef SPI_SR_RXCTR
#define SPI_SR_RXCTR 0XF0
#endif  // SPI_SR_RXCTR
#ifndef SPI_PUSHR_CONT
#define SPI_PUSHR_CONT 0X80000000
#endif   // SPI_PUSHR_CONT
#ifndef SPI_PUSHR_CTAS
#define SPI_PUSHR_CTAS(n) (((n) & 7) << 28)
#endif  // SPI_PUSHR_CTAS
//------------------------------------------------------------------------------
/**
 * Initialize hardware SPI
 * Set SCK rate to F_CPU/pow(2, 1 + spiRate) for spiRate [0,6]
 */
void spiInit(uint8_t spiRate) {
  SIM_SCGC6 |= SIM_SCGC6_SPI0;
  SPI0_MCR = SPI_MCR_MDIS | SPI_MCR_HALT;
  // spiRate = 0 or 1 : 24 or 12 Mbit/sec
  // spiRate = 2 or 3 : 12 or 6 Mbit/sec
  // spiRate = 4 or 5 : 6 or 3 Mbit/sec
  // spiRate = 6 or 7 : 3 or 1.5 Mbit/sec
  // spiRate = 8 or 9 : 1.5 or 0.75 Mbit/sec
  // spiRate = 10 or 11 : 250 kbit/sec
  // spiRate = 12 or more : 125 kbit/sec
  uint32_t ctar;
  switch (spiRate/2) {
    case 0: ctar = SPI_CTAR_DBR | SPI_CTAR_BR(0); break;
    case 1: ctar = SPI_CTAR_BR(0); break;
    case 2: ctar = SPI_CTAR_BR(1); break;
    case 3: ctar = SPI_CTAR_BR(2); break;
    case 4: ctar = SPI_CTAR_BR(3); break;
#if F_BUS == 48000000
    case 5: ctar = SPI_CTAR_PBR(1) | SPI_CTAR_BR(5); break;
    default: ctar = SPI_CTAR_PBR(1) | SPI_CTAR_BR(6);
#elif F_BUS == 24000000
    case 5: ctar = SPI_CTAR_PBR(1) | SPI_CTAR_BR(4); break;
    default: ctar = SPI_CTAR_PBR(1) | SPI_CTAR_BR(5);
#else
#error "MK20DX128 bus frequency must be 48 or 24 MHz"
#endif
  }
  // CTAR0 - 8 bit transfer
  SPI0_CTAR0 = ctar | SPI_CTAR_FMSZ(7);
  // CTAR1 - 16 bit transfer
  SPI0_CTAR1 = ctar | SPI_CTAR_FMSZ(15);
  SPI0_MCR = SPI_MCR_MSTR;
  CORE_PIN11_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
  CORE_PIN12_CONFIG = PORT_PCR_MUX(2);
  CORE_PIN13_CONFIG = PORT_PCR_DSE | PORT_PCR_MUX(2);
}
//------------------------------------------------------------------------------
/** SPI receive a byte */
uint8_t spiRec() {
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF;
  SPI0_PUSHR = 0xFF;
  while (!(SPI0_SR & SPI_SR_RXCTR)) {}
  return SPI0_POPR;
}
//------------------------------------------------------------------------------
/** SPI receive multiple bytes */
uint8_t spiRec(uint8_t* buf, size_t len) {
  // clear any data in RX FIFO
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF;
#if SPI_USE_8BIT_FRAME
  // initial number of bytes to push into TX FIFO
  int nf = len < SPI_INITIAL_FIFO_DEPTH ? len : SPI_INITIAL_FIFO_DEPTH;
  for (int i = 0; i < nf; i++) {
    SPI0_PUSHR = 0XFF;
  }
  // limit for pushing dummy data into TX FIFO
  uint8_t* limit = buf + len - nf;
  while (buf < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_PUSHR = 0XFF;
    *buf++ = SPI0_POPR;
  }
  // limit for rest of RX data
  limit += nf;
  while (buf < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    *buf++ = SPI0_POPR;
  }
#else  // SPI_USE_8BIT_FRAME
  // use 16 bit frame to avoid TD delay between frames
  // get one byte if len is odd
  if (len & 1) {
    *buf++ = spiRec();
    len--;
  }
  // initial number of words to push into TX FIFO
  int nf = len/2 < SPI_INITIAL_FIFO_DEPTH ? len/2 : SPI_INITIAL_FIFO_DEPTH;
  for (int i = 0; i < nf; i++) {
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | 0XFFFF;
  }
  uint8_t* limit = buf + len - 2*nf;
  while (buf < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | 0XFFFF;
    uint16_t w = SPI0_POPR;
    *buf++ = w >> 8;
    *buf++ = w & 0XFF;
  }
  // limit for rest of RX data
  limit += 2*nf;
  while (buf < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    uint16_t w = SPI0_POPR;
    *buf++ = w >> 8;
    *buf++ = w & 0XFF;
  }
#endif  // SPI_USE_8BIT_FRAME
  return 0;
}
//------------------------------------------------------------------------------
/** SPI send a byte */
void spiSend(uint8_t b) {
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF;
  SPI0_PUSHR = b;
  while (!(SPI0_SR & SPI_SR_RXCTR)) {}
  SPI0_POPR;  // not required?
}
//------------------------------------------------------------------------------
/** SPI send multiple bytes */
void spiSend(const uint8_t* output, size_t len) {
  // clear any data in RX FIFO
  SPI0_MCR = SPI_MCR_MSTR | SPI_MCR_CLR_RXF;
#if SPI_USE_8BIT_FRAME
  // initial number of bytes to push into TX FIFO
  int nf = len < SPI_INITIAL_FIFO_DEPTH ? len : SPI_INITIAL_FIFO_DEPTH;
  // limit for pushing data into TX fifo
  const uint8_t* limit = output + len;
  for (int i = 0; i < nf; i++) {
    SPI0_PUSHR = *output++;
  }
  // write data to TX FIFO
  while (output < limit) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_PUSHR = *output++;
    SPI0_POPR;
  }
  // wait for data to be sent
  while (nf) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_POPR;
    nf--;
  }
#else  // SPI_USE_8BIT_FRAME
  // use 16 bit frame to avoid TD delay between frames
  // send one byte if len is odd
  if (len & 1) {
    spiSend(*output++);
    len--;
  }
  // initial number of words to push into TX FIFO
  int nf = len/2 < SPI_INITIAL_FIFO_DEPTH ? len/2 : SPI_INITIAL_FIFO_DEPTH;
  // limit for pushing data into TX fifo
  const uint8_t* limit = output + len;
  for (int i = 0; i < nf; i++) {
    uint16_t w = (*output++) << 8;
    w |= *output++;
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | w;
  }
  // write data to TX FIFO
  while (output < limit) {
    uint16_t w = *output++ << 8;
    w |= *output++;
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_PUSHR = SPI_PUSHR_CONT | SPI_PUSHR_CTAS(1) | w;
    SPI0_POPR;
  }
  // wait for data to be sent
  while (nf) {
    while (!(SPI0_SR & SPI_SR_RXCTR)) {}
    SPI0_POPR;
    nf--;
  }
#endif  // SPI_USE_8BIT_FRAME
}