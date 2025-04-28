/*
 * Copyright (C) 2018 - 2019 Xilinx, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 */

/*
 * Modifications by Christopher Boerner, Virginia Tech ECE, 2025.
 * - Only using tcp_server_accept, start_application, and print_app_header from original code
 * - Added echo function for testing
 * - Changed tcp_server_accept function to work with file transferring
 */

/** Connection handle for a TCP Server session */

#include "tcp_perf_server.h"

#include "esn_main.h"

extern struct netif server_netif;

void print_app_header(void)
{
	xil_printf("TCP server listening on port %d\r\n",
			TCP_CONN_PORT);
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    if ((err != ERR_OK) || (newpcb == NULL)) {
        return ERR_VAL;
    }
//    xil_printf("Accepted new TCP client connection\r\n");

    // Optionally re-init file globals each time
    tcp_file_init();

    tcp_arg(newpcb, NULL);
    /* Use the new function from tcp_file.c */
    tcp_recv(newpcb, tcp_recv_file);
//    tcp_err(newpcb, tcp_server_err);

    return ERR_OK;
}

void start_application(void)
{
	err_t err;
	struct tcp_pcb *pcb, *lpcb;

	/* Create Server PCB */
	pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
	if (!pcb) {
		xil_printf("TCP server: Error creating PCB. Out of Memory\r\n");
		return;
	}

	err = tcp_bind(pcb, IP_ADDR_ANY, TCP_CONN_PORT);
	if (err != ERR_OK) {
		xil_printf("TCP server: Unable to bind to port %d: "
				"err = %d\r\n" , TCP_CONN_PORT, err);
		tcp_close(pcb);
		return;
	}

	/* Set connection queue limit to 1 to serve
	 * one client at a time
	 */
	lpcb = tcp_listen_with_backlog(pcb, 1);
	if (!lpcb) {
		xil_printf("TCP server: Out of memory while tcp_listen\r\n");
		tcp_close(pcb);
		return;
	}

	/* we do not need any arguments to callback functions */
	tcp_arg(lpcb, NULL);

	/* specify callback to use for incoming connections */
	tcp_accept(lpcb, tcp_server_accept);

	return;
}
