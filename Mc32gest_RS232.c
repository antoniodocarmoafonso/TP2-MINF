// Mc32Gest_RS232.C
// Canevas manipulatio TP2 RS232 SLO2 2017-18
// Fonctions d'émission et de réception des message
// CHR 20.12.2016 ajout traitement int error
// CHR 22.12.2016 evolution des marquers observation int Usart
// SCA 03.01.2018 nettoyé réponse interrupt pour ne laisser que les 3 ifs

#include <xc.h>
#include <sys/attribs.h>
#include "system_definitions.h"
// Ajout CHR
#include <GenericTypeDefs.h>
#include "app.h"
#include "GesFifoTh32.h"
#include "Mc32gest_RS232.h"
#include "gestPWM.h"
#include "Mc32CalCrc16.h"


typedef union {
        uint16_t val;
        struct {uint8_t lsb;
                uint8_t msb;} shl;
} U_manip16;


// Definition pour les messages
#define MESS_SIZE  5
// avec int8_t besoin -86 au lieu de 0xAA
#define STX_code  (-86)
//Définition de la valeur du byte de start
#define START_BYTE 0xAA 
//Définition de la valeur max de cycles
#define CYCLES_MAX 9

// Structure décrivant le message
typedef struct {
    uint8_t Start;
    int8_t  Speed;
    int8_t  Angle;
    uint8_t MsbCrc;
    uint8_t LsbCrc;
} StruMess;


// Struct pour émission des messages
StruMess TxMess;
// Struct pour réception des messages
StruMess RxMess;

// Declaration des FIFO pour réception et émission
#define FIFO_RX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages
#define FIFO_TX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages

int8_t fifoRX[FIFO_RX_SIZE];
// Declaration du descripteur du FIFO de réception
S_fifo descrFifoRX;


int8_t fifoTX[FIFO_TX_SIZE];
// Declaration du descripteur du FIFO d'émission
S_fifo descrFifoTX;


// Initialisation de la communication sérielle
void InitFifoComm(void)
{    
    // Initialisation du fifo de réception
    InitFifo ( &descrFifoRX, FIFO_RX_SIZE, fifoRX, 0 );
    // Initialisation du fifo d'émission
    InitFifo ( &descrFifoTX, FIFO_TX_SIZE, fifoTX, 0 );
    
    // Init RTS 
    RS232_RTS = 1;   // interdit émission par l'autre
   
} // InitComm

/**
* Nom de la fonction : GetMessage
* @author: Antonio Do Carmo / Diogo Ferreira
* @date: 01.02.2024
*
* @brief: Reçoit un message de l'interface de communication et le valide.
* 
* Cette fonction reçoit un message du buffer FIFO de l'interface de communication, valide l'intégrité du message
* en utilisant une somme de contrôle CRC16, et met à jour la structure de données contenant les réglages de vitesse et d'angle en conséquence. 
* Elle gère également les vérifications de redondance cyclique et le contrôle de flux des messages reçus.
* 
* @param pData Pointeur vers la structure contenant les réglages PWM.
* @return commStatus Statut de la communication (0 si le cycle de communication est terminé, 1 sinon).
*/


int GetMessage(S_pwmSettings *pData) {
    int commStatus = 0; // Statut de la communication initialisé à 0
    static uint8_t cycles = 0; // Variable statique pour suivre les cycles de communication
    uint8_t bufferDataSize = 0; // Taille des données du buffer
    uint16_t valCrc16Rx = 0xFFFF; // Variable pour stocker la valeur CRC16 calculée à partir du message, initialisée à 0xFFFF
    uint16_t messageValCrc16 = 0; // Variable pour stocker la valeur CRC16 reçue
    uint8_t message[5]; // Tableau pour stocker le message reçu
    uint8_t readFifoCounter = 0; // Compteur pour la lecture du buffer FIFO
    
    // Obtenir la taille des données disponibles dans le buffer FIFO
    bufferDataSize = GetReadSize(&descrFifoRX);
    
    // Vérifier si la taille du buffer est supérieure ou égale à la taille du message
    if(bufferDataSize >= MESS_SIZE) {
        // Récupérer le premier octet du buffer FIFO
        GetCharFromFifo(&descrFifoRX, &message[0]);
        
        // Vérifier si le premier octet est le byte de départ du message
        if(message[0] == START_BYTE) {
            // Lire les octets restants du message dans le buffer FIFO
            for(readFifoCounter = 1; readFifoCounter < MESS_SIZE; readFifoCounter ++) {
                GetCharFromFifo(&descrFifoRX, &message[readFifoCounter]);
            }
            
            // Mettre à jour la valeur CRC16 avec les octets du message reçu
            valCrc16Rx = updateCRC16(valCrc16Rx, START_BYTE);
            valCrc16Rx = updateCRC16(valCrc16Rx, message[1]);
            valCrc16Rx = updateCRC16(valCrc16Rx, message[2]); 
            
            // Combiner les octets CRC16 du message
            messageValCrc16 = message[3];
            messageValCrc16 = messageValCrc16 << 8;
            messageValCrc16 = messageValCrc16 | message[4];
            
            // Vérifier si le CRC16 calculé correspond au CRC16 reçu
            if(messageValCrc16 == valCrc16Rx) {
                // Mettre à jour les réglages de vitesse et d'angle à partir du message
                pData->SpeedSetting = message[1];
                pData->AngleSetting = message[2];
                
                // Calculer la vitesse absolue
                if(pData->SpeedSetting < 0) {
                    pData->absSpeed = (pData->SpeedSetting * -1);
                } else {
                    pData->absSpeed = pData->SpeedSetting;
                }
                
                // Réinitialiser le compteur de cycle de communication
                cycles = 0;
            } else if (cycles < CYCLES_MAX) {
                // Incrémenter le compteur de cycle et basculer la LED si la limite de cycles n'est pas atteinte
                cycles ++;
                BSP_LEDToggle(BSP_LED_6);
            }
        }
    } else if (cycles < CYCLES_MAX) {
        // Incrémenter le compteur de cycle si la taille du buffer est insuffisante et si la limite de cycles n'est pas atteinte
        cycles ++;
    }
    
    // Vérifier si le nombre maximum de cycles est atteint
    if(cycles == CYCLES_MAX) {
        commStatus = 0; // Définir le statut de communication à 0
    } else {
        commStatus = 1; // Définir le statut de communication à 1
    }
    
    // Gestion du contrôle de flux pour la réception
    if(GetWriteSpace(&descrFifoRX) >= (2*MESS_SIZE)) {
        // Autoriser la transmission en effaçant le signal RTS
        RS232_RTS = 0;
    }
    
    return commStatus; // Retourner le statut de communication
} // Fin de la fonction GetMessage




/**
* Fonction : SendMessage
* @author: Antonio Do Carmo / Diogo Ferreira
* @date: 01.02.2024
*
* @brief: Envoie un message à l'interface de communication.
* 
* Cette fonction prépare et envoie un message à l'interface de communication en respectant les contraintes de taille
* du buffer FIFO d'émission et en utilisant un CRC16 pour vérifier l'intégrité du message.
* 
* @param pData Pointeur vers la structure contenant les réglages PWM à envoyer.
* @return Aucune valeur de retour.
*/


void SendMessage(S_pwmSettings *pData) {
    int8_t freeSize; // Taille libre dans le buffer FIFO d'émission
    uint16_t valCrc16Tx = 0xFFFF; // Variable pour le calcul du CRC16

    // Obtenir la taille libre dans le buffer FIFO d'émission
    freeSize = GetWriteSpace(&descrFifoTX);
    
    // Vérifier si la taille libre dans le buffer est suffisante pour écrire un message complet
    if (freeSize >= MESS_SIZE) {
        // Compose le message
        valCrc16Tx = updateCRC16(valCrc16Tx, START_BYTE); // Met à jour le CRC16 avec le byte de départ
        valCrc16Tx = updateCRC16(valCrc16Tx, pData->SpeedSetting); // Met à jour le CRC16 avec la vitesse
        valCrc16Tx = updateCRC16(valCrc16Tx, pData->AngleSetting); // Met à jour le CRC16 avec l'angle
        
        // Place les éléments du message dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, 0xAA); // Place le byte de départ dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, pData->SpeedSetting); // Place la vitesse dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, pData->AngleSetting); // Place l'angle dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, valCrc16Tx>>8); // Place le CRC16 (partie haute) dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, valCrc16Tx&0xFF); // Place le CRC16 (partie basse) dans le buffer FIFO
    }
    
    // Si CTS = 0 et qu'il y a suffisamment d'espace libre dans le buffer FIFO d'émission
    if ((RS232_CTS == 0) && (freeSize >= MESS_SIZE)) {
        // Autorise l'interruption pour l'émission
        PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);                
    }
}


// Interruption USART1
// !!!!!!!!
// Attention ne pas oublier de supprimer la réponse générée dans system_interrupt
// !!!!!!!!
 void __ISR(_UART_1_VECTOR, ipl5AUTO) _IntHandlerDrvUsartInstance0(void)
{
    USART_ERROR UsartStatus;    
    
    uint8_t byteUsart = 0;
    uint8_t dataAvaliable = 0;
    
    uint8_t freeSize, TXSize;
    int8_t c;
    int8_t i_cts = 0;
    BOOL TxBuffFull;

    // Marque début interruption avec Led3
    LED3_W = 1;
    
    // Is this an Error interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_ERROR) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_ERROR) ) {
        /* Clear pending interrupt */
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_ERROR);
        // Traitement de l'erreur à la réception.
        while (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            PLIB_USART_ReceiverByteReceive(USART_ID_1);
        }
    }
   

    // Is this an RX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) ) {

        // Oui Test si erreur parité ou overrun
        UsartStatus = PLIB_USART_ErrorsGet(USART_ID_1);

        if ( (UsartStatus & (USART_ERROR_PARITY |
                             USART_ERROR_FRAMING | USART_ERROR_RECEIVER_OVERRUN)) == 0) {

            // Traitement RX à faire ICI
            // Lecture des caractères depuis le buffer HW -> fifo SW
			//  (pour savoir s'il y a une data dans le buffer HW RX : PLIB_USART_ReceiverDataIsAvailable())
			//  (Lecture via fonction PLIB_USART_ReceiverByteReceive())
            // ...
            //PLIB_USART_ReceiverDataIsAvailable()
            
            
            dataAvaliable = PLIB_USART_ReceiverDataIsAvailable(USART_ID_1);
            
            if(dataAvaliable == 1)
            {
                byteUsart = PLIB_USART_ReceiverByteReceive(USART_ID_1);    
                PutCharInFifo(&descrFifoRX, byteUsart);
            }         
            
            LED4_W = !LED4_R; // Toggle Led4
            // buffer is empty, clear interrupt flag
            PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_RECEIVE);
        } else {
            // Suppression des erreurs
            // La lecture des erreurs les efface sauf pour overrun
            if ( (UsartStatus & USART_ERROR_RECEIVER_OVERRUN) == USART_ERROR_RECEIVER_OVERRUN) {
                   PLIB_USART_ReceiverOverrunErrorClear(USART_ID_1);
            }
        }

        
        // Traitement controle de flux reception à faire ICI
        // Gerer sortie RS232_RTS en fonction de place dispo dans fifo reception
        // ...
        freeSize = GetWriteSpace(&descrFifoRX);
        if (freeSize <= 6){
            //controle de flux : demande stop émission
            RS232_RTS = 1 ;
        }        
    } // end if RX

    
    // Is this an TX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) ) 
    {

        // Traitement TX à faire ICI
        // Envoi des caractères depuis le fifo SW -> buffer HW

        // Avant d'émettre, on vérifie 3 conditions :
        //  Si CTS = 0 autorisation d'émettre (entrée RS232_CTS)
        //  S'il y a un caratères à émettre dans le fifo
        //  S'il y a de la place dans le buffer d'émission (PLIB_USART_TransmitterBufferIsFull)
        //   (envoi avec PLIB_USART_TransmitterByteSend())
       
        // ...
         i_cts = RS232_CTS;
         TXSize = GetReadSize (&descrFifoTX);
         TxBuffFull = PLIB_USART_TransmitterBufferIsFull(USART_ID_1);
         if ( (i_cts == 0) && ( TXSize > 0 ) && TxBuffFull == false )
         { 
             do {
                 GetCharFromFifo(&descrFifoTX, &c);
                 PLIB_USART_TransmitterByteSend(USART_ID_1, c);
                 i_cts = RS232_CTS;
                 TXSize = GetReadSize (&descrFifoTX);
                 TxBuffFull = PLIB_USART_TransmitterBufferIsFull(USART_ID_1);
             } while ( (i_cts == 0) && ( TXSize > 0 ) && TxBuffFull == false );
             // Clear the TX interrupt Flag
             // (Seulement aprés TX)
                 PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);
                 TXSize = GetReadSize (&descrFifoTX);
             if (TXSize == 0 ) {
             // pour éviter une interruption inutile
                 PLIB_INT_SourceDisable(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);
             }
         } else 
         {
         // disable TX interrupt
         PLIB_INT_SourceDisable(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);
         }
    }
        LED5_W = !LED5_R; // Toggle Led5
		
//        // disable TX interrupt (pour éviter une interrupt. inutile si plus rien à transmettre)
//        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
//        
//        // Clear the TX interrupt Flag (Seulement apres TX) 
//        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);

    // Marque fin interruption avec Led3
    LED3_W = 0;
 }

