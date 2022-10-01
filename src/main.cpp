#include <Arduino.h>
#include <HX711.h>
#include <SdFat.h>

volatile uint32_t hallTicks = 0;
volatile boolean logging = false;
SdFs sd;
FsFile file;
char filename[32];

float force = 0;
float speed = 0;
unsigned long currentTime = 0;
unsigned long prevTime = 0;
HX711 lc;

elapsedMillis etime = 0;

void select_next_filename(char *buffer, SdFs *sd);
void recordData(void);
void buttonISR(void);
void hallISR(void);

void setup() {
    lc.begin(12, 11);
    lc.set_offset(0);
    pinMode(14, INPUT);
    pinMode(13, INPUT);

    attachInterrupt(digitalPinToInterrupt(13), hallISR, RISING);
    attachInterrupt(digitalPinToInterrupt(14), buttonISR, RISING);

    IntervalTimer timer;
    timer.begin(recordData, 100000);
}

void loop() {
}

void hallISR(void) {
    hallTicks++;
}

void buttonISR(void) {
    if(!logging) {
        if (!sd.begin(BUILTIN_SDCARD)) {
            if (sd.sdErrorCode()) {
                if (sd.sdErrorCode() == SD_CARD_ERROR_ACMD41) {
                    Serial.println("Try power cycling the SD card.");
                }
                sd.printSdError(&Serial);
            }
            return;
        }
        select_next_filename(filename, &sd);
        if (!file.open(filename, O_RDWR | O_CREAT)) {
            return;
        }

        file.printf("Time (s), Force (lbs), Speed\n");
        logging = true;
        delay(1000);
    } else {
        file.truncate();
        file.flush();
        file.sync();
        file.close();
        logging = false;
        delay(1000);
    }
}

void select_next_filename(char *buffer, SdFs *sd) { //Passed buff should be of size FILENAME_SIZE
    for (int fileNum = 0; fileNum < 1000; fileNum++) {
        char fileNumber[5]; //4-character number + null
        sprintf(fileNumber, "%d", fileNum);
        strcpy(buffer, "DAT");
        strcat(buffer, fileNumber);
        strcat(buffer, ".csv");
        //debugl(buffer);
        if (!sd->exists(buffer)) {
            return;
        }
    }
}

void recordData(void) {
    currentTime = millis();
    speed = hallTicks / etime;
    force = lc.read() / 10056;
    etime = 0;

    if(logging) {
        file.printf("%f, %f, %f", millis(), speed, force);
    }
}