


// MASTER CODE (AIRBAG, OBSTACLE, ENGINE TEMPERATURE, FUEL, ANTI THEFT)


#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <string.h>
#include <stdio.h>

#define CAN_CS        10
#define CAN_INT        2
#define LED_RED        6
#define LED_GREEN      7
#define LED_YELLOW     8
#define BUZZER         5


#define ID_AIRBAG      0x001
#define ID_OBSTACLE    0x002
#define ID_TEMPERATURE 0x003

#define ID_FUEL        0x005
#define ID_THEFT       0x006

MCP_CAN CAN(CAN_CS);
LiquidCrystal_I2C lcd(0x27, 16, 2);

long unsigned int rxId;
unsigned char rxLen = 0;
unsigned char rxBuf[8];

volatile bool msgReceived = false;

void canISR() {
    msgReceived = true;
}

unsigned long buzzerStartTime = 0;
unsigned int buzzerDuration = 0;
bool buzzerActive = false;

void startBuzzer(unsigned int frequency, unsigned int durationMs) {
    tone(BUZZER, frequency);
    buzzerStartTime = millis();
    buzzerDuration = durationMs;
    buzzerActive = true;
}

void updateBuzzer() {
    if (buzzerActive && (millis() - buzzerStartTime >= buzzerDuration)) {
        noTone(BUZZER);
        buzzerActive = false;
    }
}

unsigned long alertStartTime = 0;
unsigned int alertDuration = 0;

bool alertActive() {
    return (millis() - alertStartTime) < alertDuration;
}

void setAlertExpiry(unsigned int durationMs) {
    alertStartTime = millis();
    alertDuration = durationMs;
}


char lastLine0[17] = "";
char lastLine1[17] = "";

void lcdWrite(const char* line0, const char* line1) {
    if (strcmp(line0, lastLine0) != 0) {
        lcd.setCursor(0, 0);
        lcd.print(line0);
        for (int i = strlen(line0); i < 16; i++) lcd.print(' ');
        strncpy(lastLine0, line0, 16);
        lastLine0[16] = '\0';
    }

    if (strcmp(line1, lastLine1) != 0) {
        lcd.setCursor(0, 1);
        lcd.print(line1);
        for (int i = strlen(line1); i < 16; i++) lcd.print(' ');
        strncpy(lastLine1, line1, 16);
        lastLine1[16] = '\0';
    }
}


void allLedsOff() {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
}

bool normalShown = false;

void restoreNormal() {
    if (normalShown) return;

    allLedsOff();
    digitalWrite(LED_GREEN, HIGH);
    lcdWrite(" Vehicle CAN    ", " System Normal  ");
    normalShown = true;
}

void handleAirbag() {
    if (rxLen < 1 || rxBuf[0] != 1) return;

    allLedsOff();
    digitalWrite(LED_RED, HIGH);

    lcdWrite(" AIRBAG ALERT!  ", "AIRBAG DEPLOYED ");

    startBuzzer(1500, 1000);
    setAlertExpiry(3000);

    normalShown = false;
}


void handleObstacle() {
    if (rxLen < 1) return;

    unsigned int distance = rxBuf[0];
    if (distance > 250) return;

    char line1[17];
    snprintf(line1, sizeof(line1), "Dist: %u cm", distance);

    if (distance < 20) {
        allLedsOff();
        digitalWrite(LED_RED, HIGH);
        lcdWrite("Obstacle Alert  ", line1);

        if (!buzzerActive) startBuzzer(1000, 100);
        setAlertExpiry(2000);
    }
    else if (distance < 50) {
        allLedsOff();
        digitalWrite(LED_YELLOW, HIGH);
        lcdWrite("Obstacle Near   ", line1);
        setAlertExpiry(1500);
    }
    else {
        restoreNormal();
    }

    normalShown = false;
}

void handleTemperature() {
    if (rxLen < 1) return;

    unsigned int temp = rxBuf[0];
    if (temp > 150) return;

    char line1[17];
    snprintf(line1, sizeof(line1), "Temp: %u C", temp);

    allLedsOff();
    // lcdWrite("Engine Temp     ", line1);

    if (temp > 100) {
        lcdWrite("Engine Temp     ", line1);

        digitalWrite(LED_RED, HIGH);
        if (!buzzerActive) startBuzzer(800, 500);
        setAlertExpiry(2000);
    }
    else if (temp > 80) {
        lcdWrite("Engine Temp     ", line1);

        digitalWrite(LED_YELLOW, HIGH);
        setAlertExpiry(1500);
    }
    else {
        digitalWrite(LED_GREEN, HIGH);
        setAlertExpiry(1500);
    }

    normalShown = false;
}

void handleFuel() {
    if (rxLen < 1) return;

    unsigned int fuel = rxBuf[0];
    if (fuel > 100) return;

    char line1[17];
    snprintf(line1, sizeof(line1), "Fuel: %u%%", fuel);

    lcdWrite("Fuel Status     ", line1);

    if (fuel < 20) {
        allLedsOff();
        digitalWrite(LED_RED, HIGH);
        if (!buzzerActive) startBuzzer(700, 300);
    }
    else {
        allLedsOff();
        digitalWrite(LED_GREEN, HIGH);
    }

    setAlertExpiry(1500);
    normalShown = false;
}

void handleTheft() {
    if (rxLen < 1) return;
    if (rxBuf[0] != 1) return;

    allLedsOff();
    digitalWrite(LED_RED, HIGH);

    lcdWrite("THEFT ALERT!!!  ", "Movement Found  ");

    startBuzzer(2000, 1500);
    setAlertExpiry(4000);

    normalShown = false;
}


void setup() {
    Serial.begin(9600);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(CAN_INT, INPUT);

    lcd.init();
    lcd.backlight();
    lcdWrite(" CAN Master     ", " Initializing   ");
    delay(1500);

    if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
        lcdWrite(" CAN INIT FAIL  ", "Check MCP2515   ");
        while (1);
    }

    CAN.setMode(MCP_NORMAL);
    attachInterrupt(digitalPinToInterrupt(CAN_INT), canISR, FALLING);

    restoreNormal();
}


void loop() {
    updateBuzzer();

    if (msgReceived) {
        msgReceived = false;

        while (CAN.checkReceive() == CAN_MSGAVAIL) {
            CAN.readMsgBuf(&rxId, &rxLen, rxBuf);

            switch (rxId) {
                case ID_AIRBAG:      handleAirbag(); break;
                case ID_OBSTACLE:    handleObstacle(); break;
                case ID_TEMPERATURE: handleTemperature(); break;
                case ID_FUEL:        handleFuel(); break;
                case ID_THEFT:       handleTheft(); break;
            }
        }
    }

    if (!alertActive() && !buzzerActive) {
        restoreNormal();
    }
}





