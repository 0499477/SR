#ifndef SR_H
#define SR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SEQ 8          // Maximum sequence number
#define MAX_PACKET_SIZE 1024  // Maximum packet size

// Define the pkt struct (packet structure)
struct pkt {
    int seq_num;         // Sequence number of the packet
    int ack_num;         // Acknowledgment number
    char data[MAX_PACKET_SIZE];  // Data to be transmitted
    int checksum;        // Checksum for error detection
};

// Define the msg struct (message structure)
struct msg {
    char data[MAX_PACKET_SIZE];  // Data in the message
};

// Function prototypes
extern int calculate_checksum(struct pkt packet);  // Declaration for checksum calculation
extern void tolayer3(int sender_id, struct pkt packet);  // Function for sending a packet to the network layer
extern void starttimer(int sender_id, float time);     // Function for starting a timer
extern void stoptimer(int sender_id);                  // Function for stopping a timer
extern void tolayer5(char *data);                       // Function for delivering data to the application layer

// Sliding window protocol variables
extern int window_size;        // Size of the sliding window
extern int base;               // Base sequence number for sender's window
extern int next_seq_num;      // Sequence number of the next packet to be sent
extern struct pkt send_buffer[MAX_SEQ];   // Send buffer to hold packets
extern struct pkt receive_buffer[MAX_SEQ];  // Receive buffer to hold received packets

// Function declarations for sending and receiving packets
void send_packet();
void receive_packet(struct pkt packet);
void start_sending();

#endif  // SR_H
