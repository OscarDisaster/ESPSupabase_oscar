# ESPSupabaseOscar

**A fork of [ESPSupabase](https://github.com/jhagas/ESPSupabase), a simple library to connect ESP32/8266 to Supabase via REST API and WebSockets (for Realtime), including user authentication.**

This fork includes additional features, modifications, and improvements to better suit specific use cases.

---

## Differences from the Original Project

This fork adds the following improvements and changes:
- **Custom Modifications**: Adjustments for personal or specific use cases.
- **Improved Documentation**: Updated to reflect changes made in this fork.
- **Library Name Change**: The library has been renamed to `ESPSupabaseOscar` to avoid conflicts with the original library.

---

## Installation

This library is available at [PlatformIO Library Manager](https://docs.platformio.org/en/latest/librarymanager/index.html). For Arduino, you can install it manually by downloading this repository and adding it to your Arduino libraries folder.

---

## Examples

See all examples in the `examples` folder.

---

## Supabase PostgREST API (`#include <ESPSupabaseOscar.h>`)

### Directly Makes Connection to Database

| Method                                                                                     | Description                                                                                                                                                                                                                                                                                                                                                                                                               |
| ------------------------------------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `login_email(String email_a, String password_a)`                                           | **(OPTIONAL, ONLY IF USING RLS)**, Returns http response code `int`                                                                                                                                                                                                                                                                                                                                                       |
| `login_phone(String phone_a, String password_a)`                                           | **(OPTIONAL, ONLY IF USING RLS)**, Returns http response code `int`                                                                                                                                                                                                                                                                                                                                                       |
| `begin(String url_a, String key_a);`                                                       | `url_a` is a Supabase URL and `key_a` is supabase anon key. Returns `void`                                                                                                                                                                                                                                                                                                                                                |
| `insert(String table, String json, bool upsert)`                                           | Returns http response code `int`. If you want to do upsert, set the third parameter to `true`                                                                                                                                                                                                                                                                                                                              |
| `upload(String bucket, String filename, String mime_type, Stream *stream, uint32_t size)`  | `bucket` is the name of the Supabase Storage bucket without any `/`. `filename` is the name to upload the file with, should have an extension but no `/`. Takes a `Stream*` pointer as an argument, this can be Arduino SD `File*` or SPIFFS `File*` types. Returns http response code `int`. `mime_type` is for eg. `image/jpg`. `size` is the total size in bytes of the file to upload. Returns http response code `int`. |
| `upload(String bucket, String filename, String mime_type, uint8_t *buffer, uint32_t size)` | Same function as the previous one but takes a `uint8_t*` buffer instead of a `Stream*`. Can be used for files stored in RAM.                                                                                                                                                                                                                                                                                              |
| `.doSelect()`                                                                              | Called at the end of select query chain, see [Examples](#examples). Returns http response payload (your data) from Supabase `String`                                                                                                                                                                                                                                                                                      |
| `.doUpdate(String json)`                                                                   | Called at the end of update query chain, see [Examples](#examples). Returns http response code from Supabase `int`                                                                                                                                                                                                                                                                                                        |

### Building The Queries

When building the queries, you can chain the methods like in this example.

> Remember in `.select()` method, you have to put `.limit()`, so your microcontroller's memory doesn't overflow.

```arduino
String read = db.from("table").select("*").eq("column", "value").order("column", "asc", true).limit(1).doSelect();
