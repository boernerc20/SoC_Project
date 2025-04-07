/*******************************************************************************
 * File: tcp_file.c
 * Author: Christopher Boerner
 * Date: 04-07-2025
 *
 *   Description:
 *	   Send commands over a separate Ethernet port (5002) to control ESN core.
 *
 *   Commands:
 *     - ESN: Start ESN core computation and generate output.
 *     - RESET: Soft reset all ESN arrays/values.
 *     - RDI: Just reset the data_in.
 *
 ******************************************************************************/

#include "tcp_command.h"

/* External network interface variable */
extern struct netif server_netif;

/*
 * cmd_recv_callback:
 *   This function is called by lwIP whenever a TCP segment arrives on the command port.
 *   It copies the incoming command into a local buffer, null-terminates it,
 *   and then checks the command text to call the appropriate function.
 */
static err_t cmd_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    char cmd_buf[CMD_BUF_SIZE];

    /* If p is NULL, the client closed the connection */
    if (!p) {
        xil_printf("Command connection closed by client.\r\n");
        tcp_close(tpcb);
        return ERR_OK;
    }

    /* Determine how many bytes to copy without overflowing cmd_buf */
    unsigned int copy_len = (p->tot_len < CMD_BUF_SIZE - 1) ? p->tot_len : CMD_BUF_SIZE - 1;
    memcpy(cmd_buf, p->payload, copy_len);
    cmd_buf[copy_len] = '\0';  // Null-terminate the command string

    xil_printf("Received command: %s\n\r", cmd_buf);

    /* Check the command text and call the appropriate function */
    if (strncmp(cmd_buf, "ESN", 3) == 0) {
        run_esn_calculation();
    } else if (strncmp(cmd_buf, "RESET", 5) == 0) {
        reset_arrays();
    } else if (strncmp(cmd_buf, "RDI", 3) == 0) {
        reset_data_in();
    } else {
        xil_printf("Unknown command received.\n\r");
    }

    /* Inform lwIP that we have received this data */
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}

/*
 * cmd_accept_callback:
 *   Called when a new client connection is accepted on the command server.
 *   It assigns the command receive callback to the new connection.
 */
static err_t cmd_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    if ((err != ERR_OK) || (newpcb == NULL)) {
        return ERR_VAL;
    }
    xil_printf("Accepted new command connection.\r\n");
    tcp_arg(newpcb, NULL);
    tcp_recv(newpcb, cmd_recv_callback);
    return ERR_OK;
}

/*
 * start_command_server:
 *   Initializes a new TCP server on CMD_PORT for command reception.
 *   This function creates a new PCB, binds it to the specified command port,
 *   sets it to listen with a limited backlog, and registers the accept callback.
 */
void start_command_server(void)
{
    struct tcp_pcb *cmd_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (cmd_pcb == NULL) {
        xil_printf("Command server: Error creating PCB. Out of memory.\r\n");
        return;
    }

    if (tcp_bind(cmd_pcb, IP_ADDR_ANY, CMD_PORT) != ERR_OK) {
        xil_printf("Command server: Unable to bind to port %d.\r\n", CMD_PORT);
        tcp_close(cmd_pcb);
        return;
    }

    /* Convert the PCB to a listening state with a backlog of 1 */
    struct tcp_pcb *cmd_listen_pcb = tcp_listen_with_backlog(cmd_pcb, 1);
    if (cmd_listen_pcb == NULL) {
        xil_printf("Command server: Out of memory while listening.\r\n");
        tcp_close(cmd_pcb);
        return;
    }
    tcp_accept(cmd_listen_pcb, cmd_accept_callback);
    xil_printf("Command server listening on port %d\n\r", CMD_PORT);
}
