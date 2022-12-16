/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Kleber Lima da Silva <kleber.ufu@gmail.com>
 * @version V0.1.0
 * @date    08-Dez-2021
 * @brief   Code for Automatic Pet Feeder with Alexa.
 ******************************************************************************
 */

/* Includes -----------------------------------------------------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <Espalexa.h>
#include <ESP32Servo.h>
#include <esp_task_wdt.h>

/* Defines ------------------------------------------------------------------*/

/* Constants ----------------------------------------------------------------*/
const char* SSID = "KleberW6";
const char* PASS = "wifiKleber6";
#define WDT_TIMEOUT 5

/* Macros -------------------------------------------------------------------*/

/* Pin numbers --------------------------------------------------------------*/
const uint8_t RELAY = 27;       /* Water RELAY Pin */
const uint8_t SERVO = 13;       /* Food SERVO Pin */

/* Private variables --------------------------------------------------------*/
Servo foodServo;
Espalexa espAlexa;
bool wifiConnected = false;
bool foodServoOn = false;
uint32_t waterMillis = 0, foodMillis = 0;

/* Private functions (prototypes) -------------------------------------------*/
bool connectWifi();
void addDevices();
void waterCatChanged(EspalexaDevice* d);
void foodCatChanged(EspalexaDevice* d);

/* Setup --------------------------------------------------------------------*/
void setup()
{
    /* Pins initializations */
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(RELAY, OUTPUT);
    pinMode(SERVO, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(RELAY, LOW);

    /* Serial debug initialization */
    Serial.begin(9600);
    while (!Serial);
    Serial.println("Automatic Pet Feeder with Alexa");

    /* Connect to Wi-Fi network with SSID and password */
    wifiConnected = connectWifi();

    if (wifiConnected == true)
    {
        addDevices();
    }
    else
    {
        Serial.println("Cannot connect to WiFi. So in Manual Mode");
        delay(1000);
    }

    /* Servo configurations */
    ESP32PWM::allocateTimer(0);
    foodServo.setPeriodHertz(50);
    foodServo.attach(SERVO, 1000, 2000);
    foodServo.write(90);

    /* Configuring WDT */
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);

    /* End of initializations */
    digitalWrite(LED_BUILTIN, LOW);
}


/* Main Loop ----------------------------------------------------------------*/
void loop()
{
    /* Test wifi */
    if (WiFi.status() != WL_CONNECTED)
    {
        // Serial.print("WiFi Not Connected ");
        digitalWrite(LED_BUILTIN, LOW); // Turn off WiFi LED
    }
    else
    {
        // Serial.print("WiFi Connected  ");
        digitalWrite(LED_BUILTIN, HIGH);
        if (wifiConnected == true)
        {
            espAlexa.loop();
            delay(1);
        }
        else
        {
            foodServo.write(90);
            wifiConnected = connectWifi(); // Initialise wifi connection
            if (wifiConnected == true)
            {
                addDevices();
            }
        }
    }

    /* Turn off the water relay */
    if (millis() > waterMillis)
    {
        waterMillis = UINT32_MAX;
        digitalWrite(RELAY, LOW);
        Serial.println("Turn off the water relay.");
    }

    /* Turn off the food servo */
    if (millis() > foodMillis)
    {
        foodMillis = UINT32_MAX;
        foodServo.write(90);
        foodServoOn = false;
        Serial.println("Turn off the food servo.");
    }
    /* Alternate servo direction every 1s */
    if (foodServoOn == true)
    {
        int value = ((foodMillis - millis()) / 1000) % 2 ? 60 : 120;
        foodServo.write(value);
    }
    else
    {
        foodServo.write(90);
    }
    

    esp_task_wdt_reset();
} // end of main loop

/* Connect to wifi – returns true if successful or false if not -------------*/
bool connectWifi()
{
    bool state = true;
    int i = 0;

    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PASS);
    Serial.println("");
    Serial.println("Connecting to WiFi");

    /* Wait for connection */
    Serial.print("Connecting...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        if (i > 20)
        {
            state = false;
            break;
        }
        i++;
    }
    Serial.println("");
    if (state)
    {
        Serial.print("Connected to ");
        Serial.println(SSID);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("Connection failed.");
    }
    return state;
}

/* Alexa devices ------------------------------------------------------------*/
void addDevices()
{
    espAlexa.addDevice("Água do Gato", waterCatChanged, EspalexaDeviceType::dimmable);
    espAlexa.addDevice("Ração do Gato", foodCatChanged, EspalexaDeviceType::dimmable);

    espAlexa.begin();
}

/* Callback - Water Pump ----------------------------------------------------*/
void waterCatChanged(EspalexaDevice* d)
{
    if (d == nullptr)
        return;

    uint8_t percent = d->getPercent();
    digitalWrite(RELAY, percent == 0 ? LOW : HIGH);

    uint32_t delta = 60000 + (uint32_t)percent * 1000;
    waterMillis = millis() + delta;

    Serial.print("Água do Gato: ");
    Serial.print(percent);
    Serial.print("%   -   ");
    Serial.print(delta / 1000);
    Serial.println("s");
}

/* Callback - Food Servo ----------------------------------------------------*/
void foodCatChanged(EspalexaDevice *d)
{
    if (d == nullptr)
        return;

    uint8_t percent = d->getPercent();
    foodServo.write(percent == 0 ? 90 : 120);
    if (percent != 0) foodServoOn = true;
    else foodServoOn = false;

    uint32_t delta = 1000 + (uint32_t)percent * 100;
    foodMillis = millis() + delta;

    Serial.print("Ração do Gato: ");
    Serial.print(percent);
    Serial.print("%   -   ");
    Serial.print(delta);
    Serial.println("ms");
}
