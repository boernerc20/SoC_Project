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
 ******************************************************************************/

#include "tcp_file.h"

/* Global/Static variables local to this file */
static char file_buffer[MAX_FILE_SIZE];
static unsigned int file_offset = 0;
static unsigned int expected_file_size = 0;
static int expecting_header = 1;
static int global_data_in_samples = 0;

/* Arrays for ESN Equations */
static float w_in[WIN_MAX];
static float w_x[WX_MAX];
static float w_out[WOUT_MAX];
//static float data_in[NUM_INPUTS * SAMPLES];
static float *data_in = NULL;
static int data_in_count = 0;  // Total number of floats parsed

/* Flags to track readiness */
static int w_in_ready = 0;
static int w_x_ready = 0;
static int w_out_ready = 0;
static int data_in_ready = 0;

//static void run_esn_calculation(void);
//static void reset_arrays(void);
//static void reset_data_in(void);

///* Define a struct to match file header (packed) */
//typedef struct __attribute__((__packed__)) {
//    char file_id[8];
//    uint32_t file_size;
//    char reserved[4];
//} file_header_t;

///* Init function to reset global state */
void tcp_file_init(void)
{
    memset(file_buffer, 0, sizeof(file_buffer));
    file_offset = 0;
    expected_file_size = 0;
    expecting_header = 1;
}

/* Helper function for FP value printing (6 decimal places) */
void print_fixed_6(float val)
{
    /* Handle sign */
    int negative = (val < 0.0f);
    if (negative) {
        val = -val;  /* positive for easier math */
    }

    /* Separate integer and fraction */
    int iPart = (int)val;
    float frac = val - (float)iPart;
    /* Multiply fraction by 1,000,000 to get 6 decimal places */
    int fPart = (int)((frac * 1000000.0f) + 0.5f); /* rounding */

    /* Print sign */
    if (negative) {
        xil_printf("-");
    }
    /* Print integer part, then dot, then zero‐padded fraction */
    xil_printf("%d.%06d", iPart, fPart);
}

/* Print up to 'max_to_print' elements from a float array */
void print_float_array(const float *arr, int total_count, int max_to_print)
{
    /* Decide how many elements to print: */
    int limit = (total_count < max_to_print) ? total_count : max_to_print;

    xil_printf("Printing up to %d elements (out of %d):\n\r", limit, total_count);
    for (int i = 0; i < limit; i++) {
        xil_printf("arr[%d] = ", i);
        print_fixed_6(arr[i]);
        xil_printf("\n\r");
    }
    xil_printf("\n\r");
}

/* Helper function to read lines or space-delimited values from the buffer and convert to FP values */
int parse_floats_into_array(const char *raw_text,
                                   unsigned int text_len,
                                   float *dest_array,
                                   unsigned int max_count)
{
	// Allocate a temporary buffer to hold the complete raw text plus a null terminator.
    char *local_buf = (char *)malloc(text_len + 1);
    if (local_buf == NULL) {
        xil_printf("Error: Could not allocate memory for parsing.\n\r");
        return 0;
    }

    // Copy the raw text into the temporary buffer.
    memcpy(local_buf, raw_text, text_len);
    local_buf[text_len] = '\0';  // Null-terminate the buffer

    unsigned int count = 0;
    // Tokenize the buffer using newline ("\n") as the delimiter.
    // Each token (line) should represent one float value in ASCII.
    char *line = strtok(local_buf, "\n");
    while (line && count < max_count) {
        float val = 0.0f;
        // Use sscanf to parse a float from the current line.
        // If the conversion is successful (sscanf returns 1), store the value in dest_array.
        if (sscanf(line, "%f", &val) == 1) {
            dest_array[count++] = val;
        }
        // Nezt token
        line = strtok(NULL, "\n");
    }
    xil_printf("Parsed %d floats into array.\n\r", count);
    // Free temp buffer
    free(local_buf);
    return count;
}


/* The actual TCP callback function (called in tcp_perf_server.c) */
err_t tcp_recv_file(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	// If not packet is recieved, connection has been closed by client
    if (!p) {
        xil_printf("Client closed connection.\r\n");
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Temp pointer 'q' to iterate through the pbuf chain
    struct pbuf *q = p;
    unsigned int bytes_copied = 0; // total number of bytes copied

    // Loop through all linked pbuf segments (in case packet is chained)
    while (q) {
    	// Length of current segment
        unsigned int copy_len = q->len;

        /* Avoid buffer overflow if file is too large */
        if (file_offset + copy_len > MAX_FILE_SIZE) {
            copy_len = MAX_FILE_SIZE - file_offset;
        }

        // Copy current segment's payload into file buffer at correct offset
        memcpy(&file_buffer[file_offset], q->payload, copy_len);

        // Update buffer with # of bytes copied
        file_offset += copy_len;
        // Accumulate total # of bytes copied
        bytes_copied += copy_len;

        // Next pbuf in chain
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
         * (WIN_____, WX______, WOUT____, DATAIN__, CMD)
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
            // Allocate memory for data_in dynamically
            int max_possible_floats = expected_file_size / 8;
            if (data_in != NULL) {
                free(data_in);
            }
            data_in = (float *)malloc(sizeof(float) * max_possible_floats);
            if (data_in == NULL) {
                xil_printf("Error: Unable to allocate memory for data_in.\n\r");
                return ERR_MEM;
            }

            // Now parse the floats into data_in.
            int total_floats = parse_floats_into_array(
                &file_buffer[HEADER_SIZE],
                expected_file_size,
                data_in,
                max_possible_floats
            );
            data_in_count = total_floats;
            int num_samples = total_floats / NUM_INPUTS;
            xil_printf("DATAIN file: parsed %d floats, which is %d sample(s)\n\r", total_floats, num_samples);
            data_in_ready = 1;

            // Optionally, store num_samples in a global variable for later use:
             global_data_in_samples = num_samples;
        }
//        else if (strncmp(hdr->file_id, "CMD_ESN_", 8) == 0) {
//            // This triggers ESN calculation
//            run_esn_calculation();
//        }
//        else if (strncmp(hdr->file_id, "CMD_RST_", 8) == 0) {
//        	xil_printf("Received CMD_RST_: Resetting all arrays...\n\r");
//        	reset_arrays();
//        }
//        else if (strncmp(hdr->file_id, "CMD_RDI_", 8) == 0) {
//        	xil_printf("Received CMD_RDI_: Resetting DATAIN...\n\r");
//        	reset_data_in();
//        }
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

    xil_printf("Running ESN on the loaded arrays (%d samples)...\n\r", global_data_in_samples);

    // Create ESN arrays
    float state_pre[NUM_NEURONS] = {0};
    float res_state[NUM_NEURONS];
    float state_extended[NUM_INPUTS + NUM_NEURONS];
    float data_out[NUM_OUTPUTS];

    for (int sample = 0; sample < global_data_in_samples; sample++) {
        float *current_sample = &data_in[sample * NUM_INPUTS];

        update_state(w_in, current_sample, w_x, state_pre, res_state);

        // Update state_pre for the next sample:
        for (int i = 0; i < NUM_NEURONS; i++) {
            state_pre[i] = res_state[i];
        }

        form_state_extended(current_sample, res_state, state_extended);
        compute_output(w_out, state_extended, data_out);

        // Print the output vector (using 6-decimal printing helper)
        xil_printf("Sample %d: y_out = [", sample);
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            print_fixed_6(data_out[i]);
            if (i < NUM_OUTPUTS - 1)
                xil_printf(", ");
        }
        xil_printf("]\n\r");
    }

    xil_printf("ESN computation complete.\n\r");
}

/* Soft reset function */
void reset_arrays(void)
{
    /* Clear flags for matrix files */
    w_in_ready = 0;
    w_x_ready = 0;
    w_out_ready = 0;
    data_in_ready = 0;

    /* Clear static arrays for matrices */
    memset(w_in, 0, sizeof(w_in));
    memset(w_x, 0, sizeof(w_x));
    memset(w_out, 0, sizeof(w_out));

    /* Free dynamic data_in if allocated */
    if (data_in != NULL) {
        free(data_in);
        data_in = NULL;
    }
    data_in_count = 0;

    xil_printf("Soft reset complete. Arrays cleared.\n\r");
}

/* Reset only the DATAIN array and related flags */
void reset_data_in(void)
{
    if (data_in != NULL) {
        free(data_in);
        data_in = NULL;
    }
    data_in_count = 0;
    global_data_in_samples = 0;
    data_in_ready = 0;
    xil_printf("DATAIN reset complete. DATAIN array cleared.\n\r");
}


