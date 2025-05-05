#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"

#define RTT 16.0
#define WINDOWSIZE 4
#define SEQSPACE 2 * WINDOWSIZE
#define NOTINUSE (-1)

struct pkt buffer[SEQSPACE];
bool acked[SEQSPACE];
bool valid[SEQSPACE];
int base = 0;
int nextseqnum = 0;

int expectedseqnum = 0;
int B_nextseqnum = 0;

int ComputeChecksum(struct pkt packet) {
    int checksum = packet.seqnum + packet.acknum;
    for (int i = 0; i < 20; i++)
        checksum += (int)(packet.payload[i]);
    return checksum;
}

bool IsCorrupted(struct pkt packet) {
    return packet.checksum != ComputeChecksum(packet);
}

void A_output(struct msg message) {
    if (((nextseqnum + SEQSPACE - base) % SEQSPACE) >= WINDOWSIZE) {
        if (TRACE > 0) printf("----A: Window full. Drop message.\n");
        window_full++;
        return;
    }

    struct pkt sendpkt;
    sendpkt.seqnum = nextseqnum;
    sendpkt.acknum = NOTINUSE;
    for (int i = 0; i < 20; i++)
        sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt);

    buffer[nextseqnum] = sendpkt;
    acked[nextseqnum] = false;
    valid[nextseqnum] = true;

    tolayer3(A, sendpkt);
    if (TRACE > 0) printf("A sends packet %d\n", sendpkt.seqnum);

    if (base == nextseqnum)
        starttimer(A, RTT);

    nextseqnum = (nextseqnum + 1) % SEQSPACE;
}

void A_input(struct pkt packet) {
    if (IsCorrupted(packet)) {
        if (TRACE > 0) printf("----A: Corrupted ACK\n");
        return;
    }

    int acknum = packet.acknum;

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
    if (base != nextseqnum)
        starttimer(A, RTT);
}

void A_timerinterrupt(void) {
    if (TRACE > 0) printf("----A: Timer Interrupt. Resending unacked packets.\n");
    for (int i = 0; i < WINDOWSIZE; i++) {
        int seq = (base + i) % SEQSPACE;
        if (valid[seq] && !acked[seq]) {
            tolayer3(A, buffer[seq]);
            packets_resent++;
            if (TRACE > 0) printf("----A: Resent packet %d\n", seq);
        }
    }
    starttimer(A, RTT);
}

void A_init(void) {
    base = 0;
    nextseqnum = 0;
    for (int i = 0; i < SEQSPACE; i++) {
        acked[i] = false;
        valid[i] = false;
    }
}

void B_input(struct pkt packet) {
    static struct pkt recvbuf[SEQSPACE];
    static bool received[SEQSPACE] = { false };

    if (IsCorrupted(packet)) {
        if (TRACE > 0) printf("----B: Corrupted packet\n");
        return;
    }

    int seq = packet.seqnum;
    if (!received[seq]) {
        recvbuf[seq] = packet;
        received[seq] = true;
        if (TRACE > 0) printf("----B: Packet %d stored\n", seq);
    }

    struct pkt ackpkt;
    ackpkt.seqnum = B_nextseqnum;
    ackpkt.acknum = seq;
    for (int i = 0; i < 20; i++) ackpkt.payload[i] = '0';
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
