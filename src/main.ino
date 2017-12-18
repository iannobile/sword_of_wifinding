/*
 * Project sword_of_wifinding
 * Description: Controls a sword that beeps and lights up when it gets closer to the target AP.
 * adapts a number of code samples from the particle reference docs (https://docs.particle.io/reference/firmware/photon/).
 * Author: Ian Nobile
 * Date: 2017-08-15
 */
#include "main.h"
// Globals
bool found_treasure = false;
int last_scaled = SCALE_MIN;

String apSSID = "theobjectofyourquest";

char notes[] = "gabygabyxzCDxzCDabywabywzCDEzCDEbywFCDEqywFGDEqi        azbC"; // a space represents a rest
int length = sizeof(notes); // the number of notes
int beats[] = { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1,
                2,3,3,16,};
int tempo = 75;

int segments[] = { LED0_PIN, LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN, LED5_PIN,
                    LED6_PIN, LED7_PIN, LED8_PIN, LED9_PIN };

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

void playTone(int tone, int duration) {
    for (long i = 0; i < duration * 1000L; i += tone * 2) {
        digitalWrite(SPEAKER_PIN, HIGH);
        delayMicroseconds(tone);
        digitalWrite(SPEAKER_PIN, LOW);
        delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
    /* Taken from https://gist.github.com/ianklatzco/9127560/ */
    char names[] = { 'c', 'd', 'e', 'f', 'g', 'x', 'a', 'z', 'b', 'C', 'y', 'D', 'w', 'E', 'F', 'q', 'G', 'i' };
    // c=C4, C = C5. These values have been tuned.
    int tones[] = { 1898, 1690, 1500, 1420, 1265, 1194, 1126, 1063, 1001, 947, 893, 843, 795, 749, 710, 668, 630, 594 };
  // play the tone corresponding to the note name
    for (int i = 0; i < 18; i++) {
        if (names[i] == note) {
            playTone(tones[i], duration);
        }
    }
}

int initIO() {
    pinMode(SPEAKER_PIN, OUTPUT);
    for (int i = SCALE_MIN; i < SCALE_MAX; i++ ){
        if (DEBUG) {
            Serial.printf("Testing LED : %d \n", (i+1));
            Particle.publish("Testing LED", String((i+1)));
        }
        pinMode(segments[i], OUTPUT);
        digitalWrite(segments[i], HIGH);
        delay(300);
        digitalWrite(segments[i], LOW);
        delay(300);
        tone(SPEAKER_PIN, NOTE_C5, 250);
    }
    noTone(SPEAKER_PIN);
    return 0;
}

// setup() runs once, when the device is first turned on.
void setup() {
    if (DEBUG) {
        Serial.println("In brightest day, in darkest night, no access point shall escape my sight!");
        Serial.printf("Version: %d \n", VERSION);
        Particle.publish("intializing sword_of_wifinding", PRIVATE);
        Particle.publish("version", String(VERSION), PRIVATE);
    }
    initIO();

  // Put initialization like pinMode and begin functions here.
}

ssidRSSI::ssidRSSI(){
    // initialize class
    target_rssi = 2;
}

void ssidRSSI::handle_ap(WiFiAccessPoint* wap, ssidRSSI* self) {
    self->next(*wap);
}

void ssidRSSI::next(WiFiAccessPoint& ap) {
    if (DEBUG) {
        Serial.printf("Scanning for target AP (%s) ...\n", apSSID.c_str());
        Particle.publish("scanning-for-ap", apSSID.c_str(), PRIVATE);
    }
    if (apSSID.equals(ap.ssid) == true) {
        if (DEBUG) {
            Serial.printf("Found target AP (%s) ...\n", ap.ssid);
            Particle.publish("found-target-ap", ap.ssid, PRIVATE);
        }
        target_rssi = ap.rssi;
    } else {
        if (DEBUG) {
            Serial.printf("Found AP (%s) ...\n", ap.ssid);
            //Particle.publish("found-ap", ap.ssid, PRIVATE);
            Serial.printf("%s RSSI: %d dBm\n", ap.ssid, ap.rssi);
        }
    }
}
int ssidRSSI::scan() {
    // perform the scan
    WiFi.scan(handle_ap, this);
    return target_rssi;
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    int rssi = BAD_RSSI;
    unsigned int scaled_rssi = SCALE_MIN;

    if (found_treasure == true) {
        for (int i = 0; i < length; i++) {
            if (notes[i] == ' ') {
                delay(beats[i] * tempo); // rest
            } else {
                playNote(notes[i], beats[i] * tempo);
            }
            // pause between notes
            delay(tempo / 2);
        }
        delay(100);
        found_treasure = false;
    } else {
        ssidRSSI findTargetAP;
        rssi = findTargetAP.scan();
        if (DEBUG) {
            Serial.printf("Raw RSSI: %d dBm\n", rssi);
        }
        if (rssi > 0) {
            switch (rssi) {
                case 1:
                    if (DEBUG) {
                        Serial.println("wifi-error");
                        Particle.publish("wifi-error", PRIVATE);
                    }
                case 2:
                    // In case of WiFi chipset or timeout set RSSI to the lowest value.
                    rssi = BAD_RSSI;
                    if (DEBUG) {
                        Serial.println("wifi-error");
                        Particle.publish("wifi-timeout", PRIVATE);
                    }
                    break;
                default:
                   // Getting too close to the AP will sometimes cause the RSSI value to be greater than -1.
                   rssi = GOOD_RSSI;
                   break;
            }

        }
        // Scale the RSSI value to unsigned int
        scaled_rssi = (((rssi - BAD_RSSI) * SCALE_MAX) / (GOOD_RSSI - BAD_RSSI));
        if (DEBUG) {
            Serial.printf("RSSI: %d \nScaled RSSI: %d \n",  rssi, scaled_rssi);
            Particle.publish("rssi", String(rssi), PRIVATE);
            Particle.publish("scaled_rssi", String(scaled_rssi), PRIVATE);
        }
        for (int i = SCALE_MIN; i < SCALE_MAX; i++ ){
            if (i < scaled_rssi) {
                digitalWrite(segments[i], HIGH);
            }
            else {
                digitalWrite(segments[i], LOW);
            }

            if (scaled_rssi != last_scaled) {
                if (DEBUG) {
                    Serial.printf("RSSI: %d db, Last value: %d db\n", scaled_rssi, last_scaled);
                }
                tone(SPEAKER_PIN, NOTE_C4, 250);
                last_scaled = scaled_rssi;
            }
        }
        if (scaled_rssi > 6) {
            if (DEBUG) {
                Serial.println("I found the treasure!");
                Particle.publish("found-treasure", String(scaled_rssi), PRIVATE);
            }
            found_treasure = true;
        }
    }

}
