#include "arduino_secrets.h"
#include "thingProperties.h"
#include <DHT.h>
#include <WiFiClientSecure.h>
#include <mbedtls/aes.h>

const char* ssid     = "IotSec"; 
const char* password = "vmum7999";
const char* server = "192.168.144.57";
const int port = 8090;
char* key = "abcdefghijklmnop";

char plainTemperatureText[16];
char plainLightText[16];

const char* test_root_ca = "-----BEGIN CERTIFICATE-----\n"\
"MIID6zCCAtOgAwIBAgIUPd8bXCCCtIFBByTPFkvTGEiGrqQwDQYJKoZIhvcNAQEL\n"\
"BQAwgYQxCzAJBgNVBAYTAklUMRQwEgYDVQQIDAtCcmFjaWdsaWFubzEMMAoGA1UE\n"\
"CgwDVUZGMRcwFQYDVQQDDA4xOTIuMTY4LjE0NC41NzE4MDYGCSqGSIb3DQEJARYp\n"\
"bS5lc3Bvc2l0bzI1M0BzdHVkZW50aS51bmlzYS5pdEBnbWFpbC5jb20wHhcNMjIw\n"\
"MjA3MDc0MDAwWhcNMjMwMjA3MDc0MDAwWjCBhDELMAkGA1UEBhMCSVQxFDASBgNV\n"\
"BAgMC0JyYWNpZ2xpYW5vMQwwCgYDVQQKDANVRkYxFzAVBgNVBAMMDjE5Mi4xNjgu\n"\
"MTQ0LjU3MTgwNgYJKoZIhvcNAQkBFiltLmVzcG9zaXRvMjUzQHN0dWRlbnRpLnVu\n"\
"aXNhLml0QGdtYWlsLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n"\
"ALLxvfH/0phqKMPR9GrWmszRCEh0lJrkGytzFDhLkLpTYPfUAoF/l3A6+WF+AK+O\n"\
"R3pzwc/KRY+LCqwmOstzLuUVpf/K/FyWpZLuSTyzloD53n4UuwFahoQ+Fzcy3dPg\n"\
"lNPz1rg2MeK0fkumyQdWYnaQPdxS7E5l8f+4ogm5ApKXniU8Qp3jc7ykErMmUnRJ\n"\
"aKyZWNeP9zZ4JAXz5e5Rb+oRLSOsXHc2W5Tu3aJOanluaNA/HWwJbw425cksi6Ai\n"\
"ePbh+O6wTLlCYfKdcq4wvpxnFMP78f/PUUCDpfPMqtcEr/lWgzvf2R/82hFATHaJ\n"\
"Rt/4YiI/Df4LpftO5H3DgC8CAwEAAaNTMFEwHQYDVR0OBBYEFPWg5CMjTHcQqnzM\n"\
"W3qg16XU1ql2MB8GA1UdIwQYMBaAFPWg5CMjTHcQqnzMW3qg16XU1ql2MA8GA1Ud\n"\
"EwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAJM0+rc6Me/GLmQaY4+7dBQH\n"\
"ZwiigEx+5+E3JpkF+vOA8NyRdQYZ8V8RCQzk68V7O0gbfXF01Y3KGk8y0Zf2hvwn\n"\
"SXERKLgLMd25sueOxj/ko5oVj1jB0/IPaALx9XUx+ldmrdC+qcXZDwq+FLsWCp9K\n"\
"7tI7OoVg57aQuYihRDEszIyYl3XT6Wrmet+2gVfkEFDerO10jtdYvtHURBLlMCkW\n"\
"tIAjJnN655VjH35qQeLi0TpwhYCwukpqo2CqrMnA//RDPC/cUpNoRP9mRfyv5N+5\n"\
"a9iS2dQnakIIAeyOwG1HPOJjM2aTrSZLNfMqcDsF6wjYRoeC5DNTkMlKokLZopE=\n"\
"-----END CERTIFICATE-----";


WiFiClientSecure client;

#define DHTPIN 33
#define LIGHT_SENSOR_PIN 32
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void encrypt(char * plainText, char * key, unsigned char * outputBuffer){
  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_enc( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb( &aes, MBEDTLS_AES_ENCRYPT, (const unsigned char*)plainText, outputBuffer);
  mbedtls_aes_free( &aes );
}

void decrypt(unsigned char * chipherText, char * key, unsigned char * outputBuffer){
  mbedtls_aes_context aes;
  mbedtls_aes_init( &aes );
  mbedtls_aes_setkey_dec( &aes, (const unsigned char*) key, strlen(key) * 8 );
  mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, (const unsigned char*)chipherText, outputBuffer);
  mbedtls_aes_free( &aes );
}

void addSpaces(char* plainText){
  for(int i=0; i< (16-strlen(plainText)); i++)
    strcat(plainText," ");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(1500);
  
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  
  client.setCACert(test_root_ca);
  
  Serial.println();
  
  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

void loop() {
  ArduinoCloud.update();
  unsigned char cipherTextOutput[16];
  unsigned char decipheredTextOutput[16];
  int lengthWord;
  
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  lightLevel = analogRead(LIGHT_SENSOR_PIN);
  delay(1000);
  
  
  Serial.println(temperature);
  Serial.println(lightLevel);
  
  itoa(temperature, plainTemperatureText, 10);
  addSpaces(plainTemperatureText);

  itoa(lightLevel, plainLightText, 10);
  addSpaces(plainLightText);
  
  String encryptedText = "";
  
   if (!client.connect(server, port)){
    Serial.println("Connection failed!");
  } else {
    Serial.println("Connection success!");
    
    encrypt(plainTemperatureText, key, cipherTextOutput);
   
    for (int i = 0; i < 16; i++) {
      char str[3];
      sprintf(str, "%02x", (int)cipherTextOutput[i]);
      encryptedText = encryptedText + str;
    }

    encryptedText = encryptedText + ' ';
  
    encrypt(plainLightText, key, cipherTextOutput);
   
     for (int i = 0; i < 16; i++) {
        char str[3];
        sprintf(str, "%02x", (int)cipherTextOutput[i]);
        encryptedText = encryptedText + str;
    }  
    Serial.println("Mando al server "+encryptedText);
    client.println(encryptedText);
  }
  client.stop();
  delay(1000);

}