/*
  webserver_handlers.ino - FreeDs webserver handlers
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32
  
  Copyright (C) 2020 Pablo Zerón (https://github.com/pablozg/freeds)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

void handleCnet(AsyncWebServerRequest *request)
{
  strcpy(config.ssid1, request->urlDecode(request->arg("wifi1")).c_str());
  strcpy(config.ssid2, request->urlDecode(request->arg("wifi2")).c_str());
  strcpy(config.pass1, request->urlDecode(request->arg("wifip1")).c_str());
  strcpy(config.pass2, request->urlDecode(request->arg("wifip2")).c_str());

  if (request->hasArg("wifis"))
  {
    if (config.wversion == 2)
    {
      strcpy(config.ssid_esp01, request->urlDecode(request->arg("wifis")).c_str());
    }
    // if (config.wversion == 1 || config.wversion == 9 || config.wversion == 10 || config.wversion == 11)
    if (config.wversion == 1 || (config.wversion >= 9 && config.wversion <= 12))
    {
      strcpy(config.sensor_ip, request->urlDecode(request->arg("wifis")).c_str());
    }
  }

  String host = request->urlDecode(request->arg("host"));
  host.toLowerCase(); // Nos aseguramos que este en minúsculas
  
  if (host != String(config.hostServer))
  {  
    strcpy(config.hostServer, host.c_str());
    MDNS.end();
    MDNS.begin(config.hostServer);
  }

  if (request->hasArg("ip"))
  {
    strcpy(config.ip, request->urlDecode(request->arg("ip")).c_str());
    strcpy(config.gw, request->urlDecode(request->arg("gw")).c_str());
    strcpy(config.mask, request->urlDecode(request->arg("mask")).c_str());
    strcpy(config.dns1, request->urlDecode(request->arg("dns1")).c_str());
    strcpy(config.dns2, request->urlDecode(request->arg("dns2")).c_str());
  }

  if (request->arg("dhcp") == "on")
  {
    config.dhcp = true;
  }
  else
  {
    config.dhcp = false;
  }

  saveEEPROM();
}

void handleConfig(AsyncWebServerRequest *request)
{
  config.mqtt = false;
  Error.ConexionMqtt = false;

  if (request->arg("mqttactive") == "on")
  {

    config.mqtt = true;

    config.MQTT_port = request->arg("mqttport").toInt();
    strcpy(config.MQTT_broker, request->urlDecode(request->arg("broker")).c_str());
    strcpy(config.MQTT_user, request->urlDecode(request->arg("mqttuser")).c_str());
    strcpy(config.MQTT_password, request->urlDecode(request->arg("mqttpass")).c_str());
    strcpy(config.R01_mqtt, request->urlDecode(request->arg("mqttr1")).c_str());
    strcpy(config.R02_mqtt, request->urlDecode(request->arg("mqttr2")).c_str());
    strcpy(config.R03_mqtt, request->urlDecode(request->arg("mqttr3")).c_str());
    strcpy(config.R04_mqtt, request->urlDecode(request->arg("mqttr4")).c_str());

    if (config.wversion == 3)
    {
      strcpy(config.Solax_mqtt, request->urlDecode(request->arg("solax")).c_str());
      strcpy(config.Meter_mqtt, request->urlDecode(request->arg("meter")).c_str());
    }
  }
  
  if (request->urlDecode(request->arg("oldpass")) == String(config.password))
  {
    strcpy(config.password, request->urlDecode(request->arg("newpass")).c_str());
  }
   INFOLN(String(config.password));

  strcpy(config.remote_api, request->urlDecode(request->arg("remote_api")).c_str());

  config.baudiosMeter = constrain(request->arg("baudiosmeter").toInt(), 300, 38400);
  config.idMeter = constrain(request->arg("idmeter").toInt(), 1, 250);
  SerieMeter.updateBaudRate(config.baudiosMeter);

  config.maxErrorTime = constrain(request->arg("maxerrortime").toInt(), 10000, 60000);
  config.getDataTime = constrain(request->arg("getdatatime").toInt(), 250, 60000);
  switch (config.wversion) {   
    case 0:
    case 1:
    case 9:
    case 10:
    case 11:
    case 12:
      if (config.getDataTime < 2000) config.getDataTime = 2000;
      break;
  }
  Tickers.updatePeriod(5, config.getDataTime);

  config.oledAutoOff = false;
  if (request->arg("autoPowerOff") == "on") {
    timers.OledAutoOff = millis();
    config.oledAutoOff = true;
    config.oledControlTime = constrain(request->arg("autoPowerOffTime").toInt(), 1000, 3000000);
    Tickers.updatePeriod(8, config.oledControlTime);
  } else {
    config.oledPower = true;
    turnOffOled();
  }
   
  saveEEPROM();
  mqttClient.disconnect(true);

  if (config.mqtt)
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
  else
  {
    xTimerStop(mqttReconnectTimer, 0);
  }
}

void handleRelay(AsyncWebServerRequest *request)
{
  if (request->arg("pwmactive") == "on") {
    config.P01_on = true;
  } else {
    config.P01_on = false;
    down_pwm(true);
  }

  config.pwm_min = request->arg("pwmmin").toInt();
  config.pwm_max = request->arg("pwmmax").toInt();
  config.pwmControlTime = constrain(request->arg("looppwm").toInt(), 500, 10000);
  Tickers.updatePeriod(6, config.pwmControlTime);
  config.manualControlPWM = constrain(request->arg("manpwm").toInt(), 0, 100);
  config.autoControlPWM = constrain(request->arg("autopwm").toInt(), 0, 100);
  config.pwmSlaveOn = constrain(request->arg("slavepwm").toInt(), 0, 100);
  if (request->arg("potpwmactive") == "on") {
    config.flags.potManPwmActive = true;
    config.potmanpwm = constrain(request->arg("potmanpwm").toInt(), 0, 9999);
  } else {
    config.flags.potManPwmActive = false;
    config.pwm_man = false;
  }
  
  config.R01_min = request->arg("r01min").toInt();
  config.R02_min = request->arg("r02min").toInt();
  config.R03_min = request->arg("r03min").toInt();
  config.R04_min = request->arg("r04min").toInt();

  config.R01_poton = request->arg("r01poton").toInt();
  config.R02_poton = request->arg("r02poton").toInt();
  config.R03_poton = request->arg("r03poton").toInt();
  config.R04_poton = request->arg("r04poton").toInt();

  config.R01_potoff = request->arg("r01potoff").toInt();
  config.R02_potoff = request->arg("r02potoff").toInt();
  config.R03_potoff = request->arg("r03potoff").toInt();
  config.R04_potoff = request->arg("r04potoff").toInt();

  request->arg("R01_man") == "on" ? config.R01_man = true : config.R01_man = false;
  request->arg("R02_man") == "on" ? config.R02_man = true : config.R02_man = false;
  request->arg("R03_man") == "on" ? config.R03_man = true : config.R03_man = false;
  request->arg("R04_man") == "on" ? config.R04_man = true : config.R04_man = false;

  config.pwmFrequency = constrain(request->arg("frecpwm").toInt(), 500, 3000);
  ledcWriteTone(2, config.pwmFrequency);

  relay_control_man(false); // Control de relays

  saveEEPROM();
}

String API(void)
{
  int8_t status0 = 0;
  int8_t status1 = 0; //Reserve to future
  // Return a byte with status
  // byte RL4|RL3|RL2|RL1|Error.ConexionMqtt|Error.ConexionWifi|Error.ConexionInversor|

  if (Error.ConexionInversor)
    status0 = status0 + 1;
  if (Error.ConexionWifi)
    status0 = status0 + 2;
  if (Error.ConexionMqtt)
    status0 = status0 + 4;
  status0 = (digitalRead(PIN_RL1) ? 1 : 0) * 8;
  status0 = (digitalRead(PIN_RL2) ? 1 : 0) * 16;
  status0 = (digitalRead(PIN_RL3) ? 1 : 0) * 32;
  status0 = (digitalRead(PIN_RL4) ? 1 : 0) * 64;

  return "{\"Data\":[" + String(httpcode) + "," + String(inverter.pv1c) + "," + String(inverter.pv2c) + "," + String(inverter.pv1v) + "," + String(inverter.pv2v) + "," + String(inverter.pw1) + "," + String(inverter.pw2) + "," + String(inverter.gridv) + "," + String(inverter.wsolar) + "," + String(inverter.wtoday) + "," + String(inverter.wgrid) + "," + String(inverter.wtogrid) + "," + String(invert_pwm) + "," + String(status0) + "," + String(status1) + "]}";
}

void REBOOTCAUSE(void)
{
 INFO("CPU0 reset reason: ");
 verbose_print_reset_reason(rtc_get_reset_reason(0));

 INFO("CPU1 reset reason: ");
 verbose_print_reset_reason(rtc_get_reset_reason(1));
}

String sendJsonWeb(void)
{
  DynamicJsonDocument jsonValues(512);
  uint8_t error = 0;
  uint8_t wversion = 0;

  if (Error.ConexionWifi)
    error = 1;
  if (Error.ConexionMqtt && config.mqtt)
    error = 2;
  if (Error.ConexionInversor)
    error = 3;
  if (Error.LecturaDatos)
    error = 4;
  if (!config.mqtt && config.wversion == 3)
    error = 5;

  jsonValues["error"] = error;
  jsonValues["R01"] = digitalRead(PIN_RL1);
  jsonValues["R02"] = digitalRead(PIN_RL2);
  jsonValues["R03"] = digitalRead(PIN_RL3);
  jsonValues["R04"] = digitalRead(PIN_RL4);
  jsonValues["Oled"] = config.oledPower;
  jsonValues["oledBrightness"] = config.oledBrightness;
  jsonValues["POn"] = config.P01_on;
  jsonValues["PwmMan"] = config.pwm_man;
  jsonValues["Msg"] = Message;
  jsonValues["pwmfrec"] = config.pwmFrequency;
  jsonValues["pwm"] = pro;
  jsonValues["baudiosMeter"] = config.baudiosMeter;

  if (config.wversion == 12) {
    jsonValues["wversion"] = masterMode;
    wversion = masterMode;
  } else {
    jsonValues["wversion"] = config.wversion;
    wversion = config.wversion;
  }

  char tmpString[33];
  dtostrfd(inverter.wgrid, 2, tmpString);
  jsonValues["wgrid"] = tmpString;
  
  switch(wversion)
  {
    case 4:
    case 5:
    case 6:
      dtostrfd(meter.voltage, 2, tmpString);
      jsonValues["mvoltage"] = tmpString;
      dtostrfd(meter.current, 2, tmpString);
      jsonValues["mcurrent"] = tmpString;
      dtostrfd(meter.powerFactor, 2, tmpString);
      jsonValues["mpowerFactor"] = tmpString;
      dtostrfd(meter.frequency, 2, tmpString);
      jsonValues["mfrequency"] = tmpString;
      dtostrfd(meter.importActive, 2, tmpString);
      jsonValues["mimportActive"] = tmpString;
      dtostrfd(meter.exportActive, 2, tmpString);
      jsonValues["mexportActive"] = tmpString;
      break;
    case 9:
    case 10:
      dtostrfd(meter.voltage, 2, tmpString);
      jsonValues["mvoltage"] = tmpString;
      dtostrfd(meter.powerFactor, 2, tmpString);
      jsonValues["mpowerFactor"] = tmpString;
      dtostrfd(inverter.wsolar, 2, tmpString);
      jsonValues["wsolar"] = tmpString;
      dtostrfd(meter.importActive, 2, tmpString);
      jsonValues["mimportActive"] = tmpString;
      dtostrfd(meter.exportActive, 2, tmpString);
      jsonValues["mexportActive"] = tmpString;
      break;
    default:
      dtostrfd(inverter.wtoday, 2, tmpString);
      jsonValues["wtoday"] = tmpString;
      dtostrfd(inverter.wsolar, 2, tmpString);
      jsonValues["wsolar"] = tmpString;
      dtostrfd(inverter.gridv, 2, tmpString);
      jsonValues["gridv"] = tmpString;
      dtostrfd(inverter.pv1c, 2, tmpString);
      jsonValues["pv1c"] = tmpString;
      dtostrfd(inverter.pv1v, 2, tmpString);
      jsonValues["pv1v"] = tmpString;
      dtostrfd(inverter.pw1, 2, tmpString);
      jsonValues["pw1"] = tmpString;
      dtostrfd(inverter.pv2c, 2, tmpString);
      jsonValues["pv2c"] = tmpString;
      dtostrfd(inverter.pv2v, 2, tmpString);
      jsonValues["pv2v"] = tmpString;
      dtostrfd(inverter.pw2, 2, tmpString);
      jsonValues["pw2"] = tmpString;
      break;
  }

  String response;
  serializeJson(jsonValues, response);

  return response;
}

String sendMasterData(void)
{
  DynamicJsonDocument jsonValues(512);
  
  jsonValues["wversion"] = config.wversion;
  jsonValues["PwmMaster"] = pro.toInt();
  
  char tmpString[33];
  dtostrfd(inverter.wgrid, 2, tmpString);
  jsonValues["wgrid"] = tmpString;
  
  switch(config.wversion)
  {
    case 4:
    case 5:
    case 6:
      dtostrfd(meter.voltage, 2, tmpString);
      jsonValues["mvoltage"] = tmpString;
      dtostrfd(meter.current, 2, tmpString);
      jsonValues["mcurrent"] = tmpString;
      dtostrfd(meter.powerFactor, 2, tmpString);
      jsonValues["mpowerFactor"] = tmpString;
      dtostrfd(meter.frequency, 2, tmpString);
      jsonValues["mfrequency"] = tmpString;
      dtostrfd(meter.importActive, 2, tmpString);
      jsonValues["mimportActive"] = tmpString;
      dtostrfd(meter.exportActive, 2, tmpString);
      jsonValues["mexportActive"] = tmpString;
      dtostrfd(meter.energyTotal, 2, tmpString);
      jsonValues["menergyTotal"] = tmpString;
      dtostrfd(meter.activePower, 2, tmpString);
      jsonValues["mactivePower"] = tmpString;
      dtostrfd(meter.aparentPower, 2, tmpString);
      jsonValues["maparentPower"] = tmpString;
      dtostrfd(meter.reactivePower, 2, tmpString);
      jsonValues["mreactivePower"] = tmpString;
      dtostrfd(meter.importReactive, 2, tmpString);
      jsonValues["mimportReactive"] = tmpString;
      dtostrfd(meter.exportReactive, 2, tmpString);
      jsonValues["mexportReactive"] = tmpString;
      dtostrfd(meter.phaseAngle, 2, tmpString);
      jsonValues["mphaseAngle"] = tmpString;
      break;
    case 9:
    case 10:
      dtostrfd(meter.voltage, 2, tmpString);
      jsonValues["mvoltage"] = tmpString;
      dtostrfd(meter.powerFactor, 2, tmpString);
      jsonValues["mpowerFactor"] = tmpString;
      dtostrfd(inverter.wsolar, 2, tmpString);
      jsonValues["wsolar"] = tmpString;
      dtostrfd(meter.importActive, 2, tmpString);
      jsonValues["mimportActive"] = tmpString;
      dtostrfd(meter.exportActive, 2, tmpString);
      jsonValues["mexportActive"] = tmpString;
      dtostrfd(meter.activePower, 2, tmpString);
      jsonValues["mactivePower"] = tmpString;
      dtostrfd(meter.reactivePower, 2, tmpString);
      jsonValues["mreactivePower"] = tmpString;
      dtostrfd(inverter.gridv, 2, tmpString);
      jsonValues["gridv"] = tmpString;
      break;
    default:
      dtostrfd(inverter.wtoday, 2, tmpString);
      jsonValues["wtoday"] = tmpString;
      dtostrfd(inverter.wsolar, 2, tmpString);
      jsonValues["wsolar"] = tmpString;
      dtostrfd(inverter.gridv, 2, tmpString);
      jsonValues["gridv"] = tmpString;
      dtostrfd(inverter.pv1c, 2, tmpString);
      jsonValues["pv1c"] = tmpString;
      dtostrfd(inverter.pv1v, 2, tmpString);
      jsonValues["pv1v"] = tmpString;
      dtostrfd(inverter.pw1, 2, tmpString);
      jsonValues["pw1"] = tmpString;
      dtostrfd(inverter.pv2c, 2, tmpString);
      jsonValues["pv2c"] = tmpString;
      dtostrfd(inverter.pv2v, 2, tmpString);
      jsonValues["pv2v"] = tmpString;
      dtostrfd(inverter.pw2, 2, tmpString);
      jsonValues["pw2"] = tmpString;
      break;
  }

  String response;
  serializeJson(jsonValues, response);

  return response;
}

void checkAuth(AsyncWebServerRequest *request)
{
  char *toDecode = config.password;
  unsigned char *decoded = base64_decode((const unsigned char *)toDecode, strlen(toDecode), &outputLength);

  if (!request->authenticate(www_username, (const char *)decoded))
    return request->requestAuthentication();
}

void send_events()
{
  events.send(print_Uptime().c_str(), "uptime");
  events.send(sendJsonWeb().c_str(), "jsonweb");
}

void setWebConfig(void)
{
  //////////// PAGINAS EN LA MEMORIA SPPIFS ////////
  server.on("/", [](AsyncWebServerRequest *request) {
    checkAuth(request);
    config.wifi ? request->send(SPIFFS, "/index.html", "text/html", false, processorFreeDS) : request->send(SPIFFS, "/Red.html", "text/html", false, processorRed);
    if (Flags.reboot)
    {
      restartFunction();
    }
  });

  server.on("/Red.html", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    checkAuth(request);
    request->send(SPIFFS, "/Red.html", "text/html", false, processorRed);
  });

  server.on("/Config.html", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    checkAuth(request);
    request->send(SPIFFS, "/Config.html", "text/html", false, processorConfig);
  });

  server.on("/Salidas.html", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    checkAuth(request);
    request->send(SPIFFS, "/Salidas.html", "text/html", false, processorSalidas);
  });

  server.on("/Ota.html", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    checkAuth(request);
    request->send(SPIFFS, "/Ota.html", "text/html", false, processorOta);
  });

  server.on("/weblog.html", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    checkAuth(request);
    request->send(SPIFFS, "/weblog.html", "text/html", false, processorOta);
  });

  //////////// JAVASCRIPT EN LA MEMORIA SPPIFS ////////
  
  server.on("/sb-admin-2.min.js", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/sb-admin-2.min.js.jgz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  server.on("/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/bootstrap.bundle.min.js.jgz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  server.on("/jquery-3.4.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/jquery-3.4.1.min.js.jgz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  server.on("/freeds.min.js", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/freeds.min.js.jgz", "application/javascript");
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  //////////// CSS EN LA MEMORIA SPPIFS ////////
  
  server.on("/sb-admin-2.min.css", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/sb-admin-2.min.css.jgz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  server.on("/all.min.css", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/all.min.css.jgz", "text/css");
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  //////////// FUENTES EN LA MEMORIA SPPIFS ////////

  server.on("/fa-solid-900.woff", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/fa-solid-900.woff", "text/plain");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  server.on("/fa-solid-900.woff2", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/fa-solid-900.woff2", "text/plain");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  server.on("/fa-solid-900.ttf", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/fa-solid-900.ttf.jgz", "text/plain");
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/favicon.ico", "image/x-icon");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  server.on("/freeds.png", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/freeds.png.jgz", "image/png");
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Cache-Control", "max-age=86400, must-revalidate");
    request->send(response);
  });

  //////// RESPUESTAS A LAS PULSACIONES DE LOS BOTONES //////////////////

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    checkAuth(request);
    Message = 2;
    Flags.reboot = true;
    request->redirect("/");
  });

  server.on("/selectversion", HTTP_POST, [](AsyncWebServerRequest *request) {
    //checkAuth(request);
    config.wversion = request->arg("data").toInt();
    switch (config.wversion) {
      case 3:
        xTimerStart(mqttReconnectTimer, 0);
        suscribeMqttMeter();
        break;
      
      case 0:
      case 1:
      case 9:
      case 10:
      case 11:
      case 12:
        if (config.getDataTime < 2000) config.getDataTime = 2000;
        break;
    }

    saveEEPROM();

    AsyncWebServerResponse *response = request->beginResponse(200);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/brightness", HTTP_POST, [](AsyncWebServerRequest *request) {
    //checkAuth(request);
    config.oledBrightness = request->arg("data").toInt();
    config.oledPower = true;
    Flags.setBrightness = true;
    AsyncWebServerResponse *response = request->beginResponse(200);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/tooglebuttons", HTTP_POST, [](AsyncWebServerRequest *request) {
    //checkAuth(request);
    uint8_t button = request->arg("data").toInt();

    DEBUG("Comando recibido Nº ");
    DEBUGLN(button);

    switch (button)
    {
    case 1: // Activación Manual Relé 1
      Flags.Relay01Man = !Flags.Relay01Man;
      break;
    case 2: // Activación Manual Relé 2
      Flags.Relay02Man = !Flags.Relay02Man;
      break;
    case 3: // Activación Manual Relé 3
      Flags.Relay03Man = !Flags.Relay03Man;
      break;
    case 4: // Activación Manual Relé 4
      Flags.Relay04Man = !Flags.Relay04Man;
      break;
    case 5: // Encender / Apagar OLED
      timers.OledAutoOff = millis();
      config.oledPower = !config.oledPower;
      turnOffOled();
      saveEEPROM();
      break;
    case 6: // Encender / Apagar PWM
      config.P01_on = !config.P01_on;
      saveEEPROM();
      break;
    case 7: // Encender / Apagar PWM Manual
      config.pwm_man = !config.pwm_man;
      saveEEPROM();
      break;
    }

    if (button > 0 && button < 5)
    {
      relay_control_man(false);
    }
    AsyncWebServerResponse *response = request->beginResponse(200);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", API());
    response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/masterdata", HTTP_GET, [](AsyncWebServerRequest *request) { // GET
    AsyncWebServerResponse *response = request->beginResponse(200, "text/json", sendMasterData());
    //response->addHeader("Connection", "close");
    request->send(response);
  });

  server.on("/handlecmnd", HTTP_POST, [](AsyncWebServerRequest *request) {
    
    if (request->arg("webcmnd") == "rebootcause") { REBOOTCAUSE(); }
    if (request->arg("webcmnd") == "getFreeHeap")
    { 
      String freeheap;
      freeheap = ESP.getFreeHeap();
      freeheap += "{n}";
      events.send(freeheap.c_str(), "weblog");
    }
    
    AsyncWebServerResponse *response = request->beginResponse(200);
    response->addHeader("Connection", "close");
    request->send(response);
  });

  ///////// RESPUESTAS A LAS PÁGINAS DE CONFIGURACIÓN ////////

  server.on("/handleCnet", HTTP_POST, [](AsyncWebServerRequest *request) {
    checkAuth(request);
    handleCnet(request);
    Message = 1;
    request->redirect("/");
    if (!config.wifi)
    {
      config.wifi = true;
      restartFunction();
    }
  });

  server.on("/handleConfig", HTTP_POST, [](AsyncWebServerRequest *request) {
    //checkAuth(request);
    handleConfig(request);
    Message = 1;
    request->redirect("/");
  });

  server.on("/handleRelay", HTTP_POST, [](AsyncWebServerRequest *request) {
    //checkAuth(request);
    handleRelay(request);
    Message = 1;
    request->redirect("/");
  });

  server.on("/factoryDefaults", HTTP_POST, [](AsyncWebServerRequest *request) {
    checkAuth(request);
    request->redirect("/");
    defaultValues();
    restartFunction();
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
      
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.printf("Update Start: %s\n", filename.c_str());
      for (int i = 1; i <= 5; i++) { Tickers.disable(i); }
      Flags.Updating = true;
      config.P01_on = false;
      down_pwm(false);
      if (filename == "spiffs.bin") {
        SPIFFS.end();
        if(!Update.begin(UPDATE_SIZE_UNKNOWN, 100)) { Update.printError(Serial); }
      } else {
        if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) { Update.printError(Serial); }
      }
    }
    if (Update.hasError()) {
      request->send(500);
      Update.printError(Serial);
      ESP.restart();
    } else {
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if (final) {
      if(Update.end(true)){
        request->send(200);
        Serial.printf("Update Success: %uB\n", index+len);
        delay(500);
        ESP.restart();
      } else {
        Update.printError(Serial);
      }
    } });

  events.onConnect([](AsyncEventSourceClient *client) {
    if (client->lastId())
    {
      Serial.print("Client reconnected! Last message ID that it get is: ");
      Serial.println(client->lastId());
    }
    client->send("Hello!", NULL, millis(), 1000);
    Flags.eventsConnected = true;
  });

  // attach EventSource
  server.addHandler(&events);

  String mdnsUrl = "http://";
  mdnsUrl += String(config.hostServer);
  mdnsUrl += ".local";
  mdnsUrl.toLowerCase();

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", mdnsUrl);
  server.begin();
}