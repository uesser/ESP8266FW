include "ESP8266FW.h"

// ___________________________________ ESP8266FWClass ____________________________
// - constructor
// __________________________________________________________________________________
ESP8266FWClass::ESP8266FWClass(char* ssid, char* ssidPwd, char* hostName, 
                               LogSerial logSer, String logHost, String logPort, 
                               String logURL, String logFileName, String logLevelParam, 
                               String logFunctionParam, String logStrParam, String logStrlnParam)
,_otaPort(0)
,_otaInProgress(false)
,_ntpPort(0)
,_ntpSyncInterval(0)
,_logSer(logSer)
,_logHost(logHost)
,_logPort(logPort)
{
  _ssid = (char *) malloc(sizeof(ssid));
  strcpy(_ssid, ssid);

  _ssidPwd = (char *) malloc(sizeof(pwd));
  strcpy(_ssidPwd, ssidPwd);

  if (! hostName || strlen(hostName) == 0) {
    char tmp[15];
    sprintf(tmp, "esp8266-%06x", ESP.getChipId());
    hostName = tmp;
  }
  _hostName = (char *) malloc(sizeof(hostName));
  strcpy(_hostName, hostName);

  if (_logSer != LOG_UNDEF) {
    if ((_logSerIdx = _logger.regLogDestSerial(INFO, _logSer)) < 0) {
      Serial.println();
      Serial.println("Register Serial logger failed!");
      Serial1.println();
      Serial1.println("Register Serial logger failed!");
    }
  }
  if (_logHost && _logHost.length() > 0) {
    if ((_logWifiIdx = _logger.regLogDestWifi(INFO, _logHost, _logPort, logURL, String(_hostname) + ".log", 
                                              logLevelParam, logFunctionParam, logStrParam, 
                                              logStrlnParam)) < 0) {
      Serial.println();
      Serial.println("Register Wifi logger failed!");
      Serial1.println();
      Serial1.println("Register Wifi logger failed!");
    }
  }
  
  _localTime_saveConfigTicker.attach_ms(C_MINUTE_MS, _renewTimestampAndSave);
}

// ___________________________________ ~ESP8266FWClass ___________________________
// - destructor
// __________________________________________________________________________________
ESP8266FWClass::~ESP8266FWClass() {
  if (_ssid)
    free(_ssid);
  if (_ssidPwd)
    free(_ssidPwd);
  if (_hostName)
    free(_hostName);
  if (_otaPwd)
    free(_otaPwd);
  if (_ntpHost)
    free(_ntpHost);

  _logger.unregLogDestSerial(_logSer);
  _logger.unregLogDestWifi(_logHost, _logPort);
}

// ___________________________________ wifiConnect __________________________________
// - connects to WiFi
// __________________________________________________________________________________
boolean ESP8266FWClass::wifiConnect() {
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", "");
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", String("----------------------------------------------------"));
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", String("ESP8266 (re)starting or reconnecting after WiFi lost"));
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", "");
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", String("Connecting to AP '") + _ssid + "': ");

  WiFi.hostname(_hostName);
  WiFi.mode(WIFI_STA);
//  wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.begin(_ssid, _ssidPwd);

  int connectTicks = 0;
  while (WiFi.status() != WL_CONNECTED && connectTicks++ < 20) {
    delay(500);
    Serial.print(".");
    Serial1.print(".");
  }
  Serial.println();
  Serial1.println();

  if (WiFi.status() == WL_CONNECTED) {
    IPAddress IPAddr = WiFi.localIP();

    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", "");
    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", String("----------------------------------------------------"));
    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", String("ESP8266 (re)starting or reconnecting after WiFi lost"));
    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", "");
    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", String("Connecting to AP: ") + _ssid);

    _logger.infoln("WIFI-CONNECT", String("Time to connect: ") + (connectTicks * 500) + " ms");
    _logger.infoln("WIFI-CONNECT", String("IP-Adr: ") + IPAddr[0] + "." + IPAddr[1] + "." + IPAddr[2] + "." + IPAddr[3]);
//    WiFi.printDiag(Serial);

    return true;
  }
  else {
    _logger.errorln("WIFI-CONNECT", "");
    _logger.errorln("WIFI-CONNECT", "Connect to WiFi failed!!!");

    return false;
  }
}

// ___________________________________ CheckWifiReconnect ___________________________
// - check if connected to WiFi and try reconnecting if not
// __________________________________________________________________________________
boolean ESP8266FWClass::CheckWifiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
    _logger.warnln("CHECK-WIFI-RECONNECT", "");
    _logger.warnln("CHECK-WIFI-RECONNECT", "Connection to Wifi lost. Trying to reconnect.");

    return wifiConnect();
  }
  else {          // already/still connected
    return true;
  }
}

// ___________________________________ otaSetup _____________________________________
// - setup of OTA (programming over the air) stuff
// __________________________________________________________________________________
boolean ESP8266FWClass::otaSetup(uint16_t otaPort, char * otaPwd) {
  _logger.infoln("OTA-SETUP", "");
  _logger.infoln("OTA-SETUP", String("setup OTA - hostname: ") + _hostname);

  if (WiFi.status() != WL_CONNECTED) {
    _logger.errorln("OTA-SETUP", "Wifi not connected!!!");
    
    return false;
  }

  if (lenght(_hostname) > 0)
    ArduinoOTA.setHostname(_hostname);

  if (otaPort) {
    _otaPort = otaPort;
    ArduinoOTA.setPort(_otaPort);  // default is 8266
  }

  if (otaPwd && strlen(otaPwd) > 0) {
    _otaPwd = (char *) malloc(sizeof(otaPwd));
    strcpy(_otaPwd, otaPwd);
    ArduinoOTA.setPassword(_otaPwd);
  }

  ArduinoOTA.onStart([]() {
    _otaInProgress = true;
    
    Serial.println();
    Serial.println("OTA - Start");
    Serial1.println();
    Serial1.println("OTA - Start");
  });

  ArduinoOTA.onEnd([]() {
    _otaInProgress = false;
    
    Serial.println();
    Serial.println("OTA - End");
    Serial.println("Rebooting...");
    Serial.println();
    Serial1.println();
    Serial1.println("OTA - End");
    Serial1.println("Rebooting...");
    Serial1.println();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
    Serial1.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
         if (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)     Serial.println("End Failed");
    Serial1.printf("Error[%u]: ", error);
         if (error == OTA_AUTH_ERROR)    Serial1.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)   Serial1.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial1.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial1.println("Receive Failed");
    else if (error == OTA_END_ERROR)     Serial1.println("End Failed");
  });

  _otaInProgress = false;
  
  ArduinoOTA.begin();

  _logger.infoln("OTA-SETUP", "OTA begin");

  return true;
}

// ___________________________________ ntpConnect ___________________________________
// - setup of ntp server
// __________________________________________________________________________________
boolean ESP8266FWClass::ntpConnect(char * ntpHost, uint16_t ntpPort, uint16_t ntpSyncInterval) {
  if (ntpHost && strlen(ntpHost) > 0) {
    _ntpHost = (char *) malloc(sizeof(ntpHost));
    strcpy(_ntpHost, ntpHost);

    _ntpPort         = ntpPort;          // default port is 123
    _ntpSyncInterval = ntpSyncInterval;  // default 300 sec. = 5 min.
    
    _logger.infoln("NTP-CONNECT", ""); 
    _logger.infoln("NTP-CONNECT", String("Connecting to ntp server: ") + _ntpHost);

    UDP.begin(_ntpPort);  // Start listening for UDP messages on port "_ntpPort" (default is 123)

    _logger.infoln("NTP-CONNECT", String("Local port: ") + UDP.localPort());

    _logger.infoln("NTP-CONNECT", String("Calling syncronizing function 'ESP8266FW::getNtpTime' every ") + 
                   _ntpSyncInterval + " seconds");

    setSyncProvider(getNtpTime);
    setSyncInterval(_ntpSyncInterval);
  }
  else {
    _logger.warnln("NTP-CONNECT", ""); 
    _logger.warnln("NTP-CONNECT", "No NTP-Host defined in parameter 'ntpHost'!!!");
  }
}

// ___________________________________ mDNSSetup ____________________________________
// - setup of mDNS responder
// __________________________________________________________________________________
boolean ESP8266FWClass::mDNSSetup() {
  if (! MDNS.begin(_hostName)) {  // Start the mDNS responder for <_hostname>.local
    _logger.errorln("MDNS-SETUP", "Error setting up MDNS responder!!!");
    
    return false;
  } 
  else {
    _logger.infoln("MDNS-SETUP", String("mDNS responder started - Hostname: ") + _hostname + ".local");

    return true;
  }
}

// ___________________________________ setupWebserver _______________________________
// - setup of Webserver
// __________________________________________________________________________________
boolean ESP8266FWClass::setupWebserver(int port, callback_function wsRootHandler, 
                                       callback_function wsNotFoundHandler) {
  _logger.infoln("SETUP-WEBSERVER", "");
  _logger.infoln("SETUP-WEBSERVER", "setup Webserver...");

  _webServer = new ESP8266WebServer(Port);

  if (wsRootHandler)
    _webServer.on("/", wsRootHandler);

  if (wsNotFoundHandler)
    _webServer.onNotFound(wsNotFoundHandler);
  else
    _webServer.onNotFound(_wsNotFoundHandler);

  _webServer.begin();

  _logger.infoln("SETUP-WEBSERVER", String("Webserver started, listening on port ") + String(Port));
  
  return true;
}

// ___________________________________ logWSDetails _________________________________
// - Logs webserver details
// __________________________________________________________________________________
void ESP8266FWClass::logWSDetails(LogLevel logLev) {
  String method = "Unknown";
  
  switch(_webServer.method()) {
    case HTTP_GET:
      method = "GET";
      break;
    case HTTP_POST:
      method = "POST";
      break;
    case HTTP_PUT:
      method = "PUT";
      break;
    case HTTP_PATCH:
      method = "PATCH";
      break;
    case HTTP_DELETE:
      method = "DELETE";
      break;
  }

  _logger.logln(logLev, "LOG-WS-DETAILS", String("URL is: ") + _webServer.uri());
  _logger.logln(logLev, "LOG-WS-DETAILS", String("HTTP Method on request was: ") + method);
  
  // Print how many properties we received and their names and values.
  _logger.logln(logLev, "LOG-WS-DETAILS", String("Number of query properties: ") + _webServer.args());
  for (int i = 0; i < _webServer.args(); i++) {
    _logger.logln(logLev, "LOG-WS-DETAILS", String("  - ") + _webServer.argName(i) + " = " + _webServer.arg(i));
  }
}

// ___________________________________ setupAll _____________________________________
// - setup all (wifi, ota, ...)
// __________________________________________________________________________________
boolean ESP8266FWClass::setupAll(uint16_t otaPort, char * otaPwd,
                                 char * ntpHost, uint16_t ntpPort, uint16_t ntpSyncInterval,
                                 int port, callback_function wsRootHandler, callback_function wsNotFoundHandler) {
  if (! wifiConnect())
    return false;

  if (! otaSetup(otaPort, otaPwd);
    return false;

  if (! mDNSSetup())
    return false;

  if (! ntpConnect(ntpHost, ntpOrt, ntpSyncInterval))
    return false;

  if (! setupWebserver(port, wsRootHandler, wsNotFoundHandler))
    return false;

  return true;
}

// ___________________________________ getLocalTime _________________________________
// - gets the local time
// __________________________________________________________________________________
time_t ESP8266FWClass::getLocalTime() {
  static TimeChangeRule   CEST = { "CEST", Last, Sun, Mar, 2, 120 };  // Central European Summer Time
  static TimeChangeRule   CET  = { "CET",  Last, Sun, Oct, 3,  60 };  // Central European Standard Time
  static Timezone         CE(CEST, CET);
  static TimeChangeRule * tcr;        //pointer to the time change rule, use to get the TZ abbrev
  static unsigned long    sul_millis = 0;

  _data.localTimeApprox += (millis() - sul_millis) / 1000;
  sul_millis = millis();      

  time_t t_localTime = CE.toLocal(now(), &tcr);
  
  _logger.debugln("GET-LOCAL-TIME", "");
  _logger.debugln("GET-LOCAL-TIME", "Before check TimeLocal - TimeLocalApprox deviation:");
  sprintf(g_logStr, "local time approx: %02d.%02d.%04d - %02d:%02d:%02d",
                    day(_data.localTimeApprox), month(_data.localTimeApprox), 
                    year(_data.localTimeApprox), hour(_data.localTimeApprox), 
                    minute(_data.localTimeApprox), second(_data.localTimeApprox));
  _logger.debugln("GET-LOCAL-TIME", g_logStr);
  sprintf(g_logStr, "local time       : %02d.%02d.%04d - %02d:%02d:%02d",
                    day(t_localTime), month(t_localTime), year(t_localTime),
                    hour(t_localTime), minute(t_localTime), second(t_localTime));
  _logger.debugln("GET-LOCAL-TIME", g_logStr);

  if (year(_data.localTimeApprox) > 1970 &&
      (_data.localTimeApprox - 3600) > t_localTime || (_data.localTimeApprox + 3600) < t_localTime) {
    t_localTime = _data.localTimeApprox;
  
    _logger.debugln("GET-LOCAL-TIME", 
                    "Deviation TimeLocal - TimeLocalApprox more than 1 hour, so use LocalTimeApprox");
  }

  _data.localTimeApprox = t_localTime;

  _logger.debugln("GET-LOCAL-TIME", "After check TimeLocal - TimeLocalApprox deviation:");
  sprintf(g_logStr, "local time approx: %02d.%02d.%04d - %02d:%02d:%02d",
                    day(g_skullsEyes.localTimeApprox), month(g_skullsEyes.localTimeApprox), 
                    year(g_skullsEyes.localTimeApprox), hour(g_skullsEyes.localTimeApprox), 
                    minute(g_skullsEyes.localTimeApprox), second(g_skullsEyes.localTimeApprox));
  _logger.debugln("GET-LOCAL-TIME", g_logStr);
  sprintf(g_logStr, "local time       : %02d.%02d.%04d - %02d:%02d:%02d",
                    day(t_localTime), month(t_localTime), year(t_localTime),
                    hour(t_localTime), minute(t_localTime), second(t_localTime));
  _logger.debugln("GET-LOCAL-TIME", g_logStr);

  saveConfig();

  return t_localTime;
}

// ___________________________________ loadConfig ___________________________________
// - Loads config from RTC memory
// __________________________________________________________________________________
template <class T> boolean ESP8266FWClass::loadConfig(T* userData) {
  _data.saved    = 0x0000;
  _data.userData = userData;

  EEPROM.begin(512);
  EEPROM.get(0, _data);
  EEPROM.end();
  
  if (_data.saved != 0xAAAA) {
    _logger.debugln("LOAD-CONFIG", "");
    _logger.debugln("LOAD-CONFIG", "Load config failed or not saved last time !!!");
    
    return false;
  }
  else {
    _logger.debugln("LOAD-CONFIG", "");
    _logger.debugln("LOAD-CONFIG", "Config loaded successfully");
    
    return true;
  }
}

// ___________________________________ saveConfig ___________________________________
// - Saves config to RTC memory
// __________________________________________________________________________________
template <class T> boolean ESP8266FWClass::saveConfig(T* userData) {
  _data.saved    = 0xAAAA;
  _data.userData = userData;

  EEPROM.begin(512);
  EEPROM.put(0, _data);
  delay(200);
  EEPROM.commit();
  EEPROM.end();

  _logger.debugln("SAVE-CONFIG", "");
  _logger.logln("SAVE-CONFIG", "Config saved successfully");
  
  return true;
}

//___________________________________________________________________________________
//___________________________________________________________________________________
// - PROTECTED METHODS
//___________________________________________________________________________________
//___________________________________________________________________________________

// ___________________________________ _sendNTPpacket _______________________________
// - sends ntp request via UDP
// __________________________________________________________________________________
void ESP8266FWClass::_sendNTPpacket(IPAddress& address) {
  memset(_ntpBuffer, 0, _C_NTP_PACKET_SIZE);  // set all bytes in the buffer to 0
  
  _ntpBuffer[ 0] = 0b11100011;  // Initialize values needed to form NTP request: LI, Version, Mode
  _ntpBuffer[ 1] = 0;     // Stratum, or type of clock
  _ntpBuffer[ 2] = 6;     // Polling Interval
  _ntpBuffer[ 3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  _ntpBuffer[12] = 49;
  _ntpBuffer[13] = 0x4E;
  _ntpBuffer[14] = 49;
  _ntpBuffer[15] = 52;

  UDP.beginPacket(address, _ntpPort);
  UDP.write(_ntpBuffer, _C_NTP_PACKET_SIZE);
  UDP.endPacket();
}

// ___________________________________ _getNtpTime __________________________________
// - gets the actual time calculated by the ntp time
// __________________________________________________________________________________
time_t ESP8266FWClass::_getNtpTime() {
  const uint32_t C_SEVENTY_YEARS = 2208988800UL;

  IPAddress  timeServerIP;
  uint32_t   ui_ntpTime  = 0;
  time_t     t_timeUNIX  = 0;

  while (UDP.parsePacket() > 0) ; // discard any previously received packets

    _logger.debugln("GET-NTP-TIME", "");
    _logger.debugln("GET-NTP-TIME", "Sending NTP request ...");
  
  if (! WiFi.hostByName(_ntpHost, timeServerIP)) {  // Get the IP address of the NTP server
    _logger.debugln("GET-NTP-TIME", "DNS lookup failed!");
  }
  else {
    _logger.debugln("GET-NTP-TIME", String("Time server IP: ") + timeServerIP[0] + "." + timeServerIP[1] + "." + 
                                                                 timeServerIP[2] + "." + timeServerIP[3]);

    sendNTPpacket(timeServerIP);
  
    uint32_t ui_beginWait = millis();
    while (millis() - ui_beginWait < 1000) {
      if (UDP.parsePacket() >= _C_NTP_PACKET_SIZE) {
        _logger.debugln("GET-NTP-TIME", "Receive NTP Response");
        
        UDP.read(_ntpBuffer, _C_NTP_PACKET_SIZE);  // read packet into the buffer
  
        // convert four bytes starting at location 40 to a long integer
        ui_ntpTime = (_ntpBuffer[40] << 24) | (_ntpBuffer[41] << 16) | 
                     (_ntpBuffer[42] <<  8) |  _ntpBuffer[43];
  
        t_timeUNIX = ui_ntpTime - C_SEVENTY_YEARS;
      }
    }
  
    if (t_timeUNIX == 0) {
      _logger.debugln("GET-NTP-TIME", "No NTP Response!");
   }
    else {
      sprintf(_s_logStr, "UTC time: %02d:%02d:%02d", 
                         hour(t_timeUNIX), minute(t_timeUNIX), second(t_timeUNIX));
      _logger.debugln("GET-NTP-TIME", _s_logStr);
      _logger.debugln("GET-NTP-TIME", "");
    }
  }
  
  return t_timeUNIX;
}

// ___________________________________ _wsNotFoundHandler ___________________________
// - Is called on any not defined URL
// __________________________________________________________________________________
void ESP8266FWClass::_wsNotFoundHandler() {
  _logger.infoln("WS-NOT-FOUND-HANDLER", "");
  _logger.infoln("WS-NOT-FOUND-HANDLER", "Not Found Handler");

  logDetails(INFO);

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += _webServer.uri();
  message += "\nMethod: ";
  message += (_webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += _webServer.args();
  message += "\n";

  for (uint8_t i = 0; i < _webServer.args(); i++) {
    message += " " + _webServer.argName ( i ) + ": " + _webServer.arg ( i ) + "\n";
  }

  _webServer.send(404, "text/plain", message);
}

// ___________________________________ _renewTimestampAndSave _______________________
// - Renew the local timestamp and its approx value and save config to EEPROM
// __________________________________________________________________________________
void ESP8266FWClass::_renewTimestampAndSave() {
  time_t localTime = getLocalTime();
  
  saveConfig();
}

//___________________________________________________________________________________
//___________________________________________________________________________________
// - PUBLIC SINGLETON VARIABLE
//___________________________________________________________________________________
//___________________________________________________________________________________

ESP8266FWClass ESP8266FW;
