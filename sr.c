#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"

#define RTT 16.0
#define WINDOWSIZE 4
#define SEQSPACE 8
#define NOTINUSE (-1)

/* Sender side state */
static struct pkt buffer[SEQSPACE];
static bool acked[SEQSPACE];
static bool valid[SEQSPACE];
static int base = 0;
static int nextseqnum = 0;

/* Receiver side state */
static int expectedseqnum = 0;
static int B_nextseqnum = 0;
static struct pkt recvbuf[SEQSPACE];
static bool received[SEQSPACE] = { false };

int ComputeChecksum(struct pkt packet) {
    int checksum = 0;
    int i;
    checksum = packet.seqnum + packet.acknum;
    for (i = 0; i < 20; i++) {
        checksum += (int)(packet.payload[i]);
    }
    return checksum;
}

bool IsCorrupted(struct pkt packet) {
    return packet.checksum != ComputeChecksum(packet);
}

void A_output(struct msg message) {
    int i;
    struct pkt sendpkt;

    if (((nextseqnum + SEQSPACE - base) % SEQSPACE) >= WINDOWSIZE) {
        if (TRACE > 0) printf("----A: Window full. Drop message.\n");
        window_full++;
        return;
    }

    sendpkt.seqnum = nextseqnum;
    sendpkt.acknum = NOTINUSE;
    for (i = 0; i < 20; i++) {
        sendpkt.payload[i] = message.data[i];
    }
    sendpkt.checksum = ComputeChecksum(sendpkt);

    buffer[nextseqnum] = sendpkt;
    acked[nextseqnum] = false;
    valid[nextseqnum] = true;

    tolayer3(A, sendpkt);
    if (TRACE > 0) printf("A sends packet %d\n", sendpkt.seqnum);

    if (base == nextseqnum) {
        starttimer(A, RTT);
    }

    nextseqnum = (nextseqnum + 1) % SEQSPACE;
}

void A_input(struct pkt packet) {
    int acknum;

    if (IsCorrupted(packet)) {
        if (TRACE > 0) printf("----A: Corrupted ACK received\n");
        return;
    }

    acknum = packet.acknum;

    if (!valid[acknum] || acked[acknum]) {
        if (TRACE > 0) printf("----A: Duplicate ACK %d\n", acknum);
        return;
    }

    acked[acknum] = true;
    new_ACKs++;
    total_ACKs_received++;

    while (acked[base]) {
        valid[base] = false;
        base = (base + 1) % SEQSPACE;
    }

    stoptimer(A);
    if (base != nextseqnum) {
        starttimer(A, RTT);
    }
}

void A_timerinterrupt(void) {
    int i, seq;

    if (TRACE > 0) printf("----A: Timeout, retransmitting unacked packets\n");

    for (i = 0; i < WINDOWSIZE; i++) {
        seq = (base + i) % SEQSPACE;
        if (valid[seq] && !acked[seq]) {
            tolayer3(A, buffer[seq]);
            packets_resent++;
            if (TRACE > 0) printf("----A: Resent packet %d\n", seq);
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
        valid[i] = false;
    }
}

void B_input(struct pkt packet) {
    int i;
    int seq;
    struct pkt ackpkt;

    if (IsCorrupted(packet)) {
        if (TRACE > 0) printf("----B: Corrupted packet received\n");
        return;
    }

    seq = packet.seqnum;

    if (!received[seq]) {
        recvbuf[seq] = packet;
        received[seq] = true;
        if (TRACE > 0) printf("----B: Stored packet %d\n", seq);
    }

    ackpkt.seqnum = B_nextseqnum;
    ackpkt.acknum = seq;
    for (i = 0; i < 20; i++) {
        ackpkt.payload[i] = '0';
    }
    ackpkt.checksum = ComputeChecksum(ackpkt);
    tolayer3(B, ackpkt);

    if (seq == expectedseqnum) {
        while (received[expectedseqnum]) {
            tolayer5(B, recvbuf[expectedseqnum].payload);
            packets_received++;
            received[expectedseqnum] = false;
            expectedseqnum = (expectedseqnum + 1) % SEQSPACE;
        }
    }
}

void B_init(void) {
    expectedseqnum = 0;
    B_nextseqnum = 1;
}

void B_output(struct msg message) {
    /* Not used in unidirectional transfer */
}

void B_timerinterrupt(void) {
    /* Not used in unidirectional transfer */
}
