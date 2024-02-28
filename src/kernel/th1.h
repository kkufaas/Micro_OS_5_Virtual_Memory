#ifndef TH1_H
#define TH1_H

/* Loads shell */
void loader_thread(void);

/* Runs indefinitely */
void clock_thread(void);

/* Scans USB hub ports */
void usb_thread(void);

#endif /* !TH1_H */
