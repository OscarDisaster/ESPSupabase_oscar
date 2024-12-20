
#ifndef ESP_SupabaseOscar_Realtime_h
#define ESP_SupabaseOscar_Realtime_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
#include <HTTPClient.h>
#else
#error "This library is not supported for your board! ESP32 and ESP8266"
#endif

class SupabaseRealtime
{
private:
  WebSocketsClient webSocket;

  String key;
  String hostname;

  // RLS Stuff
  String phone_or_email;
  String password;
  String data;
  String loginMethod;
  bool useAuth = false;
  int _login_process();
  unsigned int authTimeout = 0;
  unsigned long loginTime;
  String configAUTH;

  // Initial config
  const char *config = R"({"event":"phx_join","topic":"realtime:ESP","payload":{"config":{}},"ref":"ESP"})";

  // Postgres Changes
  bool isPostgresChanges = false;
  DynamicJsonDocument postgresChanges = DynamicJsonDocument(1024);

  // Presence
  const char *jsonPresence = R"({"topic":"realtime:ESP","event":"presence","payload":{"type":"presence","event":"track","payload":{"user":"","online_at":""}},"ref":"ESP"})";
  bool isPresence = false;
  String presenceConfig;

  // Realtime Config
  DynamicJsonDocument jsonRealtimeConfig = DynamicJsonDocument(1024);
  String configJSON;

  // Heartbeat
  unsigned int last_ms = millis();
  const char *jsonRealtimeHeartbeat = R"({"event":"heartbeat","topic":"phoenix","payload":{},"ref":"ESP"})";
  const char *tokenConfig = R"({"topic":"realtime:ESP","event":"access_token","payload":{"access_token":""},"ref":"ESP"})";

  void processMessage(uint8_t *payload);
  void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);

  std::function<void(String)> handler;

public:
  SupabaseRealtime() {}
  ~SupabaseRealtime() {} // Agregar destructor explícito
  void begin(String hostname, String key, void (*func)(String));
  void sendPresence(String device_name);
  // void broadcast(); // Not implemented yet
  void addChangesListener(String table, String event, String schema, String filter);
  void listen();
  void loop();
  int login_email(String email_a, String password_a);
  int login_phone(String phone_a, String password_a);
};


#endif
