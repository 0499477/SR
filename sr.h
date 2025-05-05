#ifndef SR_H
#define SR_H

#include "gbn.h"

// Define the maximum window size and buffer size
#define WINDOW_SIZE 8
#define MAX_SEQ 100

// Sender-related function declarations
void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt(void);
void A_init(void);

// Receiver-related function declarations
void B_input(struct pkt packet);
void B_init(void);

#endif

#include "sr.h"

// Calculate checksum for a given packet
int calculate_checksum(struct pkt packet) {
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;

    // Add the payload data to the checksum calculation
    for (int i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }

    // Invert the checksum to meet the required format
    checksum = ~checksum;

    return checksum;
}
