// Including all sensors libraries
#include <Wire.h>
#include <RtcDS1307.h>
RtcDS1307<TwoWire> Rtc(Wire);

#include "DHT.h"  // DHT11 Temperature/Humidity Sensor
#define DHTPIN 23
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#include <BH1750.h> // GY-30 BH1750 Luminosity Sensor
BH1750 lightMeter;

#include <SDS011.h> // SDS011 Dust Particles Sensor
SDS011 my_sds;
HardwareSerial port(1);

#include <ML8511.h> // ML8511 UV Sensor
#define UVPIN 25   
#define ENABLEPIN 17
ML8511 UVSensor(UVPIN,ENABLEPIN);

#include <SFE_BMP180.h> // BMP180 Barometer
SFE_BMP180 pressure;

// Including SD libraries
#include <string.h> 
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include "file.h" // File management library 

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
#define BAND 435E6

int count = 0;

// Definition of the different variables necessary to measurements:
float p10, p25;               // Particles P2.5 and P10
int err;
float ppmF, ppmO;             // Formaldehyde and Ozone particles
uint16_t buf[9];              
float P, T_BME, H_BME;        // Pressure, Temperature and Humidity measured w/ BME280
float T_DHT, H_DHT;           // Temperature and Humidity measured w/ DHT11
float Lux;                  // Luminosity 
float UV, DUV;                // UV Index and Diffey-Weighted UV Irradiance
char values[50];

void setup() 
{
  delay(2000);
  Serial.begin(115200);

  Wire.begin(4, 0); // choix des broches SDA et SCL sur ESP32

  spiLoRa.begin(LoRa_SCK,LoRa_MISO,LoRa_MOSI,LoRa_CS);
  LoRa.setPins(LoRa_CS,RST,DI0);
  
  if(!LoRa.begin(BAND)){
    Serial.println("Starting LoRa failed!");
    while(1);
  }

  // Initialisation µ-SD port
  spiSD.begin(SD_SCK,SD_MISO,SD_MOSI,SD_CS);
  if(!SD.begin(SD_CS,spiSD,SD_Speed)){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return; 
  } 
   
  // Initialisation DHT11
  dht.begin();
  
  // Initialisation capteur GY-30 luminosité
  lightMeter.begin();
  
  // Initialisation port série pour capteur de particules
  my_sds.begin(&port,34,35);

  // Initialisation des ports séries pour les capteurs d'ozone et de formaldhéyde
   Serial2.begin(9600, SERIAL_8N1, 32, 33); 
 
  // Initialisation pin UV en lecture
  light.setVoltsPerStep(3.3, 4095);
  light.enable();

  // Lecture et affichage capteur BME180 (pression)
  Wire.begin(21, 22);
  pressure.begin();

  // Creation of the data file:
  char header[200] = "T_DHT (°C) || P_180 (mbar) || H_DHT (%) || L (lx) || UV (mW/cm²) || P2.5 (µg/cm3) || P10 (µg/cm3) || CH2O (ppm) || O3 (ppm) \n";
  writeFile(SD,"/data.txt",header);

}

void loop(){  
  char status;
  double T,P;
  
  status = pressure.startTemperature();
  delay(status);
  
  status = pressure.getTemperature(T);
  status = pressure.startPressure(3);
  
  delay(status);
  
  status = pressure.getPressure(P,T);

  // Temperature/Humidity reading
  H_DHT = dht.readHumidity();
  T_DHT = dht.readTemperature();
  
  // Luminosity reading
  Wire.begin(4,0);
  Lux = lightMeter.readLightLevel();

  // UV Reading
  UV = light.getUV();
  DUV = light.estimateDUVindex(UV);

  // Lecture et affichage taux de particules
  err = my_sds.read(&p25, &p10);
   
  // Lecture et affichage taux CH2O et O3
  Serial2.begin(9600, SERIAL_8N1, 32, 33); //Initializing formaldehyde sensor
  
  float PPMF;
  while (!Serial2.available());
  delay(50);
  if(Serial2.available()){
    for(int i = 0 ; i <9 ; i++){
      buf[i] = Serial2.read();
    }
    ppmF = (buf[4]*256 + buf[5])/1000.00;
    PPMF = ppmF;
  } 

  Serial2.begin(9600, SERIAL_8N1, 36, 39); // Initializing ozone sensor

  float PPMO;
  while (!Serial2.available());
  delay(50);
  if (Serial2.available()){
    for (int j = 0 ; j < 9 ; j++){
      buf[j] = Serial2.read();
    }
    ppmO = (buf[4]*256 + buf[5])/1000.00;
    PPMO = ppmO;
  } 
  Serial.println("Temperature DHT11: " + String(T_DHT));
  Serial.println("Humidity DHT11: " + String(H_DHT));
  Serial.println("Temperature BME180: " + String(T));
  Serial.println("Pressure BME180: " + String(P));
  Serial.println("Luminosity BH1750: " + String(Lux));
  Serial.println("UV Flux: " + String(UV));
  Serial.println("UV Index: " + String(DUV));
  Serial.println("Formaldehyde Concentration: " + String(PPMF));
  Serial.println("Ozone Concentration: " + String(PPMO));
  Serial.println("2.5 Particles Count: " + String(p25));
  Serial.println("10 Particles Count: " + String(p10));

  sprintf(values,"%.2f \t %.2f \t %.0f \t %.2f \t %.2f \t %.2f \t %.2f \t %.2f \t %.2f \n",T_DHT,P,H_DHT,Lux,UV,p25,p10,PPMF,PPMO);
  appendFile(SD,"/data.txt",values);

  Serial.print("Sending Packet...");
  Serial.println(count);
  LoRa.beginPacket();
  LoRa.print(values);
  LoRa.endPacket();

  delay(5000);
  readFile(SD,"/data.txt");
  
}
