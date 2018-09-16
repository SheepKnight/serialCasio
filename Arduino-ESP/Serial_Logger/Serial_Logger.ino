#include <HardwareSerial.h>
//In HardwareSerial.cpp you'll have to change the RX & TX pins of serial 2, in order to plug the Casio.
#include <TFT_eSPI.h>
//TFT_eSPI is a library by Bodmer (https://github.com/Bodmer/TFT_eSPI) used for debugging
#include <WiFi.h>
#include <string.h>

//If you do not have a TFT Screen wired to your ESP, comment this next line.
#define debugScreen;
  
#ifdef debugScreen

TFT_eSPI tft = TFT_eSPI();

#endif /* debugScreen */

HardwareSerial espSerial(2); // RX, TX


char ssidAP[50];
char pwdAP[50];

#define ATMaxCMD 50

int nbrOfAT = 0;
void (*ATCommands[ATMaxCMD])(String parameter);
String ATNames[ATMaxCMD];

void registerAT(const char * ATname, void (* command)(String parameter)){
  ATCommands[nbrOfAT] = command;
  ATNames[nbrOfAT] = String(ATname);
  nbrOfAT++;
}

void ATWifiSetupAP(String useless){
 
  const char * pwd = pwdAP;
  const char * ssid = ssidAP;
if(sizeof(pwd) != 0){
    
    if(sizeof(ssid) == 0){
      Serial.println(2);
      if(WiFi.softAP(ssidAP)){
        sendToCasio("-2");
      }
    }else{
      if(WiFi.softAP(ssidAP, pwdAP)){
        sendToCasio("-3");
      }
    }
    sendToCasio("OK");
  }else{
    sendToCasio("-1");
  }
}

void ATSetAPPwd(String pwd){
  pwd.toCharArray(pwdAP, pwd.length()+1);
  sendToCasio("OK"); 
}

void ATSetAPSSID(String ssid){
  ssid.toCharArray(ssidAP, ssid.length()+1);
  sendToCasio("OK");
}

void ATSetStream(String streamMode){
  
}

void setup() {

  #ifdef debugScreen
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  #endif /* debugScreen */
  
  // On ouvre la communication serie software pour l'ESP8266
  espSerial.begin(9600);
  // On ouvre une communication serie hardware pour les messages destines au moniteur série
  Serial.begin(9600);

  delay(500);

  Serial.println("");
  Serial.println("--------------------------------");

  registerAT("SETAPSSID", ATSetAPSSID);
  registerAT("SETAPPWD", ATSetAPPwd);
  registerAT("SETUPAP", ATWifiSetupAP);

  
  Serial.print("There are ");
  Serial.print(nbrOfAT);
  Serial.println(" AT commands.");
  Serial.println("Pret.");
}

void parseAT(String toParse){
  int longueur = toParse.length();
  Serial.println("We should parse : \"" + toParse + "\"");
  if(longueur == 2){
    sendToCasio("OK");
  }else{
    int posToNext = toParse.indexOf(":");
    String cmdName;
    String param;
    if(posToNext != 0){
      
      cmdName = toParse.substring(2+1, posToNext);
      param = toParse.substring(posToNext+1);
    }else{
      //NO PARAM
      cmdName = toParse.substring(2);
    }
    Serial.println("Command name : \"" + cmdName + "\"");
    Serial.println("Parameter : \"" + param + "\"");
    bool hasFound = false;
    for(int i = 0; i < nbrOfAT; i++){
      if(cmdName == ATNames[i]){
        ATCommands[i](param);
        hasFound = true;
      }
    }
    if(hasFound == false){
      Serial.println("COMMAND NOT FOUND.");
      sendToCasio("-1");
    }
  }
  
}

String inputAT = "";
void loop() { 
  
  if (espSerial.available()) {
    int input = espSerial.read();
    Serial.println(input);
    if(input != 13 /*New line*/&& input != 59 /*;*/ && input != 0 /*\0*/){
      inputAT = inputAT + char(input);
    }else{
      if(inputAT.startsWith("AT")){
        parseAT(inputAT);
      }else{
        transmitSerial1(input);
      }
      inputAT = "";
    }
    transmitSerial1(input);
  }
  
  if (Serial.available()) {
    int input = Serial.read();
    transmitSerial2(input);
  }
}

void transmitSerial1(int input){
  Serial.print(char(input));
  #ifdef debugScreen
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(2);
  tft.print(char(input));
  #endif /* debugScreen */
}

void transmitSerial2(int input){
  espSerial.print(char(input));
  #ifdef debugScreen
  tft.setTextColor(TFT_RED);
  tft.setTextFont(2);
  tft.print(char(input));
  #endif /* debugScreen */+
}

void sendToCasio(String entree){
  espSerial.println(entree);
  #ifdef debugScreen
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.println(entree);
  #endif /* debugScreen */+
  
  
}

