#ifndef SR_H
#define SR_H

#include "emulator.h"  // Use emulator.h to avoid redefinition of structures

#define MAX_SEQ 8      /* Sequence number space */
#define WINDOW_SIZE 4  /* Window size for Selective Repeat */

/* SR state variables */
extern int next_seq_num;    /* Next sequence number to send */
extern int base;            /* Base of the sender's window */
extern int expected_seq_num; /* Expected sequence number at receiver */
extern struct pkt buffer[MAX_SEQ];  /* Buffer for storing unacknowledged packets */
extern int ack_buffer[MAX_SEQ];      /* Acknowledgment buffer */

/* Function prototypes for SR */
extern void sr_output(struct msg message);
extern void sr_input(struct pkt packet);
extern void sr_timerinterrupt(void);
extern void sr_init(void);

#endif
