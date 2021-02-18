#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "file.h"
#include <LoRa.h>

#include <alloca.h>
#include <string>
#include <cstring>
using namespace std;

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <TinyGPS++.h>
#include <SoftwareSerial.h>
static const int RXPin = 21, TXPin = 22;
static const uint32_t GPSBaud = 9600;
// The TinyGPS++ object
TinyGPSPlus gps;
// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);


// On définit deux bus SPI différents : le premier pour la carte SD, le second pour le module LoRa

SPIClass spiSD(HSPI);
#define SD_SCK 14
#define SD_MISO 2
#define SD_MOSI 15
#define SD_CS 13
#define SD_Speed 27000000

SPIClass spiLoRa(VSPI);
#define LoRa_SCK 5
#define LoRa_MISO 19
#define LoRa_MOSI 27
#define LoRa_CS 18
#define RST 14
#define DI0 26
#define BAND 450E6

 char Loradata[50];
 float longi, lati, alt;


/*  /!\ Pour utiliser le port micro-SD de la carte TTGO LoRa32 OLED v.2, définir les pins du bus SPI tels que: 
 * Le pin n°13 correspond au chipSelect de la carte SD => SD.begin(CS,...).
 * Il correspond en même temps au Slave Select du bus SPI => SPI.begin(SCK,MISO,MOSI,SS)
 *             
 * On a donc SS = CS = 13
 */

void setup(){
    
    Serial.begin(115200);
    ss.begin(9600);
    
    spiSD.begin(SD_SCK,SD_MISO,SD_MOSI,SD_CS);
    
    if(!SD.begin(SD_CS,spiSD,SD_Speed)){
        Serial.println("Card Mount Failed");
        return;
    }

    spiLoRa.begin(LoRa_SCK,LoRa_MISO,LoRa_MOSI,LoRa_CS);
    LoRa.setPins(LoRa_CS,RST,DI0);

    if(!LoRa.begin(BAND)){
      Serial.println("Starting LoRa failed!");
      while(1);
    }
    
    LoRa.receive();
         
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    char header[100]="     lon      ||     lat     ||     alt \n";
    writeFile(SD,"/donnees.txt",header);
    writeFile(SD,"/donnees2.txt","Header");

    
}
void loop(){
   // This sketch displays information every time a new sentence is correctly encoded.
 while (ss.available() > 0){
   gps.encode(ss.read());
   if (gps.location.isUpdated()){
     Serial.print("Latitude= "); 
     Serial.print(gps.location.lat(), 6);
     Serial.print(" Longitude= "); 
     Serial.println(gps.location.lng(), 6);
 Serial.print("altitude= ");
 Serial.print(gps.altitude.meters(), 6);

     
   }
 }
  int Size = LoRa.parsePacket();
  if(Size){
    Serial.println("Reçu 5/5");
    String LoRaData = LoRa.readString();
    Serial.println(LoRaData);

    float longi = gps.location.lng() ;   
                                 
    float lati = gps.location.lat(); 

    float alt = gps.altitude.meters(); 

    Serial.println(longi);
    Serial.println(lati);
    Serial.println(alt);


    sprintf(Loradata," %f \t %f \t %f \n ", longi, lati, alt);
     
    appendFile(SD,"/donnees.txt",Loradata);
       
    LoRaData = LoRaData + "\n";
    
    appendFile(SD,"/donnees2.txt",LoRaData);
    
  }
 

}
