/*--------------------------------------------------------*/
// GestPWM.c
/*--------------------------------------------------------*/
//	Description :	Gestion des PWM 
//			        pour TP1 2023-2024
//
//	Auteur 		: 	SPN et ADO
//
//	Version		:	V1.1
//	Compilateur	:	XC32 V1.42 + Harmony 1.08
//
/*--------------------------------------------------------*/



#include <GenericTypeDefs.h>
#include "GestPWM.h"
#include "peripheral/oc/plib_oc.h"
#include "app.h"
#include "Mc32DriverLcd.h"
#include "Mc32DriverAdc.h"
#include "stdlib.h"
#include <math.h>

/**
 * Function name :GPWM_Initialize
 * @author Samuel Pitton 
 * @date 10.01.24
 *
 * @brief Initialise le module de PWM général.
 * 
 * Cette fonction initialise l'état du pont en H, met le pont en H en mode stop,
 * démarre les timers nécessaires et active les OC associées.
 * 
 * @return Aucune valeur de retour.
 */
void GPWM_Initialize()
{ 
   //Initialisation de l'état du pont en H
    BSP_EnableHbrige();
   //Mise en mode stop du pont en H
    PLIB_PORTS_PinClear(PORTS_ID_0, AIN1_HBRIDGE_PORT, AIN1_HBRIDGE_BIT); 
    PLIB_PORTS_PinClear(PORTS_ID_0, AIN2_HBRIDGE_PORT, AIN2_HBRIDGE_BIT);
    
   //Démarrage des timers 
   DRV_TMR0_Start();
   DRV_TMR1_Start();
   DRV_TMR2_Start();
   
   //Démarrage des OC
   DRV_OC0_Start();
   DRV_OC1_Start();   
}

/**
 * Function name :GPWM_GetSettings
 * @author Samuel Pitton 
 * @date 10.01.24
 *
 * @brief Obtient la vitesse et l'angle en mettant à jour les champs de la structure.
 * 
 * Cette fonction effectue la lecture des convertisseurs analogiques-numériques (ADC),
 * réalise une moyenne glissante des valeurs lues, puis convertit ces moyennes en
 * pourcentages pour la vitesse et en degrés pour l'angle. Enfin, elle met à jour
 * les champs de la structure S_pwmSettings avec les valeurs calculées.
 * 
 * @param pData Pointeur vers la structure S_pwmSettings pour mettre à jour les valeurs.
 * 
 * @return Aucune valeur de retour.
 */
void GPWM_GetSettings(S_pwmSettings *pData)	
{
    //Initilisation des variabless locales
    S_ADCResults ADC_value;
    //Initialisation du channel 0 à 512 pour démarrage du code (
    static uint16_t Channel_0[10]={512,512,512,512,512,512,512,512,512,512};
    static uint16_t Channel_1[10]={0};
    static uint8_t cnt_moyenne = 0;
    float moyenne_chan0 = 0;
    float moyenne_chan1 = 0;
    int8_t Angle, Vitesse; 
    int i;

    //Lecture du convertisseur AD 
     ADC_value = BSP_ReadAllADC(); //Lecture de l'ADC 
     Channel_0[cnt_moyenne] = ADC_value.Chan0 ;
     Channel_1[cnt_moyenne] = ADC_value.Chan1 ;
	 
	////////Moyenne glisante////////////////////////////////////////////
	
	//Mise à jour du compteur de case du tableau
    cnt_moyenne++;
    if(cnt_moyenne > 9)
    {
        cnt_moyenne = 0;
    }
     
    //Addition des valeurs des cases des tableaux 
    for (i = 0; i < 10; i++) 
    {
        moyenne_chan0 = Channel_0[i]+ moyenne_chan0 ; 
        moyenne_chan1 = Channel_1[i]+ moyenne_chan1 ; 
    }
	
    //Fait la moyenne de la valeur des valeurs de l'ADC
    moyenne_chan0 = moyenne_chan0 / 10;
    moyenne_chan1 = moyenne_chan1 / 10;
    
	/////// Conversion des valeurs en % et ° /////////////////////////////
      
    //Conversion de la moyenne glisante du channel 0 en vitesse avec arrondi
   Vitesse = (DELTA_VIT/RESOL_ADC)* moyenne_chan0 + 0.5;
   Vitesse = Vitesse - VIT_MIN;
   
   //Conversion de la moyenne glissante du channel 1 en angle avec arrondi
   Angle = (DELTA_ANGLE_F/RESOL_ADC)* moyenne_chan1 +0.5;
   Angle = Angle - ANGLE_MAX;
  
  ////////////////////////////////////////////////////////////////////////////
   
  
    //Actualisation de la valeur de la vitesse
    pData->SpeedSetting = Vitesse;
    pData->absSpeed = abs(Vitesse);
    //Actualisation de la valeur de l'angle 
    pData->AngleSetting = Angle;
	//Ajoute de l'offset pour avoir une valeur entre 0 et 180 °
    pData->absAngle = Angle + ANGLE_MAX; 
	
}

/**
 * Function name : GPWM_DispSettings
 * @author Antonio Do Carmo 
 * @date 10.01.24
 *
 * @brief Affiche les informations en exploitant la structure S_pwmSettings.
 * 
 * Cette fonction prend en paramètre un pointeur vers une structure S_pwmSettings
 * et affiche les informations telles que la vitesse, la vitesse absolue et l'angle
 * sur l'écran LCD. Elle affiche également si l'utilisateur se trouve en mode local ou en mode remote.
 * 
 * @param pData Pointeur vers la structure contenant les paramètres PWM.
 * 
 * @return Aucune valeur de retour.
 */
 
void GPWM_DispSettings(S_pwmSettings *pData, int Remote)
{
    //Positionne le curseur du LCD à la ligne 1, colonne 1
    
    
    static oldRemote = 0;    
            
    //Affiche le mode
    
    if((Remote == 1) && (Remote != oldRemote))
    {
        lcd_ClearLine(1);
        lcd_gotoxy(1, 1);
        printf_lcd("** Remote Settings");       //Affiche "Remote settings" en mode remote
    }
    
    else if ((Remote == 0) && (Remote != oldRemote))
    {
        lcd_ClearLine(1);
        lcd_gotoxy(1, 1);
        printf_lcd("Local Settings");           //Affiche "Local settings" en mode local
    }
    
    //Positionne le curseur du LCD à la ligne 2, colonne 1
    lcd_gotoxy(1, 2);
    
    //Affiche "SpeedSetting" suivi de la valeur de SpeedSetting
    printf_lcd("SpeedSetting");
    lcd_gotoxy(14, 2);
    printf_lcd("%3d ", pData->SpeedSetting);
    
    //Positionne le curseur du LCD à la ligne 3, colonne 1
    lcd_gotoxy(1, 3);
    
    //Affiche "absSpeed" suivi de la valeur de absSpeed
    printf_lcd("absSpeed");
    lcd_gotoxy(15, 3);
    printf_lcd("%2d ", pData->absSpeed);
    
    //Positionne le curseur LCD à la ligne 4, colonne 1
    lcd_gotoxy(1, 4);
    
    //Affiche "Angle" suivi de la valeur de AngleSetting
    printf_lcd("Angle");
    lcd_gotoxy(14, 4);
    printf_lcd("%3d", pData->AngleSetting);
    
    oldRemote = Remote;
}


/**
 * Function name : GPWM_ExecPWM
 * @author Antonio Do Carmo 
 * @date 10.01.24
 *
 * @brief Exécute les PWM et gère les moteur en fonction des informations dans la structure.
 * 
 * Cette fonction prend en compte les informations  la vitesse et l'angle provenant de
 * la structure S_pwmSettings et configure le PWM en conséquence. Elle gère également la
 * direction du moteur en fonction de la vitesse.
 * 
 * @param pData Pointeur vers la structure S_pwmSettings contenant les informations de consigne.
 * 
 * @return Aucune valeur de retour.
 */
void GPWM_ExecPWM(S_pwmSettings *pData)
{
    uint16_t OC2_PulseWidth; 
    uint16_t OC3_PulseWidth; 
	
    //Conversion de la vitesse en nombre de ticks à comparer pour l'OC2 
    //Pour la génération du PWM du moteur DC
    OC2_PulseWidth = (pData->absSpeed * DELTA_TIC_TMR2 / DELTA_V);      
    PLIB_OC_PulseWidth16BitSet(OC_ID_2, OC2_PulseWidth);
    
    //Conversion de l'angle en nombre de ticks à comparer pour l'OC3 
    //Pour la génération du PWM du servomoteur 
    OC3_PulseWidth = ((pData->AngleSetting +90) * DELTA_TIC_TMR3 / DELTA_ANGLE) + OFFSET_TIC_TM3;
    PLIB_OC_PulseWidth16BitSet(OC_ID_3, OC3_PulseWidth); 
    
    //Test si la vitesse est positive 
    if(pData->SpeedSetting > 0)
    {
		//Rotation du moteur dans le sens CCW (PIN AIN1 = 0, AIN2 = 1)
        PLIB_PORTS_PinClear(PORTS_ID_0, AIN2_HBRIDGE_PORT, AIN2_HBRIDGE_BIT);
        PLIB_PORTS_PinSet(PORTS_ID_0, AIN1_HBRIDGE_PORT, AIN1_HBRIDGE_BIT); 
    }
    else if(pData->SpeedSetting < 0)	// Test si la vitesse est négative 
    {
		//Rotation du moteur dans le sens CW (PIN AIN1 = 1, AIN2 = 0)
        PLIB_PORTS_PinClear(PORTS_ID_0, AIN1_HBRIDGE_PORT, AIN1_HBRIDGE_BIT); 
        PLIB_PORTS_PinSet(PORTS_ID_0, AIN2_HBRIDGE_PORT, AIN2_HBRIDGE_BIT);
    }
    else //Si la vitesse = 0
    {
		//Arrêt du moteur DC (AIN1 = 1, AIN2 = 0, pont en H en mode stop)
        PLIB_PORTS_PinClear(PORTS_ID_0, AIN1_HBRIDGE_PORT, AIN1_HBRIDGE_BIT); 
        PLIB_PORTS_PinClear(PORTS_ID_0, AIN2_HBRIDGE_PORT, AIN2_HBRIDGE_BIT);
    }
}

/**
 * Function name: GPWM_ExecPWMSoft
 * @author Samuel Pitton et Antonio Do Carmo
 * @date 10.01.24
 *
 * @brief Génère un PWM logiciel sur la LED2.
 * 
 * Cette fonction utilise une méthode logicielle pour simuler le PWM. Elle ajuste
 * l'état de LED 2 en fonction de la vitesse provenant de la structure S_pwmSettings.
 * 
 * @param pData Pointeur vers la structure S_pwmSettings contenant les informations PWM.
 * 
 * @return Aucune valeur de retour.
 */
void GPWM_ExecPWMSoft(S_pwmSettings *pData)
{
	//Initialisation du compteur de PWM
   static uint8_t cnt_PWM = 0; 
   //Initialisation de l'état de la LED
   static uint8_t state_LED = 0; 
   
	//Test si le temps du PWM est terminé 
    if ((cnt_PWM >= pData->absSpeed))
    {
        //Si la led est éteinte
        if (state_LED == 0)
        {
            //Activation de la LED2
            BSP_LEDOn(BSP_LED_2);
            state_LED = 1;
        }
    }
    else //Temps off du PWM
    {
        //Si La Led est allumée
        if (state_LED == 1)
        {
            //Désactivation de la LED2
            BSP_LEDOff(BSP_LED_2);
            state_LED = 0;
        }
    }
 
 //Incrémentation du compteur de PWM 
    cnt_PWM++;
	
 //Gestion de la remise à 0 du compteur (période du PWM)
    if (cnt_PWM > 99)
    {
        cnt_PWM = 0;
    }
}


