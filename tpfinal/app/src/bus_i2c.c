/*
 * bus_i2c.c - Arbitro del bus I2C (una transaccion por vuelta)
 */

#include "bus_i2c.h"

static bool b_bus_taken;

volatile uint32_t g_bus_i2c_deferred;

void bus_i2c_new_cycle(void)
{
	b_bus_taken = false;
}

bool bus_i2c_request(void)
{
	if (b_bus_taken)
	{
		g_bus_i2c_deferred++;
		return false;
	}

	b_bus_taken = true;
	return true;
}

/********************** end of file ******************************************/
