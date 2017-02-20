/*
 *  Disclaimer: The code used to connect to the router does not belong to me. I got it from 
 *  Boris Shobat's Gsender code.
 *  
 *  Folks on the Facebook group ESP8266 provided invaluable comments and help to iron 
 *  out the nuances. I encourage you to join the Facebook group ESP8266 at https://goo.gl/nb1rVq
 *  
 *  The purpose of this code is to use an ESP8266 to update the IP on domains.google.com 
 *  The paradigm is simple: The code is in three major parts 
 *      Connect to your router.
 *      A GET request to https://api.ipify.org/ returns the public IP of your router.
 *      A GET request to domains.google.com with payload that include username:password 
 *      base64 encoded, the domain name that needs DDNS with the public IP   
 *      
 *  Author: Raj Jain
 *  Date: Feb 20 2017
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

/*
 * User specific variables and loging credentials that need to be set
 */

const char* ssid = "myRouterSSID";                // WIFI network name
const char* password = "myRouterPWD";             // WIFI network password
const char* host = "domains.google.com";
const int httpsPort = 443;
const char* credentialsBase64 = "myBase64EncodedLoginCredentials"; //encode username:password as one string
const char* dynDNSHostName = "subdomain.mydomain.com";

/*
 * Global variables
*/

uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again   
  
int recheck_interval = 10000;
unsigned long timeStamp;                         // when should I recheck, updated by millis() + recheck_interval
String o_ipAddress;

uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr){
  static uint16_t attempt = 0;
  Serial.print("Connecting to ");
  if(nSSID) {
    WiFi.begin(nSSID, nPassword);  
    Serial.println(nSSID);
  } else {
    WiFi.begin(ssid, password);
    Serial.println(ssid);
  }

  uint8_t i = 0;
  while(WiFi.status()!= WL_CONNECTED && i++ < 50) {
    delay(200); //200
    Serial.print(".");   
  }
  
  ++attempt;
  Serial.println("");
  if(i == 51) {
    Serial.print("Connection: TIMEOUT on attempt: ");
    Serial.println(attempt);
    
    if(attempt % 2 == 0)
      Serial.println("Check if access point available or SSID and Password\r\n");
    return false;
  }
  Serial.println("Connection: ESTABLISHED");
  Serial.print("Got IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void Awaits(){  
  uint32_t ts = millis();
  while(!connection_state){
    delay(50);
    if(millis() > (ts + reconnect_interval) && !connection_state){
      connection_state = WiFiConnect();
      ts = millis();
    }
  }
}

void connectToRouter(){    
    connection_state = WiFiConnect();
    if(!connection_state)   // if not connected to WIFI
      Awaits();             // constantly trying to connect   
}


void setup(){
  Serial.begin(115200);
  connectToRouter();  
  
  //initialize global variables
  timeStamp = millis()+5000;
  o_ipAddress = String("");   

}

String getPublicIP(){   
  WiFiClient client;
  if (client.connect("api.ipify.org", 80)) {
  //Serial.println("connected");
  client.println("GET / HTTP/1.1");
  client.println("Host: api.ipify.org");
  client.println(); 
  delay(500);
  String line = client.readString();  
  return line.substring((line.lastIndexOf('\n'))+1, line.length()); //+1 is to begin from the next character.
  
  } else {
    Serial.println("Ipify Connection failed");
    return String("");
  }
}

String updateDynDNS (String newIPAddress){
  WiFiClientSecure client;
  Serial.print("Connecting to ");
  Serial.println(host);

  if (client.connect(host, httpsPort)) {
    Serial.println("Connected to Google");
    String url = "/nic/update?hostname=" + String(dynDNSHostName) + "&myip=" + newIPAddress;;

    //Serial.print("requesting URL: ");
    //Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" + 
           "Host: " + host + "\r\n" +
           "Authorization: Basic " + credentialsBase64 + "\r\n" +
           "User-Agent: ESP8266\r\n" +
           "Connection: close\r\n\r\n");

    Serial.println("request sent \n");
    String result = client.readString();
    return result;
  } else {
     Serial.println("Google Connection failed");
     return String("");
  }
}

void loop(){    
  if(millis() > timeStamp){
    timeStamp = millis()+ recheck_interval; 
    if (WiFi.status()== WL_CONNECTED){ 
      String ipAddress = getPublicIP();
      if (ipAddress != String("")){
        Serial.println("Public IP: " + ipAddress); 
        if (ipAddress != o_ipAddress){
          Serial.println("Public IP: " + ipAddress);
          o_ipAddress = ipAddress;
          String result = updateDynDNS(ipAddress);
          //Serial.println(result);
          Serial.println(result.substring((result.lastIndexOf('\n'))+1, result.length())); //+1 is to begin from the next character.         
        }
      }
    }  else {
    o_ipAddress = String("");
    }
  }
}

