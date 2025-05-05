#ifndef SR_H
#define SR_H

/* Required external function declarations */
extern void tolayer3(int, struct pkt);
extern void tolayer5(int, char[20]);
extern void starttimer(int, double);
extern void stoptimer(int);

/* Statistics variables */
extern int TRACE;
extern int total_ACKs_received;
extern int packets_resent;
extern int new_ACKs;
extern int packets_received;
extern int window_full;

/* Constants */
#define A 0
#define B 1

/* Packet structure */
struct pkt {
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

/* Message structure */
struct msg {
    char data[20];
};

/* Function prototypes */
void A_init(void);
void B_init(void);
void A_output(struct msg);
void A_input(struct pkt);
void A_timerinterrupt(void);
void B_input(struct pkt);

/* Optional for bidirectional (not needed here) */
void B_output(struct msg);
void B_timerinterrupt(void);

#endif
