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
