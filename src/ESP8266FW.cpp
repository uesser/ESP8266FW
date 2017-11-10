include "ESP8266FW.h"

// ___________________________________ ESP8266FWClass _______________________________
// - constructor
// __________________________________________________________________________________
ESP8266FWClass::ESP8266FWClass()
,_otaPort(0)
,_otaInProgress(false)
,_ntpPort(0)
,_ntpSyncInterval(0)
,_logSer(logSer)
,_logHost(logHost)
,_logPort(logPort)
{
  _data.saved           = 0x0;
  _data.localTimeApprox = 0;
  _data.userData        = 0;

  _loadConfig();

#ifndef ENERGY_EFFICIENT
  _localTime_saveConfigTicker.attach_ms(C_MINUTE_MS, _renewTimestampAndSave);
#endif
}

// ___________________________________ ~ESP8266FWClass ______________________________
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

#ifndef ENERGY_EFFICIENT
  _logger.unregLogDestSerial(_logSer);
  _logger.unregLogDestWifi(_logHost, _logPort);
#endif
}

// ___________________________________ setupLogger __________________________________
// - sets logging stuff
// __________________________________________________________________________________
void ESP8266FWClass::setupLogger(LogSerial logSer, String logHost, String logPort,
                                 String logURL, String logFileName, String logLevelParam,
                                 String logFunctionParam, String logStrParam,
                                 String logStrlnParam) {
#ifndef ENERGY_EFFICIENT
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
#endif
}

// ___________________________________ setupWifi ____________________________________
// - sets ssid, ssid password and hostname
// __________________________________________________________________________________
void ESP8266FWClass::setupWifi(char* ssid, char* ssidPwd, char* hostName) {
  if (_ssid) {
    free(_ssid);
  }
  _ssid = (char *) malloc(sizeof(ssid));
  strcpy(_ssid, ssid);

  if (_ssidPwd) {
    free(_ssidPwd);
  }
  _ssidPwd = (char *) malloc(sizeof(pwd));
  strcpy(_ssidPwd, ssidPwd);

  if (_hostName) {
    free(_hostName);
  }
  if (! hostName || strlen(hostName) == 0) {
    char tmp[15];
    sprintf(tmp, "esp8266-%06x", ESP.getChipId());
    hostName = tmp;
  }
  _hostName = (char *) malloc(sizeof(hostName));
  strcpy(_hostName, hostName);
}

// ___________________________________ wifiConnect __________________________________
// - connects to WiFi
// __________________________________________________________________________________
boolean ESP8266FWClass::wifiConnect() {
#ifndef ENERGY_EFFICIENT
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", "");
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", String("----------------------------------------------------"));
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", String("ESP8266 (re)starting or reconnecting after WiFi lost"));
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", "");
  _logger.infoln(_logSerIdx, "WIFI-CONNECT", String("Connecting to AP '") + _ssid + "': ");
#endif

  WiFi.hostname(_hostName);
  WiFi.mode(WIFI_STA);
//  wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.begin(_ssid, _ssidPwd);

  int connectTicks = 0;
  while (WiFi.status() != WL_CONNECTED && connectTicks++ < 20) {
    delay(500);
#ifndef ENERGY_EFFICIENT
    Serial.print(".");
    Serial1.print(".");
#endif
  }
#ifndef ENERGY_EFFICIENT
  Serial.println();
  Serial1.println();
#endif

  if (WiFi.status() == WL_CONNECTED) {
#ifndef ENERGY_EFFICIENT
    IPAddress IPAddr = WiFi.localIP();

    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", "");
    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", String("----------------------------------------------------"));
    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", String("ESP8266 (re)starting or reconnecting after WiFi lost"));
    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", "");
    _logger.infoln(_logWifiIdx, "WIFI-CONNECT", String("Connecting to AP: ") + _ssid);

    _logger.infoln("WIFI-CONNECT", String("Time to connect: ") + (connectTicks * 500) + " ms");
    _logger.infoln("WIFI-CONNECT", String("IP-Adr: ") + IPAddr[0] + "." + IPAddr[1] + "." + IPAddr[2] + "." + IPAddr[3]);
//    WiFi.printDiag(Serial);
#endif

    return true;
  }
  else {
#ifndef ENERGY_EFFICIENT
    _logger.errorln("WIFI-CONNECT", "");
    _logger.errorln("WIFI-CONNECT", "Connect to WiFi failed!!!");
#endif

    return false;
  }
}

// ___________________________________ checkWifiReconnect ___________________________
// - check if connected to WiFi and try reconnecting if not
// __________________________________________________________________________________
boolean ESP8266FWClass::checkWifiReconnect() {
  if (WiFi.status() != WL_CONNECTED) {
#ifndef ENERGY_EFFICIENT
    _logger.warnln("CHECK-WIFI-RECONNECT", "");
    _logger.warnln("CHECK-WIFI-RECONNECT", "Connection to Wifi lost. Trying to reconnect.");
#endif

    return wifiConnect();
  }
  else {          // already/still connected
    return true;
  }
}

// ___________________________________ otaConnect ___________________________________
// - setup of OTA (programming over the air) stuff
// __________________________________________________________________________________
boolean ESP8266FWClass::otaConnect(uint16_t otaPort, char * otaPwd) {
#ifndef ENERGY_EFFICIENT
  _logger.infoln("OTA-CONNECT", "");
  _logger.infoln("OTA-CONNECT", String("setup OTA - hostname: ") + _hostname);
#endif

  if (WiFi.status() != WL_CONNECTED) {
#ifndef ENERGY_EFFICIENT
    _logger.errorln("OTA-CONNECT", "Wifi not connected!!!");
#endif

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

#ifndef ENERGY_EFFICIENT
    Serial.println();
    Serial.println("OTA - Start");
    Serial1.println();
    Serial1.println("OTA - Start");
#endif
  });

  ArduinoOTA.onEnd([]() {
    _otaInProgress = false;

#ifndef ENERGY_EFFICIENT
    Serial.println();
    Serial.println("OTA - End");
    Serial.println("Rebooting...");
    Serial.println();
    Serial1.println();
    Serial1.println("OTA - End");
    Serial1.println("Rebooting...");
    Serial1.println();
#endif
  });

#ifndef ENERGY_EFFICIENT
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
    Serial1.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
#endif

  ArduinoOTA.onError([](ota_error_t error) {
    _otaInProgress = false;

#ifndef ENERGY_EFFICIENT
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
#endif
  });

  _otaInProgress = false;

  ArduinoOTA.begin();

#ifndef ENERGY_EFFICIENT
  _logger.infoln("OTA-CONNECT", "OTA begin");
#endif

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

#ifndef ENERGY_EFFICIENT
    _logger.infoln("NTP-CONNECT", "");
    _logger.infoln("NTP-CONNECT", String("Connecting to ntp server: ") + _ntpHost);
#endif

    if (! WiFi.hostByName(_ntpHost, _ntpTimeServerIP)) {  // Get the IP address of the NTP server
      _ntpTimeServerIP = INADDR_NONE;
#ifndef ENERGY_EFFICIENT
      _logger.errorln("NTP-CONNECT", "DNS lookup failed!!!");
#endif
    }
#ifndef ENERGY_EFFICIENT
    else {
      _logger.infoln("NTP-CONNECT", String("Time server IP: ") + _ntpTimeServerIP[0] + "." +
                     _ntpTimeServerIP[1] + "." + _ntpTimeServerIP[2] + "." + _ntpTimeServerIP[3]);
    }
#endif

    UDP.begin(_ntpPort);  // Start listening for UDP messages on port "_ntpPort" (default is 123)

#ifndef ENERGY_EFFICIENT
    _logger.infoln("NTP-CONNECT", String("Local port: ") + UDP.localPort());

    _logger.infoln("NTP-CONNECT", String("Calling syncronizing function 'ESP8266FW::getNtpTime' every ") +
                   _ntpSyncInterval + " seconds");
#endif

    setSyncProvider(getNtpTime);
    setSyncInterval(_ntpSyncInterval);
  }
#ifndef ENERGY_EFFICIENT
  else {
    _logger.warnln("NTP-CONNECT", "");
    _logger.warnln("NTP-CONNECT", "No NTP-Host defined in parameter 'ntpHost'!!!");
  }
#endif
}

// ___________________________________ mDNSConnect __________________________________
// - setup of mDNS responder
// __________________________________________________________________________________
boolean ESP8266FWClass::mDNSConnect() {
  if (WiFi.status() != WL_CONNECTED) {
#ifndef ENERGY_EFFICIENT
    _logger.errorln("MDNS-CONNECT", "Wifi not connected!!!");
#endif

    return false;
  }

  if (! MDNS.begin(_hostName)) {  // Start the mDNS responder for <_hostname>.local
#ifndef ENERGY_EFFICIENT
    _logger.errorln("MDNS-CONNECT", "Error setting up MDNS responder!!!");
#endif

    return false;
  }
  else {
#ifndef ENERGY_EFFICIENT
    _logger.infoln("MDNS-CONNECT", String("mDNS responder started - Hostname: ") + _hostname + ".local");
#endif

    return true;
  }
}

// ___________________________________ webserverConnect _____________________________
// - setup of Webserver
// __________________________________________________________________________________
boolean ESP8266FWClass::webserverConnect(int port, callback_function wsRootHandler,
                                         callback_function wsNotFoundHandler) {
#ifndef ENERGY_EFFICIENT
  _logger.infoln("WEBSERVER-CONNECT", "");
  _logger.infoln("WEBSERVER-CONNECT", "setup Webserver...");
#endif

  _webServer = new ESP8266WebServer(Port);

  if (wsRootHandler)
    _webServer.on("/", wsRootHandler);

  if (wsNotFoundHandler)
    _webServer.onNotFound(wsNotFoundHandler);
  else
    _webServer.onNotFound(_wsNotFoundHandler);

  _webServer.begin();

#ifndef ENERGY_EFFICIENT
  _logger.infoln("WEBSERVER-CONNECT", String("Webserver started, listening on port ") + String(Port));
#endif

  return true;
}

// ___________________________________ logWSDetails _________________________________
// - Logs webserver details
// __________________________________________________________________________________
void ESP8266FWClass::logWSDetails(LogLevel logLev) {
#ifndef ENERGY_EFFICIENT
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
#endif
}

// ___________________________________ setupConnectAll ______________________________
// - setup and connect all (wifi, ota, ...)
// __________________________________________________________________________________
boolean ESP8266FWClass::setupConnectAll(LogSerial logSer, String logHost, String logPort,
                                        String logURL, String logFileName, String logLevelParam,
                                        String logFunctionParam, String logStrParam, String logStrlnParam,
                                        char* ssid, char* ssidPwd, char* hostName,
                                        uint16_t otaPort, char * otaPwd,
                                        char * ntpHost, uint16_t ntpPort, uint16_t ntpSyncInterval,
                                        int port, callback_function wsRootHandler,
                                        callback_function wsNotFoundHandler) {
  boolean connectOK = true;

  setupLogger(logSer, logHost, logPort, logURL, logFileName, logLevelParam,
              logFunctionParam, logStrParam, logStrlnParam);

  setupWifi(ssid, ssidPwd, hostName);

  if (! wifiConnect())
    connectOK = false;

  if (! otaConnect(otaPort, otaPwd);
    connectOK = false;

  if (! mDNSConnect())
    connectOK = false;

  if (! ntpConnect(ntpHost, ntpPort, ntpSyncInterval))
    connectOK = false;

  if (! webserverConnect(port, wsRootHandler, wsNotFoundHandler))
    connectOK = false;

  return connectOK;
}

// ___________________________________ getLocalTime _________________________________
// - gets the local time
// __________________________________________________________________________________
time_t ESP8266FWClass::getLocalTime() {
  static TimeChangeRule   CEST = { "CEST", Last, Sun, Mar, 2, 120 };  // Central European Summer Time
  static TimeChangeRule   CET  = { "CET",  Last, Sun, Oct, 3,  60 };  // Central European Standard Time
  static Timezone         CE(CEST, CET);
  static TimeChangeRule * tcr;  //pointer to the time change rule, use to get the TZ abbrev

  return CE.toLocal(now(), &tcr);
}

// ___________________________________ deepSleep ____________________________________
// - Sends esp8266 to deep sleep for sleepTime microseconds
// __________________________________________________________________________________
void ESP8266FWClass::deepSleep(uint32_t sleepTime) {
  if (sleepTime > C_MICROSECS_PER_HOUR)
    sleepTime = C_MICROSECS_PER_HOUR;

  // set the localTimeApprox to the time the esp8266 will wake up
  setTime(getLocalTime() + sleepTime / 1000 / 1000);
  _saveConfig();

  ESP.deepSleep(sleepTime);
  delay(5000);
}

// ___________________________________ loadUserConfig _______________________________
// - Loads config from RTC memory
// __________________________________________________________________________________
template <class T> boolean ESP8266FWClass::loadUserConfig(T* userData) {
  EEPROM.begin(512);
  EEPROM.get(512, userData);
  EEPROM.end();

#ifndef ENERGY_EFFICIENT
  _logger.debugln("LOAD-USER-CONFIG", "");
  _logger.debugln("LOAD-USER-CONFIG", "User config loaded successfully");
#endif

  return true;
}

// ___________________________________ saveUserConfig _______________________________
// - Saves config to RTC memory
// __________________________________________________________________________________
template <class T> boolean ESP8266FWClass::saveUserConfig(T* userData) {
  EEPROM.begin(512);
  EEPROM.put(512, userData);
  delay(200);
  EEPROM.commit();
  EEPROM.end();

#ifndef ENERGY_EFFICIENT
  _logger.debugln("SAVE-USER-CONFIG", "");
  _logger.logln("SAVE-USER-CONFIG", "User config saved successfully");
#endif

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
  uint32_t   ui_ntpTime  = 0;
  time_t     t_timeUNIX  = 0;

  while (UDP.parsePacket() > 0) ; // discard any previously received packets

#ifndef ENERGY_EFFICIENT
    _logger.debugln("GET-NTP-TIME", "");
    _logger.debugln("GET-NTP-TIME", "Sending NTP request ...");
#endif

    if (_ntpTimeServerIP == INADDR_NONE) {
      if (! WiFi.hostByName(_ntpHost, _ntpTimeServerIP)) {  // Get the IP address of the NTP server
#ifndef ENERGY_EFFICIENT
        _logger.errorln("NTP-CONNECT", "DNS lookup failed!!!");
#endif
      }
#ifndef ENERGY_EFFICIENT
      else {
        _logger.infoln("NTP-CONNECT", String("Time server IP: ") + _ntpTimeServerIP[0] + "." +
                       _ntpTimeServerIP[1] + "." + _ntpTimeServerIP[2] + "." + _ntpTimeServerIP[3]);
      }
#endif
    }

    for (int i = 1; i <= 2; i++) {
      sendNTPpacket(_ntpTimeServerIP);

      uint32_t ui_beginWait = millis();
      while (millis() - ui_beginWait < 3000) {
        if (UDP.parsePacket() >= _C_NTP_PACKET_SIZE) {
#ifndef ENERGY_EFFICIENT
          _logger.debugln("GET-NTP-TIME", "Receive NTP Response");
#endif

          UDP.read(_ntpBuffer, _C_NTP_PACKET_SIZE);  // read packet into the buffer

          // convert four bytes starting at location 40 to a long integer
          ui_ntpTime = (_ntpBuffer[40] << 24) | (_ntpBuffer[41] << 16) |
                       (_ntpBuffer[42] <<  8) |  _ntpBuffer[43];

          t_timeUNIX = ui_ntpTime - C_SEVENTY_YEARS;

          break;
        }
      }

      if (t_timeUNIX > 0)
        break;
    }

#ifndef ENERGY_EFFICIENT
    if (t_timeUNIX == 0) {
      _logger.debugln("GET-NTP-TIME", "No NTP Response!");
    }
    else {
      sprintf(_s_logStr, "UTC time: %02d.%02d.%04d - %02d:%02d:%02d",
                         day(t_timeUNIX), month(t_timeUNIX), year(t_timeUNIX),
                         hour(t_timeUNIX), minute(t_timeUNIX), second(t_timeUNIX));
      _logger.debugln("GET-NTP-TIME", _s_logStr);
    }
#endif
  }

  return t_timeUNIX;
}

// ___________________________________ _wsNotFoundHandler ___________________________
// - Is called on any not defined URL
// __________________________________________________________________________________
void ESP8266FWClass::_wsNotFoundHandler() {
#ifndef ENERGY_EFFICIENT
  _logger.infoln("WS-NOT-FOUND-HANDLER", "");
  _logger.infoln("WS-NOT-FOUND-HANDLER", "Not Found Handler");

  logDetails(INFO);
#endif

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
  _saveConfig();
}

//___________________________________________________________________________________
//___________________________________________________________________________________
// - PRIVATE METHODS
//___________________________________________________________________________________
//___________________________________________________________________________________

// ___________________________________ _loadConfig __________________________________
// - Loads config from RTC memory
// __________________________________________________________________________________
boolean ESP8266FWClass::_loadConfig() {
  _data.saved = 0x0;

  EEPROM.begin(512);
  EEPROM.get(0, _data);
  EEPROM.end();

  if (_data.saved != 0xAAAA) {
#ifndef ENERGY_EFFICIENT
    _logger.debugln("LOAD-CONFIG", "");
    _logger.debugln("LOAD-CONFIG", "Load config failed or not saved last time !!!");
#endif

    return false;
  }
  else {
    setTime(_data.localTimeApprox);

#ifndef ENERGY_EFFICIENT
    _logger.debugln("LOAD-CONFIG", "");
    _logger.debugln("LOAD-CONFIG", "Config loaded successfully");
#endif

    return true;
  }
}

// ___________________________________ _saveConfig __________________________________
// - Saves config to RTC memory
// __________________________________________________________________________________
boolean ESP8266FWClass::_saveConfig() {
  _data.saved           = 0xAAAA;
  _data.localTimeApprox = getLocalTime();

  EEPROM.begin(512);
  EEPROM.put(0, _data);
  delay(200);
  EEPROM.commit();
  EEPROM.end();

#ifndef ENERGY_EFFICIENT
  _logger.debugln("SAVE-CONFIG", "");
  _logger.logln("SAVE-CONFIG", "Config saved successfully");
#endif

  return true;
}

//___________________________________________________________________________________
//___________________________________________________________________________________
// - PUBLIC SINGLETON VARIABLE
//___________________________________________________________________________________
//___________________________________________________________________________________

ESP8266FWClass ESP8266FW;
