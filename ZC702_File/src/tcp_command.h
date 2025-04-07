#ifndef TCP_COMMAND_H
#define TCP_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "tcp_file.h"   // For run_esn_calculation(), reset_arrays(), reset_data_in(), etc.
#include "esn_core.h"   // If any ESN functions are needed directly
#include "xil_printf.h"
#include <string.h>
#include <stdlib.h>

/* Define the TCP port to be used for command reception */
#define CMD_PORT 5002
/* Define a reasonable command buffer size */
#define CMD_BUF_SIZE 64

/* Function prototype to start the command server */
void start_command_server(void);

#ifdef __cplusplus
}
#endif

#endif /* TCP_COMMAND_H */
