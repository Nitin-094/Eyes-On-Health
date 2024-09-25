#include <Wire.h>
#include "MAX30105.h"
#include <Arduino_LSM6DS3.h>
#include "heartRate.h"
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
MAX30105 particleSensor;

// for wifi setup
// please enter your sensitive data in the Secret tab
char ssid[] = "Redmi Note 9 Pro"; // your network SSID (name)
char pass[] = "#6618AC19";   // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS; // the Wi-Fi radio's status
int ledState = LOW;          // ledState used to set the LED

// playing with the server
char serverAddress[] = "192.168.97.1"; // server address
int port = 3000;
String route = "/";

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

String contentType = "application/x-www-form-urlencoded";
String postData;

//for fall detection
bool fall = false;
float x, y, z;



// --------------------------------------------------------------------------

const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred

float temperatureF;
float beatsPerMinute;
int beatAvg;
String s1 = "beatAvg=" ;

int button = 12; // d2 on arduino
int buttonState = 0;

bool flag = true;
int value = 0;
void setup()
{

    Serial.begin(9600);
    Serial.println("Initializing...");

    while (status != WL_CONNECTED)
    {
        Serial.print("Attempting to connect to network: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    // Initialize sensor
    if (particleSensor.begin(Wire, I2C_SPEED_FAST) == false) // Use default I2C port, 400kHz speed
    {
        Serial.println("MAX30105 was not found. Please check wiring/power. ");
        while (1)
            ;
    }

    particleSensor.setup();                    // 0 for turning the light off
    particleSensor.setPulseAmplitudeRed(0x0A); // Turn Red LED to low to indicate sensor is running
    IMU.gyroscopeSampleRate();

    pinMode(button, INPUT_PULLUP); //


    Serial.println("You're connected to the network");
    Serial.println("---------------------------------------");
}
void loop()
{

    buttonState = digitalRead(button); // High = 1 , low = 0
                                       // Serial.println(buttonState); // button check

    if (buttonState)
    {

        value++;
        long irValue = particleSensor.getIR();

        if (checkForBeat(irValue) == true)
        {
            // We sensed a beat!
            long delta = millis() - lastBeat;
            lastBeat = millis();

            beatsPerMinute = 60 / (delta / 1000.0);

            if (beatsPerMinute < 255 && beatsPerMinute > 20)
            {
                rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
                rateSpot %= RATE_SIZE;                    // Wrap variable

                // Take average of readings
                beatAvg = 0;
                for (byte x = 0; x < RATE_SIZE; x++)
                    beatAvg += rates[x];
                beatAvg /= RATE_SIZE;
            }
        }

        // Serial.print("IR=");
        // Serial.print(irValue);
        Serial.print("BPM=");
        Serial.print(beatsPerMinute);
        Serial.print(", Avg BPM=");
        Serial.print(beatAvg);

        
        postData = s1 + beatAvg+"& temperature = "+temperatureF+"& fall ="+fall;

        if (irValue < 50000)
            Serial.print(" No finger?");

        // float x, y, z;
        if (IMU.gyroscopeAvailable())
        {
            IMU.readGyroscope(x, y, z);

            Serial.print('\t');
            Serial.print("( ");
            Serial.print(x);
            Serial.print('\t');
            Serial.print(y);
            Serial.print('\t');
            Serial.print(z);
            Serial.print(" )");
            Serial.println();
        }
    }
    else
    {
        // for measuring the temperature in degree celcius.
        // float temperature = particleSensor.readTemperature();
        // Serial.print("temperatureC=");
        // Serial.print(temperature, 4);
        value= value+25;
        temperatureF = particleSensor.readTemperatureF() + 15; // Because I am a bad global citizen

        Serial.print(" temperatureF=");
        Serial.print(temperatureF, 4);
        // String s = "temperatureF=";
        postData = s1 + beatAvg+"& temperature = "+temperatureF+"& fall ="+fall;
        // postData = s + temperatureF;
        

        if (IMU.gyroscopeAvailable())
        {
            IMU.readGyroscope(x, y, z);

            Serial.print('\t');
            Serial.print("( ");
            Serial.print(x);
            Serial.print('\t');
            Serial.print(y);
            Serial.print('\t');
            Serial.print(z);
            Serial.print(" )");
            Serial.println();
        }
    }


    if(abs(y) >=4 ){
        fall = true;
        postData = s1 + beatAvg+"& temperature = "+temperatureF+"& fall ="+fall;
    }
    if(abs(y)<4){
      fall = false;
    }


    if (value > 500)
    {
        int statusCode = client.responseStatusCode();
        String response = client.responseBody();
        
        client.post(route, contentType, postData);
        
        Serial.print("Status code: ");
        Serial.println(statusCode);
        Serial.print("Response: ");
        Serial.println(response);
        Serial.println("Wait five seconds");
        delay(5000);
        value = 0;
    }
    Serial.println();
}
