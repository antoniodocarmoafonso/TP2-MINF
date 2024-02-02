/*
 * File: fonction.c
 * Author: Samuel Pitton - Antonio Do Carmo
 * date: 28.11.2023
 * Description: Ce fichier contient l'impl?mentation de diff?rentes fonctions.
Chaque fonction a un objectif sp?cifique au sein du programme
 */ 

#include "fonction.h"


 /* Function Name: Gestion_LED
 * Description: s'occupe d allumer ou ?taindre tout les LEDs
 * Parameters:
 *   - param1: ?tat des lED .
 *   .
 * Returns: -
 */
void Gestion_LED(int LED_State)
{
    //Allume les LEDs
    if(LED_State != 0)
    {
        BSP_LEDOn(BSP_LED_0);
        BSP_LEDOn(BSP_LED_1);
        BSP_LEDOn(BSP_LED_2);
        BSP_LEDOn(BSP_LED_3);
        BSP_LEDOn(BSP_LED_4);
        BSP_LEDOn(BSP_LED_5);
        BSP_LEDOn(BSP_LED_6);
        BSP_LEDOn(BSP_LED_7);
    }
    else    //Eteint les LEDs
    {   
        BSP_LEDOff(BSP_LED_0);
        BSP_LEDOff(BSP_LED_1);
        BSP_LEDOff(BSP_LED_2);
        BSP_LEDOff(BSP_LED_3);
        BSP_LEDOff(BSP_LED_4);
        BSP_LEDOff(BSP_LED_5);
        BSP_LEDOff(BSP_LED_6);
        BSP_LEDOff(BSP_LED_7);
    }               
}


 /* Function Name: LED_Chenillard
 * Description: s'occupe de cr?? un chenillard 
 * Parameters:
 *   - param1: -
 *   .
 * Returns: -
 */
void LED_Chenillard(void)
{ 
    static int Cnt_chenillard = 0; //Enregistre l'état du chenillard 
    
    if (Cnt_chenillard < 7) //Gestion de l'état du chenillard
            {
                Cnt_chenillard++;
            }  
            else
            {
                Cnt_chenillard = 0;
            }
    //Eteint ou allume les leds en fonction de l'état du chenillard
    switch(Cnt_chenillard)
    {
        case 0:
                BSP_LEDOff(BSP_LED_7);
                BSP_LEDOn(BSP_LED_0);
            break;
        
        case 1:
                BSP_LEDOff(BSP_LED_0);
                BSP_LEDOn(BSP_LED_1);
            break;
        
        case 2:
                BSP_LEDOff(BSP_LED_1);
                BSP_LEDOn(BSP_LED_2);
            break;
        
        case 3:
                BSP_LEDOff(BSP_LED_2);
                BSP_LEDOn(BSP_LED_3);
            break;
        
        case 4:
                BSP_LEDOff(BSP_LED_3);
                BSP_LEDOn(BSP_LED_4);
            break;
        
        case 5:
                BSP_LEDOff(BSP_LED_4);
                BSP_LEDOn(BSP_LED_5);
            break;
        
        case 6:
                BSP_LEDOff(BSP_LED_5);
                BSP_LEDOn(BSP_LED_6);
            break;
        
        case 7:
                BSP_LEDOff(BSP_LED_6);
                BSP_LEDOn(BSP_LED_7);
            break;
    }
}