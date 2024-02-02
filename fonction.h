/* 
 * File:   fonction.h
 * Author: sampitton
 *
 * Created on 28. novembre 2023, 11:32
 */

#ifndef FONCTION_H
#define	FONCTION_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "system_config.h"
#include "system_definitions.h"


 //Définition du ON/OFF des LEDs 
#define ON 1    
#define OFF 0

//Prototype des fonction     
void Gestion_LED(int LED_State);
void LED_Chenillard();


#ifdef	__cplusplus
}
#endif

#endif	/* FONCTION_H */ 

