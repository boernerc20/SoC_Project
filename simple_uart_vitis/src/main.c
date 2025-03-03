#include <stdio.h>
#include <string.h>     // For strlen() and strtok()
#include <stdlib.h>     // For atoi()
#include "xparameters.h"
#include "xuartps.h"
#include "platform.h"
#include "sleep.h"      // For sleep() and usleep()

// Using UART0 for data transfer
static XUartPs UartPs0;

// Define buffer sizes
#define RX_BUFFER_SIZE 100
#define TX_BUFFER_SIZE 100

int main()
{
    // Initialize the platform (this enables caches, etc.)
    init_platform();

    // === UART0 Initialization (Data Transfer Port) ===
    // (In Vitis, UART0 is mapped to XPAR_XUARTPS_1_DEVICE_ID.)
    XUartPs_Config *Config0 = XUartPs_LookupConfig(XPAR_XUARTPS_1_DEVICE_ID);
    if (!Config0) {
        xil_printf("UART0 Config Error.\n\r");
        return XST_FAILURE;
    }
    // Initialize the UART peripheral with the config parameters.
    XUartPs_CfgInitialize(&UartPs0, Config0, Config0->BaseAddress);
    // Set the baud rate to 115200.
    XUartPs_SetBaudRate(&UartPs0, 115200);

    // Debug message via xil_printf (which sends via the USB-UART bridge)
    xil_printf("UART Data Transfer Initialized. Waiting for CSV data...\n\r");

    // === Buffers for Data Processing ===
    // rx_buffer will store the incoming CSV string.
    // tx_buffer will store the formatted result to be sent back.
    u8 rx_buffer[RX_BUFFER_SIZE];
    u8 tx_buffer[TX_BUFFER_SIZE];
    // An array to store up to 10 numbers parsed from the CSV.
    int num_array[10];
    int num_count = 0;

    // === Main Loop ===
    // Continuously wait for data, process it, and send back the result.
    while (1) {
        // Attempt to receive up to RX_BUFFER_SIZE-1 bytes over UART0.
        int received = XUartPs_Recv(&UartPs0, rx_buffer, RX_BUFFER_SIZE - 1);

        if (received > 0) {
            // Null-terminate the received string.
            rx_buffer[received] = '\0';

//            // (Optional) Remove a trailing newline if present.
//            if (rx_buffer[received - 1] == '\n') {
//                rx_buffer[received - 1] = '\0';
//            }

            // Debug print of the received data (this goes out via xil_printf)
            xil_printf("Received: %s\n\r", rx_buffer);

            // === Parse CSV Data into an Integer Array ===
            // Tokenize the received string using comma and newline as delimiters.
            char *token = strtok((char *)rx_buffer, ",\n\r");
            num_count = 0;
            while (token != NULL && num_count < 10) {
                // Convert each token to an integer.
                num_array[num_count++] = atoi(token);
                token = strtok(NULL, ",\n\r");
            }

            // === Simple Algorithm: Add 10 to Each Number ===
            for (int i = 0; i < num_count; i++) {
                num_array[i] += 10;
            }

            // === Format the Result as a CSV String ===
            int tx_len = 0;
            for (int i = 0; i < num_count; i++) {
                // Append each modified number followed by a comma.
                tx_len += sprintf((char *)tx_buffer + tx_len, "%d,", num_array[i]);
            }
            // Replace the last comma with a newline and null-terminate the string.
            if (tx_len > 0) {
                tx_buffer[tx_len - 1] = '\n';
                tx_buffer[tx_len] = '\0';
            } else {
                // If no data was parsed, send just a newline.
                strcpy((char *)tx_buffer, "\n");
            }

            // === Send the Modified CSV String Back via UART0 ===
            XUartPs_Send(&UartPs0, tx_buffer, strlen((char *)tx_buffer));
            while (XUartPs_IsSending(&UartPs0)) {
                // Wait until the transmission is complete.
            }
            xil_printf("Sent: %s\n\r", tx_buffer);

            // Clear the receive buffer for the next iteration.
            memset(rx_buffer, 0, RX_BUFFER_SIZE);
        }
        // Add a small delay (1ms) to prevent overloading the UART buffer.
        usleep(1000);
    }

    cleanup_platform();
    return 0;
}


//#include <stdio.h>
//#include <string.h>     // For strlen() and strtok()
//#include <stdlib.h>     // For atoi()
//#include "xparameters.h"
//#include "xuartps.h"
//#include "platform.h"
//#include <unistd.h>     // Use standard sleep() and usleep()
//
//// UART0 Instance
//static XUartPs UartPs0;
//
//// Buffer sizes
//#define RX_BUFFER_SIZE 100
//#define TX_BUFFER_SIZE 100
//
//int main()
//{
//    // Initialize the platform (enables caches, etc.)
//    init_platform();
//
//    // Initialize UART0
//    XUartPs_Config *Config0 = XUartPs_LookupConfig(XPAR_XUARTPS_1_DEVICE_ID);
//    if (!Config0) {
//        xil_printf("UART Config Error.\n\r");
//        return XST_FAILURE;
//    }
//    XUartPs_CfgInitialize(&UartPs0, Config0, Config0->BaseAddress);
//    XUartPs_SetBaudRate(&UartPs0, 115200);
//
//    // Buffers for received and transmitted data
//    u8 rx_buffer[RX_BUFFER_SIZE];
//    u8 tx_buffer[TX_BUFFER_SIZE];
//    int num_array[10];
//    int num_count = 0;
//
//    xil_printf("Waiting for data...\n\r");
//
//    // Infinite loop: wait for data, process it, and echo back the results
//    while (1) {
//        // Step 1: Receive data from UART0
//        int received = XUartPs_Recv(&UartPs0, rx_buffer, RX_BUFFER_SIZE - 1);
//
//        if (received > 0) {
//            // Null-terminate the received string
//            rx_buffer[received] = '\0';
//
//            // (Optional) Remove any trailing newline or carriage return
//            if (rx_buffer[received - 1] == '\n' || rx_buffer[received - 1] == '\r') {
//                rx_buffer[received - 1] = '\0';
//            }
//
//            // Debug: Print the received string
//            xil_printf("Received: %s\n\r", rx_buffer);
//
//            // Step 2: Parse the CSV string into an integer array
//            char *token = strtok((char *)rx_buffer, ",\n\r");
//            num_count = 0;
//            while (token != NULL && num_count < 10) {
//                num_array[num_count++] = atoi(token);
//                token = strtok(NULL, ",\n\r");
//            }
//
//            // Step 3: Perform a simple algorithm (e.g., add 10 to each element)
//            for (int i = 0; i < num_count; i++) {
//                num_array[i] += 10;
//            }
//
//            // Step 4: Format the modified array back into a CSV string
//            int tx_len = 0;
//            for (int i = 0; i < num_count; i++) {
//                tx_len += sprintf((char *)tx_buffer + tx_len, "%d,", num_array[i]);
//            }
//            // Replace the last comma with a newline and null-terminate
//            tx_buffer[tx_len - 1] = '\n';
//            tx_buffer[tx_len] = '\0';
//
//            // Step 5: Send the modified CSV string back over UART0
//            XUartPs_Send(&UartPs0, tx_buffer, strlen((char *)tx_buffer));
//            while (XUartPs_IsSending(&UartPs0)) {
//                // Wait until the send operation is complete
//            }
//            xil_printf("Sent: %s\n\r", tx_buffer);
//
//            // Clear the receive buffer for the next transmission
//            memset(rx_buffer, 0, RX_BUFFER_SIZE);
//        }
//
//        // Small delay to avoid overwhelming the UART (1ms)
//        usleep(1000);
//    }
//
//    cleanup_platform();
//    return 0;
//}


// Simple printing program (xil_printf):
//
//#include <stdio.h>
//#include <string.h>
//#include "xparameters.h"
//#include "platform.h"
//#include "sleep.h"      // for sleep()
//
//int main()
//{
//    // Initialize the platform (enables caches, etc.)
//    init_platform();
//
//    // Simple xil_printf statement to test UART1
//    while (1) {
//        xil_printf("Hello from UART1!\n\r");
//        sleep(1); // Delay for 1 second
//    }
//
//    cleanup_platform();
//    return 0;
//}

//// UART0 Echo (Working):
//
//#include <stdio.h>
//#include "xparameters.h"
//#include "xuartps.h"
//#include "platform.h"
//
//// UART Instance
////static XUartPs UartPs0;
//static XUartPs UartPs1;
//
//int main()
//{
//    // Initialize the platform (enables caches, etc.)
//    init_platform();
//
//    // UART Initialization
//    XUartPs_Config *Config1 = XUartPs_LookupConfig(XPAR_XUARTPS_1_DEVICE_ID);
//    if (!Config1) {
//        xil_printf("UART Config Error.\n\r");
//        return XST_FAILURE;
//    }
//
//    XUartPs_CfgInitialize(&UartPs1, Config1, Config1->BaseAddress);
//
//    // Setting Baud Rate and enabling interrupts
//    XUartPs_SetBaudRate(&UartPs1, 115200);
//    XUartPs_SetOperMode(&UartPs1, XUARTPS_OPER_MODE_NORMAL);
//
//    // Enable UART RX and TX interrupts
//    XUartPs_SetInterruptMask(&UartPs1, XUARTPS_IXR_RXOVR | XUARTPS_IXR_RXFULL);
//
//    // Buffer for received data
//    u8 buffer[1];
//    xil_printf("UART Loopback Test. Type something...\n\r");
//
//    while (1) {
//        // Receive a single character
//        int received = XUartPs_Recv(&UartPs1, buffer, 1);
//
//        // If a character is received, echo it back
//        if (received > 0) {
//            xil_printf("Received: %c\n\r", buffer[0]);
//
//            // Echo back the received character
//            XUartPs_Send(&UartPs1, buffer, 1);
//            while (XUartPs_IsSending(&UartPs1)) {
//                // Spin until complete
//            }
//        }
//    }
//
//    // Cleanup and exit
//    cleanup_platform();
//    return 0;
//}

//// UART1 Echo (Not Working):
//
//#include <stdio.h>
//#include "xparameters.h"
//#include "xuartps.h"
//#include "platform.h"
//
//// UART Instance
//static XUartPs UartPs0;
//
//int main()
//{
//    // Initialize the platform (enables caches, etc.)
//    init_platform();
//
//    // UART Initialization
//    XUartPs_Config *Config0 = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
//    if (!Config0) {
//        xil_printf("UART Config Error.\n\r");
//        return XST_FAILURE;
//    }
//
//    XUartPs_CfgInitialize(&UartPs0, Config0, Config0->BaseAddress);
//
//    // Setting Baud Rate and enabling interrupts
//    XUartPs_SetBaudRate(&UartPs0, 115200);
//    XUartPs_SetOperMode(&UartPs0, XUARTPS_OPER_MODE_NORMAL);
//
//    // Enable UART RX and TX interrupts
//    XUartPs_SetInterruptMask(&UartPs0, XUARTPS_IXR_RXOVR | XUARTPS_IXR_RXFULL);
//
//    // Buffer for received data
//    u8 buffer[1];
//    xil_printf("UART Loopback Test. Type something...\n\r");
//
//    while (1) {
//        // Receive a single character
//        int received = XUartPs_Recv(&UartPs0, buffer, 1);
//
//        // If a character is received, echo it back
//        if (received > 0) {
//            xil_printf("%c", buffer[0]);
//
//            // Echo back the received character
////            XUartPs_Send(&UartPs0, buffer, 1);
//            while (XUartPs_IsSending(&UartPs0)) {
//                // Spin until complete
//            }
//        }
//    }
//
//    // Cleanup and exit
//    cleanup_platform();
//    return 0;
//}

// Both UART0 and UART1 working together:
//
//#include <stdio.h>
//#include <string.h>     // for strlen()
//#include "xparameters.h"
//#include "xuartps.h"
//#include "platform.h"
//#include "sleep.h"      // for sleep()
//
//// Two driver instances, one for each UART
//static XUartPs UartPs0;
//static XUartPs UartPs1;
//
//int main()
//{
//    // Initialize the platform (enables caches, etc.)
//    init_platform();
//
//    // 1) Look up and initialize UART1 (which is XPAR_XUARTPS_0_DEVICE_ID)
//    XUartPs_Config *Config0 = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
//    if (!Config0) {
//        return XST_FAILURE;
//    }
//    XUartPs_CfgInitialize(&UartPs0, Config0, Config0->BaseAddress);
//    XUartPs_SetBaudRate(&UartPs0, 115200);
//
//    // 1) Look up and initialize UART0 (which is XPAR_XUARTPS_1_DEVICE_ID)
//    XUartPs_Config *Config1 = XUartPs_LookupConfig(XPAR_XUARTPS_1_DEVICE_ID);
//    if (!Config1) {
//        return XST_FAILURE;
//    }
//    XUartPs_CfgInitialize(&UartPs1, Config1, Config1->BaseAddress);
//    XUartPs_SetBaudRate(&UartPs1, 115200);
//
//    // Main loop: transmit "Hello World" on both ports every second
//    while (1) {
//        const char *msg = "Hello from UART0!\n\r";
//        size_t len = strlen(msg);
//
//        // Send on UART0
//        XUartPs_Send(&UartPs1, (const u8 *)msg, len);
//
//        // Send on UART1
//        xil_printf("Hello from UART1!\n\r");
//
//        while (XUartPs_IsSending(&UartPs1)) {
//            // spin until complete
//        }
//
//        // Delay for 1 second
//        sleep(1);
//    }
//
//    cleanup_platform();
//    return 0;
//}


