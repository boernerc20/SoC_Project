#ifndef ESN_TRAINING_H
#define ESN_TRAINING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esn_core.h"   // Must define NUM_INPUTS, NUM_NEURONS, NUM_OUTPUTS, and EXTENDED_STATE_SIZE
#include "xil_printf.h"
#include <string.h>
#include <stdlib.h>

/* Define the forgetting factor for RLS training */
#define RLS_FORGETTING_FACTOR  0.999f

/**
 * init_rls
 * --------
 * Initializes the RLS training module.
 * This function sets up the initial output weight matrix and the inverse
 * correlation matrix (Psi) used in the training updates.
 */
void init_rls(void);

/**
 * update_training_rls
 * -------------------
 * Performs an RLS update for a single sample.
 *
 * @param z         The extended state vector for the current sample
 *                  (size: EXTENDED_STATE_SIZE).
 * @param y_target  The desired (target) output vector for the current sample
 *                  (size: NUM_OUTPUTS).
 */
void update_training_rls(const float *z, const float *y_target);

/**
 * enable_training / disable_training
 * -----------------------------------
 * Turn RLS training on or off.
 */
void enable_training(void);
void disable_training(void);

/**
 * get_W_out
 * ---------
 * Returns a pointer to the current output weight matrix W_out.
 * This matrix is used by the ESN core to compute network outputs.
 */
float *get_W_out(void);

/**
 * set_W_out
 * ---------
 * Updates the global W_out matrix with new values provided by new_W_out.
 *
 * @param new_W_out A pointer to the external array containing updated weights.
 *                  Its length should be NUM_OUTPUTS * EXTENDED_STATE_SIZE.
 */
void set_W_out(const float *new_W_out);

#ifdef __cplusplus
}
#endif

#endif /* ESN_TRAINING_H */
