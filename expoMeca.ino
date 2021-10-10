#include <common.h>
#include <FirebaseESP32.h>
#include <FirebaseFS.h>
#include <Utils.h>

#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define PIN        19 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 64 // Popular NeoPixel ring size

#define DEVICE_UID "1X"
#define WIFI_SSID "TeleCentro-fa9a"
#define WIFI_PASSWORD "UDNYYJMF2NHZ"

#define API_KEY "AIzaSyAvC4LdRmxopYmOXa1c-c2I3POeHgh3DPo"
// Your Firebase Realtime database URL
#define DATABASE_URL "https://pantallaexpomeca-default-rtdb.firebaseio.com/"

#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"


FirebaseData fbdo;
FirebaseData stream;
// Firebase Authentication Object
FirebaseAuth auth;
// Firebase configuration Object
FirebaseConfig config;
// Firebase database path
String databasePath = "";
// Firebase Unique Identifier
String fuid = "";
// Stores the elapsed time from device start up
unsigned long elapsedMillis = 0;
// The frequency of sensor updates to firebase, set to 10seconds
unsigned long update_interval = 1000;
// Dummy counter to test initial firebase updates
int count = 0;
// Store device authentication status
bool isAuthenticated = false;

struct point{
  public:
    int r;
    int g;
    int b;
};
point matrix[NUMPIXELS];
String parentPath = "/matrix";
String childPath[NUMPIXELS];

size_t childPathSize = NUMPIXELS;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  for (int i = 0; i < NUMPIXELS; i++) {
    childPath[i] = "/" + i;
  }
  Serial.begin(115200);
  wifi_Init();
  // Initialise firebase configuration and signup anonymously
  firebase_init();
  // put your setup code here, to run once:
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  pixels.begin();
  pixels.clear();
  pixels.setBrightness(10);
  database_test();
  if (!Firebase.beginMultiPathStream(stream, parentPath, childPath, childPathSize))
    Serial.printf("sream begin error, %s\n\n", stream.errorReason().c_str());

  Firebase.setMultiPathStreamCallback(stream, streamCallback, streamTimeoutCallback);
}

void loop() {
}

void wifi_Init() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void firebase_init() {
  // configure firebase API Key
  config.api_key = API_KEY;
  // configure firebase realtime database url
  config.database_url = DATABASE_URL;
  // Enable WiFi reconnection
  Firebase.reconnectWiFi(true);
  Serial.println("------------------------------------");
  Serial.println("Sign up new user...");
  // Sign in to firebase Anonymously
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Success");
    isAuthenticated = true;
    // Set the database path where updates will be loaded for this device
    fuid = auth.token.uid.c_str();
  }
  else
  {
    Serial.printf("Failed, %s\n", config.signer.signupError.message.c_str());
    isAuthenticated = false;
  }
  // Assign the callback function for the long running token generation task, see addons/TokenHelper.h
  config.token_status_callback = tokenStatusCallback;
  // Initialise the firebase library
  Firebase.begin(&config, &auth);
}

void database_test() {
  // Check that 10 seconds has elapsed before, device is authenticated and the firebase service is ready.
  if (isAuthenticated && Firebase.ready() && millis() - elapsedMillis > update_interval)
  {
    elapsedMillis = millis();
    if (Firebase.getArray(fbdo, "/matrix/"))
    {
      FirebaseJsonArray &json = fbdo.jsonArray();;
      FirebaseJsonData  red;
      FirebaseJsonData green;
      FirebaseJsonData blue;
      for (int i = 0; i < NUMPIXELS; i++) {
        String r = (String)i + "]/red";
        String g = (String)i + "]/green";
        String b = (String)i + "]/blue";
        json.get(red, "[" + r);
        json.get(green, "[" + g);
        json.get(blue, "[" + b);
        if (red.success) {
          matrix[i].r = red.intValue;
          matrix[i].g = green.intValue;
          matrix[i].b = blue.intValue;
          pixels.setPixelColor(i, pixels.Color(red.intValue, green.intValue, blue.intValue));
          pixels.show();
        }
        Serial.println(r);
      }
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }
  }
}

void streamCallback(MultiPathStreamData stream)
{
  Serial.println("hola");
  for (int i = 0; i < NUMPIXELS; i++)
  {
    if (stream.get(childPath[i])) {
      Serial.println(stream.dataPath.c_str());
      Serial.println(stream.type.c_str());
      Serial.println(stream.value.c_str());
      
      String color = stream.dataPath.c_str();
      color = color.substring(1);
      int index = color.indexOf("/");
      String id = color.substring(0,index);
      color = color.substring(index+1);
      Serial.println(color);
      if (color.equals("red")) {
        //Serial.println("hola");
        matrix[id.toInt()].r = stream.value.toInt();
        pixels.setPixelColor(id.toInt(), pixels.Color(stream.value.toInt(), matrix[id.toInt()].g, matrix[id.toInt()].b));
        pixels.show();
      }
      if (color.equals("green")) {
        //Serial.println("hola");
        matrix[id.toInt()].g = stream.value.toInt();
        pixels.setPixelColor(id.toInt(), pixels.Color(matrix[id.toInt()].r, stream.value.toInt(), matrix[id.toInt()].b));
        pixels.show();
      }
      if (color.equals("blue")) {
        //Serial.println("hola");
        matrix[id.toInt()].b = stream.value.toInt();
        pixels.setPixelColor(id.toInt(), pixels.Color(matrix[id.toInt()].r, matrix[id.toInt()].g, stream.value.toInt()));
        pixels.show();
      }
    }
  }

}

void streamTimeoutCallback(bool timeout)
{
  if (timeout) {
    //Stream timeout occurred
    Serial.println("Stream timeout, resume streaming...");
  }
}
