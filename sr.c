#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "emulator.h"
#include "sr.h"

#define RTT 16.0
#define WINDOWSIZE 6
#define SEQSPACE 12 // > 2 * WINDOWSIZE to avoid ambiguity
#define NOTINUSE (-1)

// Utility
int ComputeChecksum(struct pkt packet) {
    int checksum = packet.seqnum + packet.acknum;
    for (int i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    return checksum;
}

bool IsCorrupted(struct pkt packet) {
    return packet.checksum != ComputeChecksum(packet);
}

/************ Sender A ************/

static struct pkt A_buffer[SEQSPACE]; // All possible seqnum packets
static bool A_acked[SEQSPACE];
static bool A_sent[SEQSPACE];
static double A_timer[SEQSPACE];
static int A_base = 0;
static int A_nextseq = 0;

void A_output(struct msg message) {
    if ((A_nextseq - A_base + SEQSPACE) % SEQSPACE >= WINDOWSIZE) {
        if (TRACE > 0)
            printf("----A: Window full, message dropped\n");
        window_full++;
        return;
    }

    struct pkt packet;
    packet.seqnum = A_nextseq;
    packet.acknum = NOTINUSE;
    memcpy(packet.payload, message.data, 20);
    packet.checksum = ComputeChecksum(packet);

    A_buffer[A_nextseq] = packet;
    A_sent[A_nextseq] = true;
    A_acked[A_nextseq] = false;

    if (TRACE > 0)
        printf("Sending packet %d to layer 3\n", packet.seqnum);
    tolayer3(A, packet);
    starttimer(A, RTT);
    A_timer[packet.seqnum] = RTT;

    A_nextseq = (A_nextseq + 1) % SEQSPACE;
}

void A_input(struct pkt packet) {
    if (IsCorrupted(packet)) {
        if (TRACE > 0)
            printf("----A: Corrupted ACK received, ignoring\n");
        return;
    }

    total_ACKs_received++;

    int acknum = packet.acknum;
    if (!A_acked[acknum]) {
        A_acked[acknum] = true;
        stoptimer(A);
        new_ACKs++;

        // Slide window base
        while (A_acked[A_base]) {
            A_sent[A_base] = false;
            A_base = (A_base + 1) % SEQSPACE;
        }

        // Restart timer if needed
        for (int i = 0; i < SEQSPACE; i++) {
            if (A_sent[i] && !A_acked[i]) {
                starttimer(A, RTT);
                break;
            }
        }
    } else {
        if (TRACE > 0)
            printf("----A: Duplicate ACK %d received, ignored\n", acknum);
    }
}

void A_timerinterrupt(void) {
    if (TRACE > 0)
        printf("----A: Timer interrupt\n");

    for (int i = 0; i < SEQSPACE; i++) {
        if (A_sent[i] && !A_acked[i]) {
            tolayer3(A, A_buffer[i]);
            packets_resent++;
            if (TRACE > 0)
                printf("---A: Timeout, resend packet %d\n", i);
            starttimer(A, RTT);
            break;
        }
    }
}

void A_init(void) {
    for (int i = 0; i < SEQSPACE; i++) {
        A_acked[i] = false;
        A_sent[i] = false;
    }
    A_base = 0;
    A_nextseq = 0;
}

/************ Receiver B ************/

static bool B_received[SEQSPACE];
static struct pkt B_buffer[SEQSPACE];
static int B_base = 0;

void B_input(struct pkt packet) {
    if (IsCorrupted(packet)) {
        if (TRACE > 0)
            printf("----B: Corrupted packet received, ignored\n");
        return;
    }

    int seq = packet.seqnum;
    packets_received++;

    if (!B_received[seq]) {
        B_received[seq] = true;
        B_buffer[seq] = packet;

        if (TRACE > 0)
            printf("----B: Packet %d received and buffered\n", seq);

        // Deliver all in-order packets
        while (B_received[B_base]) {
            tolayer5(B, B_buffer[B_base].payload);
            B_received[B_base] = false;
            B_base = (B_base + 1) % SEQSPACE;
        }
    } else {
        if (TRACE > 0)
            printf("----B: Duplicate packet %d received, re-ACK\n", seq);
    }

    // Send ACK regardless
    struct pkt ack;
    ack.seqnum = 0;
    ack.acknum = seq;
    memset(ack.payload, 0, 20);
    ack.checksum = ComputeChecksum(ack);
    tolayer3(B, ack);
}

void B_init(void) {
    for (int i = 0; i < SEQSPACE; i++) {
        B_received[i] = false;
    }
    B_base = 0;
}

void B_output(struct msg message) {}
void B_timerinterrupt(void) {}
