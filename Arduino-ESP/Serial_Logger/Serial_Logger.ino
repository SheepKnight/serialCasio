#include <HardwareSerial.h>
#include <TFT_eSPI.h>
#define debugScreen;
  
//#ifdef debugScreen

TFT_eSPI tft = TFT_eSPI();

//#endif /* debugScreen */

String currentBuffer[] = "";
int bufferPos = 0;
HardwareSerial espSerial(2); // RX, TX

void AT(){

  
  
}

void setup() {

  //#ifdef debugScreen
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  //#endif /* debugScreen */
  
  // On ouvre la communication serie software pour l'ESP8266
  espSerial.begin(9600);
  // On ouvre une communication serie hardware pour les messages destines au moniteur série
  Serial.begin(9600);

  delay(1000);
  Serial.println("Pret.");
}
void loop() { 
  if (espSerial.available()) {
    int input = espSerial.read();
    transmitSerial1(input);
  }
  
  if (Serial.available()) {
    int input = Serial.read();
    transmitSerial2(input);
  }
}

void transmitSerial1(int input){
  Serial.print(char(input));
  //#ifdef debugScreen
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(2);
  tft.print(char(input));
  //#endif /* debugScreen */
}

void transmitSerial2(int input){
  espSerial.print(char(input));
  //#ifdef debugScreen
  tft.setTextColor(TFT_RED);
  tft.setTextFont(2);
  tft.print(char(input));
  //#endif /* debugScreen */+
}

void sendToCasio(String entree){
  espSerial.println(entree);
  #ifdef debugScreen
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.print(entree);
  #endif /* debugScreen */+
  
  
}

