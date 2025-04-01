/*******************************************************************************
 * File: tcp_file.c
 * Author: Christopher Boerner
 * Date: 04-01-2025
 *
 * Description:
 *	   Store files sent over Ethernet in a 2MB buffer and parse for ESN
 *	   equations.
 *
 *   Expected Files:
 *     - DATAIN file: 40 float values.
 *     - WIN file: 320 float values.
 *     - WX file:  64 float values.
 *     - WOUT file: 192 float values.
 *
 *   Operation:
 *     1.
 *
 ******************************************************************************/

#include "tcp_file.h"
#include "xil_printf.h"
#include <string.h> // for memcpy, memset

/* The file header format: 8 chars + 4-byte size + 4 reserved = 16 bytes */
#define HEADER_SIZE 16

#define PRINT_BYTES 100  // how many bytes to print for debugging

/* Global/Static variables local to this file */
static char file_buffer[MAX_FILE_SIZE];
static unsigned int file_offset = 0;
static unsigned int expected_file_size = 0;
static int expecting_header = 1;

/* We can define a struct to match your file header (packed) */
typedef struct __attribute__((__packed__)) {
    char file_id[8];
    uint32_t file_size;
    char reserved[4];
} file_header_t;

/* Optional init function to reset global state */
void tcp_file_init(void)
{
    memset(file_buffer, 0, sizeof(file_buffer));
    file_offset = 0;
    expected_file_size = 0;
    expecting_header = 1;
}

/* Helper function for FP value printing */
static void print_fixed_4(float val)
{
    /* Handle sign */
    int negative = (val < 0.0f);
    if (negative) {
        val = -val;  /* make it positive for easier math */
    }

    /* Separate integer and fraction */
    int iPart = (int)val;
    float frac = val - (float)iPart;
    /* Multiply fraction by 10,000 to get 4 decimal places */
    int fPart = (int)((frac * 10000.0f) + 0.5f); /* rounding */

    /* Print sign if needed */
    if (negative) {
        xil_printf("-");
    }
    /* Print integer part, then dot, then zero‐padded fraction */
    xil_printf("%d.%04d", iPart, fPart);
}


/* The actual TCP callback function */
err_t tcp_recv_file(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p) {
        xil_printf("Client closed connection.\r\n");
        tcp_close(tpcb);
        return ERR_OK;
    }

    struct pbuf *q = p;
    unsigned int bytes_copied = 0;
    while (q) {
        unsigned int copy_len = q->len;
        /* Avoid buffer overflow if file is too large */
        if (file_offset + copy_len > MAX_FILE_SIZE) {
            copy_len = MAX_FILE_SIZE - file_offset;
        }
        memcpy(&file_buffer[file_offset], q->payload, copy_len);
        file_offset += copy_len;
        bytes_copied += copy_len;

        q = q->next;
    }

    /* Free pbuf after copying */
    pbuf_free(p);

    /* Check if we've parsed the header yet */
    if (expecting_header && file_offset >= HEADER_SIZE) {
        file_header_t *hdr = (file_header_t*)file_buffer;
        expected_file_size = hdr->file_size;

        char file_id_str[9];
        memcpy(file_id_str, hdr->file_id, 8);
        file_id_str[8] = '\0';
        xil_printf("Header -> ID: %s, Size: %u bytes\n\r",
                   file_id_str, expected_file_size);

        expecting_header = 0;
    }

    /* Check if we've received the full file payload */
    if (!expecting_header &&
        file_offset >= (HEADER_SIZE + expected_file_size)) {

        xil_printf("Full file received, size: %u\n\r", expected_file_size);

//        /* Convert payload to C-string */
//        char *payload = &file_buffer[HEADER_SIZE];
//        payload[expected_file_size] = '\0'; // ensure safe string ops
//
//        /* Tokenize line by line */
//        char *line = strtok(payload, "\n");
//        while (line != NULL) {
//            float val;
//            if (sscanf(line, "%f", &val) == 1) {
//                xil_printf("Token: ");
//                print_fixed_4(val);
//                xil_printf("\n\r");
//            } else {
//                xil_printf("Skipping line, not a float: %s\n\r", line);
//            }
//            line = strtok(NULL, "\n");
//        }

        /* Echo to client */
        char msg[] = "File transfer complete\n\r";
        tcp_write(tpcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);

        /* Reset for next file */
        file_offset = 0;
        expected_file_size = 0;
        expecting_header = 1;
        memset(file_buffer, 0, sizeof(file_buffer));
    }

    /* Let lwIP know we've consumed these bytes */
    tcp_recved(tpcb, bytes_copied);
    return ERR_OK;
}
