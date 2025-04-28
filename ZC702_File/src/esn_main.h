#ifndef TCP_FILE_H
#define TCP_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "esn_core.h"
#include "xil_printf.h"
#include "rls_training.h"
#include <string.h> // for memcpy, memset

/* Buffer size for File Reception Buffer */
#define MAX_FILE_SIZE   (3072 * 3072)  /* 3MB (can be adjusted) */
#define MAX_BUFFER_SIZE (1024 * 1024)  /* 1MB (can be adjusted) */

/* The file header format:
 *  8 bytes for ID
 * +4 bytes for file_size
 * +4 bytes reserved
 * = 16 bytes total
 */
#define HEADER_SIZE 16

#define SAMPLES     140

/* Expected integer counts for each file: */
#define WIN_MAX     	(NUM_NEURONS * NUM_INPUTS)
#define WX_MAX      	(NUM_NEURONS * NUM_NEURONS)
#define WOUT_MAX    	(NUM_OUTPUTS * (NUM_INPUTS + NUM_NEURONS))
#define DATA_OUT_MAX    (NUM_OUTPUTS * SAMPLES)

/* Define a struct to match file header (packed) */
typedef struct __attribute__((__packed__)) {
    char file_id[8];
    uint32_t file_size;
    char reserved[4];
} file_header_t;

/* Init function to reset global state */
void tcp_file_init(void);

/* Helper function for FP value printing (6 decimal places) */
void print_fixed_6(float val);

/* Print up to 'max_to_print' elements from a float array */
void print_float_array(const float *arr, int total_count, int max_to_print);

int parse_floats_into_array(const char *raw_text,
                                   unsigned int text_len,
                                   float *dest_array,
                                   unsigned int max_count);
/*
 * tcp_recv_file:
 *   The main callback function handling file data arrival.
 *   - arg, tpcb, p, err are lwIP parameters for the TCP callback.
 *   - returns an lwIP err_t status.
 */
err_t tcp_recv_file(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

/*
 * tcp_file_init:
 *   Resets any global state for file reception,
 *   clearing buffers/flags. Call at startup or after a reset command.
 */



/* ESN-Related Function Prototypes */
void run_esn_calculation(int num_samples_in_chunk);
void reset_arrays(void);
void reset_data_in(void);

#ifdef __cplusplus
}
#endif

#endif /* TCP_FILE_H */
