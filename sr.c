#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gbn.h"

// Define maximum window size (can be adjusted based on actual requirements)
#define WINDOW_SIZE 8

// Define maximum sequence number
#define MAX_SEQ 100

// Define the sending window
int send_base = 0;  // Starting sequence number of the sending window
int next_seqnum = 0;  // Sequence number of the next packet to send
int expected_ack[MAX_SEQ];  // To track the expected ACK sequence numbers

// Define the receiving window
int recv_base = 0;  // Starting sequence number of the receiving window

// Buffer for storing received packets
struct pkt receive_buffer[MAX_SEQ];

// Sending a packet
void A_output(struct msg message) {
    if (next_seqnum < send_base + WINDOW_SIZE) {
        struct pkt packet;
        packet.seqnum = next_seqnum;
        packet.acknum = -1;  // Ack number is not important when sending data packets
        memcpy(packet.payload, message.data, sizeof(message.data));

        // Calculate the checksum
        packet.checksum = calculate_checksum(packet);

        // Send the packet
        tolayer3(0, packet);

        // If it's the first packet, start the timer
        if (send_base == next_seqnum) {
            starttimer(0, 16.0);
        }

        // Update the next sequence number
        next_seqnum++;
    }
}

// Handle received packet at sender's side
void A_input(struct pkt packet) {
    // If the received packet's checksum matches and it's within the window
    if (packet.checksum == calculate_checksum(packet) && packet.seqnum >= send_base && packet.seqnum < next_seqnum) {
        expected_ack[packet.seqnum] = 1;  // Mark this packet as acknowledged
    }

    // Slide the window forward if the first packet in the window is acknowledged
    while (expected_ack[send_base] == 1 && send_base < next_seqnum) {
        send_base++;
    }

    // Stop the timer if the sending window is empty
    if (send_base == next_seqnum) {
        stoptimer(0);
    } else {
        starttimer(0, 16.0);
    }
}

// Timer interrupt for retransmissions
void A_timerinterrupt(void) {
    // Retransmit the entire window
    for (int i = send_base; i < next_seqnum; i++) {
        struct pkt packet = receive_buffer[i];
        tolayer3(0, packet);
    }

    // Restart the timer
    starttimer(0, 16.0);
}

// Initialize the sender
void A_init(void) {
    send_base = 0;
    next_seqnum = 0;
    memset(expected_ack, 0, sizeof(expected_ack));
}

// Handle the packet received by the receiver
void B_input(struct pkt packet) {
    if (packet.checksum == calculate_checksum(packet) && packet.seqnum >= recv_base && packet.seqnum < recv_base + WINDOW_SIZE) {
        // Deliver the data to the application layer
        tolayer5(1, packet.payload);

        // Update the receiving window
        if (packet.seqnum == recv_base) {
            while (expected_ack[recv_base] == 1 && recv_base < next_seqnum) {
                recv_base++;
            }
        }
    }
}

// Initialize the receiver
void B_init(void) {
    recv_base = 0;
}
