#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <AntaresESP8266HTTP.h >   // Inisiasi library HTTP Antares
#include <ESP8266HTTPClient.h>    // Inisiasi HTTP client esp8266
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


// Set to true to reset eeprom before to write something
#define RESET_EEPROM false


#define ACCESSKEY "antares account"       // Akun Antares LabIot

#define projectName "your project name"
#define deviceNameStatus "your device name status"
#define deviceNameRead "your device name read"
#define token "your token"

#define LED_INTERVAL        2000
#define SENDAMPS_INTERVAL   60000
#define GET_INTERVAL        30000
#define CHECKAMPS_INTERVAL  10000

int addr_ssid = 0;         // ssid index
int addr_password = 30;    // password index
String ssid1 = "your ssid"; // wifi ssid
String pass1 = "your password"; // and password
String ssid2 = "your ssid 2";
String pass2 = "your password 2";
String ssid3 = "your ssid 3";
String pass3 = "your password 3";

String serverName = "your server name"; // server name
String reading;

AntaresESP8266HTTP antares(ACCESSKEY);    // Buat objek antares
ESP8266WiFiMulti wifiMulti;

const int analogIn = A0;
const int relay = 16;
const int led = 2;
const uint32_t connTimeOut = 5000;

int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module and 185 for 5A Module
int RawValue = 0;
int ACSoffset = 2500;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
double Wattage = 0;
bool LedState = false;
unsigned long led_millis = 0;
unsigned long amps_millis = 0;
unsigned long getantares_millis = 0;
unsigned long checkamps_millis = 0;
String RelayStatus, device_token;

void reseteeprom(int start = 0, int end = 512) {
  for (int i = start; i < end; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

//void initEEPROM() {
//  Serial.println("");
//  Serial.print("Write WiFi SSID at address "); Serial.println(addr_ssid);
//  Serial.print("");
//  int ssid_eeprom_index = addr_ssid;
//  for (int i = 0; i < ssid.length(); i++)
//  {
//    EEPROM.write(ssid_eeprom_index, ssid[i]);
//    Serial.print(ssid[i]); Serial.print("");
//    ssid_eeprom_index++;
//  }
//
//  Serial.println("");
//  Serial.print("Write WiFi Password at address "); Serial.println(addr_password);
//  Serial.print("");
//  int pass_eeprom_index = addr_password;
//  for (int j = 0; j < password.length(); j++)
//  {
//    EEPROM.write(pass_eeprom_index, password[j]);
//    Serial.print(password[j]); Serial.print("");
//    pass_eeprom_index++;
//  }
//
//  Serial.println("");
//  if (EEPROM.commit()) {
//    Serial.println("Data successfully committed");
//  } else {
//    Serial.println("ERROR! Data commit failed");
//  }
//}

void checkEEPROM(int ssidLength, int passLength) {
  Serial.println("");
  Serial.println("\t Check EEPROM");
  String ssid_eeprom;
  for (int k = addr_ssid; k < ssidLength; k++)
  {
    ssid_eeprom += char(EEPROM.read(k));
  }
  Serial.print("SSID: ");
  Serial.println(ssid_eeprom);
  ssid2 = ssid_eeprom;

  String password_eeprom;
  for (int l = addr_password; l < (passLength + addr_password); l++)
  {
    password_eeprom += char(EEPROM.read(l));
  }
  Serial.print("PASSWORD: ");
  Serial.println(password_eeprom);
  pass2 = password_eeprom;
}

String httpGETRequest(String serverName) {
  WiFiClient client;
  HTTPClient http;  //Declare an object of class HTTPClient
  http.begin(client, serverName.c_str());  //Specify request destination
  int httpCode = http.GET(); //Send the request
  String payload = "{}";

  if (httpCode > 0) { //Check the returning code
    payload = http.getString();   //Get the request response payload
    //Serial.println(payload);             //Print the response payload
  }

  http.end();   //Close connection
  return payload;
}

void setup() {
  Serial.begin(115200);// Buka komunikasi serial dengan baudrate 115200
  EEPROM.begin(512);

  Serial.println("");
  if (RESET_EEPROM) {
    reseteeprom(0, 512);
    delay(500);
  }
  //  reseteeprom(0, 512);
  //  initEEPROM();
  checkEEPROM(30, 512);
  Serial.println(ESP.getFreeHeap());

  WiFi.mode(WIFI_STA);

  wifiMulti.addAP(ssid1.c_str(), pass1.c_str());
  wifiMulti.addAP(ssid3.c_str(), pass3.c_str());
  //  WiFi.begin(ssid, password);
  //  int con = 0;
  //  while (WiFi.status() != WL_CONNECTED)
  //  {
  //    delay(1);
  //    //    Serial.print(".");
  //    con++;
  //  }
  //  Serial.println("");
  //  Serial.print("WiFi connected "); Serial.print(con); Serial.println(" Second");
  //  Serial.print("IP address:\t");
  //  Serial.print(WiFi.localIP());


  pinMode(analogIn, INPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  pinMode(led, OUTPUT);
  antares.setDebug(true);      
 // Nyalakan debug. Set menjadi "false" jika tidak ingin pesan-pesan tampil di serial monitor
  if (wifiMulti.run(connTimeOut) == WL_CONNECTED) { //Check WiFi connection status
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.localIP());}
  antares.wifiConnection(ssid2, pass2);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  //antares.wifiConnectionNonSecure(ssid_eeprom, password_eeprom);
}

void loop() {
  ArduinoOTA.handle();
  DynamicJsonBuffer jsonBuffer;
  if (wifiMulti.run(connTimeOut) == WL_CONNECTED) { //Check WiFi connection status
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.localIP());
    reading = httpGETRequest(serverName);
    //    Serial.println(reading);

    JsonObject& root = jsonBuffer.parseObject(reading);
    String new_ssid = root["data"]["ssid_name_device"].as<String>();
    String new_password = root["data"]["ssid_password_device"].as<String>();
    //    Serial.println(new_ssid);
    const char* n_ssid = new_ssid.c_str();
    const char* n_pass = new_password.c_str();
    Serial.println("Check EEPROM");
    String ssid_eeprom_lama;
    for (int k = addr_ssid; k < new_ssid.length(); k++)
    {
      ssid_eeprom_lama += char(EEPROM.read(k));
    }
    Serial.print("SSID: ");
    Serial.println(ssid_eeprom_lama);

    String password_eeprom_lama;
    for (int l = addr_password; l < (new_password.length() + addr_password); l++)
    {
      password_eeprom_lama += char(EEPROM.read(l));
    }
    Serial.print("PASSWORD: ");
    Serial.println(password_eeprom_lama);

    if (ssid_eeprom_lama != new_ssid && password_eeprom_lama != new_password) {
      reseteeprom(0, 512);
      Serial.println("");
      Serial.print("Write WiFi SSID at address "); Serial.println(addr_ssid);
      Serial.print("");
      int ssid_eeprom_index = addr_ssid;
      wifiMulti.addAP(n_ssid, n_pass);
      for (int i = 0; i < new_ssid.length(); i++)
      {
        EEPROM.write(ssid_eeprom_index, new_ssid[i]);
        Serial.print(new_ssid[i]); Serial.print("");
        ssid_eeprom_index++;
      }

      Serial.println("");
      Serial.print("Write WiFi Password at address "); Serial.println(addr_password);
      Serial.print("");
      int pass_eeprom_index = addr_password;
      for (int j = 0; j < new_password.length(); j++)
      {
        EEPROM.write(pass_eeprom_index, new_password[j]);
        Serial.print(new_password[j]); Serial.print("");
        pass_eeprom_index++;
      }

      Serial.println("");
      if (EEPROM.commit()) {
        Serial.println("Data successfully committed");
      } else {
        Serial.println("ERROR! Data commit failed");
      }
      checkEEPROM(new_ssid.length(), new_password.length());

      WiFi.disconnect();
      ESP.restart();
//       antares.wifiConnectionNonSecure(ssid_eeprom, password_eeprom);
    }
  }

//  delay(3000);    //Send a request every 3 seconds

  if (millis() >= led_millis + LED_INTERVAL)
  {
    led_millis += LED_INTERVAL;

    LedState = !LedState;
    digitalWrite(led, LedState);
  }

  if (millis() >= checkamps_millis + CHECKAMPS_INTERVAL)
  {
    checkamps_millis += CHECKAMPS_INTERVAL;

    Serial.println("Start Check Amps");
    Voltage = getVPP();
    VRMS = (Voltage / 2.0) * 0.707;       //sq root
    float AmpsReal = (VRMS * 1000) / mVperAmp;
    float RoundAmps = (int) (AmpsReal * 1000);
    AmpsRMS = RoundAmps / 1000;
    float WattReal = (220 * AmpsReal) - 18; //Observed 18-20 Watt when no load was connected, so substracting offset value to get real consumption.
    float RoundWatt = (int) (WattReal * 1000);
    Wattage = RoundWatt / 1000;

    Serial.print("Volt = "); Serial.println((float)VRMS);
    Serial.print("Amps = "); Serial.print(AmpsRMS); Serial.print(" / "); Serial.println(AmpsReal);
    Serial.print("Watt = "); Serial.print(WattReal); Serial.print(" / "); Serial.println(WattReal);
    Serial.println();
  }

  if (millis() >= amps_millis + SENDAMPS_INTERVAL)
  {
    amps_millis += SENDAMPS_INTERVAL;
    Serial.println("Start Send Amps to Antares");
    Voltage = getVPP();
    VRMS = (Voltage / 2.0) * 0.707;       //sq root
    float AmpsReal = (VRMS * 1000) / mVperAmp;
    float RoundAmps = (int) (AmpsReal * 1000);
    AmpsRMS = RoundAmps / 1000;
    float WattReal = (220 * AmpsReal) - 20; //Observed 18-20 Watt when no load was connected, so substracting offset value to get real consumption.
    float RoundWatt = (int) (WattReal * 1000);
    Wattage = RoundWatt / 1000;
    //Serial.println(AmpsReal);
    //Serial.println(WattReal);
    Serial.print(AmpsRMS);
    Serial.println(" Amps RMS ");
    Serial.print(Wattage);
    Serial.println(" Watt ");

    // Memasukkan nilai-nilai variabel ke penampungan data sementara
    antares.add("token", token);
    antares.add("Amps RMS", AmpsRMS);
    antares.add("Watt", Wattage);

    // Kirim dari penampungan data ke Antares
    antares.send(projectName, deviceNameStatus);
  }

  if (millis() >= getantares_millis + GET_INTERVAL)
  {
    getantares_millis += GET_INTERVAL;
    Serial.println("Start Read Antares");
    antares.get(projectName, deviceNameRead);
    if (antares.getSuccess()) {
      device_token = antares.getString("device_token");
      RelayStatus = antares.getString("mode");
      Serial.print("StatusRelay: ");
      Serial.println(RelayStatus);
      Serial.print("Device_Token:");
      Serial.println(device_token);
      if (device_token == token)
      {
        Serial.println("Device Token Benar");
        if (RelayStatus == "ON")
        {
          digitalWrite(relay, LOW);
          Serial.println("Relay Switch ON");
          Serial.println();
        } else if (RelayStatus == "OFF") {
          digitalWrite(relay, HIGH);
          Serial.println("Relay Switch OFF");
          Serial.println();
        }
      }
    }
  }
}

float getVPP()
{
  float result;

  int readValue;             //value read from the sensor
  int maxValue = 0;          // store max value here
  int minValue = 1024;          // store min value here

  uint32_t start_time = millis();

  while ((millis() - start_time) < 1000) //sample for 1 Sec
  {
    readValue = analogRead(analogIn);
    // see if you have a new maxValue
    if (readValue > maxValue)
    {
      /*record the maximum sensor value*/
      maxValue = readValue;
    }
    if (readValue < minValue)
    {
      /*record the maximum sensor value*/
      minValue = readValue;
    }
    /*Serial.print(readValue);
      Serial.println(" readValue ");
      Serial.print(maxValue);
      Serial.println(" maxValue ");
      Serial.print(minValue);
      Serial.println(" minValue ");*/
    //delay(1000);
  }

  // Subtract min from max
  Serial.println(maxValue);
  Serial.println(minValue);
  result = ((maxValue - minValue) * 5) / 1024.0;
  Serial.println((float)result);
  Serial.println();
  return result;
}
