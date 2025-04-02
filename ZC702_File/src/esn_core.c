/*******************************************************************************
 * File: esn_core.c
 * Author: Christopher Boerner
 * Date: 04-01-2025
 *
 * Description:
 *	   Echo State Network (ESN) core equations to perform on the arrays passed
 *	   from tcp_file.c. Currently for one sample.
 *
 *   Expected Files:
 *     - DATAIN file: 40 float values.
 *     - WIN file: 320 float values.
 *     - WX file:  64 float values.
 *     - WOUT file: 192 float values.
 *
 *   Generate:
 *     - state_pre
 *     - res_state
 *     - state_ext
 *     - data_out
 *
 ******************************************************************************/

#include "esn_core.h"
#include <string.h>

/*
 * Update reservoir state based on:
 *   state(i) = tanh( W_in(i,:)*dataIn + W_x(i,:)*state_pre )
 */
void update_state(const float *W_in,
                  const float *dataIn,
                  const float *W_x,
                  const float *state_pre,
                  float *state)
{
    float temp1[NUM_NEURONS] = {0};
    float temp2[NUM_NEURONS] = {0};

    /* temp1[i] = sum_j( W_in[i * NUM_INPUTS + j] * dataIn[j] ) */
    for (int i = 0; i < NUM_NEURONS; i++) {
        for (int j = 0; j < NUM_INPUTS; j++) {
            temp1[i] += W_in[i * NUM_INPUTS + j] * dataIn[j];
        }
    }

    /* temp2[i] = sum_j( W_x[i * NUM_NEURONS + j] * state_pre[j] ) */
    for (int i = 0; i < NUM_NEURONS; i++) {
        for (int j = 0; j < NUM_NEURONS; j++) {
            temp2[i] += W_x[i * NUM_NEURONS + j] * state_pre[j];
        }
    }

    /* state[i] = tanh(temp1[i] + temp2[i]) */
    for (int i = 0; i < NUM_NEURONS; i++) {
        float sum_val = temp1[i] + temp2[i];
        /* Use tanhf for single-precision float math.
           If you're using double math, use tanh(). */
        state[i] = tanh(sum_val);
    }
}

/*
 * Create the "extended" state vector, which appends
 * the raw inputs to the reservoir state for final output layer.
 *
 * state_extended = [reservoir_state; input_data]
 *
 * so it ends up length (NUM_NEURONS + NUM_INPUTS).
 */
void form_state_extended(const float *dataIn,
                         const float *state,
                         float *state_extended)
{
    /* Copy reservoir state first */
    for (int i = 0; i < NUM_NEURONS; i++) {
        state_extended[i] = state[i];
    }
    /* Then copy input data after that */
    for (int i = 0; i < NUM_INPUTS; i++) {
        state_extended[NUM_NEURONS + i] = dataIn[i];
    }
}

/*
 * Compute ESN output, e.g.:
 *   data_out[k] = sum_j( W_out[k*TOTAL + j] * state_extended[j] )
 *
 * Where TOTAL = (NUM_INPUTS + NUM_NEURONS),
 * and k goes over however many output dimensions you have (i.e., 4).
 */
void compute_output(const float *W_out,
                    const float *state_extended,
                    float *data_out)
{
    /* Suppose you want 4 outputs.
       If you have more or fewer, you can parametrize it or adjust code. */
    const int output_dim = 4;
    int total = NUM_INPUTS + NUM_NEURONS;

    for (int i = 0; i < output_dim; i++) {
        data_out[i] = 0.0f;
        for (int j = 0; j < total; j++) {
            data_out[i] += W_out[i * total + j] * state_extended[j];
        }
    }
}
