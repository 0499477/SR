#ifndef SR_H
#define SR_H

#include "emulator.h"  // includes struct pkt and struct msg definitions

// Constants
#define BIDIRECTIONAL 0

// Function declarations
extern void A_init(void);
extern void B_init(void);
extern void A_output(struct msg);
extern void A_input(struct pkt);
extern void A_timerinterrupt(void);
extern void B_input(struct pkt);
extern void B_output(struct msg);         // required even if unused
extern void B_timerinterrupt(void);       // required even if unused

#endif
