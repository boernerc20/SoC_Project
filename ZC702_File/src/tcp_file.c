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
#include "esn_core.h"
#include "xil_printf.h"
#include <string.h> // for memcpy, memset

/* Global/Static variables local to this file */
static char file_buffer[MAX_FILE_SIZE];
static unsigned int file_offset = 0;
static unsigned int expected_file_size = 0;
static int expecting_header = 1;

/* Arrays for ESN Equations */
static float w_in[WIN_MAX];
static float w_x[WX_MAX];
static float w_out[WOUT_MAX];
static float data_in[DATAIN_MAX];

float state_pre[NUM_NEURONS] = {0};
float res_state[NUM_NEURONS];
float state_ext[NUM_INPUTS + NUM_NEURONS];
float data_out[4];

/* Flags to track readiness */
static int w_in_ready = 0;
static int w_x_ready = 0;
static int w_out_ready = 0;
static int data_in_ready = 0;

static void run_esn_calculation(void);

/* We can define a struct to match your file header (packed) */
typedef struct __attribute__((__packed__)) {
    char file_id[8];
    uint32_t file_size;
    char reserved[4];
} file_header_t;

/* Init function to reset global state */
void tcp_file_init(void)
{
    memset(file_buffer, 0, sizeof(file_buffer));
    file_offset = 0;
    expected_file_size = 0;
    expecting_header = 1;
}

/* Helper function for FP value printing (6 decimal places) */
static void print_fixed_6(float val)
{
    /* Handle sign */
    int negative = (val < 0.0f);
    if (negative) {
        val = -val;  /* make it positive for easier math */
    }

    /* Separate integer and fraction */
    int iPart = (int)val;
    float frac = val - (float)iPart;
    /* Multiply fraction by 1,000,000 to get 6 decimal places */
    int fPart = (int)((frac * 1000000.0f) + 0.5f); /* rounding */

    /* Print sign if needed */
    if (negative) {
        xil_printf("-");
    }
    /* Print integer part, then dot, then zero‐padded fraction */
    xil_printf("%d.%06d", iPart, fPart);
}

/* Print up to 'max_to_print' elements from a float array using print_fixed_6. */
static void print_float_array(const float *arr, int total_count, int max_to_print)
{
    /* Decide how many elements you'll actually print: */
    int limit = (total_count < max_to_print) ? total_count : max_to_print;

    xil_printf("Printing up to %d elements (out of %d):\n\r", limit, total_count);
    for (int i = 0; i < limit; i++) {
        xil_printf("arr[%d] = ", i);
        print_fixed_6(arr[i]);  // Use the new function
        xil_printf("\n\r");
    }
    xil_printf("\n\r");
}

/* Helper function to read lines or space-delimited values from the buffer and convert to FP values */
static void parse_floats_into_array(const char *raw_text,
                                    unsigned int text_len,
                                    float *dest_array,
                                    unsigned int max_count)
{
    /* Make a copy or work in-place if safe. Null-terminate. */
    char local_buf[4096]; /* or dynamic approach if file can be large */
    if (text_len >= sizeof(local_buf)) {
        text_len = sizeof(local_buf) - 1;
    }
    memcpy(local_buf, raw_text, text_len);
    local_buf[text_len] = '\0';

    /* Tokenize line by line (or space by space) */
    unsigned int count = 0;
    char *line = strtok(local_buf, "\n");
    while (line && count < max_count) {
        float val = 0.0f;
        if (sscanf(line, "%f", &val) == 1) {
            dest_array[count++] = val;
        }
        line = strtok(NULL, "\n");
    }
    xil_printf("Parsed %d floats into array.\n\r", count);
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

        /* Re-interpret the header to get the file ID */
        file_header_t *hdr = (file_header_t*)file_buffer;
        char file_id_str[9];
        memcpy(file_id_str, hdr->file_id, 8);
        file_id_str[8] = '\0';  // null-terminate

        /*
         * Now decide what to do based on file ID.
         * (WIN_____, WX______, WOUT____, DATAIN__)
         */
        if (strncmp(hdr->file_id, "WIN_____", 8) == 0) {
            // This is w_in.dat
            parse_floats_into_array(
                &file_buffer[HEADER_SIZE],
                expected_file_size,
                w_in,
                WIN_MAX
            );
            w_in_ready = 1;

            /* OPTIONAL: Print first 10 elements of w_in[] to verify */
//            print_float_array(w_in, WIN_MAX, 10);
        }
        else if (strncmp(hdr->file_id, "WX______", 8) == 0) {
            // w_x
            parse_floats_into_array(
                &file_buffer[HEADER_SIZE],
                expected_file_size,
                w_x,
                WX_MAX
            );
            w_x_ready = 1;

            /* OPTIONAL: Print first 10 elements of w_x[] */
//            print_float_array(w_x, WX_MAX, 10);
        }
        else if (strncmp(hdr->file_id, "WOUT____", 8) == 0) {
            // w_out
            parse_floats_into_array(
                &file_buffer[HEADER_SIZE],
                expected_file_size,
                w_out,
                WOUT_MAX
            );
            w_out_ready = 1;

            /* OPTIONAL: Print first 10 elements of w_out[] */
//            print_float_array(w_out, WOUT_MAX, 10);
        }
        else if (strncmp(hdr->file_id, "DATAIN__", 8) == 0) {
            // data_in
            parse_floats_into_array(
                &file_buffer[HEADER_SIZE],
                expected_file_size,
                data_in,
                DATAIN_MAX
            );
            data_in_ready = 1;

            /* OPTIONAL: Print all elements if only 40 total */
//            print_float_array(data_in, DATAIN_MAX, 40);
        }
        else if (strncmp(hdr->file_id, "CMD_ESN_", 8) == 0) {
            // This triggers ESN calculation
            run_esn_calculation();
        }

        /* Reset for the next file */
        file_offset = 0;
        expected_file_size = 0;
        expecting_header = 1;
        memset(file_buffer, 0, sizeof(file_buffer));
    }
    /* Let lwIP know we've consumed these bytes */
    tcp_recved(tpcb, bytes_copied);
    return ERR_OK;
}

/* ESN core calling function with error checking */
void run_esn_calculation(void)
{
    /* Check if each required file/array is ready. If not, say so. */
    int missing = 0;

    if (!w_in_ready || !w_x_ready || !w_out_ready || !data_in_ready) {
        xil_printf("Cannot run ESN. The following files are missing:\n\r");
        if (!w_in_ready) {
            xil_printf("  - w_in.dat (WIN_____)\n\r");
            missing++;
        }
        if (!w_x_ready) {
            xil_printf("  - w_x.dat (WX______)\n\r");
            missing++;
        }
        if (!w_out_ready) {
            xil_printf("  - w_out.dat (WOUT____)\n\r");
            missing++;
        }
        if (!data_in_ready) {
            xil_printf("  - data_in file (DATAIN__)\n\r");
            missing++;
        }
        xil_printf("Total missing: %d file(s).\n\r", missing);
        return;
    }

    xil_printf("Running ESN on the loaded arrays...\n\r");

    // ESN core computation
    update_state(w_in, data_in, w_x, state_pre, res_state);
    form_state_extended(data_in, res_state, state_ext);
    compute_output(w_out, state_ext, data_out);

    // Results for one sample
    xil_printf("ESN Output:\n\r");
    print_float_array(data_out, 4, 4);

    xil_printf("ESN computation done.\n\r");
}

