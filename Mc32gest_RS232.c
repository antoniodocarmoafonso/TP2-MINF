// Mc32Gest_RS232.C
// Canevas manipulatio TP2 RS232 SLO2 2017-18
// Fonctions d'�mission et de r�ception des message
// CHR 20.12.2016 ajout traitement int error
// CHR 22.12.2016 evolution des marquers observation int Usart
// SCA 03.01.2018 nettoy� r�ponse interrupt pour ne laisser que les 3 ifs

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
//D�finition de la valeur du byte de start
#define START_BYTE 0xAA 
//D�finition de la valeur max de cycles
#define CYCLES_MAX 9

// Structure d�crivant le message
typedef struct {
    uint8_t Start;
    int8_t  Speed;
    int8_t  Angle;
    uint8_t MsbCrc;
    uint8_t LsbCrc;
} StruMess;


// Struct pour �mission des messages
StruMess TxMess;
// Struct pour r�ception des messages
StruMess RxMess;

// Declaration des FIFO pour r�ception et �mission
#define FIFO_RX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages
#define FIFO_TX_SIZE ( (4*MESS_SIZE) + 1)  // 4 messages

int8_t fifoRX[FIFO_RX_SIZE];
// Declaration du descripteur du FIFO de r�ception
S_fifo descrFifoRX;


int8_t fifoTX[FIFO_TX_SIZE];
// Declaration du descripteur du FIFO d'�mission
S_fifo descrFifoTX;


// Initialisation de la communication s�rielle
void InitFifoComm(void)
{    
    // Initialisation du fifo de r�ception
    InitFifo ( &descrFifoRX, FIFO_RX_SIZE, fifoRX, 0 );
    // Initialisation du fifo d'�mission
    InitFifo ( &descrFifoTX, FIFO_TX_SIZE, fifoTX, 0 );
    
    // Init RTS 
    RS232_RTS = 1;   // interdit �mission par l'autre
   
} // InitComm

/**
* Nom de la fonction : GetMessage
* @author: Antonio Do Carmo / Diogo Ferreira
* @date: 01.02.2024
*
* @brief: Re�oit un message de l'interface de communication et le valide.
* 
* Cette fonction re�oit un message du buffer FIFO de l'interface de communication, valide l'int�grit� du message
* en utilisant une somme de contr�le CRC16, et met � jour la structure de donn�es contenant les r�glages de vitesse et d'angle en cons�quence. 
* Elle g�re �galement les v�rifications de redondance cyclique et le contr�le de flux des messages re�us.
* 
* @param pData Pointeur vers la structure contenant les r�glages PWM.
* @return commStatus Statut de la communication (0 si le cycle de communication est termin�, 1 sinon).
*/


int GetMessage(S_pwmSettings *pData) {
    int commStatus = 0; // Statut de la communication initialis� � 0
    static uint8_t cycles = 0; // Variable statique pour suivre les cycles de communication
    uint8_t bufferDataSize = 0; // Taille des donn�es du buffer
    uint16_t valCrc16Rx = 0xFFFF; // Variable pour stocker la valeur CRC16 calcul�e � partir du message, initialis�e � 0xFFFF
    uint16_t messageValCrc16 = 0; // Variable pour stocker la valeur CRC16 re�ue
    uint8_t message[5]; // Tableau pour stocker le message re�u
    uint8_t readFifoCounter = 0; // Compteur pour la lecture du buffer FIFO
    
    // Obtenir la taille des donn�es disponibles dans le buffer FIFO
    bufferDataSize = GetReadSize(&descrFifoRX);
    
    // V�rifier si la taille du buffer est sup�rieure ou �gale � la taille du message
    if(bufferDataSize >= MESS_SIZE) {
        // R�cup�rer le premier octet du buffer FIFO
        GetCharFromFifo(&descrFifoRX, &message[0]);
        
        // V�rifier si le premier octet est le byte de d�part du message
        if(message[0] == START_BYTE) {
            // Lire les octets restants du message dans le buffer FIFO
            for(readFifoCounter = 1; readFifoCounter < MESS_SIZE; readFifoCounter ++) {
                GetCharFromFifo(&descrFifoRX, &message[readFifoCounter]);
            }
            
            // Mettre � jour la valeur CRC16 avec les octets du message re�u
            valCrc16Rx = updateCRC16(valCrc16Rx, START_BYTE);
            valCrc16Rx = updateCRC16(valCrc16Rx, message[1]);
            valCrc16Rx = updateCRC16(valCrc16Rx, message[2]); 
            
            // Combiner les octets CRC16 du message
            messageValCrc16 = message[3];
            messageValCrc16 = messageValCrc16 << 8;
            messageValCrc16 = messageValCrc16 | message[4];
            
            // V�rifier si le CRC16 calcul� correspond au CRC16 re�u
            if(messageValCrc16 == valCrc16Rx) {
                // Mettre � jour les r�glages de vitesse et d'angle � partir du message
                pData->SpeedSetting = message[1];
                pData->AngleSetting = message[2];
                
                // Calculer la vitesse absolue
                if(pData->SpeedSetting < 0) {
                    pData->absSpeed = (pData->SpeedSetting * -1);
                } else {
                    pData->absSpeed = pData->SpeedSetting;
                }
                
                // R�initialiser le compteur de cycle de communication
                cycles = 0;
            } else if (cycles < CYCLES_MAX) {
                // Incr�menter le compteur de cycle et basculer la LED si la limite de cycles n'est pas atteinte
                cycles ++;
                BSP_LEDToggle(BSP_LED_6);
            }
        }
    } else if (cycles < CYCLES_MAX) {
        // Incr�menter le compteur de cycle si la taille du buffer est insuffisante et si la limite de cycles n'est pas atteinte
        cycles ++;
    }
    
    // V�rifier si le nombre maximum de cycles est atteint
    if(cycles == CYCLES_MAX) {
        commStatus = 0; // D�finir le statut de communication � 0
    } else {
        commStatus = 1; // D�finir le statut de communication � 1
    }
    
    // Gestion du contr�le de flux pour la r�ception
    if(GetWriteSpace(&descrFifoRX) >= (2*MESS_SIZE)) {
        // Autoriser la transmission en effa�ant le signal RTS
        RS232_RTS = 0;
    }
    
    return commStatus; // Retourner le statut de communication
} // Fin de la fonction GetMessage




/**
* Fonction : SendMessage
* @author: Antonio Do Carmo / Diogo Ferreira
* @date: 01.02.2024
*
* @brief: Envoie un message � l'interface de communication.
* 
* Cette fonction pr�pare et envoie un message � l'interface de communication en respectant les contraintes de taille
* du buffer FIFO d'�mission et en utilisant un CRC16 pour v�rifier l'int�grit� du message.
* 
* @param pData Pointeur vers la structure contenant les r�glages PWM � envoyer.
* @return Aucune valeur de retour.
*/


void SendMessage(S_pwmSettings *pData) {
    int8_t freeSize; // Taille libre dans le buffer FIFO d'�mission
    uint16_t valCrc16Tx = 0xFFFF; // Variable pour le calcul du CRC16

    // Obtenir la taille libre dans le buffer FIFO d'�mission
    freeSize = GetWriteSpace(&descrFifoTX);
    
    // V�rifier si la taille libre dans le buffer est suffisante pour �crire un message complet
    if (freeSize >= MESS_SIZE) {
        // Compose le message
        valCrc16Tx = updateCRC16(valCrc16Tx, START_BYTE); // Met � jour le CRC16 avec le byte de d�part
        valCrc16Tx = updateCRC16(valCrc16Tx, pData->SpeedSetting); // Met � jour le CRC16 avec la vitesse
        valCrc16Tx = updateCRC16(valCrc16Tx, pData->AngleSetting); // Met � jour le CRC16 avec l'angle
        
        // Place les �l�ments du message dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, 0xAA); // Place le byte de d�part dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, pData->SpeedSetting); // Place la vitesse dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, pData->AngleSetting); // Place l'angle dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, valCrc16Tx>>8); // Place le CRC16 (partie haute) dans le buffer FIFO
        PutCharInFifo(&descrFifoTX, valCrc16Tx&0xFF); // Place le CRC16 (partie basse) dans le buffer FIFO
    }
    
    // Si CTS = 0 et qu'il y a suffisamment d'espace libre dans le buffer FIFO d'�mission
    if ((RS232_CTS == 0) && (freeSize >= MESS_SIZE)) {
        // Autorise l'interruption pour l'�mission
        PLIB_INT_SourceEnable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);                
    }
}


// Interruption USART1
// !!!!!!!!
// Attention ne pas oublier de supprimer la r�ponse g�n�r�e dans system_interrupt
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

    // Marque d�but interruption avec Led3
    LED3_W = 1;
    
    // Is this an Error interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_ERROR) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_ERROR) ) {
        /* Clear pending interrupt */
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_ERROR);
        // Traitement de l'erreur � la r�ception.
        while (PLIB_USART_ReceiverDataIsAvailable(USART_ID_1))
        {
            PLIB_USART_ReceiverByteReceive(USART_ID_1);
        }
    }
   

    // Is this an RX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_RECEIVE) ) {

        // Oui Test si erreur parit� ou overrun
        UsartStatus = PLIB_USART_ErrorsGet(USART_ID_1);

        if ( (UsartStatus & (USART_ERROR_PARITY |
                             USART_ERROR_FRAMING | USART_ERROR_RECEIVER_OVERRUN)) == 0) {

            // Traitement RX � faire ICI
            // Lecture des caract�res depuis le buffer HW -> fifo SW
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

        
        // Traitement controle de flux reception � faire ICI
        // Gerer sortie RS232_RTS en fonction de place dispo dans fifo reception
        // ...
        freeSize = GetWriteSpace(&descrFifoRX);
        if (freeSize <= 6){
            //controle de flux : demande stop �mission
            RS232_RTS = 1 ;
        }        
    } // end if RX

    
    // Is this an TX interrupt ?
    if ( PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) &&
                 PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT) ) 
    {

        // Traitement TX � faire ICI
        // Envoi des caract�res depuis le fifo SW -> buffer HW

        // Avant d'�mettre, on v�rifie 3 conditions :
        //  Si CTS = 0 autorisation d'�mettre (entr�e RS232_CTS)
        //  S'il y a un carat�res � �mettre dans le fifo
        //  S'il y a de la place dans le buffer d'�mission (PLIB_USART_TransmitterBufferIsFull)
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
             // (Seulement apr�s TX)
                 PLIB_INT_SourceFlagClear(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);
                 TXSize = GetReadSize (&descrFifoTX);
             if (TXSize == 0 ) {
             // pour �viter une interruption inutile
                 PLIB_INT_SourceDisable(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);
             }
         } else 
         {
         // disable TX interrupt
         PLIB_INT_SourceDisable(INT_ID_0,INT_SOURCE_USART_1_TRANSMIT);
         }
    }
        LED5_W = !LED5_R; // Toggle Led5
		
//        // disable TX interrupt (pour �viter une interrupt. inutile si plus rien � transmettre)
//        PLIB_INT_SourceDisable(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);
//        
//        // Clear the TX interrupt Flag (Seulement apres TX) 
//        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_USART_1_TRANSMIT);

    // Marque fin interruption avec Led3
    LED3_W = 0;
 }

