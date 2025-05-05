#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SEQ 8
#define MAX_PACKET_SIZE 1024

/* Define the pkt struct (packet structure) */
struct pkt {
    int seq_num;      // Sequence number of the packet
    int ack_num;      // Acknowledgment number
    char data[MAX_PACKET_SIZE];  // Data to be transmitted
    int checksum;     // Checksum for error detection
};

/* Define the msg struct (message structure) */
struct msg {
    char data[MAX_PACKET_SIZE];  // Data in the message
};

/* Function declarations for external functions */
extern int calculate_checksum(struct pkt packet);
extern void tolayer3(int sender_id, struct pkt packet);
extern void starttimer(int sender_id, float time);
extern void stoptimer(int sender_id);
extern void tolayer5(char *data);

int window_size = 4;  // Size of the sliding window
int base = 0;         // Base sequence number for sender's window
int next_seq_num = 0; // Sequence number of the next packet to be sent
struct pkt send_buffer[MAX_SEQ];   // Send buffer to hold packets
struct pkt receive_buffer[MAX_SEQ];  // Receive buffer to hold received packets

/* Function to calculate the checksum of a packet */
int calculate_checksum(struct pkt packet) {
    int checksum = 0;
    checksum += packet.seq_num;  // Add sequence number
    checksum += packet.ack_num;  // Add acknowledgment number
    for (int i = 0; i < strlen(packet.data); i++) {
        checksum += packet.data[i];  // Add each byte of the data
    }
    return checksum;
}

/* Function to send the packet to the network layer */
void tolayer3(int sender_id, struct pkt packet) {
    // Logic for sending the packet to the network layer
}

/* Function to start a timer */
void starttimer(int sender_id, float time) {
    // Logic for starting the timer
}

/* Function to stop the timer */
void stoptimer(int sender_id) {
    // Logic for stopping the timer
}

/* Function to send data to the application layer */
void tolayer5(char *data) {
    // Logic for delivering the data to the application layer
}

/* Function to send a packet */
void send_packet() {
    struct pkt packet;
    packet.seq_num = next_seq_num;  // Set the sequence number of the packet
    packet.ack_num = -1;            // No acknowledgment number for data packet
    strcpy(packet.data, "Hello, World!");  // Set the data in the packet
    packet.checksum = calculate_checksum(packet);  // Calculate checksum

    send_buffer[next_seq_num % MAX_SEQ] = packet;  // Store packet in the send buffer
    tolayer3(0, packet);  // Send packet to network layer
    next_seq_num++;  // Increment the sequence number

    // If the next sequence number is within the window size, start the timer
    if (next_seq_num < base + window_size) {
        starttimer(0, 20.0);  // Start timer for the sender
    }
}

/* Function to receive and process a packet */
void receive_packet(struct pkt packet) {
    if (packet.checksum == calculate_checksum(packet)) {  // If checksum is valid
        if (packet.seq_num == base) {  // If the packet is expected (in-order)
            tolayer5(packet.data);  // Deliver data to the application layer
            base++;  // Increment the base sequence number
            if (base == next_seq_num) {
                stoptimer(0);  // Stop the timer if all packets are received
            } else {
                starttimer(0, 20.0);  // Start timer if there are more packets to be received
            }
        } else {
            /* Ignore out-of-order packets */
        }
    } else {
        /* Drop packets with invalid checksum */
    }
}

/* Function to start sending packets within the window size */
void start_sending() {
    while (next_seq_num < base + window_size) {  // Send packets until the window is full
        send_packet();
    }
}

/* Main function where the sender logic is executed */
int main() {
    // Initialize and start the sending process
    start_sending();

    return 0;
}
