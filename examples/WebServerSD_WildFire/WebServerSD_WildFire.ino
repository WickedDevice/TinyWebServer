// -*- c++ -*-
//
// Copyright 2010 Ovidiu Predescu <ovidiu@gmail.com>
// Date: June 2010
// Updated: 08-JAN-2012 for Arduno IDE 1.0 by <Hardcore@hardcoreforensics.com>
//

#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <CC3000_MDNS.h>
#include <WildFire.h>
#include <Flash.h>
#include <SdFat.h>
#include <TinyWebServer.h>

WildFire wf;
SdFat sd;

boolean file_handler(TinyWebServer& web_server);
boolean index_handler(TinyWebServer& web_server);

Adafruit_CC3000 cc3000 = Adafruit_CC3000(SPI_CLOCK_DIV2); // you can change this clock speed
#define LISTEN_PORT           80    // What TCP port to listen on for connections.
MDNSResponder mdns;
boolean attemptSmartConfigReconnect(void);
boolean attemptSmartConfigCreate(void);

TinyWebServer::PathHandler handlers[] = {
  {"/", TinyWebServer::GET, &index_handler },
  {"/" "*", TinyWebServer::GET, &file_handler },
  {NULL},
};

boolean file_handler(TinyWebServer& web_server) {
  char* filename = TinyWebServer::get_file_from_path(web_server.get_path());
  send_file_name(web_server, filename);
  free(filename);
  return true;
}

void send_file_name(TinyWebServer& web_server, const char* filename) {

    
  if (!filename) {
    web_server.send_error_code(404);
    web_server.print(F("Could not parse URL"));
  } else {
    TinyWebServer::MimeType mime_type
      = TinyWebServer::get_mime_type_from_filename(filename);
    web_server.send_error_code(200);

    Serial.print(F("Attempting to open file named "));
    Serial.println(filename);
    
    if (sd.exists(filename)) {     
      Serial.println(F("File Opened Successfully..."));
      web_server.send_content_type(mime_type);
      web_server.end_headers();
      Serial.print(F("Read file ")); Serial.println(filename);
      web_server.send_file(filename);
      Serial.println(F("File sent "));
    } else {
      web_server.send_content_type("text/plain");
      web_server.end_headers();
      Serial.print(F("Could not find file: ")); Serial.println(filename);
      web_server.print(F("Could not find file: ")); 
      web_server.print(filename);
      web_server.println();
    }
  }
  
}

boolean index_handler(TinyWebServer& web_server) {
  web_server.send_error_code(200);
  web_server.end_headers();
  web_server.print(F("<html><body><h1>Hello World!</h1></body></html>\n"));
  return true;
}

boolean has_ip_address = false;
TinyWebServer web = TinyWebServer(handlers, NULL);

const char* ip_to_str(const uint8_t* ipAddr)
{
  static char buf[16];
  sprintf(buf, "%d.%d.%d.%d\0", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
  return buf;
}

void setup() {
  wf.begin();
  Serial.begin(9600);

  //Serial.print(F("Free RAM: "));
  //Serial.print(FreeRam());
  //Serial.println();
  
  Serial.print(F("Setting up SD card...\n"));

  // see if the card is present and can be initialized:
  if (!sd.begin(4, SPI_HALF_SPEED)) sd.initErrorHalt();
  
  Serial.println("card initialized.");

  Serial.print(F("Setting up the WiFi connection...\n"));
  if(!attemptSmartConfigReconnect()){
    while(!attemptSmartConfigCreate()){
      Serial.println(F("Waiting for Smart Config Create"));
    }
  }
  
  while(!displayConnectionDetails());
  // Start multicast DNS responder
  if (!mdns.begin("wildfire", cc3000)) {
    Serial.println(F("Error setting up MDNS responder!"));
    while(1); 
  }

  // Start the web server.
  Serial.print(F("Web server starting...\n"));
  web.begin();
  
  //Serial.print(F("Free RAM: "));
  //Serial.print(FreeRam());
  //Serial.println();

  Serial.print(F("Ready to accept HTTP requests.\n\n"));
  
}

void loop() {
  web.process();
}

bool displayConnectionDetails(void) {
  uint32_t addr, netmask, gateway, dhcpserv, dnsserv;

  if(!cc3000.getIPAddress(&addr, &netmask, &gateway, &dhcpserv, &dnsserv))
    return false;

  Serial.print(F("IP Addr: ")); cc3000.printIPdotsRev(addr);
  Serial.print(F("\r\nNetmask: ")); cc3000.printIPdotsRev(netmask);
  Serial.print(F("\r\nGateway: ")); cc3000.printIPdotsRev(gateway);
  Serial.print(F("\r\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
  Serial.print(F("\r\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
  Serial.println();
  return true;
}

boolean attemptSmartConfigReconnect(void){
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!! Note the additional arguments in .begin that tell the   !!! */
  /* !!! app NOT to deleted previously stored connection details !!! */
  /* !!! and reconnected using the connection details in memory! !!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */  
  if (!cc3000.begin(false, true))
  {
    Serial.println(F("Unable to re-connect!? Did you run the SmartConfigCreate"));
    Serial.println(F("sketch to store your connection details?"));
    return false;
  }

  /* Round of applause! */
  Serial.println(F("Reconnected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("\nRequesting DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  return true;
}

boolean attemptSmartConfigCreate(void){
  /* Initialise the module, deleting any existing profile data (which */
  /* is the default behaviour)  */
  Serial.println(F("\nInitialisTinyWebServer.hing the CC3000 ..."));
  if (!cc3000.begin(false))
  {
    return false;
  }  
  
  /* Try to use the smart config app (no AES encryption), saving */
  /* the connection details if we succeed */
  Serial.println(F("Waiting for a SmartConfig connection (~60s) ..."));
  if (!cc3000.startSmartConfig(false))
  {
    Serial.println(F("SmartConfig failed"));
    return false;
  }
  
  Serial.println(F("Saved connection details and connected to AP!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) 
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  
  return true;
}
