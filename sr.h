#ifndef SR_H
#define SR_H

#include "emulator.h"

#define MAX_SEQ 8      // Sequence number space
#define WINDOW_SIZE 4  // Window size for Selective Repeat

// Packet structure
struct pkt {
    int seqnum;        // Sequence number of the packet
    int acknum;        // Acknowledgment number
    int checksum;      // Checksum for error detection
    char payload[20];  // Payload data
};

// Message structure
struct msg {
    char data[20];     // Data in the message
};

// Declare function prototypes
extern int calculate_checksum(struct pkt packet);
extern void tolayer3(int calling_entity, struct pkt packet);
extern void starttimer(int calling_entity, float time);
extern void stoptimer(int calling_entity);
extern void tolayer5(int calling_entity, char *data);

// SR state variables
extern int next_seq_num;    // Next sequence number to send
extern int base;            // Base of the sender's window
extern int window_size;     // Size of the sender's window
extern int expected_seq_num; // Expected sequence number at receiver
extern struct pkt buffer[MAX_SEQ];  // Buffer for storing unacknowledged packets
extern int ack_buffer[MAX_SEQ];      // Acknowledgment buffer

// SR functions to be implemented
void sr_output(struct msg message);
void sr_input(struct pkt packet);
void sr_timerinterrupt(void);
void sr_init(void);
void sr_receive(struct pkt packet);

#endif
