#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>      // For uint32_t
#include <math.h>        // For tanh()
#include "xparameters.h"
#include "xuartps.h"
#include "platform.h"
#include "sleep.h"       // For usleep()

// Data UART port: using UART0 (XPAR_XUARTPS_0_DEVICE_ID)
static XUartPs UartData;
// Debug UART port: using UART1 (XPAR_XUARTPS_1_DEVICE_ID)
static XUartPs UartDebug;

#define RX_BUFFER_SIZE 100
#define FILE_BUFFER_SIZE 4096   // Adjust as needed for your file sizes
#define HEADER_SIZE 16
#define TIMEOUT_USEC 500000     // 500 ms timeout

// Expected sample counts
#define DATAIN_SAMPLE_COUNT 40    // one_sample.dat
#define WIN_SAMPLE_COUNT (8 * 40)   // W_in: 320 samples
#define WX_SAMPLE_COUNT  (8 * 8)    // W_x: 64 samples
#define WOUT_SAMPLE_COUNT (4 * (40 + 8))  // W_out: 4 x 48 = 192 samples

// Define dimensions for the echo state update:
#define NUM_INPUTS 40      // same as DATAIN_SAMPLE_COUNT
#define NUM_NEURONS 8      // number of reservoir neurons

// Header structure (16 bytes)
typedef struct {
    char id[8];       // e.g., "DATAIN", "WIN____", "WX_____", "WOUT____"
    uint32_t size;    // File size in bytes (assumed in host order)
    char reserved[4]; // Reserved
} FileHeader;

// Debug print: sends message over debug UART
void debug_print(const char *msg) {
    XUartPs_Send(&UartDebug, (u8 *)msg, strlen(msg));
    while (XUartPs_IsSending(&UartDebug));
}

// Trim header ID (remove stray newline, carriage return, or space)
void trim_header_id(char *id) {
    for (int i = 0; i < 8; i++) {
        if (id[i] == '\n' || id[i] == '\r' || id[i] == ' ')
            id[i] = '\0';
    }
}

// Flush the UART receive buffer (read any leftover bytes)
void flush_uart() {
    u8 dummy[128];
    int r;
    int count = 0;
    while ((r = XUartPs_Recv(&UartData, dummy, sizeof(dummy))) > 0 && count < 1024) {
        count += r;
        usleep(1000);
    }
}

// Processing functions for each file type (for demonstration)
void process_data_in(float samples[DATAIN_SAMPLE_COUNT]) {
    debug_print("Processing DATAIN file:\n\r");
    for (int i = 0; i < DATAIN_SAMPLE_COUNT; i++) {
        char buf[64];
        sprintf(buf, "DATAIN Sample %d: %f\n\r", i + 1, samples[i]);
        debug_print(buf);
    }
}

void process_w_in(float matrix[WIN_SAMPLE_COUNT]) {
    debug_print("Processing W_in matrix (one column per element):\n\r");
    for (int i = 0; i < WIN_SAMPLE_COUNT; i++) {
        char buf[64];
        sprintf(buf, "W_in[%d]: %f\n\r", i, matrix[i]);
        debug_print(buf);
    }
}

void process_w_x(float matrix[WX_SAMPLE_COUNT]) {
    debug_print("Processing W_x matrix (one column per element):\n\r");
    for (int i = 0; i < WX_SAMPLE_COUNT; i++) {
        char buf[64];
        sprintf(buf, "W_x[%d]: %f\n\r", i, matrix[i]);
        debug_print(buf);
    }
}

void process_w_out(float matrix[WOUT_SAMPLE_COUNT]) {
    debug_print("Processing W_out matrix (one column per element):\n\r");
    for (int i = 0; i < WOUT_SAMPLE_COUNT; i++) {
        char buf[64];
        sprintf(buf, "W_out[%d]: %f\n\r", i, matrix[i]);
        debug_print(buf);
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

// ---- Echo State Network Functions ----
// Update reservoir state: state = tanh(W_in * dataIn + W_x * state_pre)
void update_state(float *W_in, float *dataIn, float *W_x, float *state_pre, float *state) {
    float temp1[NUM_NEURONS] = {0};  // for W_in * dataIn
    float temp2[NUM_NEURONS] = {0};  // for W_x * state_pre

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

// Form extended state vector: state_extended = { state; dataIn }
void form_state_extended(float *dataIn, float *state, float *state_extended) {
    // Note: now we want the first 8 elements to be the state,
    // followed by the 40 dataIn samples.
    for (int i = 0; i < NUM_NEURONS; i++) {
        state_extended[i] = state[i];
    }
    for (int i = 0; i < NUM_INPUTS; i++) {
        state_extended[NUM_NEURONS + i] = dataIn[i];
    }
}

// Compute ESN output: data_out = W_out * state_extended
// W_out is 4x48 (4 rows, 48 columns), state_extended is 48x1, data_out is 4x1.
void compute_output(float *W_out, float *state_extended, float *data_out) {
    int total = NUM_INPUTS + NUM_NEURONS; // 48
    for (int i = 0; i < 4; i++) {
        data_out[i] = 0;
        for (int j = 0; j < total; j++) {
            data_out[i] += W_out[i * total + j] * state_extended[j];
        }
    }
}

int main(void)
{
    init_platform();

    // === UART0 Data Initialization (for receiving files) ===
    XUartPs_Config *DataConfig = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
    if (!DataConfig) {
        debug_print("UART Data Config Error.\n\r");
        return XST_FAILURE;
    }
    XUartPs_CfgInitialize(&UartData, DataConfig, DataConfig->BaseAddress);
    XUartPs_SetBaudRate(&UartData, 115200);

    // === UART1 Debug Initialization (for debug output) ===
    XUartPs_Config *DebugConfig = XUartPs_LookupConfig(XPAR_XUARTPS_1_DEVICE_ID);
    if (!DebugConfig) {
        debug_print("UART Debug Config Error.\n\r");
        return XST_FAILURE;
    }
    XUartPs_CfgInitialize(&UartDebug, DebugConfig, DebugConfig->BaseAddress);
    XUartPs_SetBaudRate(&UartDebug, 115200);

    debug_print("UART Debug Initialized. Waiting for files...\n\r");

    // Pre-allocate arrays for each file type.
    float dataIn[DATAIN_SAMPLE_COUNT];
    float wIn[WIN_SAMPLE_COUNT];
    float wX[WX_SAMPLE_COUNT];
    float wOut[WOUT_SAMPLE_COUNT];

    // Variables for echo state network computation.
    float state_pre[NUM_NEURONS] = {0}; // initial reservoir state (zeros)
    float state[NUM_NEURONS];
    float state_extended[NUM_INPUTS + NUM_NEURONS];
    float data_out[4];

    // Temporary buffer for file reception.
    char fileBuffer[FILE_BUFFER_SIZE];

    // Flags to indicate successful receipt of each file.
    int dataInReady = 0, wInReady = 0, wXReady = 0, wOutReady = 0;

    // Main loop: receive files one by one.
    while (1) {
        u8 headerBuf[HEADER_SIZE];
        receive_bytes(headerBuf, HEADER_SIZE);
        FileHeader header;
        memcpy(&header, headerBuf, HEADER_SIZE);
        trim_header_id(header.id);
        uint32_t fileSize = header.size;
        {
            char dbg[128];
            sprintf(dbg, "Received header: ID=%.8s, size=%u bytes\n\r", header.id, fileSize);
            debug_print(dbg);
        }
        if (fileSize >= FILE_BUFFER_SIZE) {
            debug_print("Error: File size exceeds buffer capacity!\n\r");
            continue;
        }
        int bytesReceived = receive_file_data(fileBuffer, fileSize);
        {
            char dbg[128];
            sprintf(dbg, "Complete file received for ID %.8s\n\r", header.id);
            debug_print(dbg);
        }
        usleep(500000);

        if (strncmp(header.id, "DATAIN", 6) == 0) {
            int count = 0;
            char *token = strtok(fileBuffer, " ,\n\r");
            while (token != NULL && count < DATAIN_SAMPLE_COUNT) {
                if (strlen(token) > 0) {
                    dataIn[count++] = strtof(token, NULL);
                }
                token = strtok(NULL, " ,\n\r");
            }
            {
                char dbg[128];
                sprintf(dbg, "Parsed DATAIN: expected %d samples, got %d\n\r", DATAIN_SAMPLE_COUNT, count);
                debug_print(dbg);
            }
            if (count == DATAIN_SAMPLE_COUNT) {
                dataInReady = 1;
                debug_print("DATAIN file ready.\n\r");
            } else {
                debug_print("Error parsing DATAIN.\n\r");
            }
        }
        else if (strncmp(header.id, "WIN____", 7) == 0) {
            int count = 0;
            char *token = strtok(fileBuffer, " ,\n\r");
            while (token != NULL && count < WIN_SAMPLE_COUNT) {
                if (strlen(token) > 0) {
                    wIn[count++] = strtof(token, NULL);
                }
                token = strtok(NULL, " ,\n\r");
            }
            {
                char dbg[128];
                sprintf(dbg, "Parsed W_in: expected %d samples, got %d\n\r", WIN_SAMPLE_COUNT, count);
                debug_print(dbg);
            }
            if (count == WIN_SAMPLE_COUNT) {
                wInReady = 1;
                debug_print("W_in file ready.\n\r");
            } else {
                debug_print("Error parsing W_in.\n\r");
            }
        }
        else if (strncmp(header.id, "WX_____", 7) == 0) {
            int count = 0;
            char *token = strtok(fileBuffer, " ,\n\r");
            while (token != NULL && count < WX_SAMPLE_COUNT) {
                if (strlen(token) > 0) {
                    wX[count++] = strtof(token, NULL);
                }
                token = strtok(NULL, " ,\n\r");
            }
            {
                char dbg[128];
                sprintf(dbg, "Parsed W_x: expected %d samples, got %d\n\r", WX_SAMPLE_COUNT, count);
                debug_print(dbg);
            }
            if (count == WX_SAMPLE_COUNT) {
                wXReady = 1;
                debug_print("W_x file ready.\n\r");
            } else {
                debug_print("Error parsing W_x.\n\r");
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
            {
                char dbg[128];
                sprintf(dbg, "Parsed W_out: expected %d samples, got %d\n\r", WOUT_SAMPLE_COUNT, count);
                debug_print(dbg);
            }
            if (count == WOUT_SAMPLE_COUNT) {
                wOutReady = 1;
                debug_print("W_out file ready.\n\r");
            } else {
                debug_print("Error parsing W_out.\n\r");
            }
        }
        else {
            char unknown[64];
            sprintf(unknown, "Unknown file ID received: %.8s\n\r", header.id);
            debug_print(unknown);
        }

        // If all four files have been received, break out of the loop.
        if (dataInReady && wInReady && wXReady && wOutReady) {
            debug_print("All files received. Proceeding to echo state network computation...\n\r");
            break;
        }
    }

    // --- First part: Compute the reservoir state ---
    // Here, dataIn (40 elements), wIn (8x40), and wX (8x8) are used.
    update_state(wIn, dataIn, wX, state_pre, state);

    // For this first sample, state_pre remains zeros (already set above).
    // Debug print the reservoir state:
    debug_print("Reservoir state (state):\n\r");
    for (int i = 0; i < NUM_NEURONS; i++) {
        char buf[64];
        sprintf(buf, "state[%d] = %f\n\r", i, state[i]);
        debug_print(buf);
        usleep(50000);
    }

    // Form the extended state vector: now defined as { state; dataIn }
    form_state_extended(dataIn, state, state_extended);

    debug_print("Extended state vector (state concatenated with dataIn):\n\r");
    for (int i = 0; i < NUM_INPUTS + NUM_NEURONS; i++) {
        char buf[64];
        sprintf(buf, "state_extended[%d] = %f\n\r", i, state_extended[i]);
        debug_print(buf);
        usleep(50000);
    }

    // --- Second part: Compute the ESN output ---
    // Compute data_out = W_out * state_extended, where W_out is 4x48 and state_extended is 48x1.
//    float data_out[4];
    compute_output(wOut, state_extended, data_out);

    debug_print("ESN output (data_out):\n\r");
    for (int i = 0; i < 4; i++) {
        char buf[64];
        sprintf(buf, "data_out[%d] = %f\n\r", i, data_out[i]);
        debug_print(buf);
        usleep(50000);
    }

    // Continue with further processing as needed...
    cleanup_platform();
    return 0;
}
