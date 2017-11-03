#ifndef __ESP8266FW__
#define __ESP8266FW__

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <EEPROM.h>
#include <ESP8266Logger.h>

#define C_MINUTE_MS 60000

class ESP8266FWClass {
  public:
    typedef void (*callback_function)(void);
  
    const uint32_t  C_SEVENTY_YEARS       = 2208988800UL;
    const uint32_t  C_MICROSECS_PER_HOUR  = 3600 * 1000 * 1000;
    
    ESP8266FWClass(char* ssid, char* ssidPwd, char* hostName, 
                   LogSerial logSer, String logHost, String logPort, 
                   String logURL, String logFileName, String logLevelParam, 
                   String logFunctionParam, String logStrParam, String logStrlnParam);
    ~ESP8266FWClass();

    boolean wifiConnect();
    boolean checkWifiReconnect();

    boolean otaSetup(uint16_t otaPort = 0, char * otaPwd = 0);
    boolean ntpConnect(char * ntpHost = 0, uint16_t ntpPort = 123, uint16_t ntpSyncInterval = 300);
    boolean mDNSSetup();

    boolean setupWebserver(int port, callback_function wsRootHandler, 
                           callback_function wsNotFoundHandler = _wsNotFoundHandler);
    void logWSDetails(LogLevel logLev);
                                      
    boolean setupAll(uint16_t otaPort, char * otaPwd,
                     char * ntpHost, uint16_t ntpPort, uint16_t ntpSyncInterval,
                     int port, callback_function wsRootHandler, callback_function wsNotFoundHandler);

    time_t getLocalTime();
    
    void deepSleep(uint32_t sleepTime);

    template <class T> boolean loadUserConfig(T* data);
    template <class T> boolean saveUserConfig(T* data);
    
    // getter / setter
    ESP8266WebServer * getWebServer()       { return &_webServer; };
    ESP8266Logger *    getLogger()          { return &_logger; };
    char *             getSSID()            { return _ssid; };
    char *             getHostName()        { return _hostName; };
    uint16_t           getOtaPort()         { return _otaPort; };
    boolean            getOtaInProgress()   { return _otaInProgress; };
    char *             getNtpHost()         { return _ntpHost; };
    uint16_t           getNtpPort()         { return _ntpPort; };
    uint16_t           getNtpSyncInterval() { return _ntpSyncInterval; };
    
  protected:
    void    _sendNTPpacket(IPAddress& address);
    time_t  _getNtpTime();
    void    _wsNotFoundHandler();
    void    _renewTimestampAndSave();
  
  private:
    class Data {
      uint16_t saved;
      uint32_t localTimeApprox;
    };
    static Data        _data;
                       
    const int          _C_NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
                       
    LogSerial          _logSer;
    String             _logHost;
    String             _logPort;
    ESP8266Logger      _logger;
    static char        _s_logStr[255];
    int                _logSerIdx;
    int                _logWifiIdx;
                       
    char*              _ssid;
    char*              _ssidPwd;
                       
    char*              _hostName;
                       
    uint16_t           _otaPort;
    char*              _otaPwd;
    boolean            _otaInProgress;
                       
    char*              _ntpHost;
    uint16_t           _ntpPort;
    uint16_t           _ntpSyncInterval;                // default 300 sec. = 5 min.
    static byte        _ntpBuffer[_C_NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets
    IPAddress          _ntpTimeServerIP;
    
    ESP8266WebServer   _webServer;
    
    Ticker             _localTime_saveConfigTicker;

    boolean            _loadConfig();
    boolean            _saveConfig();
}

extern ESP8266FWClass ESP8266FW;

#endif  // __ESP8266FW__
