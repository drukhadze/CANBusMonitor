/*
  CAN_Monitor_V2.ino - CAN bus monitoring sketch. Allows receive and send messages to and from the CAN bus. Tested with BMW E39 CAN bus
  Written by Dimitri Rukhadze in September 2014
*/


//#include <avr/pgmspace.h>
#include <SPI.h>
#include <MCP2515.h>

#define LED_2          8
#define LED_3          7

byte CS_CAN_PIN = 10;

#define CAN_HEADER    "CAN"
#define CAN_HEADER_ARBID    "0"

#define highWord(w) ((w) >> 16)
#define lowWord(w) ((w) & 0xffff)
#define makeLong(hi, low) ((hi) << 16 | (low))

///////////////////////////////////

MCP2515 CANOBJ;
byte receiveMsgBuffer[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
byte sendMsgBuffer[] = {'C','A','N',8,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define SERIAL_CAN_MSG_LEN  18
unsigned long msgSendTimestamp = 0;
unsigned long currentTimestamp = 0;
word lowWord = 0;
word hiWord = 0;
byte msgSendPeriod = 0;
byte CRCbyte=0;

word    tmpWord=0;
unsigned long tmpArbid=0;

CANMSG *canMsg;
CANMSG *canMsgForSend;

void setup()
{
  //set up serial port for debugging with data to PC screen
  canMsg = (CANMSG*)malloc(sizeof(CANMSG));
  canMsgForSend = (CANMSG*)malloc(sizeof(CANMSG));
  memset(canMsg, 0, sizeof(CANMSG));
  memset(canMsgForSend, 0, sizeof(CANMSG));
  
  Serial.begin(512000);
  // init all CS PINS
  pinMode(CS_CAN_PIN, OUTPUT);

  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  //SPI.setClockDivider(SPI_CLOCK_DIV4);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  digitalWrite(CS_CAN_PIN,HIGH);
  if (CANOBJ.initCAN(CS_CAN_PIN, CAN_BAUD_500K))
  {
    //Serial.println("ok");
  }

  if (CANOBJ.setCANNormalMode(CS_CAN_PIN, LOW))
  {
//    Serial.println(F("normal mode ok"));
  }
  digitalWrite(CS_CAN_PIN,HIGH);  // disable
  
  pinMode(LED_3, OUTPUT);

  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_2,HIGH);
  
}

void loop()
{
    while (CANOBJ.receiveCANMessage(CS_CAN_PIN, canMsg, 2))
    {
      sendToMonitor();
    }
    if (Serial.available()>=SERIAL_CAN_MSG_LEN)
    {
      while (Serial.peek()!='C')
      {
        Serial.read();
      }

      if (Serial.available()>=SERIAL_CAN_MSG_LEN)
      {
        if (Serial.readBytes((char*)receiveMsgBuffer, SERIAL_CAN_MSG_LEN)==18)
        {
          // parse Msg, first check CRC
          CRCbyte = 0;
          for (int i=0; i<SERIAL_CAN_MSG_LEN-1; i++)
            CRCbyte ^= receiveMsgBuffer[i];
          if (CRCbyte==receiveMsgBuffer[SERIAL_CAN_MSG_LEN-1])
          {
            canMsgForSend->adrsValue=makeLong(word(receiveMsgBuffer[7], receiveMsgBuffer[6]), word(receiveMsgBuffer[5], receiveMsgBuffer[4]));
            canMsgForSend->isExtendedAdrs = (receiveMsgBuffer[8] & 0x40) ? true : false;
            canMsgForSend->rtr = (receiveMsgBuffer[8] & 0x80) ? true : false;
            
            if (!canMsgForSend->rtr)
            {
              canMsgForSend->dataLength = receiveMsgBuffer[8] & 0x0F;
  
              for (int i=0; i<canMsgForSend->dataLength; i++ )
              {
                canMsgForSend->data[i] = receiveMsgBuffer[9+i];
              }
            }
            msgSendPeriod = receiveMsgBuffer[3];
            msgSendTimestamp = 0;
          }
        }
      }
    }
    if (msgSendPeriod>2)
    {
      currentTimestamp = millis();      
      if ((currentTimestamp-msgSendTimestamp) >= msgSendPeriod)
      {
        msgSendTimestamp = currentTimestamp;
        CANOBJ.transmitCANMessage(CS_CAN_PIN, *canMsgForSend, 2);
      }
    }

}// End of loop


//********************* Process message ***************
void sendToMonitor()
{
//  ARBID
      tmpWord =lowWord(canMsg->adrsValue);
      sendMsgBuffer[4] = lowByte(tmpWord);
      sendMsgBuffer[5] = highByte(tmpWord);
      tmpWord =highWord(canMsg->adrsValue);
      sendMsgBuffer[6] = lowByte(tmpWord);
      sendMsgBuffer[7] = highByte(tmpWord);
      
      sendMsgBuffer[8] = 0;
      if (canMsg->isExtendedAdrs)
        sendMsgBuffer[8] = sendMsgBuffer[8] | 0x40;
      if (canMsg->rtr)
        sendMsgBuffer[8] = sendMsgBuffer[8] | 0x80;
      else
      {
        sendMsgBuffer[8] = sendMsgBuffer[8] | (canMsg->dataLength>8 ? 8 : canMsg->dataLength);
        for (int i=0; i<((canMsg->dataLength>8) ? 8 : canMsg->dataLength); i++ )
        {
          sendMsgBuffer[9+i] = canMsg->data[i];
        }
      }
      CRCbyte = 0;
      for (int i=0; i<SERIAL_CAN_MSG_LEN-1; i++ )
      {
        CRCbyte ^= sendMsgBuffer[i];
      }
      sendMsgBuffer[SERIAL_CAN_MSG_LEN-1] = CRCbyte;
      
     Serial.write(sendMsgBuffer, SERIAL_CAN_MSG_LEN);
}


