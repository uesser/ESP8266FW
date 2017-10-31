#ifndef __ESP8266FW__
#define __ESP8266FW__

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <EEPROM.h>
#include <ESP8266Logger.h>

class ESP8266FWClass {
  public:
    ESP8266FWClass(char* ssid, char* ssidPwd, char* hostName, uint16_t otaPort,
                   char* otaPwd, char* ntpHost, uint16_t ntpPort,LogSerial logSer);
    ESP8266FWClass(char* ssid, char* ssidPwd, char* hostName, uint16_t otaPort,
                   char* otaPwd, LogSerial logSer);
    ESP8266FWClass(char* ssid, char* ssidPwd, char* hostName, LogSerial logSer);
    ~ESP8266FWClass();

    boolean wifiConnect();
    boolean checkWifiReconnect();

    boolean otaSetup();
    boolean mDNSSetup();
    boolean ntpConnect();

    boolean setupAll();

    boolean otaInProgress();
    time_t getLocalTime();

  protected:
    void sendNTPpacket(IPAddress& address);
    time_t getNtpTime();
  
  private:
    const int      _C_NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message

    LogSerial      _logSer;
    ESP8266Logger  _logger;
    static char    _s_logStr[255];

    char*          _ssid;
    char*          _ssidPwd;
                   
    char*          _hostName;
                   
    uint16_t       _otaPort;
    char*          _otaPwd;
    boolean        _otaInProgress;
                   
    char*          _ntpHost;
    uint16_t       _ntpPort;
    uint16_t       _ntpSyncInterval;
    time_t         _localTimeApprox;
    static byte    _ntpBuffer[_C_NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets
}

extern ESP8266FWClass ESP8266FW;

#endif  // __ESP8266FW__
