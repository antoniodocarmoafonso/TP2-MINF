#ifndef GestPWM_H
#define GestPWM_H
/*--------------------------------------------------------*/
// GestPWM.h
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

#include <stdint.h>


//Définition des valeur de conversion

//////Valeur de conversion pour ADC ////////////////////////////////

#define RESOL_ADC 1023.0  	// resolution de l'ADC
#define DELTA_VIT 198.0	 	// Excurtion totale de la vitesse
#define VIT_MAX 99			// Valeur max de la vitesse absolue
#define VIT_MIN 99

#define DELTA_ANGLE_F 180.0	// Excurtion totale de l'angle float
#define	ANGLE_MAX 90		// Valeur max de l'angle absolue

////////////Valeur de conversion OC ////////////////////////////////

#define DELTA_TIC_TMR2 2000	   // totale de TIC du timer 2
#define DELTA_V	100			   // Excurtion de la vitesse 0->99 pour OC 

#define DELTA_TIC_TMR3 9000    // Excurtion de TIC pour timer 3
#define DELTA_ANGLE 180		   // Excurtion totale de l'angle entier
#define OFFSET_TIC_TM3 2999    // Offest de tic pour 0 degré 

/*--------------------------------------------------------*/
// Définition des fonctions prototypes
/*--------------------------------------------------------*/


typedef struct {
    uint8_t absSpeed;    // vitesse 0 ? 99
    uint8_t absAngle;    // Angle  0 ? 180
    int8_t SpeedSetting; // consigne vitesse -99 ? +99
    int8_t AngleSetting; // consigne angle  -90 ? +90
} S_pwmSettings;



void GPWM_Initialize();		//Initialisation des OC et des Timers, pont en H

// Ces 3 fonctions ont pour paramètre un pointeur sur la structure S_pwmSettings.
void GPWM_GetSettings(S_pwmSettings *pData);	// Obtention vitesse et angle
void GPWM_DispSettings(S_pwmSettings *pData, int Remote);	// Affichage
void GPWM_ExecPWM(S_pwmSettings *pData);		// Execution PWM et gestion moteur.
void GPWM_ExecPWMSoft(S_pwmSettings *pData);	// Execution PWM software.


S_pwmSettings pData;
S_pwmSettings PWMData;
S_pwmSettings PWMDataToSend;

#endif
