#include <EEPROM.h>
#include <OneWire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>


//Pin 0 et 1 reservé pour le port serie (debug)
//Pin 2 reservé pour une sonde de temperature onewire
//Pin 3 à 9 libre
//Pin 10 à 13 reservé pour le shield ethernet
//Pin 14 à 40 reservé pour l utilisation de relais avec sauvegarde de l etat
//Pin 41 à 50 reservé en input avec sauvegarde de l etat



byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 10, 177);
EthernetServer server(80);
// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
String EepromValues;

OneWire  ds(2);  // on pin 2 (a 4.7K resistor is necessary)
const byte TELEINFO_PIN = 3;   //Connexion TELEINFO

char domoticz_IP = "192.168.10.28";               // Adresse du server Domoticz
char domoticz_Port = "8080";
String TempIdx= "31";
String EDFIdx= "73";
String Din1Idx="74";
String Din2Idx="75";
String Din4Idx="76";
String Din5Idx="77";
String Din6Idx="78";
String Din7Idx="79";
String Din8Idx="80";
String Din9Idx="81";
String Din10Idx="82";


unsigned long old_time;
unsigned long new_time;
unsigned long intervaltime;
unsigned long period = 60000;  // delai de 60sec

String toString(float value, byte decimals) {
  String sValue = String(value, decimals);
  sValue.trim();
  return sValue;
}

void clearEEPROM() {
  for (int i = 0; i < 100; ++i) { EEPROM.write(i, 0); }
  //EEPROM.commit();
  return;
}

String readEEPROM(int debut,int fin) {
  //Serial.println(valeur);
  int value;
  Serial.println("");
  EepromValues = "";
  for (int i = debut; i < fin+1; ++i)
    {
    value = EEPROM.read(i)-48;
    //Serial.print("Read: ");
    //Serial.print(value); 
    //Serial.print(" in: ");
    //Serial.println(i);
    EepromValues += toString(value,0);
    } 
  //EEPROM.commit();
  return EepromValues;
}

void writeEEPROM(int debut,String valeur) {
  //Serial.println(valeur);
  Serial.println("");
  for (int i = debut; i < debut+valeur.length(); ++i)
    {
    EEPROM.update(i, valeur[i-debut]);
    //Serial.print("Wrote: ");
    //Serial.print(valeur[i-debut]);
    //Serial.print(" in: ");
    //Serial.println(i); 
    } 

  return;
}

void SaveToEEPROM () {
  String fullstate = "";
  for (int i=14;i<51;i++) {
    fullstate += toString(digitalRead(i),0);
  }
  writeEEPROM (0,fullstate);
  Serial.println(fullstate);
}

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }  
  String EEPROMvalues=readEEPROM(0,36);
  Serial.println(EEPROMvalues);
  
//paramétrage des pin digital en output pour radiateur et relay en Low par defaut

    String mode ="";
    for (int i=14;i<41;i++) {
    mode = EEPROMvalues.substring(i-14,i-13);
    pinMode(i, OUTPUT);
    if (mode=="1") {digitalWrite(i, HIGH);}
    if (mode=="0") {digitalWrite(i, LOW);}

    }

//paramétrage des pin digital en input pour contacteur ouverture porte/fenetre

    for (int i=41;i<51;i++) {
      pinMode(i, INPUT);
    }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
}

void receivedata() {
  EthernetClient client = server.available();
    if (client) {
    Serial.println("new client");
  String line;
  String command;
  //String type;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;

  
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
    line +=c;
        if (c == '\n' && currentLineIsBlank) {
      String s;
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from MegaHomeServer <br><br>";
      String TempC=temperature();
      s += "<br>Temperature : ";
      s += TempC;
      for (int i=14;i<41;i++) { 
        String x =toString(i,0);   
        s += "</br>D-out";
        s +=x;
        s +=":";
        String state = toString(digitalRead(i),0);
        s +=state;
      }     
      client.println(s);
      for (int digitChannel = 41; digitChannel < 51; digitChannel++) {
        int DigitSensorReading = digitalRead(digitChannel);
        client.print("<br>D-In");
        client.print(digitChannel);
        client.print(":");
        client.println(DigitSensorReading);
      }
      
      for (int analogChannel = 0; analogChannel < 16; analogChannel++) {
        int AnaSensorReading = analogRead(analogChannel);
        client.print("<br>A-In");
        client.print(analogChannel);
        client.print(":");
        client.println(AnaSensorReading);
      }
      client.println("</html>\r\n\r\n");
      break;
        }
        if (c == '\n') {
        // you're starting a new line
      String tmpline = "";
      tmpline = line.substring(4,line.indexOf('=')+1);
      if (tmpline == "/a?command=") {
        command = line.substring(line.lastIndexOf('=')+1);
        command = command.substring(0,command.lastIndexOf(' '));
        Serial.print ("command : ");
        Serial.println(command);
        if (command == "reset") {
          Serial.println("Reset EEPROM to off");
          for (int i=14;i<41;i++) { digitalWrite(i, HIGH); }
          clearEEPROM();
        }   
        for (int i=14;i<41;i++) { 
          String Tmp = toString(i,0);
          Tmp += "on";
          if (command == Tmp)
          {
             digitalWrite(i, LOW); // on
             Serial.print("pin ");
             Serial.print(i);
             Serial.println(" on ");
             //Serial.println(type);
          }
          Tmp =  toString(i,0);
          Tmp += "off";
          if (command == Tmp)
          {
             digitalWrite(i, HIGH); // off
             Serial.print("pin ");
             Serial.print(i);
             Serial.println(" off ");
             //Serial.println(type);
          }
        }
        SaveToEEPROM ();
        }
        currentLineIsBlank = true;
        line ="";
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

String temperature() {
 byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  String result;
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
  result = "DS18B20 absent";
    return result;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
  return toString(celsius,2);
}


void SendToDomoticz()
{
// Domoticz format /json.htm?type=command&param=udevice&idx=IDX&nvalue=0&svalue=TEMP

    client.connect(domoticz_IP,domoticz_Port);
    Serial.println("Send temperature to domoticz");
    client.print("GET /json.htm?type=command&param=udevice&idx=");
    Serial.print("GET /json.htm?type=command&param=udevice&idx=");
    client.print(TempIdx);
    Serial.print(TempIdx);
    client.print("&nvalue=0&svalue=");
    Serial.print("&nvalue=0&svalue=");
    client.print(temperature());
    Serial.println(temperature());
    client.print(";");
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.print(domoticz_IP);
    client.print(":");
    client.println(domoticz_Port);
    client.println("User-Agent: Arduino-ethernet");
    client.println("Connection: close");
    client.println();
    client.stop();
}




void loop() {
  
   new_time = millis();
   //Serial.println(new_time - old_time);
   intervaltime = new_time - old_time;
   
   
  if (intervaltime > period)
  {
   SendToDomoticz();
   old_time = new_time;
   Serial.println("envoie vers domoticz");
    
  }
  
  // listen for incoming clients
  receivedata();
}


