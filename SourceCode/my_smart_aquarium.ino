// firebase ==================================================================================================================================================================================

#if defined(ESP32)
    #include <WiFi.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info and the RTDB payload printing info and other helper functions.
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// insert your network credentials.
#define WIFI_SSID "ASUS_X00TD"
#define WIFI_PASSWORD "12345678"

// insert Firebase project API key and RTDB URLefine the RTDB URL.
#define API_KEY "AIzaSyDoOF8gymF6MQ9oFaX03NdhsOGCmsvkBMs"
#define DATABASE_URL "https://my-smartaquarium-default-rtdb.firebaseio.com/"

// define the Firebase Data object.
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

// relay =====================================================================================================================================================================================

int relay1 = 13;
int relay2 = 12;

// hc-sr05 ===================================================================================================================================================================================

#include <NewPing.h>

#define TRIG_PIN 2
#define ECHO_PIN 0

NewPing sonar(TRIG_PIN, ECHO_PIN);

// lcd =======================================================================================================================================================================================

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

byte Dist[8] =
{
 B00100,
 B01110,
 B10101,
 B00100,
 B00100,
 B10101,
 B01110,
 B00100
};

byte Temp[8] =
{
 B00100,
 B01010,
 B01010,
 B01110,
 B01110,
 B11111,
 B11111,
 B01110
};

byte Water[8] =
{
 B00100,
 B00100,
 B01110,
 B01110,
 B11111,
 B11101,
 B11101,
 B01110,
};

byte Light[8] =
{
 B00100,
 B10001,
 B01110,
 B11101,
 B11101,
 B01110,
 B01110,
 B00100
};

byte Signal[8] =
{
 B00000,
 B00000,
 B00000,
 B00000,
 B00000,
 B00000,
 B00001,
 B00001
};

byte Signal2[8] =
{
 B00000,
 B00000,
 B00000,
 B00001,
 B00101,
 B10101,
 B10101,
 B10101
};

byte Signal3[8] =
{
 B00000,
 B00000,
 B00000,
 B00000,
 B00000,
 B10101,
 B10010,
 B10101
};

// thermistor-ntc3950-10k ====================================================================================================================================================================

// which analog pin to connect
#define THERMISTORPIN A0         
// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25+1.5
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    

int samples[NUMSAMPLES];

// target ====================================================================================================================================================================================

int targetDist = 4;

// millis ====================================================================================================================================================================================

unsigned long previousMillis_task = 0;
unsigned long interval_task = 200;

unsigned long previousMillis_task0 = 0;
unsigned long interval_task0 = 1000;

unsigned long previousMillis_task1Eks = 0;
unsigned long interval_task1Eks = 1000;

unsigned long previousMillis_task1Manual = 0;
unsigned long interval_task1Manual = 1000;

unsigned long previousMillis_task1Auto = 0;
unsigned long interval_task1Auto = 1000;

unsigned long previousMillis_task2 = 0;
unsigned long interval_task2 = 1000;

unsigned long previousMillis_task3 = 0;
unsigned long interval_task3 = 1000;

unsigned long previousMillis_task4 = 0;
unsigned long interval_task4 = 1000;

// void setup ================================================================================================================================================================================

void setup() {
    Serial.begin(115200);
    analogReference(EXTERNAL);

    // relay =================================================================================================================================================================================
    
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
    
    pinMode(relay1, OUTPUT);
    pinMode(relay2, OUTPUT);

    // lcd ===================================================================================================================================================================================

    lcd.init();
    lcd.backlight();
    lcd.createChar(1, Dist);
    lcd.createChar(2, Temp);
    lcd.createChar(3, Water);
    lcd.createChar(4, Light);
    lcd.createChar(5, Signal);
    lcd.createChar(6, Signal2);
    lcd.createChar(7, Signal3);

    // firebase ==============================================================================================================================================================================

    // connect to wifi.
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting");

    unsigned long startTime = millis();
    
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        lcd.print(".");
        delay(500);

        if (millis() - startTime >= 10000) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Connection");
            lcd.setCursor(0, 1);
            lcd.print("failed");
            delay(2000);
            lcd.clear();
            Serial.println("Switching to modeOffline");
            return;
        }
    }

    Serial.println();
    Serial.print("connected: ");
    Serial.println(WiFi.localIP());
    Serial.println();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connected:");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());

    // assign the API key and RTDB URL to the config object.
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    // sign up
    if (Firebase.signUp(&config, &auth, "", "")){
        Serial.println("Sign up OK");
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sign up OK");
        delay(1500);
        lcd.clear();

        signupOK = true;
        
    }
    else {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    // assign the callback function for the long running token generation task.
    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

}

// void loop =================================================================================================================================================================================

void loop() {

    unsigned long currentMillis = millis(); // grab current time

    // modeOnline ============================================================================================================================================================================

    if (WiFi.status() == WL_CONNECTED) {

        // task3 =============================================================================================================================================================================

        unsigned int distance = sonar.ping_cm();

        if (currentMillis - previousMillis_task3 >= interval_task3 || previousMillis_task3 == 0) {

            distance = (distance > 9) ? 9 : distance;

            Serial.print("Distance: ");
            Serial.print(distance);
            Serial.println(" cm");

            if (Firebase.RTDB.setString(&fbdo, "My_SmartAquarium_v140324/distance", distance)) {

            }
            else {
                Serial.println("task3: FAILED");
                Serial.println("REASON: " + fbdo.errorReason());
            }
        
            previousMillis_task3 = currentMillis;
        }

        // task4 =============================================================================================================================================================================

        uint8_t i;
        float average;

        // take N samples in a row
        for (i=0; i< NUMSAMPLES; i++) {
            samples[i] = analogRead(THERMISTORPIN);
            delay(10);
        }

        // average all the samples out
        average = 0;
        for (i=0; i< NUMSAMPLES; i++) {
            average += samples[i];
        }
        average /= NUMSAMPLES;

        // convert the value to resistance
        average = 1023 / average - 1;
        average = SERIESRESISTOR / average;

        float steinhart;
        steinhart = average / THERMISTORNOMINAL;          // (R/Ro)
        steinhart = log(steinhart);                       // ln(R/Ro)
        steinhart /= BCOEFFICIENT;                        // 1/B * ln(R/Ro)
        steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
        steinhart = 1.0 / steinhart;                      // Invert
        steinhart -= 273.15;                              // convert absolute temp to C
            
        if (currentMillis - previousMillis_task4 >= interval_task4 || previousMillis_task4 == 0) {

            steinhart = (steinhart > 99) ? 99 : steinhart;

            String formattedValue = String(steinhart, 2);

            Serial.print("Temperature "); 
            Serial.print(formattedValue);
            Serial.println(" *C");

            if (Firebase.RTDB.setString(&fbdo, "My_SmartAquarium_v140324/steinhart", formattedValue)) {
              
            }
            else {
                Serial.println("task4: FAILED");
                Serial.println("REASON: " + fbdo.errorReason());
            }

            previousMillis_task4 = currentMillis;
        }

        // task0 =============================================================================================================================================================================

        if (currentMillis - previousMillis_task0 >= interval_task0 || previousMillis_task0 == 0) {

            // signal
            lcd.setCursor(14, 0);
            lcd.write(5);
            lcd.setCursor(15, 0);
            lcd.write(6);

            // distance
            lcd.setCursor(0, 0);
            lcd.write(1);
            lcd.setCursor(2, 0);
            lcd.print(distance);
            lcd.setCursor(4, 0);
            lcd.print("cm");

            // temp
            lcd.setCursor(0, 1);
            lcd.write(2);
            lcd.setCursor(2, 1);
            lcd.print(steinhart);
            lcd.print((char)223);
            lcd.print("C");
        
            previousMillis_task0 = currentMillis;
        }

        // task1Eks ==========================================================================================================================================================================
    
        static bool task1Eks = false;
    
        if (currentMillis - previousMillis_task1Eks >= interval_task1Eks || previousMillis_task1Eks == 0) {
            if (Firebase.RTDB.getString(&fbdo, "My_SmartAquarium_v140324/A1")) {
                if (fbdo.dataType() == "string") {
                    int Value = fbdo.stringData().toInt();
                    if (Value == 1) {
                        task1Eks = true;
                    }
                    else if (Value == 0) {
                        task1Eks = false;
                    }
                }
            }
            else {
                Serial.println("task1Eks: relay 1 FAILED");
                Serial.println("REASON: " + fbdo.errorReason());
            }

            previousMillis_task1Eks = currentMillis;
        }

        // task1Manual =======================================================================================================================================================================

        if ((currentMillis - previousMillis_task1Manual >= interval_task1Manual || previousMillis_task1Manual == 0) && (task1Eks == false)) {
            if (Firebase.RTDB.getString(&fbdo, "My_SmartAquarium_v140324/C1")) {
                if (fbdo.dataType() == "string") {
                    int Value = fbdo.stringData().toInt();
                    if (Value == 1) {

                        lcd.setCursor(12, 0);
                        lcd.write(3);

                        digitalWrite(relay1, HIGH);
                    }
                    else if (Value == 0) {

                        lcd.setCursor(12, 0);
                        lcd.print(" ");
                      
                        digitalWrite(relay1, LOW);
                    }
                }
            }
            else {
                Serial.println("task1Manual: relay 1 FAILED");
                Serial.println("REASON: " + fbdo.errorReason());
            }

            previousMillis_task1Manual = currentMillis;
        }

        // task1Auto =========================================================================================================================================================================

        if ((currentMillis - previousMillis_task1Auto >= interval_task1Auto || previousMillis_task1Auto == 0) && (task1Eks == true)) {
            if (distance > targetDist) {

                lcd.setCursor(12, 0);
                lcd.write(3);

                digitalWrite(relay1, HIGH);
          
                int value = 1;
                if (Firebase.RTDB.setString(&fbdo, "My_SmartAquarium_v140324/C1", String(value))) {
                } else {
                    Serial.println("task1Auto: relay 1 FAILED");
                    Serial.println("REASON: " + fbdo.errorReason());
                }

                if (Firebase.RTDB.setString(&fbdo, "My_SmartAquarium_v140324/targetDist", String(targetDist))) {
                } else {
                    Serial.println("task1Auto: relay 1 FAILED");
                    Serial.println("REASON: " + fbdo.errorReason());
                }
                
            }
            else {
              
                int value = 0;

                lcd.setCursor(12, 0);
                lcd.print(" ");
                
                digitalWrite(relay1, LOW);
            
                if (Firebase.RTDB.setString(&fbdo, "My_SmartAquarium_v140324/C1", String(value))) {
                } else {
                    Serial.println("task1Auto: relay 1 FAILED");
                    Serial.println("REASON: " + fbdo.errorReason());
                }

                if (Firebase.RTDB.setString(&fbdo, "My_SmartAquarium_v140324/targetDist", String(targetDist))) {
                } else {
                    Serial.println("task1Auto: relay 1 FAILED");
                    Serial.println("REASON: " + fbdo.errorReason());
                }
                
            }

            previousMillis_task1Auto = currentMillis;
        }

        // task2 =============================================================================================================================================================================

        if (currentMillis - previousMillis_task2 >= interval_task2 || previousMillis_task2 == 0) {
            if (Firebase.RTDB.getString(&fbdo, "My_SmartAquarium_v140324/C2")) {
                if (fbdo.dataType() == "string") {
                    int Value = fbdo.stringData().toInt();
                    if (Value == 1) {
                      
                        lcd.setCursor(13, 0);
                        lcd.write(4);

                        digitalWrite(relay2, HIGH);
                    }
                    else if (Value == 0) {
                      
                        lcd.setCursor(13, 0);
                        lcd.print(" ");

                        digitalWrite(relay2, LOW);
                    }
                }
            }
            else {
                Serial.println("task2: relay 2 FAILED");
                Serial.println("REASON: " + fbdo.errorReason());
            }

            previousMillis_task2 = currentMillis;
        }

        // task ==============================================================================================================================================================================

        if (currentMillis - previousMillis_task >= interval_task || previousMillis_task == 0) {
          
            Firebase.reconnectWiFi(true);

            previousMillis_task = currentMillis;
        }
    }

    // modeOffline ===========================================================================================================================================================================

    else if (WiFi.status() != WL_CONNECTED) {

        // task3 =============================================================================================================================================================================

        unsigned int distance = sonar.ping_cm();

        if (currentMillis - previousMillis_task3 >= interval_task3 || previousMillis_task3 == 0) {

            distance = (distance > 9) ? 9 : distance;

            Serial.print("Distance: ");
            Serial.print(distance);
            Serial.println(" cm");
        
            previousMillis_task3 = currentMillis;
        }

        // task4 =============================================================================================================================================================================

        uint8_t i;
        float average;

        // take N samples in a row
        for (i=0; i< NUMSAMPLES; i++) {
            samples[i] = analogRead(THERMISTORPIN);
            delay(10);
        }

        // average all the samples out
        average = 0;
        for (i=0; i< NUMSAMPLES; i++) {
            average += samples[i];
        }
        average /= NUMSAMPLES;

        // convert the value to resistance
        average = 1023 / average - 1;
        average = SERIESRESISTOR / average;

        float steinhart;
        steinhart = average / THERMISTORNOMINAL;          // (R/Ro)
        steinhart = log(steinhart);                       // ln(R/Ro)
        steinhart /= BCOEFFICIENT;                        // 1/B * ln(R/Ro)
        steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
        steinhart = 1.0 / steinhart;                      // Invert
        steinhart -= 273.15;                              // convert absolute temp to C
            
        if (currentMillis - previousMillis_task4 >= interval_task4 || previousMillis_task4 == 0) {

            steinhart = (steinhart > 99) ? 99 : steinhart;

            String formattedValue = String(steinhart, 2);

            Serial.print("Temperature "); 
            Serial.print(formattedValue);
            Serial.println(" *C");

            previousMillis_task4 = currentMillis;
        }

        // task0 =============================================================================================================================================================================

        if (currentMillis - previousMillis_task0 >= interval_task0 || previousMillis_task0 == 0) {

            // signal
            lcd.setCursor(14, 0);
            lcd.write(5);
            lcd.setCursor(15, 0);
            lcd.write(7);
            
            // distance
            lcd.setCursor(0, 0);
            lcd.write(1);
            lcd.setCursor(2, 0);
            lcd.print(distance);
            lcd.setCursor(4, 0);
            lcd.print("cm");

            // temp
            lcd.setCursor(0, 1);
            lcd.write(2);
            lcd.setCursor(2, 1);
            lcd.print(steinhart);
            lcd.print((char)223);
            lcd.print("C");
        
            previousMillis_task0 = currentMillis;
        }

        // task1Auto =========================================================================================================================================================================

        if (currentMillis - previousMillis_task1Auto >= interval_task1Auto || previousMillis_task1Auto == 0) {
            if (distance > targetDist) {

                lcd.setCursor(12, 0);
                lcd.write(3);
              
                digitalWrite(relay1, HIGH);
            }
            else {

                lcd.setCursor(12, 0);
                lcd.print(" ");
              
                digitalWrite(relay1, LOW);
            }

            previousMillis_task1Auto = currentMillis;
        }

        // task2 =============================================================================================================================================================================

        if (currentMillis - previousMillis_task2 >= interval_task2 || previousMillis_task2 == 0) {

            lcd.setCursor(13, 0);
            lcd.write(4);

            digitalWrite(relay2, HIGH);

            previousMillis_task2 = currentMillis;
        }
          
        // task ==============================================================================================================================================================================

        if (currentMillis - previousMillis_task >= interval_task || previousMillis_task == 0) {
          
            Firebase.reconnectWiFi(true);
            
            previousMillis_task = currentMillis;
        }
    }
    
}
