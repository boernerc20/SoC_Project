#include "platform.h"
#include "xil_cache.h"

/* Initialize caches or any other low-level setup needed. */
void init_platform(void)
{
    Xil_ICacheEnable();   /* Enable instruction cache */
    Xil_DCacheEnable();   /* Enable data cache */
}

/* Disable caches or free resources before exiting. */
void cleanup_platform(void)
{
    Xil_DCacheDisable();  /* Disable data cache */
    Xil_ICacheDisable();  /* Disable instruction cache */
}
