#include "rls_training.h"

/* Global variables for RLS training */
static float W_out[NUM_OUTPUTS * EXTENDED_STATE_SIZE]; // Dimensions: NUM_OUTPUTS x EXTENDED_STATE_SIZE
static float Psi[EXTENDED_STATE_SIZE * EXTENDED_STATE_SIZE]; // Inverse correlation matrix
static int trainingEnabled = 0;  // 1: enabled; 0: disabled

/**
 * init_rls
 * --------
 * Initializes the RLS training module.
 * W_out is set to zero and Psi is initialized as an identity matrix.
 */
void init_rls(void)
{
    // Optionally initialize W_out to zeros.
    memset(W_out, 0, sizeof(W_out));

    // Initialize Psi as an identity matrix.
    for (int i = 0; i < EXTENDED_STATE_SIZE; i++) {
        for (int j = 0; j < EXTENDED_STATE_SIZE; j++) {
            if (i == j)
                Psi[i * EXTENDED_STATE_SIZE + j] = 1.0f; // or a tuned initial value
            else
                Psi[i * EXTENDED_STATE_SIZE + j] = 0.0f;
        }
    }

    xil_printf("RLS training module initialized (OFF).\n\r");
}

/**
 * update_training_rls
 * -------------------
 * Performs one update step of the recursive least squares (RLS) algorithm.
 * It updates the output weight matrix W_out based on the error between the
 * computed output and the target output, using the extended state vector z.
 *
 * This corresponds to the following steps:
 * 1. Compute the predicted output: y_pred = W_out * z.
 * 2. Calculate the error: e = y_target - y_pred.
 * 3. Compute the gain vector k = (Psi * z) / (lambda + z^T * Psi * z).
 * 4. Update W_out: W_out = W_out + error * k^T.
 * 5. Update Psi: Psi = (Psi - k * (z^T * Psi)) / lambda.
 *
 * @param z         Extended state vector (size: EXTENDED_STATE_SIZE)
 * @param y_target  Desired target output vector (size: NUM_OUTPUTS)
 */
void update_training_rls(const float *z, const float *y_target)
{
    // If training is disabled, simply return.
    if (!trainingEnabled) {
        return;
    }

    float y_pred[NUM_OUTPUTS] = {0};
    // Step 1: Compute the predicted output y_pred = W_out * z.
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        for (int j = 0; j < EXTENDED_STATE_SIZE; j++) {
            y_pred[i] += W_out[i * EXTENDED_STATE_SIZE + j] * z[j];
        }
    }

    // Step 2: Compute the error vector: error = y_target - y_pred.
    float error[NUM_OUTPUTS];
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        error[i] = y_target[i] - y_pred[i];
    }

    // Step 3: Compute temp vector = Psi * z.
    float temp_vec[EXTENDED_STATE_SIZE] = {0};
    for (int i = 0; i < EXTENDED_STATE_SIZE; i++) {
        for (int j = 0; j < EXTENDED_STATE_SIZE; j++) {
            temp_vec[i] += Psi[i * EXTENDED_STATE_SIZE + j] * z[j];
        }
    }

    // Compute denominator: d = lambda + z^T * (Psi * z).
    float d = RLS_FORGETTING_FACTOR;
    for (int i = 0; i < EXTENDED_STATE_SIZE; i++) {
        d += z[i] * temp_vec[i];
    }

    // Step 4: Compute gain vector k = (Psi * z) / d.
    float k[EXTENDED_STATE_SIZE] = {0};
    for (int i = 0; i < EXTENDED_STATE_SIZE; i++) {
        k[i] = temp_vec[i] / d;
    }

    // Step 5: Update the output weight matrix W_out.
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        for (int j = 0; j < EXTENDED_STATE_SIZE; j++) {
            W_out[i * EXTENDED_STATE_SIZE + j] += error[i] * k[j];
        }
    }

    // Step 6: Compute z^T * Psi (store as a row vector).
    float zTPsi[EXTENDED_STATE_SIZE] = {0};
    for (int j = 0; j < EXTENDED_STATE_SIZE; j++) {
        for (int i = 0; i < EXTENDED_STATE_SIZE; i++) {
            zTPsi[j] += z[i] * Psi[i * EXTENDED_STATE_SIZE + j];
        }
    }

    // Step 7: Update Psi: Psi = (Psi - k * (z^T * Psi)) / lambda.
    for (int i = 0; i < EXTENDED_STATE_SIZE; i++) {
        for (int j = 0; j < EXTENDED_STATE_SIZE; j++) {
            Psi[i * EXTENDED_STATE_SIZE + j] = (Psi[i * EXTENDED_STATE_SIZE + j] -
                k[i] * zTPsi[j]) / RLS_FORGETTING_FACTOR;
        }
    }
}

void enable_training(void)
{
    trainingEnabled = 1;
    xil_printf("RLS training enabled.\n\r");
}

void disable_training(void)
{
    trainingEnabled = 0;
    xil_printf("RLS training disabled.\n\r");
}

float *get_W_out(void)
{
    return W_out;
}

void set_W_out(const float *new_W_out)
{
    // Overwrite the existing W_out matrix with new values.
    memcpy(W_out, new_W_out, sizeof(W_out));
    xil_printf("W_out successfully updated from external source.\n\r");
}
