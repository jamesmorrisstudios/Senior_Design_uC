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
 
#ifndef nativeSPI_h
#define nativeSPI_h

#include "WProgram.h"
#include "pins_arduino.h"

void spiInit(uint8_t spiRate);
uint8_t spiRec();
uint8_t spiRec(uint8_t* buf, size_t len);
void spiSend(uint8_t b);
void spiSend(const uint8_t* output, size_t len);

#endif  // nativeSPI_h
