// NODE 2 (FUEL + ANTI-THEFT)


#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS          10
#define FUEL_PIN        A0     
#define IR_PIN          9      

#define ID_FUEL         0x005
#define ID_THEFT        0x006

MCP_CAN CAN(CAN_CS);

unsigned int lastFuel = 255;
bool lastTheftState = HIGH;

void setup() {
    Serial.begin(9600);

    pinMode(IR_PIN, INPUT);

    if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
        Serial.println("Slave2 CAN OK");
    } else {
        Serial.println("Slave2 CAN FAIL");
        while (1);
    }

    CAN.setMode(MCP_NORMAL);
    Serial.println("Slave 2 ready — Fuel + Theft");
}

unsigned int readFuelLevel() {
    int raw = analogRead(FUEL_PIN);
    
    unsigned int fuelPercent = map(raw, 0, 1023, 0, 100);

    return fuelPercent;
}


void loop() {

   
    unsigned int fuel = readFuelLevel();

    Serial.print("Fuel Level: ");
    Serial.print(fuel);
    Serial.println("%");

    if (abs((int)fuel - (int)lastFuel) > 2) {
        byte txData[1];
        txData[0] = (byte)fuel;

        if (CAN.sendMsgBuf(ID_FUEL, 0, 1, txData) == CAN_OK) {
            Serial.println("Fuel sent OK");
            lastFuel = fuel;
        }
    }


    bool theftState = digitalRead(IR_PIN);

    if (lastTheftState == HIGH && theftState == LOW) {
        Serial.println("THEFT DETECTED");

        byte txData[1] = {1};

        if (CAN.sendMsgBuf(ID_THEFT, 0, 1, txData) == CAN_OK) {
            Serial.println("Theft signal sent");
        }
    }

    lastTheftState = theftState;

    delay(300);
}
