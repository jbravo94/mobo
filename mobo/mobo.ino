#include <Arduino.h>
#include "HWCDC.h"
#include <WiFi.h>
#include "sys/time.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "mbedtls/md.h"
#include "time.h"

#include "display.h"

//#include <ESP_IOExpander_Library.h>
#define LOG(text) { if (debug) { USBSerial.print(text); } }
#define LOGLN(text) { if (debug) { USBSerial.println(text); } }
#define LOGF(text, ...) { if (debug) { USBSerial.printf(text, ##__VA_ARGS__); } }

// Login session

bool isLoggedIn = false;
bool debug = true;
uint32_t loggedInMillisTimestamp;

int logoutTimeout = 30;

String adminPassword = "BackupBuddyAdmin";

Preferences preferences;


int http_delay = 5000;
int loop_delay = 100;

int loop_delay_counter = 0;


// Wifi & HTTPS Client

char* server = "ozi5ej3rgtoyuqqrlndad7ucju0rvjxf.lambda-url.eu-central-1.on.aws";

String method = "POST";
String service = "lambda";
String region = "eu-central-1";
String content_type = "application/json";
String canonical_uri = "/";
String canonical_querystring = "";

String access_key = "AKIAVLKHVHWXVFBHYJSU";
String secret_key = "****************************************";

// NTP server to request epoch time
char* ntpServer = "at.pool.ntp.org";

// Variable to save current epoch time
unsigned long epochTime;
bool isSnoozed = false;
int last_day = 0;

bool backupsSuccessful = false;

const char* test_root_ca = 
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
  "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
  "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
  "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
  "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
  "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
  "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
  "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
  "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
  "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
  "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
  "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
  "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
  "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
  "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
  "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
  "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
  "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
  "-----END CERTIFICATE-----\n";

WiFiClientSecure client;

// USB Serial

HWCDC USBSerial;
String receivedMessage = "";


// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);

  return now;
}

struct tm getTimeInfo() {

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  return timeinfo;
}

bool isSnoozeOver () {

  /*if (!isSnoozed) {
    return false;
  }

  struct tm timeinfo = getTimeInfo();

  if ((last_day == 365 || last_day == 364) && timeinfo.tm_yday == 0) {
    isSnoozed = false;
    return true;
  }

  //bool isSnoozeOver = timeinfo.tm_yday > last_day && timeinfo.tm_hour > (8 - 2);

  bool isSnoozeOver = timeinfo.tm_sec > ((last_day + 10) % 60);

  if (isSnoozeOver) {
    isSnoozed = false;
  }

  last_day = timeinfo.tm_sec;

  //last_day = timeinfo.tm_yday;

  return isSnoozeOver;*/

  return false;
}


void setup_preferences() {
  preferences.begin("backup-buddy", false); 
}

void setup_serial() {
  USBSerial.begin(115200);
  LOGLN("Serial initialized.");
}

void setup_wifi() {

  String ssid = preferences.getString("wifi-ssid", "backup-buddy-wifi-ssid");
  String password = preferences.getString("wifi-password", "backup-buddy-wifi-password");

  WiFi.mode(WIFI_STA);  //Optional
  WiFi.begin(ssid, password);

  int retry_timeout = 5000;
  int retry_counter = 0;

  LOGLN("Connecting to Wifi.");

  while (WiFi.status() != WL_CONNECTED) {
    LOG(".");

    if ((100 * retry_counter) >= retry_timeout) {
      break;
    }

    retry_counter += 1;
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    LOGLN("\nFailed to connect to network...");
    return;
  }

  LOGLN("\nConnected to the WiFi network.");
  LOG("Local IP: ");
  LOGLN(WiFi.localIP());
}



void setup_httpclient() {
  client.setCACert(test_root_ca);
  //client.setCertificate(test_client_cert); // for client verification
  //client.setPrivateKey(test_client_key);	// for client verification
  /*String server_ = preferences.getString("aws-lambda-dn", "ozi5ej3rgtoyuqqrlndad7ucju0rvjxf.lambda-url.eu-central-1.on.aws"); //"ozi5ej3rgtoyuqqrlndad7ucju0rvjxf.lambda-url.eu-central-1.on.aws";
  LOGLN(server_);
  server = (char*) server_.c_str(); 
  
  region = preferences.getString("aws-region", "eu-central-1"); //"eu-central-1";

  access_key = preferences.getString("aws-access-key", "AKIAVLKHVHWXVFBHYJSU"); //"AKIAVLKHVHWXVFBHYJSU";
  secret_key = preferences.getString("aws-secret-key", "****************************************"); //"****************************************";*/
}


byte* get_signature_key(String key, String date_stamp, String region_name, String service_name) {

  String combined_key = "AWS4" + key;

  byte* kDate = get_hmac_sha256_hash((char*) combined_key.c_str(), combined_key.length(), date_stamp);
  byte* kRegion = get_hmac_sha256_hash((char*) kDate, 32, region_name);
  byte* kService = get_hmac_sha256_hash((char*) kRegion, 32, service_name);
  byte* kSigning = get_hmac_sha256_hash((char*) kService, 32, "aws4_request");

  return kSigning;
}

byte* get_hmac_sha256_hash(char* key, size_t keyLength, String payload_string) {
    char *payload = (char*) payload_string.c_str();
    static byte hmacResult[32];
  
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
    const size_t payloadLength = strlen(payload);
  
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, keyLength);
    mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payloadLength);
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
  
    return hmacResult;
}

String get_byte_array_as_hex_string(byte *b, int length) {
  String hmac_string = "";

  for(int i= 0; i < length; i++){
    char str[3];

    sprintf(str, "%02x", (int)*(b + i));
    hmac_string.concat(str);
  }

  return hmac_string;
}


String get_sha256_hash(String s) {
  
  String hash = "";

  unsigned char output2[32];
  mbedtls_sha256_context ctx2;

  mbedtls_sha256_init(&ctx2);
  mbedtls_sha256_starts(&ctx2, 0); /* SHA-256, not 224 */

  /* Simulating multiple fragments */
  mbedtls_sha256_update(&ctx2, (unsigned char*) s.c_str(), s.length());
  mbedtls_sha256_finish(&ctx2, output2);
  //print_hex("Method 2", output2, sizeof output2);

  /* Or you could re-use the context by doing mbedtls_sha256_starts() again */
  mbedtls_sha256_free(&ctx2);

  for(int i = 0; i < 32; i++) {

    char str[3];

    sprintf(str, "%02x", (int)output2[i]);
    //LOG(str);

    hash.concat(str);
  }
  
  return hash;
}


void loop_httpclient() {

  LOGLN(server);

  if (!client.connect((const char*) server, 443)) {
    LOGLN("Connection failed!");
  } else {
    
    if (debug) {
      LOGLN("Connected to server!");
    }

    epochTime = getTime();

    struct tm timeInfo = getTimeInfo();

    char amz_date_array[80];

    strftime(amz_date_array, 80, "%Y%m%dT%H%M%SZ", &timeInfo);

    String amz_date = String(amz_date_array);

    char date_stamp_array[80];

    strftime(date_stamp_array, 80, "%Y%m%d", &timeInfo);

    String date_stamp = String(date_stamp_array);

    String canonical_headers = "content-type:" + content_type + "\n" + "host:" + server + "\n" + "x-amz-date:" + amz_date + "\n";
    String signed_headers = "content-type;host;x-amz-date";

    String payload = "{\"requestType\": \"getRecentBackupStatus\"}";

    String payload_hash = get_sha256_hash(payload);

    String canonical_request = method + "\n" + canonical_uri + "\n" + canonical_querystring + "\n" + canonical_headers + "\n" + signed_headers + "\n" + payload_hash;

    String algorithm = "AWS4-HMAC-SHA256";
    String credential_scope = String(date_stamp) + "/" + region + "/" + service + "/" + "aws4_request";

    String string_to_sign = algorithm + "\n" + String(amz_date) + "\n" + credential_scope + "\n" + get_sha256_hash(canonical_request);

    byte* signature_key = get_signature_key(secret_key, String(date_stamp), region, service);

    byte* signature = get_hmac_sha256_hash((char*) signature_key, 32, string_to_sign);

    String authorization_header = algorithm + " " + "Credential=" + access_key + "/" + credential_scope + ", " + "SignedHeaders=" + signed_headers + ", " + "Signature=" + get_byte_array_as_hex_string(signature, 32);

    // Make a HTTP request:
    client.print("POST https://");
    client.print(server);
    client.println("/ HTTP/1.0");
    client.println("Content-Type: " + content_type);
    client.print("Host: ");
    client.println(server);
    client.println("X-Amz-Date: " + String(amz_date));
    client.println("Authorization: " + authorization_header);
    client.print("Content-Length: ");
    client.println(payload.length());
    client.println();
    client.println(payload);

    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        if (debug) {
          LOGLN("headers received");
        }
        break;
      }
    }
    
    // if there are incoming bytes available
    // from the server, read them and print them:
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, client);
    if (error) {
      LOG(F("deserializeJson() failed: "));
      LOGLN(error.f_str());

      while (client.available()) {
        char c = client.read();
        USBSerial.write(c);
      }

      client.stop();
      return;
    }

    backupsSuccessful = false;

    String resultMessage = "";

    if (doc.containsKey("failedBackups") && doc["failedBackups"].as<int>() == 0) {
      backupsSuccessful = true;
      resultMessage = "All backups successful!";
    } else {
      resultMessage = "Failed backups require attention!";
    }

    if (debug) {
      LOGLN(resultMessage);
    }

    client.stop();
  }
}

void setup_time() {
  //ntpServer = (char*) preferences.getString("ntp-server-dn", "at.pool.ntp.org").c_str(); //"at.pool.ntp.org";

  configTime(0, 0, (const char*) ntpServer);
}

void setup(void) {
  setup_preferences();
  setup_serial();

  setup_wifi();

  setup_time();

  bootstrap_wire_touchpad_display();

  setup_httpclient();

  //setup_button();

  delay(2000);  // 5 seconds
}

void handle_http_response() {
  if (backupsSuccessful) {
    set_circle_green();

    if (isSnoozeOver() && !is_backlight_on()) {
      toggle_backlight();
    }
  } else {
    set_circle_red();

    isSnoozed = false;

    if (!is_backlight_on()) {
      toggle_backlight();
    }
  }

   /*if (backupsSuccessful) {
    set_display_rectangle(200, GREEN);
  } else {
    set_display_rectangle(200, RED);
  }*/
}


/*

void setup_button() {
  
  expander = new EXAMPLE_CHIP_CLASS(TCA95xx_8bit,
                                    (i2c_port_t)0, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000,
                                    IIC_SCL, IIC_SDA);

  expander->init();
  expander->begin();
  expander->pinMode(5, INPUT);
  expander->pinMode(4, INPUT);
}

void loop_button() {
  int backlight_ctrl = expander->digitalRead(4);

  if (backlight_ctrl == HIGH) {
    while (expander->digitalRead(4) == HIGH) {
      delay(50);
    }
    toggleBacklight();
  }
}
*/

void loop_serial() {
  while (USBSerial.available()) {
    char incomingChar = USBSerial.read();  // Read each character from the buffer
    
    if (incomingChar == '\n') {  // Check if the user pressed Enter (new line character)

      LOG("You sent: ");
      LOGLN(receivedMessage);

      String auth_command = "AUTH";
      String get_command = "GET";
      String put_command = "PUT";
      String quit_command = "QUIT";

      if (isLoggedIn && quit_command.equals(receivedMessage)) {

        isLoggedIn = false;
        loggedInMillisTimestamp = 0;

        LOGLN("Successfully logged out.");
        receivedMessage = "";
        return;
      }

      int index = receivedMessage.indexOf(' ');
      if (index == -1) {
        receivedMessage = "";
        LOGLN("Unknown command.");
        return;
      }

      String command = receivedMessage.substring(0, index);
      String value = receivedMessage.substring(index + 1);

      if (auth_command.equals(command) && adminPassword.equals(value)) {
        isLoggedIn = true;
        loggedInMillisTimestamp = millis();
        LOGLN("Login successful.");
        receivedMessage = "";
        return;
      }

      if ((millis() - loggedInMillisTimestamp) >= (logoutTimeout * 1000)) {
        isLoggedIn = false;
      }

      if (!isLoggedIn) {
        LOGLN("Unauthorized.");
        receivedMessage = "";
        return;
      }

      if (get_command.equals(command)) {
        LOG("Value: ");
        LOGLN(preferences.getString(value.c_str(), ""));
        receivedMessage = "";
      }

      if (put_command.equals(command)) {

        int idx = value.indexOf(' ');

        if (idx == -1) {
          receivedMessage = "";
          LOGLN("Syntax incorrect. Use 'PUT key value");
          return;
        }

        String key = value.substring(0, idx);
        String val = value.substring(idx + 1);

        LOGF("Key %s, Val %s ", key, val);
        preferences.putString(key.c_str(), val.c_str());

        preferences.putBytes(key.c_str(), val.c_str(), (size_t) value.length());

        size_t valueLen = preferences.getBytesLength(key.c_str());
        char buffer[valueLen];


        preferences.getBytes(key.c_str(), &buffer, valueLen);
        LOGLN(buffer);

        receivedMessage = "";
      }
            
      // Clear the message buffer for the next input
      receivedMessage = "";
    } else {
      // Append the character to the message string
      receivedMessage += incomingChar;
    }
  }
}

void loop() {
  //loop_beacon();

  if (http_delay <= (loop_delay_counter * loop_delay)) {
    loop_httpclient();
    handle_http_response();
    loop_delay_counter = 0;
  }


  refresh_ui();
  loop_serial();

  loop_delay_counter += 1;
  delay(100);
}

