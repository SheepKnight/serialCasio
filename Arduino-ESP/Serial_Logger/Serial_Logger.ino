
#include <HardwareSerial.h>
//In HardwareSerial.cpp you'll have to change the RX & TX pins of serial 2, in order to plug the Casio.
#include <TFT_eSPI.h>
//TFT_eSPI is a library by Bodmer (https://github.com/Bodmer/TFT_eSPI) used for debugging
#include <WiFi.h>
#include <string.h>
#include <HTTPClient.h>

//If you do not have a TFT Screen wired to your ESP, comment this next line.
#define debugScreen;

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

bool BLEUartStarted = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


#ifdef debugScreen

TFT_eSPI tft = TFT_eSPI();

#endif /* debugScreen */



HardwareSerial espSerial(2); // RX, TX, configured in the ESP32 HardwareSerial.cpp library.


void sendToCasio(String entree) {
  espSerial.println(entree);
#ifdef debugScreen
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.println(entree);
#endif /* debugScreen */
}
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if(rxValue.length() > 0){
        String bufferInput;
        for (int i = 0; i < rxValue.length(); i++){
          bufferInput = bufferInput + char(rxValue[i]);
          Serial.print(rxValue[i]);
        }        
        sendToCasio(bufferInput);
      }
    }
};
BLEAdvertisedDevice scannedBLE[32];
int scanIndex = 0;
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());
    scannedBLE[scanIndex]= advertisedDevice;
    scanIndex++;
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(BLEUUID(SERVICE_UUID))) {

      // 
      Serial.print("Found our device!  address: "); 
      //advertisedDevice.getScan()->stop();

      //pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      //doConnect = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


char ssidAP[50];
char pwdAP[50];
char ssidWifi[50];
char pwdWifi[50];

int dumpStream = 0;

#define ATMaxCMD 50

int nbrOfAT = 0;
void (*ATCommands[ATMaxCMD])(String parameter);
String ATNames[ATMaxCMD];

void registerAT(const char * ATname, void (* command)(String parameter)) {
  ATCommands[nbrOfAT] = command;
  ATNames[nbrOfAT] = String(ATname);
  nbrOfAT++;
}

void ATBLEUARTBegin(String UARTName){
  // Create the BLE Device
  //char * newName;
  //UARTName.toCharArray(newName, UARTName.length() + 1);
  //const char * finalName = newName;
  BLEDevice::init(UARTName.c_str());

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  
  //pServer->getAdvertising()->setAppearance(0b0000001111000101);
  //Serial.println(pServer->getAdvertising()->getServiceUUID());
  //pServer->getAdvertising()->addServiceUUID(BLEUUID(SERVICE_UUID));
  pServer->getAdvertising()->start();
  BLEUartStarted = true;
  sendToCasio("OK");
  Serial.println("Sucessfully started BLE Uart");
}

void ATGetStatus(String parameter){
  if(parameter== "BLE"){
    sendToCasio(String('{'+BLEUartStarted+','+deviceConnected+'}'));
  }else if(parameter == "WIFI"){
    sendToCasio(String('{'+WiFi.status()+','+WiFi.localIP().toString()+'}'));
  }else{
    sendToCasio(String('{'+BLEUartStarted+','+deviceConnected+','+WiFi.status()+','+WiFi.localIP().toString()+'}'));
  }
}

void ATWifiSSID(String ssid) {
  ssid.toCharArray(ssidWifi, ssid.length() + 1);
  sendToCasio("OK");
}

void ATWifiPWD(String pass) {
  pass.toCharArray(pwdWifi, pass.length() + 1);
  sendToCasio("OK");
}

void ATConnectToWifi(String useless) {
  const char* ssid = ssidWifi;
  const char* password = pwdWifi;
  WiFi.begin(ssid, password);
  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED && tentatives < 20) {
    tentatives++;
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    sendToCasio("-1");
  } else {
    sendToCasio("OK");
  }
}

void ATGETIP(String useless) {
  if (WiFi.status() != WL_CONNECTED) {
    sendToCasio("-0");
  } else {
    sendToCasio(String("{IP:\"") + WiFi.localIP().toString() + String("\"}"));
  }
}

void ATHTTP(String url) {
  if (WiFi.status() != WL_CONNECTED) {
    sendToCasio("-0");
  } else {
    HTTPClient http;

    http.begin(url); //Specify the URL
    int httpCode = http.GET();                                        //Make the request

    if (httpCode > 0) { //Check for the returning code

      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
      sendToCasio(payload);
    }
    else {
      Serial.println("Error on HTTP request");
      sendToCasio("-" + httpCode);
    }
    http.end(); //Free the resources
  }
}

void ATGetWifi(String useless) {
  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
    sendToCasio("-1");
  } else {
    String message;
    message = '[' + n + "]:";
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      message = message + "{" + WiFi.SSID(i) + "," + WiFi.RSSI(i) + "," + (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) + "}";
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
      if (i + 1 < n) {
        message = message + ",";
      }
    }
    message = message + "}";
    sendToCasio(message);
  }
  Serial.println("");
}

void ATWifiSetupAP(String useless) {

  const char * pwd = pwdAP;
  const char * ssid = ssidAP;
  if (sizeof(pwd) != 0) {

    if (sizeof(ssid) == 0) {
      Serial.println(2);
      if (WiFi.softAP(ssidAP)) {
        sendToCasio("-2");
      }
    } else {
      if (WiFi.softAP(ssidAP, pwdAP)) {
        sendToCasio("-3");
      }
    }
    sendToCasio("OK");
  } else {
    sendToCasio("-1");
  }
}

void ATSetAPPwd(String pwd) {
  pwd.toCharArray(pwdAP, pwd.length() + 1);
  sendToCasio("OK");
}

void ATSetAPSSID(String ssid) {
  ssid.toCharArray(ssidAP, ssid.length() + 1);
  sendToCasio("OK");
}

void ATBLEScan(String filter){
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(32);
  
}

void ATSetStream(String streamMode) {
  if(streamMode == "BLEUartServer"){
    dumpStream = 1;
    sendToCasio("OK");
    Serial.println("Stream is now BLE.");
  }else{
    dumpStream = 0;
    sendToCasio("-1");
  }
  
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

  int freq = 2000;
  int channel = 0;
  int resolution = 8;

  Serial.begin(115200);
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(12, channel);

  delay(500);
  
  Serial.println("");
  Serial.println("--------------------------------");

  registerAT("SETAPSSID", ATSetAPSSID);
  registerAT("SETAPPWD", ATSetAPPwd);
  registerAT("SETUPAP", ATWifiSetupAP);
  registerAT("GETWIFI", ATGetWifi);
  registerAT("WIFISSID", ATWifiSSID);
  registerAT("WIFIPWD", ATWifiPWD);
  registerAT("WIFIBEGIN", ATConnectToWifi);
  registerAT("HTTP", ATHTTP);
  registerAT("GETIP", ATGETIP);
  registerAT("SETSTREAM", ATSetStream);
  registerAT("BLEUARTBEGIN", ATBLEUARTBegin);
  registerAT("STATUS", ATGetStatus);
  registerAT("BLESCAN", ATBLEScan);
  
  Serial.print("There are ");
  Serial.print(nbrOfAT);
  Serial.println(" AT commands.");
  Serial.println("Pret.");
}

void parseAT(String toParse) {
  int longueur = toParse.length();
  Serial.println("We should parse : \"" + toParse + "\"");
  if (longueur == 2) {
    sendToCasio("OK");
  } else {
    int posToNext = toParse.indexOf(":");
    String cmdName;
    String param;
    if (posToNext != 0) {

      cmdName = toParse.substring(2 + 1, posToNext);
      param = toParse.substring(posToNext + 1);
    } else {
      //NO PARAM
      cmdName = toParse.substring(2);
    }
    Serial.println("Command name : \"" + cmdName + "\"");
    Serial.println("Parameter : \"" + param + "\"");
    bool hasFound = false;
    for (int i = 0; i < nbrOfAT; i++) {
      if (cmdName == ATNames[i]) {
        ATCommands[i](param);
        hasFound = true;
      }
    }
    if (hasFound == false) {
      Serial.println("COMMAND NOT FOUND.");
      sendToCasio("-0");
    }
  }

}

String inputAT = "";
void loop() {

  if (espSerial.available()) {
    int input = espSerial.read();
    Serial.println(input);
    if(dumpStream == 1){
      if(pTxCharacteristic != NULL ){
        uint8_t toTransmit = (uint8_t)input;
        Serial.println("BLE IS NICE : "+ toTransmit);
        pTxCharacteristic->setValue(&toTransmit, 1);
        pTxCharacteristic->notify();
      }
    }
    if (input != 13 /*New line*/ && input != 59 /*;*/ && input != 0 /*\0*/) {
      inputAT = inputAT + char(input);
    } else {
      if ((inputAT.startsWith("AT") && inputAT.length() == 2) || inputAT.startsWith("AT+")) {
        parseAT(inputAT);
        inputAT = "";
      } else {
        transmitSerial1(input);
      }
      Serial.println("Cleared buffer.");
      inputAT = "";
    }
    
    transmitSerial1(input);
  }

  if (Serial.available()) {
    int input = Serial.read();
    transmitSerial2(input);
  }
  if(BLEUartStarted == true){
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
  }
}

void transmitSerial1(int input) {
  Serial.print(char(input));
#ifdef debugScreen
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(2);
  tft.print(char(input));
#endif /* debugScreen */
}

void transmitSerial2(int input) {
  espSerial.print(char(input));
#ifdef debugScreen
  tft.setTextColor(TFT_RED);
  tft.setTextFont(2);
  tft.print(char(input));
#endif /* debugScreen */+
}


