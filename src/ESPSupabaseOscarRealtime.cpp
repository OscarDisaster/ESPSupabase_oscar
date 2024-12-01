#include "ESP_SupabaseOscar_Realtime.h"

// Internal functions
String getEventTable(DynamicJsonDocument &result)
{
  String table = result["payload"]["data"]["table"];
  return table;
}

String getEventType(DynamicJsonDocument &result)
{
  String type = result["payload"]["data"]["type"];
  return type;
}

int SupabaseRealtime::_login_process()
{
  HTTPClient Loginhttps;
  WiFiClientSecure *clientLogin = new WiFiClientSecure();

  clientLogin->setInsecure();

  int httpCode;
  DynamicJsonDocument doc(1024); // Adjusted size for login
  String url = "https://" + hostname + "/auth/v1/token?grant_type=password";
  Serial.println("Beginning to login to " + url);

  if (Loginhttps.begin(*clientLogin, url))
  {
    Loginhttps.addHeader("apikey", key);
    Loginhttps.addHeader("Content-Type", "application/json");

    String query = "{\"" + loginMethod + "\": \"" + phone_or_email + "\", \"password\": \"" + password + "\"}";
    httpCode = Loginhttps.POST(query);

    if (httpCode > 0)
    {
      String data = Loginhttps.getString();
      deserializeJson(doc, data);
      if (doc.containsKey("access_token") && !doc["access_token"].isNull() && doc["access_token"].is<String>() && !doc["access_token"].as<String>().isEmpty())
      {
        String USER_TOKEN = doc["access_token"].as<String>();
        authTimeout = doc["expires_in"].as<int>() * 1000;
        Serial.println("Login Success");

        DynamicJsonDocument authConfig(1024);
        deserializeJson(authConfig, tokenConfig);
        authConfig["payload"]["access_token"] = USER_TOKEN;
        serializeJson(authConfig, configAUTH);
      }
      else
      {
        Serial.println("Login Failed: Invalid access token in response");
      }
    }
    else
    {
      Serial.print("Login Failed : ");
      Serial.println(httpCode);
    }

    Loginhttps.end();
    delete clientLogin;
    clientLogin = NULL;

    loginTime = millis();
  }
  else
  {
    return -100;
  }

  return httpCode;
}

void SupabaseRealtime::addChangesListener(String table, String event, String schema, String filter)
{
  isPostgresChanges = true;
  DynamicJsonDocument tableObj(512);

  tableObj["event"] = event;
  tableObj["schema"] = schema;
  tableObj["table"] = table;

  if (filter != "")
  {
    tableObj["filter"] = filter;
  }

  postgresChanges.add(tableObj);
}

void SupabaseRealtime::sendPresence(String device_name)
{
  DynamicJsonDocument presence(512);
  isPresence = true;

  deserializeJson(presence, jsonPresence);
  presence["payload"]["payload"]["user"] = device_name;
  serializeJson(presence, presenceConfig);
}

void SupabaseRealtime::listen()
{
  DynamicJsonDocument jsonRealtimeConfig(1024);
  deserializeJson(jsonRealtimeConfig, config);
  if (isPostgresChanges)
  {
    jsonRealtimeConfig["payload"]["config"]["postgres_changes"] = postgresChanges;
  }
  if (isPresence)
  {
    jsonRealtimeConfig["payload"]["config"]["presence"]["key"] = "";
  }
  serializeJson(jsonRealtimeConfig, configJSON);

  String slug = "/realtime/v1/websocket?apikey=" + String(key) + "&vsn=1.0.0";

  webSocket.beginSSL(
      hostname.c_str(),
      443,
      slug.c_str());

  webSocket.onEvent(std::bind(&SupabaseRealtime::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void SupabaseRealtime::processMessage(uint8_t *payload)
{
  DynamicJsonDocument result(1024);
  deserializeJson(result, payload);
  String table = getEventTable(result);
  if (table != "null")
  {
    String data = result["payload"]["data"];
    handler(data);
  }
}

void SupabaseRealtime::webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.println("[WSc] Disconnected!");
    break;
  case WStype_CONNECTED:
    Serial.println("[WSc] Connected!");
    webSocket.sendTXT(configJSON);
    if (useAuth)
      webSocket.sendTXT(configAUTH);
    if (isPresence)
      webSocket.sendTXT(presenceConfig);
    break;
  case WStype_TEXT:
    processMessage(payload);
    break;
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    break;
  case WStype_ERROR:
    Serial.printf("[WSc] Error: %s\n", payload);
    break;
  case WStype_PING:
  case WStype_PONG:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
}

void SupabaseRealtime::loop()
{
  if (useAuth && millis() - loginTime > authTimeout / 1.2)
  {
    webSocket.disconnect();
    _login_process();
  }
  else
  {
    webSocket.loop();
  }

  if (millis() - last_ms > 30000)
  {
    last_ms = millis();
    webSocket.sendTXT(jsonRealtimeHeartbeat);
    if (useAuth)
      webSocket.sendTXT(configAUTH);
  }
}

void SupabaseRealtime::begin(String hostname, String key, void (*func)(String))
{
  hostname.replace("https://", "");
  this->hostname = hostname;
  this->key = key;
  this->handler = func;
}

int SupabaseRealtime::login_email(String email_a, String password_a)
{
  useAuth = true;
  loginMethod = "email";
  phone_or_email = email_a;
  password = password_a;

  int httpCode = 0;
  while (httpCode <= 0)
  {
    httpCode = _login_process();
  }
  return httpCode;
}

int SupabaseRealtime::login_phone(String phone_a, String password_a)
{
  useAuth = true;
  loginMethod = "phone";
  phone_or_email = phone_a;
  password = password_a;

  int httpCode = 0;
  while (httpCode <= 0)
  {
    httpCode = _login_process();
  }
  return httpCode;
}
