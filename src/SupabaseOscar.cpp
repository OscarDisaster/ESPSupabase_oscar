#include "ESPSupabaseOscar.h"

void Supabase::_check_last_string()
{
    unsigned int last = url_query.length() - 1;
    if (url_query[last] != '?')
    {
        url_query += "&";
    }
}

int Supabase::_login_process()
{
    int httpCode;
    DynamicJsonDocument doc(1024); // Cambié a DynamicJsonDocument para evitar errores de tamaño.
    Serial.println("Beginning to login..");

    if (https.begin(client, hostname + "/auth/v1/token?grant_type=password"))
    {
        https.addHeader("apikey", key);
        https.addHeader("Content-Type", "application/json");

        String query = "{\"" + loginMethod + "\": \"" + phone_or_email + "\", \"password\": \"" + password + "\"}";
        httpCode = https.POST(query);

        if (httpCode > 0)
        {
            String data = https.getString();
            deserializeJson(doc, data);
            if (doc.containsKey("access_token") && !doc["access_token"].isNull() && doc["access_token"].is<String>() && !doc["access_token"].as<String>().isEmpty())
            {
                USER_TOKEN = doc["access_token"].as<String>();
                authTimeout = doc["expires_in"].as<int>() * 1000;
                Serial.println("Login Success");
            }
            else
            {
                Serial.println("Login Failed: Invalid access token in response");
            }
        }
        else
        {
            Serial.println(phone_or_email);
            Serial.println(password);

            Serial.print("Login Failed : ");
            Serial.println(httpCode);
        }

        https.end();
        loginTime = millis();
    }
    else
    {
        return -100;
    }

    return httpCode;
}

void Supabase::begin(String hostname_a, String key_a)
{
    client.setInsecure();
    hostname = hostname_a;
    key = key_a;
}

String Supabase::getQuery()
{
    String temp = url_query;
    urlQuery_reset();
    return hostname + "/rest/v1/" + temp;
}

void Supabase::urlQuery_reset()
{
    url_query = "";
}

Supabase &Supabase::from(String table)
{
    url_query += (table + "?");
    return *this;
}

int Supabase::insert(String table, String json, bool upsert)
{
    int httpCode;
    if (https.begin(client, hostname + "/rest/v1/" + table))
    {
        https.addHeader("apikey", key);
        https.addHeader("Content-Type", "application/json");

        String preferHeader = "return=representation";
        if (upsert)
        {
            preferHeader += ",resolution=merge-duplicates";
        }
        https.addHeader("Prefer", preferHeader);

        if (useAuth)
        {
            unsigned long t_now = millis();
            if (t_now - loginTime >= authTimeout)
            {
                _login_process();
            }
            https.addHeader("Authorization", "Bearer " + USER_TOKEN);
        }
        httpCode = https.POST(json);
        https.end();
    }
    else
    {
        return -100;
    }
    return httpCode;
}

Supabase &Supabase::select(String colls)
{
    url_query += ("select=" + colls);
    return *this;
}

Supabase &Supabase::update(String table)
{
    url_query += (table + "?");
    return *this;
}
int Supabase::upload(String bucket, String filename, String mime_type, uint8_t *buffer, uint32_t size)
{
    int httpCode = 0;

    int index = hostname.indexOf("//");
    String hostname_no_prot = hostname.substring(index + 2, hostname.length());

    char hostname_char[hostname_no_prot.length() + 1];
    hostname_no_prot.toCharArray(hostname_char, hostname_no_prot.length() + 1);

    String finalPath = hostname + "/storage/v1/object/" + bucket + "/" + filename;

    String boundary = "esp32-supabase-boundary";

    String httpMainHeader = "POST " + finalPath + " HTTP/1.1\r\n";
    httpMainHeader += "Host: " + hostname_no_prot + "\r\n";
    httpMainHeader += "apikey: " + key + "\r\n";
    httpMainHeader += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";

    if (useAuth)
    {
        unsigned long t_now = millis();
        if (t_now - loginTime >= authTimeout)
        {
            _login_process();
        }

        httpMainHeader += "Authorization: Bearer " + USER_TOKEN + "\r\n";
    }

    httpMainHeader += "User-Agent: ESP32/Supabase\r\n";
    httpMainHeader += "Keep-Alive: 3600\r\n";
    httpMainHeader += "Connection: keep-alive\r\n";

    String contentHeader = "--" + boundary + "\r\n";
    contentHeader += "Content-Disposition: form-data; name=\"" + filename + "\"; filename=\"" + filename + "\"\r\n";
    contentHeader += "Content-Type: " + mime_type + "\r\n\r\n";

    String endingHeader = "\r\n--" + boundary + "--\r\n\r\n";

    int contentLength = contentHeader.length() + endingHeader.length();
    httpMainHeader += "Content-Length: " + String(contentLength + size) + "\n\n";

    Serial.printf("Hostname: %s\n\r", hostname_char);

    if (!client.connected())
    {
        client.connect(hostname_char, 443);

        if (!client.connected())
        {
            return 0;
        }
    }

    client.write((uint8_t *)httpMainHeader.c_str(), httpMainHeader.length());
    client.write((uint8_t *)contentHeader.c_str(), contentHeader.length());

    const uint16_t chunkSize = 255;
    uint32_t remainingBytes = size;

    while (remainingBytes)
    {
        uint8_t bytesToRead = remainingBytes >= chunkSize ? chunkSize : remainingBytes;
        client.write(buffer + (size - remainingBytes), bytesToRead);
        remainingBytes -= bytesToRead;
    }

    client.write((uint8_t *)endingHeader.c_str(), endingHeader.length());

    bool firstLine = true;

    while (client.connected())
    {
        String line = client.readStringUntil('\n');
        line.trim();

        if (firstLine)
        {
            if (line == "")
                continue;

            int codePos = line.indexOf(' ') + 1;
            httpCode = line.substring(codePos, line.indexOf(' ', codePos)).toInt();
            firstLine = false;
        }

        if (line == "")
        {
            break;
        }
    }

    return httpCode;
}

int Supabase::upload(String bucket, String filename, String mime_type, Stream *stream, uint32_t size)
{
    int httpCode = 0;

    int index = hostname.indexOf("//");
    String hostname_no_prot = hostname.substring(index + 2, hostname.length());

    char hostname_char[hostname_no_prot.length() + 1];
    hostname_no_prot.toCharArray(hostname_char, hostname_no_prot.length() + 1);

    String finalPath = hostname + "/storage/v1/object/" + bucket + "/" + filename;

    String boundary = "esp32-supabase-boundary";

    String httpMainHeader = "POST " + finalPath + " HTTP/1.1\r\n";
    httpMainHeader += "Host: " + hostname_no_prot + "\r\n";
    httpMainHeader += "apikey: " + key + "\r\n";
    httpMainHeader += "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n";

    if (useAuth)
    {
        unsigned long t_now = millis();
        if (t_now - loginTime >= authTimeout)
        {
            _login_process();
        }

        httpMainHeader += "Authorization: Bearer " + USER_TOKEN + "\r\n";
    }

    httpMainHeader += "User-Agent: ESP32/Supabase\r\n";
    httpMainHeader += "Keep-Alive: 3600\r\n";
    httpMainHeader += "Connection: keep-alive\r\n";

    String contentHeader = "--" + boundary + "\r\n";
    contentHeader += "Content-Disposition: form-data; name=\"" + filename + "\"; filename=\"" + filename + "\"\r\n";
    contentHeader += "Content-Type: " + mime_type + "\r\n\r\n";

    String endingHeader = "\r\n--" + boundary + "--\r\n\r\n";

    int contentLength = contentHeader.length() + endingHeader.length();
    httpMainHeader += "Content-Length: " + String(contentLength + size) + "\n\n";

    Serial.printf("Hostname: %s\n\r", hostname_char);

    if (!client.connected())
    {
        client.connect(hostname_char, 443);

        if (!client.connected())
        {
            return 0;
        }
    }

    client.write((uint8_t *)httpMainHeader.c_str(), httpMainHeader.length());
    client.write((uint8_t *)contentHeader.c_str(), contentHeader.length());

    const uint16_t chunkSize = 255;
    uint64_t availableBytes = stream->available();

    while (availableBytes)
    {
        uint8_t bytesToRead = availableBytes >= chunkSize ? chunkSize : availableBytes;

        uint8_t buffer[bytesToRead];
        uint8_t bytesRead = stream->readBytes(buffer, bytesToRead);
        client.write(buffer, bytesRead);
        availableBytes = stream->available();
    }

    client.write((uint8_t *)endingHeader.c_str(), endingHeader.length());

    bool firstLine = true;

    while (client.connected())
    {
        String line = client.readStringUntil('\n');
        line.trim();

        if (firstLine)
        {
            if (line == "")
                continue;

            int codePos = line.indexOf(' ') + 1;
            httpCode = line.substring(codePos, line.indexOf(' ', codePos)).toInt();
            firstLine = false;
        }

        if (line == "")
        {
            break;
        }
    }

    return httpCode;
}

// Supabase& Supabase::drop(String table){
//   url_query += (table+"?");
//   return *this;
// }
// Operadores de Comparación
Supabase &Supabase::eq(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=eq." + conditions);
    return *this;
}

Supabase &Supabase::gt(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=gt." + conditions);
    return *this;
}

Supabase &Supabase::gte(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=gte." + conditions);
    return *this;
}

Supabase &Supabase::lt(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=lt." + conditions);
    return *this;
}

Supabase &Supabase::lte(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=lte." + conditions);
    return *this;
}

Supabase &Supabase::neq(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=neq." + conditions);
    return *this;
}

Supabase &Supabase::in(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=in.(" + conditions + ")");
    return *this;
}

Supabase &Supabase::is(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=is." + conditions);
    return *this;
}

Supabase &Supabase::cs(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=cs.{" + conditions + "}");
    return *this;
}

Supabase &Supabase::cd(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=cd.{" + conditions + "}");
    return *this;
}

Supabase &Supabase::ov(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=ov.{" + conditions + "}");
    return *this;
}

Supabase &Supabase::sl(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=sl.(" + conditions + ")");
    return *this;
}

Supabase &Supabase::sr(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=sr.(" + conditions + ")");
    return *this;
}

Supabase &Supabase::nxr(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=nxr.(" + conditions + ")");
    return *this;
}

Supabase &Supabase::nxl(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=nxl.(" + conditions + ")");
    return *this;
}

Supabase &Supabase::adj(String coll, String conditions)
{
    _check_last_string();
    url_query += (coll + "=adj.(" + conditions + ")");
    return *this;
}

// Operaciones de Ordenación
Supabase &Supabase::order(String coll, String by, bool nulls)
{
    String subq[] = {"nullsfirst", "nullslast"};
    _check_last_string();
    url_query += ("order=" + coll + "." + by + "." + subq[(int)nulls]);
    return *this;
}

Supabase &Supabase::limit(unsigned int by)
{
    _check_last_string();
    url_query += ("limit=" + String(by));
    return *this;
}

Supabase &Supabase::offset(int by)
{
    _check_last_string();
    url_query += ("offset=" + String(by));
    return *this;
}
// Ejecutar selección después de construir tu consulta
String Supabase::doSelect()
{
    https.begin(client, hostname + "/rest/v1/" + url_query);
    https.addHeader("apikey", key);
    https.addHeader("Content-Type", "application/json");

    if (useAuth)
    {
        unsigned long t_now = millis();
        if (t_now - loginTime >= authTimeout)
        {
            _login_process();
        }
        https.addHeader("Authorization", "Bearer " + USER_TOKEN);
    }

    int httpCode = 0;
    while (httpCode <= 0)
    {
        httpCode = https.GET();
    }

    if (httpCode > 0)
    {
        data = https.getString();
    }
    https.end();
    urlQuery_reset();
    return data;
}

// Ejecutar actualización después de construir tu consulta
int Supabase::doUpdate(String json)
{
    int httpCode;
    if (https.begin(client, hostname + "/rest/v1/" + url_query))
    {
        https.addHeader("apikey", key);
        https.addHeader("Content-Type", "application/json");
        if (useAuth)
        {
            unsigned long t_now = millis();
            if (t_now - loginTime >= authTimeout)
            {
                _login_process();
            }
            https.addHeader("Authorization", "Bearer " + USER_TOKEN);
        }
        httpCode = https.PATCH(json);
        https.end();
    }
    else
    {
        return -100;
    }
    urlQuery_reset();
    return httpCode;
}
int Supabase::login_email(String email_a, String password_a)
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

int Supabase::login_phone(String phone_a, String password_a)
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
String Supabase::rpc(String func_name, String json_param)
{
    int httpCode;

    if (!https.begin(client, hostname + "/rest/v1/rpc/" + func_name))
    {
        return String(-100);
    }
    https.addHeader("apikey", key);
    https.addHeader("Content-Type", "application/json");

    if (useAuth)
    {
        unsigned long t_now = millis();
        if (t_now - loginTime >= authTimeout)
        {
            _login_process();
        }
        https.addHeader("Authorization", "Bearer " + USER_TOKEN);
    }

    httpCode = https.POST(json_param);
    if (httpCode > 0)
    {
        data = https.getString();
        https.end();
        return data;
    }

    https.end();
    return String(httpCode);
}
