#ifndef ESP_SupabaseOscar_h
#define ESP_SupabaseOscar_h

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

#if defined(ESP8266)
#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
#include <HTTPClient.h>
#else
#error "This library is not supported for your board! Only ESP32 and ESP8266"
#endif

class Supabase
{
private:
    // Miembros privados
    String hostname;                 // Hostname de Supabase
    String key;                      // Clave de la API
    String url_query;                // Cadena de consulta para las peticiones
    WiFiClientSecure client;         // Cliente seguro para conexiones HTTPS
    HTTPClient https;                // Cliente HTTP

    // Autenticación y RLS
    String phone_or_email;
    String password;
    String data;
    String loginMethod;
    bool useAuth = false;            // Indica si se utiliza autenticación
    unsigned int authTimeout = 0;    // Tiempo de expiración del token
    unsigned long loginTime;         // Marca de tiempo del último login
    String USER_TOKEN;               // Token de usuario autenticado

    // Métodos privados
    void _check_last_string();       // Verifica la última cadena en una consulta
    int _login_process();            // Procesa el inicio de sesión

public:
    // Constructor y destructor
    Supabase() {}
    ~Supabase() {}

    // Métodos públicos
    void begin(String hostname_a, String key_a); // Configura el hostname y la clave
    void setupRealtime(String hostname_a, String key_a); // Configura Realtime
    String getQuery(); // Obtiene la consulta URL generada
    void urlQuery_reset(); // Reinicia la consulta URL

    // Métodos de construcción de consultas
    Supabase &from(String table); // Define la tabla para la consulta
    Supabase &select(String colls); // Define las columnas a seleccionar
    Supabase &update(String table); // Define la tabla para actualización
    int insert(String table, String json, bool upsert); // Inserta datos en la tabla
    int upload(String bucket, String filename, String mime_type, Stream *stream, uint32_t size); // Sube un archivo desde un Stream
    int upload(String bucket, String filename, String mime_type, uint8_t *buffer, uint32_t size); // Sube un archivo desde un buffer

    // Acceso al cliente WiFiClientSecure
    WiFiClientSecure &getClient() { return client; } // Método público para obtener el cliente
    
    // Operadores de comparación para filtros
    Supabase &eq(String coll, String conditions);
    Supabase &gt(String coll, String conditions);
    Supabase &gte(String coll, String conditions);
    Supabase &lt(String coll, String conditions);
    Supabase &lte(String coll, String conditions);
    Supabase &neq(String coll, String conditions);
    Supabase &in(String coll, String conditions);
    Supabase &is(String coll, String conditions);
    Supabase &cs(String coll, String conditions);
    Supabase &cd(String coll, String conditions);
    Supabase &ov(String coll, String conditions);
    Supabase &sl(String coll, String conditions);
    Supabase &sr(String coll, String conditions);
    Supabase &nxr(String coll, String conditions);
    Supabase &nxl(String coll, String conditions);
    Supabase &adj(String coll, String conditions);

    // Métodos de ordenamiento y paginación
    Supabase &order(String coll, String by, bool nulls);
    Supabase &limit(unsigned int by);
    Supabase &offset(int by);

    // Métodos de ejecución de consultas
    String doSelect(); // Ejecuta una consulta SELECT
    int doUpdate(String json); // Ejecuta una actualización

    // Autenticación
    int login_email(String email_a, String password_a);
    int login_phone(String phone_a, String password_a);

    // Llamadas a procedimientos remotos (RPC)
    String rpc(String func_name, String json_param = "");


};

#endif
