#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"

/* ******************************************************************
   Selective Repeat protocol.  Adapted from Go Back N implementation
**********************************************************************/

#define RTT  16.0       /* round trip time.  MUST BE SET TO 16.0 when submitting assignment */
#define WINDOWSIZE 6    /* the maximum number of buffered unacked packets */
#define SEQSPACE 7      /* the sequence space must be at least windowsize + 1 */
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */

/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver */
int ComputeChecksum(struct pkt packet)
{
  int checksum = 0;
  int i;

  checksum = packet.seqnum;
  checksum += packet.acknum;
  for ( i=0; i<20; i++) 
    checksum += (int)(packet.payload[i]);

  return checksum;
}

bool IsCorrupted(struct pkt packet)
{
  if (packet.checksum == ComputeChecksum(packet))
    return false;
  else
    return true;
}

/********* Sender (A) variables and functions for Selective Repeat ************/

static struct pkt buffer[WINDOWSIZE];  /* array for storing packets waiting for ACK */
static int windowfirst, windowlast;    /* array indexes of the first/last packet awaiting ACK */
static int windowcount;                /* the number of packets currently awaiting an ACK */
static int A_nextseqnum;               /* the next sequence number to be used by the sender */
static bool A_ack_received[WINDOWSIZE];  /* array to track received ACKs */

/* called from layer 5 (application layer), passed the message to be sent to the other side */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  int i;

  /* if not blocked waiting on ACK */
  if (windowcount < WINDOWSIZE) {
    if (TRACE > 1)
      printf("----A: New message arrives, send window is not full, send new message to layer3!\n");

    /* create packet */
    sendpkt.seqnum = A_nextseqnum;
    sendpkt.acknum = NOTINUSE;
    for (i = 0; i < 20; i++) 
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt); 

    /* put packet in window buffer */
    windowlast = (windowlast + 1) % WINDOWSIZE; 
    buffer[windowlast] = sendpkt;
    A_ack_received[windowlast] = false;
    windowcount++;

    /* send out packet */
    if (TRACE > 0)
      printf("Sending packet %d to layer 3\n", sendpkt.seqnum);
    tolayer3(A, sendpkt);

    /* start timer if first packet in window */
    if (windowcount == 1)
      starttimer(A, RTT);

    /* get next sequence number, wrap back to 0 */
    A_nextseqnum = (A_nextseqnum + 1) % SEQSPACE;
  } else {
    if (TRACE > 0)
      printf("----A: New message arrives, send window is full\n");
  }
}

/* called from layer 3, when a packet arrives for layer 4 
   In this practical, this will always be an ACK as B never sends data. */
void A_input(struct pkt packet)
{
  int i;

  /* if received ACK is not corrupted */
  if (!IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----A: uncorrupted ACK %d is received\n", packet.acknum);
    total_ACKs_received++;

    /* find the index of the ACK */
    int ack_index = (packet.acknum - windowfirst + WINDOWSIZE) % WINDOWSIZE;

    /* if the ACK has not been received yet */
    if (!A_ack_received[ack_index]) {
      /* Mark the ACK as received */
      A_ack_received[ack_index] = true;

      /* slide the window */
      windowfirst = (windowfirst + 1) % WINDOWSIZE;
      windowcount--;

      /* start timer again if there are still more unacknowledged packets in the window */
      stoptimer(A);
      if (windowcount > 0)
        starttimer(A, RTT);
    }
  } else {
    if (TRACE > 0)
      printf("----A: corrupted ACK is received, do nothing!\n");
  }
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
  int i;

  if (TRACE > 0)
    printf("----A: timeout, resending packets!\n");

  /* resend all unacknowledged packets */
  for (i = 0; i < WINDOWSIZE; i++) {
    if (!A_ack_received[i]) {
      if (TRACE > 0)
        printf("----A: resending packet %d\n", buffer[(windowfirst + i) % WINDOWSIZE].seqnum);
      tolayer3(A, buffer[(windowfirst + i) % WINDOWSIZE]);
      packets_resent++;
    }
  }

  /* restart the timer if there are unacknowledged packets */
  if (windowcount > 0)
    starttimer(A, RTT);
}

/* initialization for the sender */
void A_init(void)
{
  A_nextseqnum = 0;
  windowfirst = 0;
  windowlast = -1;  /* windowlast starts at -1 */
  windowcount = 0;
  for (int i = 0; i < WINDOWSIZE; i++) {
    A_ack_received[i] = false;
  }
}

/********* Receiver (B) variables and functions for Selective Repeat ************/

static int expectedseqnum; /* the sequence number expected next by the receiver */
static int B_nextseqnum;   /* the sequence number for the next packets sent by B */

/* called from layer 3, when a packet arrives for layer 4 at B */
void B_input(struct pkt packet)
{
  struct pkt sendpkt;
  int i;

  /* if packet is not corrupted */
  if (!IsCorrupted(packet)) {
    /* check if the packet is in order or out of order */
    if (packet.seqnum == expectedseqnum) {
      if (TRACE > 0)
        printf("----B: packet %d is correctly received, sending ACK!\n", packet.seqnum);
      packets_received++;

      /* deliver to the receiving application */
      tolayer5(B, packet.payload);

      /* send an ACK for the received packet */
      sendpkt.acknum = expectedseqnum;

      /* update expected sequence number */
      expectedseqnum = (expectedseqnum + 1) % SEQSPACE;
    } else {
      if (TRACE > 0)
        printf("----B: packet %d is out of order, resend last ACK!\n", packet.seqnum);
      sendpkt.acknum = expectedseqnum - 1;
      if (sendpkt.acknum < 0)
        sendpkt.acknum = SEQSPACE - 1;
    }

    /* create and send the ACK packet */
    sendpkt.seqnum = B_nextseqnum;
    B_nextseqnum = (B_nextseqnum + 1) % 2;  /* B only uses 0 and 1 for seq numbers */
    for (i = 0; i < 20; i++) 
      sendpkt.payload[i] = '0';  /* no actual data is sent by B */
    sendpkt.checksum = ComputeChecksum(sendpkt);
    tolayer3(B, sendpkt);
  } else {
    if (TRACE > 0)
      printf("----B: corrupted packet, resend last ACK!\n");

    /* resend last ACK in case of corruption */
    sendpkt.acknum = expectedseqnum - 1;
    if (sendpkt.acknum < 0)
      sendpkt.acknum = SEQSPACE - 1;

    sendpkt.seqnum = B_nextseqnum;
    B_nextseqnum = (B_nextseqnum + 1) % 2;
    for (i = 0; i < 20; i++)
      sendpkt.payload[i] = '0';
    sendpkt.checksum = ComputeChecksum(sendpkt);
    tolayer3(B, sendpkt);
  }
}

/* initialization for the receiver */
void B_init(void)
{
  expectedseqnum = 0;
  B_nextseqnum = 1;
}
