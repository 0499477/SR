#include "sr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SR state variables
int next_seq_num = 0;
int base = 0;
int expected_seq_num = 0;
int window_size = WINDOW_SIZE;
struct pkt buffer[MAX_SEQ];  // Buffer for storing unacknowledged packets
int ack_buffer[MAX_SEQ];     // Acknowledgment buffer

// Calculate checksum for a packet (simple sum of fields)
int calculate_checksum(struct pkt packet) {
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    return checksum;
}

// sr_output: Called when a message arrives at the sender (A)
void sr_output(struct msg message) {
    struct pkt packet;
    packet.seqnum = next_seq_num;
    packet.acknum = -1;  // Not used for sender packets
    memcpy(packet.payload, message.data, sizeof(packet.payload));
    packet.checksum = calculate_checksum(packet);
    
    buffer[next_seq_num % MAX_SEQ] = packet;  // Store packet in buffer
    tolayer3(0, packet);  // Send packet to the network
    
    if (next_seq_num < base + window_size) {
        starttimer(0, 16.0);  // Start timer for the window
    }
    
    next_seq_num = (next_seq_num + 1) % MAX_SEQ;
}

// sr_input: Called when a packet arrives at the receiver (B)
void sr_input(struct pkt packet) {
    // Validate checksum
    if (calculate_checksum(packet) != packet.checksum) {
        return;  // Corrupted packet, ignore it
    }
    
    // Check if the packet is within the expected range
    if (packet.seqnum == expected_seq_num) {
        tolayer5(1, packet.payload);  // Deliver data to upper layer
        expected_seq_num = (expected_seq_num + 1) % MAX_SEQ;
    }

    // Send ACK for the received packet
    struct pkt ack_packet;
    ack_packet.seqnum = -1;
    ack_packet.acknum = packet.seqnum;
    ack_packet.checksum = calculate_checksum(ack_packet);
    tolayer3(1, ack_packet);
}

// sr_timerinterrupt: Called when the timer expires
void sr_timerinterrupt(void) {
    // Retransmit all unacknowledged packets
    for (int i = base; i < next_seq_num; i++) {
        tolayer3(0, buffer[i % MAX_SEQ]);
    }
    starttimer(0, 16.0);  // Restart the timer for the window
}

// sr_init: Initialize state variables
void sr_init(void) {
    base = 0;
    next_seq_num = 0;
    expected_seq_num = 0;
    window_size = WINDOW_SIZE;
    memset(buffer, 0, sizeof(buffer));  // Clear buffer
    memset(ack_buffer, 0, sizeof(ack_buffer));  // Clear acknowledgment buffer
}
