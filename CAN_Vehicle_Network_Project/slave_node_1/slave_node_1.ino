


// NODE 1(HCSR04 + LM35 + PUSH_BUTTON)


#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS        10
#define TRIG_PIN       3
#define ECHO_PIN       4
#define LM35_PIN      A1
#define AIRBAG_PIN     6   

#define ID_AIRBAG     0x001   
#define ID_OBSTACLE   0x002
#define ID_TEMPERATURE 0x003

MCP_CAN CAN(CAN_CS);

unsigned int lastDistance = 999;
unsigned int lastTemp = 999;
bool lastAirbagState = HIGH;   

void setup() {
    Serial.begin(9600);

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(AIRBAG_PIN, INPUT_PULLUP);   

    digitalWrite(TRIG_PIN, LOW);
    delay(500);

    if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        Serial.println("Slave1 CAN OK");
    } else {
        Serial.println("Slave1 CAN FAIL");
        while (1);
    }

    CAN.setMode(MCP_NORMAL);
    Serial.println("Slave 1 ready — HC-SR04 + LM35 + AIRBAG");
}


unsigned int measureDistance() {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(4);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000);

    if (duration == 0) return 999;

    unsigned int distance = duration / 58;
    if (distance < 2 || distance > 250) return 999;

    return distance;
}


unsigned int readTemperature() {
    int raw = analogRead(LM35_PIN);
    float voltage = raw * (1.1 / 1023.0);
    float tempC = voltage * 100.0;

    if (tempC < 0.0 || tempC > 150.0) return 255;

    return (unsigned int) tempC;
}

void loop() {

    
    bool currentState = digitalRead(AIRBAG_PIN);

    if (lastAirbagState == HIGH && currentState == LOW) {
        Serial.println("AIRBAG TRIGGERED");

        byte txData[1] = {1};

        CAN.sendMsgBuf(ID_AIRBAG, 0, 1, txData);
    }

    lastAirbagState = currentState;

    
    unsigned int distance = measureDistance();

    if (distance == 999) {
        Serial.println("Distance: out of range");
    } else {
        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm");

        if (distance < 50 && abs((int)distance - (int)lastDistance) > 5) {

            byte txData[1];
            txData[0] = (byte) distance;

            if (CAN.sendMsgBuf(ID_OBSTACLE, 0, 1, txData) == CAN_OK) {
                Serial.println("Distance sent OK");
                lastDistance = distance;
            }
        }
    }

    unsigned int temp = readTemperature();

    if (temp == 255) {
        Serial.println("Temperature: invalid reading");
    } else {
        Serial.print("Temperature: ");
        Serial.print(temp);
        Serial.println(" C");

        if (abs((int)temp - (int)lastTemp) > 1) {

            byte txData[1];
            txData[0] = (byte) temp;

            if (CAN.sendMsgBuf(ID_TEMPERATURE, 0, 1, txData) == CAN_OK) {
                Serial.println("Temperature sent OK");
                lastTemp = temp;
            }
        }
    }

    delay(300);
}




