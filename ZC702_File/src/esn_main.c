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

#include "esn_main.h"

/* Global/Static variables local to this file */
static char file_buffer[MAX_FILE_SIZE];
static unsigned int file_offset = 0;
static unsigned int expected_file_size = 0;
static int expecting_header = 1;
//static int global_data_in_samples = 0;

/* Arrays for ESN Equations */
static float w_in[WIN_MAX];
static float w_x[WX_MAX];
static float w_out[WOUT_MAX];
static float golden_data_out[DATA_OUT_MAX];
static int golden_sample_count = 0;

static float *data_in = NULL;
static int data_in_count = 0;  // Total number of floats parsed
//static float *data_out = NULL;
//static int data_out_count = 0;

/* Flags to track readiness */
static int w_in_ready = 0;
static int w_x_ready = 0;
//static int w_out_ready = 0;
static int golden_data_out_ready = 0;

// Keep state_pre consistent between chunks
static float state_pre[NUM_NEURONS] = {0};
static int total_samples_processed = 0;    // cumulative sample count

///* Init function to reset global state */
void tcp_file_init(void)
{
    memset(file_buffer, 0, sizeof(file_buffer));
    file_offset = 0;
    expected_file_size = 0;
    expecting_header = 1;
}

static void print_scientific(float val)
{
    char buf[32];  // Buffer size for the formatted string.
    // Format the float in scientific notation (exponent form).
    sprintf(buf, "%e", val);
    xil_printf("%s", buf);
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

///* Helper function to read lines or space-delimited values from the buffer and convert to FP values */
//int parse_floats_into_array(const char *raw_text,
//                                   unsigned int text_len,
//                                   float *dest_array,
//                                   unsigned int max_count)
//{
//	// Allocate a temporary buffer to hold the complete raw text plus a null terminator.
//    char *local_buf = (char *)malloc(text_len + 1);
//    if (local_buf == NULL) {
//        xil_printf("Error: Could not allocate memory for parsing.\n\r");
//        return 0;
//    }
//
//    // Copy the raw text into the temporary buffer.
//    memcpy(local_buf, raw_text, text_len);
//    local_buf[text_len] = '\0';  // Null-terminate the buffer
//
//    unsigned int count = 0;
//    // Tokenize the buffer using newline ("\n") as the delimiter.
//    // Each token (line) should represent one float value in ASCII.
//    char *line = strtok(local_buf, "\n");
//    while (line && count < max_count) {
//        float val = 0.0f;
//        // Use sscanf to parse a float from the current line.
//        // If the conversion is successful (sscanf returns 1), store the value in dest_array.
//        if (sscanf(line, "%f", &val) == 1) {
//            dest_array[count++] = val;
//        }
//        // Nezt token
//        line = strtok(NULL, "\n");
//    }
//    xil_printf("Parsed %d floats into array.\n\r", count);
//    // Free temp buffer
//    free(local_buf);
//    return count;
//}

/* Helper function to read lines or space-delimited values from the buffer and convert to FP values */
int parse_floats_into_array(const char *raw_text,
                                   unsigned int text_len,
                                   float *dest_array,
                                   unsigned int max_count)
{
    // Using static buffer instead of malloc() for data_out
    static char static_buf[250000]; // Allocate 250k bytes
    if (text_len >= sizeof(static_buf)) {
        xil_printf("Error: File too large for static buffer.\n\r");
        return 0;
    }

    memcpy(static_buf, raw_text, text_len);
    static_buf[text_len] = '\0';  // Null-terminate the buffer

    unsigned int count = 0;
    char *line = strtok(static_buf, "\n");
    while (line && count < max_count) {
        float val = 0.0f;
        if (sscanf(line, "%f", &val) == 1) {
            dest_array[count++] = val;
        }
        line = strtok(NULL, "\n");
    }
//    xil_printf("Parsed %d floats into array.\n\r", count);
    return count;
}



/* The actual TCP callback function (called in tcp_perf_server.c) */
err_t tcp_recv_file(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	// If not packet is recieved, connection has been closed by client
    if (!p) {
//        xil_printf("Client closed connection.\r\n");
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

//        xil_printf("Full file received, size: %u\n\r", expected_file_size);

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
        	int parsedCount = parse_floats_into_array(
                &file_buffer[HEADER_SIZE],
                expected_file_size,
                w_out,
                WOUT_MAX
            );

            // Optionally check that the expected number of floats was parsed.
            if (parsedCount != WOUT_MAX) {
                xil_printf("Warning: Expected %d floats for W_out but parsed %d floats.\n\r", WOUT_MAX, parsedCount);
            }

            // Use the setter function to update the global W_out matrix.
            set_W_out(w_out);
//            w_out_ready = 1;


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
//            data_in_ready = 1;

//            // Optionally, store num_samples in a global variable for later use:
//            global_data_in_samples = num_samples;

            run_esn_calculation(num_samples);
        }
        else if (strncmp(hdr->file_id, "DATAOUT_", 8) == 0) {
        	// In your DATAOUT branch (in tcp_recv_file or a separate routine):
        	int total_floats = parse_floats_into_array(&file_buffer[HEADER_SIZE],
        	                                            expected_file_size,
        	                                            golden_data_out,
        	                                            DATA_OUT_MAX);
        	golden_sample_count = total_floats / NUM_OUTPUTS;
        	xil_printf("Golden DATAOUT file: parsed %d floats, which is %d sample(s)\n\r", total_floats, golden_sample_count);
//            // Allocate memory for data_in dynamically
//            int max_possible_floats = expected_file_size / 8;
//            if (data_out != NULL) {
//                free(data_out);
//            }
//            data_out = (float *)malloc(sizeof(float) * max_possible_floats);
//            if (data_out == NULL) {
//                xil_printf("Error: Unable to allocate memory for data_out.\n\r");
//                return ERR_MEM;
//            }
//
//            // Now parse the floats into data_out.
//            int total_floats = parse_floats_into_array(
//                &file_buffer[HEADER_SIZE],
//                expected_file_size,
//                data_out,
//                max_possible_floats
//            );
//            data_out_count = total_floats;
//            xil_printf("DATAOUT file: parsed %d floats", total_floats);

            /* OPTIONAL: Print first 10 elements of w_in[] to verify */
//            print_float_array(golden_data_out, DATA_OUT_MAX, 10);

            golden_data_out_ready = 1;
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
void run_esn_calculation(int num_samples_in_chunk)
{
    /* Check if each required file/array is ready. If not, say so. */
    int missing = 0;
    if (!w_in_ready || !w_x_ready || !golden_data_out_ready) {
        xil_printf("Cannot run ESN. The following are missing:\n\r");
        if (!w_in_ready) {
            xil_printf("  - w_in.dat (WIN_____)\n\r");
            missing++;
        }
        if (!w_x_ready) {
            xil_printf("  - w_x.dat (WX______)\n\r");
            missing++;
        }
        if (!golden_data_out_ready) {
            xil_printf("  - golden_data_out file (DATAOUT_)\n\r");
            missing++;
        }
        xil_printf("Total missing: %d file(s).\n\r", missing);
        return;
    }

    // Create ESN arrays (can be reset from chunk to chunk)
    float res_state[NUM_NEURONS];
    float state_extended[EXTENDED_STATE_SIZE];
    float data_out[NUM_OUTPUTS];

    // For overall error accumulation:
    float total_mse = 0.0f;
    int samples_compared = 0;

    for (int sample = 0; sample < num_samples_in_chunk; sample++) {
        // Pointer to current sample in the new chunk data_in
        float *current_sample = &data_in[sample * NUM_INPUTS];
        // Use the current updated W_out:
        float *current_W_out = get_W_out();

        // Process current sample using the persistent state_pre
        update_state(w_in, current_sample, w_x, state_pre, res_state);

        // Update state_pre for the next sample
        for (int i = 0; i < NUM_NEURONS; i++) {
            state_pre[i] = res_state[i];
        }

        form_state_extended(current_sample, res_state, state_extended);

        compute_output(current_W_out, state_extended, data_out);

        // Optionally, print output
        xil_printf("Sample %d: y_out = [", (total_samples_processed + sample + 1));
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            print_fixed_6(data_out[i]);
            if (i < NUM_OUTPUTS - 1)
                xil_printf(", ");
        }
        xil_printf("]\n\r");

        // Compare output with golden output for the current sample, if available
        if ((total_samples_processed + sample) < golden_sample_count) {
            // Pointer to the corresponding golden output (4 floats per sample)
            float *golden_sample = &golden_data_out[(total_samples_processed + sample) * NUM_OUTPUTS];
            float mse = compute_mse(data_out, golden_sample, NUM_OUTPUTS);

            total_mse += mse;
            samples_compared++;

            // Update the output weights using the online RLS training function.
            update_training_rls(state_extended, golden_sample);
        }
        else {
            xil_printf("No golden output available for sample %d.\n\r", sample);
        }
    }

    // If at least one sample was compared, print the average MSE.
    if (samples_compared > 0) {
        float avg_mse = total_mse / samples_compared;
        xil_printf("Average MSE over %d sample(s): ", samples_compared);
        print_scientific(avg_mse);
        xil_printf("\n\r");
    } else {
        xil_printf("No samples were compared with golden data.\n\r");
    }

    total_samples_processed += num_samples_in_chunk;
    xil_printf("Chunk processed. Total samples processed: %d\n\r", total_samples_processed);
}

/* Soft reset function */
void reset_arrays(void)
{
    /* Clear flags for matrix files */
    w_in_ready = 0;
    w_x_ready = 0;
//    w_out_ready = 0;
    golden_data_out_ready = 0;

    /* Clear static arrays for matrices */
    memset(w_in, 0, sizeof(w_in));
    memset(w_x, 0, sizeof(w_x));
    memset(w_out, 0, sizeof(w_out));
    set_W_out(w_out);
    memset(state_pre, 0, sizeof(state_pre));
    memset(w_out, 0, sizeof(w_out));

    /* Free dynamic data_in if allocated */
    if (data_in != NULL) {
        free(data_in);
        data_in = NULL;
    }
    data_in_count = 0;
    total_samples_processed = 0;

    disable_training();

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
    total_samples_processed = 0;
    memset(state_pre, 0, sizeof(state_pre));
//    global_data_in_samples = 0;
//    data_in_ready = 0;
    xil_printf("DATAIN reset complete. DATAIN array cleared.\n\r");
}


