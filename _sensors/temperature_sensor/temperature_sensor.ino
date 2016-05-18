#include <WiFi101.h>
#include <WiFiClient.h>
#include <OneWire.h>

#include <thethingsiO_mkr1000.h>


OneWire  ds(2);

#define WIFI_AP "NextFab"
#define WIFI_PWD "makeithere"
#define SERVER "172.16.37.59"
#define URI "/api/v1/temp"
#define PORT 5000

int status = -1;
int millis_start;

WiFiClient client;

void setup(void) {
  millis_start = millis();
  Serial.begin(115200);
  startWifi();
}

void loop(void) {
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
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
  if (client.connect(SERVER, PORT)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.print("POST ");
    client.print(URI);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(SERVER);
    client.println("Content-Type: application/json");
    client.println("Content-Length: 14");
    client.println("");
    client.print("{\"temp\":");
    client.print(fahrenheit);
    client.println("}");
    client.println("Connection: close");
    client.println();

    Serial.print("POST ");
    Serial.print(URI);
    Serial.println(" HTTP/1.1");
    Serial.print("Host: ");
    Serial.println(SERVER);
    Serial.println("Content-Type: application/json");
    Serial.println("Content-Length: 14");
    Serial.println("");
    Serial.print("{\"temp\":");
    Serial.print(fahrenheit);
    Serial.println("}");
    //Serial.println("Connection: close");
    Serial.println();

    
    client.stop();
    Serial.println("data sent");
  }

    
    millis_start = millis();
  }
}

void startWifi(){
  Serial.println("Connecting MKR1000 to network...");
//  WiFi.begin();
   // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED ) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_AP);
    WiFi.begin(WIFI_AP, WIFI_PWD);
    // wait 10 seconds for connection:
    delay(10000);
    status = WiFi.status();
  }
}
