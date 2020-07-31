/*
 *  AnalogReadFast.h
 * 
 *  Copyright (C) 2016  Albert van Dalen http://www.avdweb.nl
 * 
 *  This file is part of CommandStation.
 *
 *  CommandStation is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  CommandStation is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMMANDSTATION_DCC_ANALOGREADFAST_H_
#define COMMANDSTATION_DCC_ANALOGREADFAST_H_

#include <Arduino.h>
#include "DIAG.h"

int inline analogReadFast(uint8_t ADCpin);

#if defined(ARDUINO_ARCH_SAMD) 
int inline analogReadFast(uint8_t ADCpin)
{ 
DIAG(F("starting analogReadFast SAMD\n"));  
  
  ADC->CTRLA.bit.ENABLE = 0;              // disable ADC
  while( ADC->STATUS.bit.SYNCBUSY == 1 ); // wait for synchronization

  int CTRLBoriginal = ADC->CTRLB.reg;
  int AVGCTRLoriginal = ADC->AVGCTRL.reg;
  int SAMPCTRLoriginal = ADC->SAMPCTRL.reg;
  
  ADC->CTRLB.reg &= 0b1111100011111111;          // mask PRESCALER bits
  ADC->CTRLB.reg |= ADC_CTRLB_PRESCALER_DIV64;   // divide Clock by 64
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |   // take 1 sample 
                     ADC_AVGCTRL_ADJRES(0x00ul); // adjusting result by 0
  ADC->SAMPCTRL.reg = 0x00;                      // sampling Time Length = 0

  ADC->CTRLA.bit.ENABLE = 1;                     // enable ADC
//DIAG(F("before while(ADC->STATUS.bit.SYNCBUSY == 1)\n"));  
  while(ADC->STATUS.bit.SYNCBUSY == 1);          // wait for synchronization
//DIAG(F("after   while(ADC->STATUS.bit.SYNCBUSY == 1)\n"));  

//DIAG(F("before analogRead(ADCpin)\n"));  
  int adc = analogRead(ADCpin); 
//DIAG(F("after  analogRead(ADCpin)\n"));  
  
  ADC->CTRLB.reg = CTRLBoriginal;
  ADC->AVGCTRL.reg = AVGCTRLoriginal;
  ADC->SAMPCTRL.reg = SAMPCTRLoriginal;
   
DIAG(F("returning analogReadFast SAMD\n"));  
  return adc;
}

#elif defined(ARDUINO_ARCH_SAMC) 

int inline analogReadFast(uint8_t ADCpin)
{ 
DIAG(F("starting analogReadFast SAMC\n"));  
  Adc* ADC;
  if ( (g_APinDescription[ADCpin].ulPeripheralAttribute & PER_ATTR_ADC_MASK) == PER_ATTR_ADC_STD ) {
    ADC = ADC0;
  } else {
    ADC = ADC1;
  }
  
  ADC->CTRLA.bit.ENABLE = 0;              // disable ADC
  while( ADC->SYNCBUSY.bit.ENABLE == 1 ); // wait for synchronization

  int CTRLBoriginal = ADC->CTRLB.reg;
  int AVGCTRLoriginal = ADC->AVGCTRL.reg;
  int SAMPCTRLoriginal = ADC->SAMPCTRL.reg;
  
  ADC->CTRLB.reg &= 0b1111100011111111;          // mask PRESCALER bits
  ADC->CTRLB.reg |= ADC_CTRLB_PRESCALER_DIV64;   // divide Clock by 64
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |   // take 1 sample 
                     ADC_AVGCTRL_ADJRES(0x00ul); // adjusting result by 0
  ADC->SAMPCTRL.reg = 0x00;                      // sampling Time Length = 0

  ADC->CTRLA.bit.ENABLE = 1;                     // enable ADC
  while(ADC->SYNCBUSY.bit.ENABLE == 1);          // wait for synchronization

  int adc = analogRead(ADCpin); 
  
  ADC->CTRLB.reg = CTRLBoriginal;
  ADC->AVGCTRL.reg = AVGCTRLoriginal;
  ADC->SAMPCTRL.reg = SAMPCTRLoriginal;
   
  return adc;
}

#else
int inline analogReadFast(uint8_t ADCpin) 
{ 
DIAG(F("starting analogReadFast else\n"));  
  
  byte ADCSRAoriginal = ADCSRA; 
  ADCSRA = (ADCSRA & B11111000) | 4; 
  int adc = analogRead(ADCpin);  
  ADCSRA = ADCSRAoriginal;
  return adc;
}
#endif
#endif  // COMMANDSTATION_DCC_ANALOGREADFAST_H_
