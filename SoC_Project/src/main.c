#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "xparameters.h"
#include "xuartps.h"
#include "platform.h"
#include "sleep.h"
#include "xil_cache.h"

/*--- Definitions ---*/
#define RX_BUFFER_SIZE 100
#define FILE_BUFFER_SIZE 4096   // Adjust as needed
#define HEADER_SIZE 16

// Expected sample counts for each file:
#define DATAIN_SAMPLE_COUNT 40         // one_sample.dat
#define WIN_SAMPLE_COUNT (8 * 40)        // w_in.dat: 320 samples
#define WX_SAMPLE_COUNT  (8 * 8)         // w_x.dat: 64 samples
#define WOUT_SAMPLE_COUNT (4 * (40 + 8))   // w_out.dat: 192 samples

// ESN dimensions:
#define NUM_INPUTS 40      // same as DATAIN_SAMPLE_COUNT
#define NUM_NEURONS 8      // number of reservoir neurons

typedef struct {
    char id[8];       // e.g., "DATAIN__", "WIN_____", "WX______", "WOUT____"
    uint32_t size;    // File size in bytes
    char reserved[4]; // Reserved
} FileHeader;

/*--- Global UART Instances ---*/
static XUartPs UartData;
static XUartPs UartDebug;

static char dbg_buf[128];

/*--- Unchanged Helper Functions (use your existing implementations) ---*/
//void xil_printf(const char *msg) {
//    XUartPs_Send(&UartDebug, (u8 *)msg, strlen(msg));
//    while (XUartPs_IsSending(&UartDebug));
//}

void flush_uart() {
    u8 dummy[128];
    int r;
    int count = 0;
    while ((r = XUartPs_Recv(&UartData, dummy, sizeof(dummy))) > 0 && count < 1024) {
        count += r;
        usleep(5000);
    }
}

void trim_header_id(char *id) {
    for (int i = 0; i < 8; i++) {
        if (id[i] == '\n' || id[i] == '\r' || id[i] == ' ')
            id[i] = '\0';
    }
}

// Utility: Receive exactly n bytes into dest.
int receive_bytes(u8 *dest, int n) {
    int received = 0;
    while (received < n) {
        int r = XUartPs_Recv(&UartData, dest + received, n - received);
        if (r > 0)
            received += r;
        else
            usleep(1000);
    }
    return received;
}

// Receive file data: first read fileSize bytes, then continue reading until the EOF marker "<EOF>\n" is detected.
int receive_file_data(char *buffer, uint32_t fileSize) {
    int total = 0;
    // Read exactly fileSize bytes into buffer.
    total = receive_bytes((u8*)buffer, fileSize);
    buffer[total] = '\0';

    // Now, read extra bytes until we detect the EOF marker.
    char marker[8] = "";
    int marker_index = 0;
    while (1) {
        u8 byte;
        int r = XUartPs_Recv(&UartData, &byte, 1);
        if (r > 0) {
            if (marker_index < 7) {
                marker[marker_index++] = byte;
                marker[marker_index] = '\0';
            } else {
                memmove(marker, marker+1, 6);
                marker[6] = byte;
                marker[7] = '\0';
            }
            if (strcmp(marker, "<EOF>\n") == 0 || strcmp(marker, "<EOF>") == 0) {
                break;
            }
        } else {
            usleep(1000);
        }
    }
    // Flush any stray characters after the EOF marker.
    flush_uart();
    return total;
}


/*--- ESN and Processing Functions (unchanged, use your current versions) ---*/
// update_state, form_state_extended, compute_output,
// process_data_in, process_w_in, process_w_x, process_w_out, process_y_out
// For brevity, these are assumed to be defined as in your current implementation.

void update_state(float *W_in, float *dataIn, float *W_x, float *state_pre, float *state) {
    float temp1[NUM_NEURONS] = {0};
    float temp2[NUM_NEURONS] = {0};
    for (int i = 0; i < NUM_NEURONS; i++) {
        for (int j = 0; j < NUM_INPUTS; j++) {
            temp1[i] += W_in[i * NUM_INPUTS + j] * dataIn[j];
        }
    }
    for (int i = 0; i < NUM_NEURONS; i++) {
        for (int j = 0; j < NUM_NEURONS; j++) {
            temp2[i] += W_x[i * NUM_NEURONS + j] * state_pre[j];
        }
    }
    for (int i = 0; i < NUM_NEURONS; i++) {
        state[i] = tanh(temp1[i] + temp2[i]);
    }
}

void form_state_extended(float *dataIn, float *state, float *state_extended) {
    for (int i = 0; i < NUM_NEURONS; i++) {
        state_extended[i] = state[i];
    }
    for (int i = 0; i < NUM_INPUTS; i++) {
        state_extended[NUM_NEURONS + i] = dataIn[i];
    }
}

void compute_output(float *W_out, float *state_extended, float *data_out) {
    int total = NUM_INPUTS + NUM_NEURONS;
    for (int i = 0; i < 4; i++) {
        data_out[i] = 0;
        for (int j = 0; j < total; j++) {
            data_out[i] += W_out[i * total + j] * state_extended[j];
        }
    }
}

void process_data_in(float samples[DATAIN_SAMPLE_COUNT]) {
    xil_printf("Processing DATAIN file:\n\r");
    static char buf[64];
    for (int i = 0; i < DATAIN_SAMPLE_COUNT; i++) {
        sprintf(buf, "DATAIN Sample %d: %f\n\r", i + 1, samples[i]);
        xil_printf(buf);
    }
}

void process_w_in(float matrix[WIN_SAMPLE_COUNT]) {
    xil_printf("Processing W_in matrix (one column per element):\n\r");
    for (int i = 0; i < WIN_SAMPLE_COUNT; i++) {
        sprintf(dbg_buf, "W_in[%d]: %f\n\r", i, matrix[i]);
        xil_printf(dbg_buf);
    }
}

void process_w_x(float matrix[WX_SAMPLE_COUNT]) {
    xil_printf("Processing W_x matrix (one column per element):\n\r");
    for (int i = 0; i < WX_SAMPLE_COUNT; i++) {
        sprintf(dbg_buf, "W_x[%d]: %f\n\r", i, matrix[i]);
        xil_printf(dbg_buf);
    }
}

void process_w_out(float matrix[WOUT_SAMPLE_COUNT]) {
    xil_printf("Processing W_out matrix (one column per element):\n\r");
    for (int i = 0; i < WOUT_SAMPLE_COUNT; i++) {
        sprintf(dbg_buf, "W_out[%d]: %f\n\r", i, matrix[i]);
        xil_printf(dbg_buf);
    }
}

void process_y_out(float y_out[4]) {
    xil_printf("ESN output (y_out):\n\r");
    for (int i = 0; i < 4; i++) {
        sprintf(dbg_buf, "y_out[%d] = %f\n\r", i, y_out[i]);
        xil_printf(dbg_buf);
    }
}

/*--- State Machine Definitions ---*/
typedef enum {
    STATE_WAIT_MATRIX_HEADER,
    STATE_READ_MATRIX_DATA,
    STATE_WAIT_DATAIN_HEADER,
    STATE_READ_DATAIN_DATA,
    STATE_PROCESS_FILES
} ReceiveState;

/*--- Main function ---*/
int main(void)
{
    // Initialization (unchanged)
    init_platform();
//    Xil_ICacheInvalidate();
//    Xil_DCacheInvalidate();
//    flush_uart();

    // Initialize Data UART (UART0)
    XUartPs_Config *DataConfig = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
    if (!DataConfig) {
        xil_printf("UART Data Config Error.\n\r");
        return XST_FAILURE;
    }
    XUartPs_CfgInitialize(&UartData, DataConfig, DataConfig->BaseAddress);
    XUartPs_SetBaudRate(&UartData, 115200);

    // Initialize Debug UART (UART1)
    XUartPs_Config *DebugConfig = XUartPs_LookupConfig(XPAR_XUARTPS_1_DEVICE_ID);
    if (!DebugConfig) {
        xil_printf("UART Debug Config Error.\n\r");
        return XST_FAILURE;
    }
    XUartPs_CfgInitialize(&UartDebug, DebugConfig, DebugConfig->BaseAddress);
    XUartPs_SetBaudRate(&UartDebug, 115200);
    xil_printf("UART Debug Initialized. Waiting for files...\n\r");

    /* Pre-allocate arrays and temporary buffers */
    float dataIn[DATAIN_SAMPLE_COUNT];
    float wIn[WIN_SAMPLE_COUNT];
    float wX[WX_SAMPLE_COUNT];
    float wOut[WOUT_SAMPLE_COUNT];

    static char fileBuffer[FILE_BUFFER_SIZE];
    static u8 headerBuf[HEADER_SIZE];

    /* Flags to indicate successful file reception */
    int wInReady = 0, wXReady = 0, wOutReady = 0, dataInReady = 0;

    /* State machine variable */
    ReceiveState state = STATE_WAIT_MATRIX_HEADER;

    FileHeader header; // to hold the current header

    while (state != STATE_PROCESS_FILES) {
        switch (state) {
        case STATE_WAIT_MATRIX_HEADER:
            // Attempt to receive a header for a matrix file.
            if (receive_bytes(headerBuf, HEADER_SIZE) < HEADER_SIZE) {
            	xil_printf("Timeout receiving matrix header. Flushing RX...\n\r");
                flush_uart();
                continue;
            }
            memcpy(&header, headerBuf, HEADER_SIZE);
            {
                char id_str[9];
                memcpy(id_str, header.id, 8);
                id_str[8] = '\0';
                trim_header_id(id_str);
                sprintf(dbg_buf, "Received matrix header: ID=%.8s, size=%u bytes\n\r", id_str, header.size);
                xil_printf(dbg_buf);
            }
            state = STATE_READ_MATRIX_DATA;
            break;
            case STATE_READ_MATRIX_DATA:
                {
                    uint32_t fileSize = header.size;
                    if (fileSize >= FILE_BUFFER_SIZE) {
                    	xil_printf("Error: Matrix file size exceeds buffer capacity!\n\r");
                        flush_uart();
                        state = STATE_WAIT_MATRIX_HEADER;
                        break;
                    }
                    memset(fileBuffer, 0, sizeof(fileBuffer));
                    receive_file_data(fileBuffer, fileSize);
                    usleep(500000);

                    char id_str[9];
                    memcpy(id_str, header.id, 8);
                    id_str[8] = '\0';
                    trim_header_id(id_str);


                    if (strncmp(header.id, "WIN_____", 8) == 0) {
                        int count = 0;
                        char *token = strtok(fileBuffer, " ,\n\r");
                        while (token != NULL && count < WIN_SAMPLE_COUNT) {
                            if (strlen(token) > 0) {
                                wIn[count++] = strtof(token, NULL);
                            }
                            token = strtok(NULL, " ,\n\r");
                        }
                        sprintf(dbg_buf, "Parsed W_in: expected %d, got %d\n\r", WIN_SAMPLE_COUNT, count);
                        xil_printf(dbg_buf);
                        if (count == WIN_SAMPLE_COUNT) {
                            wInReady = 1;
                            xil_printf("W_in file received successfully.\n\r");
                        } else {
                            xil_printf("Error parsing W_in. Ask PC to resend.\n\r");
                            flush_uart();
                        }
                    }
                    else if (strncmp(header.id, "WX______", 8) == 0) {
                        int count = 0;
                        char *token = strtok(fileBuffer, " ,\n\r");
                        while (token != NULL && count < WX_SAMPLE_COUNT) {
                            if (strlen(token) > 0) {
                                wX[count] = strtof(token, NULL);
                                count++;
                            }
                            token = strtok(NULL, " ,\n\r");
                        }
                        {
                            sprintf(dbg_buf, "Parsed W_x: expected %d samples, got %d\n\r", WX_SAMPLE_COUNT, count);
                            xil_printf(dbg_buf);
                        }
                        if (count == WX_SAMPLE_COUNT) {
                            wXReady = 1;
                            xil_printf("W_x file received successfully.\n\r");
                        } else {
                            xil_printf("Error parsing W_x. Ask PC to resend.\n\r");
                            flush_uart();
                        }
                    }
                    else if (strncmp(header.id, "WOUT____", 8) == 0) {
                        int count = 0;
                        char *token = strtok(fileBuffer, " ,\n\r");
                        while (token != NULL && count < WOUT_SAMPLE_COUNT) {
                            if (strlen(token) > 0) {
                                wOut[count++] = strtof(token, NULL);
                            }
                            token = strtok(NULL, " ,\n\r");
                        }
                        sprintf(dbg_buf, "Parsed W_out: expected %d, got %d\n\r", WOUT_SAMPLE_COUNT, count);
                        xil_printf(dbg_buf);
                        if (count == WOUT_SAMPLE_COUNT) {
                            wOutReady = 1;
                            xil_printf("W_out file received successfully.\n\r");
                        } else {
                            xil_printf("Error parsing W_out. Ask PC to resend.\n\r");
                            flush_uart();
                        }
                    }
                    else {
                        char unknown[64];
                        sprintf(unknown, "Unknown matrix file received: %.8s\n\r", id_str);
                        xil_printf(unknown);
                        flush_uart();
                    }
                    // If any matrix file was successfully received, go back to waiting for next header.
                    if (wInReady && wXReady && wOutReady){
                    	xil_printf("Finished matrices, ready for data.\n\r");
                        state = STATE_WAIT_DATAIN_HEADER;
                    }
                    else
                        state = STATE_WAIT_MATRIX_HEADER;
                }
                break;
            case STATE_WAIT_DATAIN_HEADER:
                // Now wait for a header with DATAIN.
                if (receive_bytes(headerBuf, HEADER_SIZE) < HEADER_SIZE) {
                    xil_printf("Timeout receiving DATAIN header. Flushing RX...\n\r");
                    flush_uart();
                    continue;
                }
                memcpy(&header, headerBuf, HEADER_SIZE);
                {
                    char id_str[9];
                    memcpy(id_str, header.id, 8);
                    id_str[8] = '\0';
                    trim_header_id(id_str);
                    sprintf(dbg_buf, "Received DATAIN header: ID=%.8s, size=%u bytes\n\r", id_str, header.size);
                    xil_printf(dbg_buf);
                }
                state = STATE_READ_DATAIN_DATA;
                break;
            case STATE_READ_DATAIN_DATA:
                {
                    uint32_t fileSize = header.size;
                    if (fileSize >= FILE_BUFFER_SIZE) {
                        xil_printf("Error: DATAIN file too large!\n\r");
                        flush_uart();
                        state = STATE_WAIT_DATAIN_HEADER;
                        break;
                    }
                    memset(fileBuffer, 0, sizeof(fileBuffer));
                    receive_file_data(fileBuffer, fileSize);
                    usleep(500000);
                    int count = 0;
                    char *token = strtok(fileBuffer, " ,\n\r");
                    while (token != NULL && count < DATAIN_SAMPLE_COUNT) {
                        if (strlen(token) > 0) {
                            // Fill DATAIN array.
                            dataIn[count++] = strtof(token, NULL);
                        }
                        token = strtok(NULL, " ,\n\r");
                    }
                    {
                        char dbg[128];
                        sprintf(dbg_buf, "Parsed DATAIN: expected %d, got %d\n\r", DATAIN_SAMPLE_COUNT, count);
                        xil_printf(dbg_buf);
                    }
                    if (count == DATAIN_SAMPLE_COUNT) {
                        dataInReady = 1;
                        xil_printf("DATAIN file received successfully.\n\r");
                        state = STATE_PROCESS_FILES;
                    } else {
                        xil_printf("Error parsing DATAIN. Please resend DATAIN file.\n\r");
                        flush_uart();
                        state = STATE_WAIT_DATAIN_HEADER;
                    }
                }
                break;
            default:
                break;
        }
    }

    xil_printf("All files received. Proceeding to ESN computation...\n\r");

    // === ESN Computation ===
    float state_pre[NUM_NEURONS] = {0};  // initial reservoir state (zeros)
    float res_state[NUM_NEURONS];
    float state_extended[NUM_INPUTS + NUM_NEURONS];
    float data_out[4];

    update_state(wIn, dataIn, wX, state_pre, res_state);

    form_state_extended(dataIn, res_state, state_extended);

    compute_output(wOut, state_extended, data_out);

    process_y_out(data_out);

    cleanup_platform();
    return 0;
}
