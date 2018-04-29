#include <stdio.h>

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#include <RH_RF69.h>
#include <RHReliableDatagram.h>

#define RF_CS_PIN RPI_V2_GPIO_P1_24  // Slave Select on CE0 so P1 connector pin #24
#define RF_RST_PIN RPI_V2_GPIO_P1_18 // RST on GPIO24 so P1 connector pin #18

// Our RFM69 Configuration
//#define RF_FREQUENCY  433.00
#define RF_FREQUENCY 915.00
#define RF_NODE_ID 1  // We're node ID 1 (Gateway)
#define RF_GROUP_ID 1 // Moteino default is 100, I'm using 69 on all my house

#define TEMP_ID 0 
#define IR_ID 1
#define MICROPHONE_AUDIO_ID 2 
#define MICROPHONE_ENVEL_ID 3
#define CO2_ID 4

// Create an instance of a driver
RH_RF69 rf69(RF_CS_PIN);
RHReliableDatagram rf69_manager(rf69, RF_NODE_ID);

// allows access to the floating point number for processing
// and byte representation for transmission
typedef union {
    float number;
    uint8_t bytes[4];
} FLOATUNION_t;

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

void sig_handler(int sig)
{
    printf("\n%s Break received, exiting!\n", __BASEFILE__);
    force_exit = true;
}

float convertToFloat(uint8_t* buf) {
    FLOATUNION_t x; 

    for (int i = 0; i < 4; i++) {
        x.bytes[i] = *(buf + i);
    }

    return x.number;
}

int main(int argc, char *argv[])
{
    uint8_t data[] = "And hello back to you";
    uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
    int index;

    signal(SIGINT, sig_handler);
    printf("%s\n", __BASEFILE__);

    if (!bcm2835_init())
    {
        fprintf(stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__);
        return 1;
    }

    if (!bcm2835_spi_begin())
    {
        printf("Failed\n");
        return 1;
    }

    printf("RF69 CS=GPIO%d", RF_CS_PIN);

    printf(", RST=GPIO%d", RF_RST_PIN);
    // Pulse a reset on module
    pinMode(RF_RST_PIN, OUTPUT);
    digitalWrite(RF_RST_PIN, HIGH);
    bcm2835_delay(100);
    digitalWrite(RF_RST_PIN, LOW);
    bcm2835_delay(100);

    if (!rf69_manager.init())
    {
        fprintf(stderr, "\nRF69 module init failed, Please verify wiring/module\n");
    }
    else
    {
        printf("\nRF69 module seen OK!\r\n");

        if (!rf69.setFrequency(RF_FREQUENCY))
        {
            printf("setFrequency failed\n");
        }

        rf69.setTxPower(20);

        while (!force_exit)
        {
            if (rf69_manager.available())
            {
                // Wait for a message addressed to us from the client
                uint8_t len = sizeof(buf);
                uint8_t from;
                if (rf69_manager.recvfromAck(buf, &len, &from))
                {
                    buf[len] = 0; // zero out remaining string
                    index = 0; 
                    // TODO: CONVERT TO WHILE AND CASE STATEMENT
                    uint8_t packetNum = buf[index++];
                    printf("#%d", packetNum);
                    if (buf[index++] == TEMP_ID) {
                        float temp = convertToFloat(&buf[index]);
                        index += 4;
                        printf(" %2.2fC", temp);
                    }
                    if (buf[index++] == IR_ID) {
                        float ir = convertToFloat(&buf[index]);
                        index += 4;
                        printf(" %2.2fV", ir);
                    }
                    if (buf[index++] == MICROPHONE_ENVEL_ID) {
                        float mic = convertToFloat(&buf[index]);
                        index += 4;
                        printf(" %2.2fdB", mic);
                    }
                    if (buf[index++] == CO2_ID) {
                        float c = convertToFloat(&buf[index]);
                        index += 4;
                        printf(" %2.2f", c);
                    }
                    printf("\n");

                }
            }
        }
    }

    return 0;
}