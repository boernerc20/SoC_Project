#ifndef TCP_FILE_H
#define TCP_FILE_H

#include "lwip/tcp.h"
#include "lwip/pbuf.h"

/* Buffer size for File Recieption Buffer */
#define MAX_FILE_SIZE (3072 * 3072)  /* 3MB (data_in is 2.7 MB) */

err_t tcp_recv_file(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

/* Optionally declare an init or helper function */
void tcp_file_init(void);

#endif /* TCP_FILE_H */
