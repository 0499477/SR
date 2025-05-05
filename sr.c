#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"

#define RTT 16.0
#define WINDOWSIZE 4
#define SEQSPACE (2 * WINDOWSIZE)
#define NOTINUSE -1

static struct pkt send_buffer[SEQSPACE];
static bool acked[SEQSPACE];
static bool sent[SEQSPACE];
static int base = 0;
static int nextseqnum = 0;

static struct pkt recv_buffer[SEQSPACE];
static bool received[SEQSPACE];
static int expectedseqnum = 0;

int ComputeChecksum(struct pkt packet) {
    int checksum = 0;
    int i;
    checksum += packet.seqnum + packet.acknum;
    for (i = 0; i < 20; i++) {
        checksum += (int)packet.payload[i];
    }
    return checksum;
}

bool IsCorrupted(struct pkt packet) {
    return ComputeChecksum(packet) != packet.checksum;
}

/* Called when layer 5 provides a message to A */
void A_output(struct msg message) {
    int i;
    struct pkt packet;

    if (((nextseqnum + SEQSPACE - base) % SEQSPACE) >= WINDOWSIZE) {
        if (TRACE > 0) printf("----A: Window full. Drop message.\n");
        window_full++;
        return;
    }

    packet.seqnum = nextseqnum;
    packet.acknum = NOTINUSE;
    for (i = 0; i < 20; i++) {
        packet.payload[i] = message.data[i];
    }
    packet.checksum = ComputeChecksum(packet);

    send_buffer[nextseqnum] = packet;
    sent[nextseqnum] = true;
    acked[nextseqnum] = false;

    tolayer3(A, packet);
    if (TRACE > 0) printf("----A: Sent packet %d\n", packet.seqnum);

    if (base == nextseqnum)
        starttimer(A, RTT);

    nextseqnum = (nextseqnum + 1) % SEQSPACE;
}

/* Called when ACK arrives at A */
void A_input(struct pkt packet) {
    int acknum;

    if (IsCorrupted(packet)) {
        if (TRACE > 0) printf("----A: Corrupted ACK received\n");
        return;
    }

    acknum = packet.acknum;

    if (!sent[acknum] || acked[acknum]) {
        if (TRACE > 0) printf("----A: Duplicate or invalid ACK %d\n", acknum);
        return;
    }

    acked[acknum] = true;
    new_ACKs++;
    total_ACKs_received++;

    while (acked[base]) {
        acked[base] = false;
        sent[base] = false;
        base = (base + 1) % SEQSPACE;
    }

    stoptimer(A);
    if (base != nextseqnum)
        starttimer(A, RTT);
}

/* Timer expired: retransmit unacked packets */
void A_timerinterrupt(void) {
    int i;
    if (TRACE > 0) printf("----A: Timer expired, resend all unacked packets\n");

    for (i = 0; i < WINDOWSIZE; i++) {
        int idx = (base + i) % SEQSPACE;
        if (sent[idx] && !acked[idx]) {
            tolayer3(A, send_buffer[idx]);
            packets_resent++;
            if (TRACE > 0) printf("----A: Resent packet %d\n", idx);
        }
    }
    starttimer(A, RTT);
}

void A_init(void) {
    int i;
    base = 0;
    nextseqnum = 0;
    for (i = 0; i < SEQSPACE; i++) {
        acked[i] = false;
        sent[i] = false;
    }
}

/* B receives packets and delivers in order */
void B_input(struct pkt packet) {
    int i;
    struct pkt ackpkt;

    if (IsCorrupted(packet)) {
        if (TRACE > 0) printf("----B: Corrupted packet received, discard\n");
        return;
    }

    if (!received[packet.seqnum]) {
        recv_buffer[packet.seqnum] = packet;
        received[packet.seqnum] = true;
        if (TRACE > 0) printf("----B: Stored packet %d\n", packet.seqnum);
    }

    ackpkt.seqnum = 0;
    ackpkt.acknum = packet.seqnum;
    for (i = 0; i < 20; i++) ackpkt.payload[i] = '0';
    ackpkt.checksum = ComputeChecksum(ackpkt);
    tolayer3(B, ackpkt);

    while (received[expectedseqnum]) {
        tolayer5(B, recv_buffer[expectedseqnum].payload);
        packets_received++;
        received[expectedseqnum] = false;
        expectedseqnum = (expectedseqnum + 1) % SEQSPACE;
    }
}

void B_init(void) {
    int i;
    expectedseqnum = 0;
    for (i = 0; i < SEQSPACE; i++) {
        received[i] = false;
    }
}

void B_output(struct msg message) {
    /* unused */
}

void B_timerinterrupt(void) {
    /* unused */
}
