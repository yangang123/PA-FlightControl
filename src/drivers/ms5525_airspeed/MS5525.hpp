/****************************************************************************
 *
 *   Copyright (c) 2017 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef DRIVERS_MS5525_AIRSPEED_HPP_
#define DRIVERS_MS5525_AIRSPEED_HPP_

#include <drivers/airspeed/airspeed.h>
#include <drivers/device/i2c.h>
#include <drivers/drv_airspeed.h>
#include <math.h>
#include <mathlib/math/filter/LowPassFilter2p.hpp>
#include <px4_config.h>
#include <sys/types.h>
#include <systemlib/airspeed.h>
#include <systemlib/perf_counter.h>
#include <uORB/topics/differential_pressure.h>
#include <uORB/uORB.h>

/* The MS5525DSO address is 111011Cx, where C is the complementary value of the pin CSB */
static constexpr uint8_t I2C_ADDRESS_1_MS5525DSO = 0x76;

static constexpr const char PATH_MS5525[] = "/dev/ms5525";

/* Measurement rate is 100Hz */
static constexpr unsigned MEAS_RATE = 100;
static constexpr float MEAS_DRIVER_FILTER_FREQ = 1.2f;
static constexpr uint64_t CONVERSION_INTERVAL = (1000000 / MEAS_RATE); /* microseconds */

class MS5525: public Airspeed
{
public:
	MS5525(uint8_t bus, uint8_t address = I2C_ADDRESS_1_MS5525DSO, const char *path = PATH_MS5525) :
				Airspeed(bus, address, CONVERSION_INTERVAL, path)
	{
	}
	
	~MS5525() override = default;
	
private:
	
	/**
	 * Perform a poll cycle; collect from the previous measurement
	 * and start a new one.
	 */
	void cycle() override;

	int measure() override;
	int collect() override;

	// temperature is read once every 10 cycles
	math::LowPassFilter2p _filter { MEAS_RATE * 0.9, MEAS_DRIVER_FILTER_FREQ };

	static constexpr uint8_t CMD_RESET = 0x1E; // ADC reset command
	static constexpr uint8_t CMD_ADC_READ = 0x00; // ADC read command
	
	static constexpr uint8_t CMD_PROM_START = 0xA0; // Prom read command (first)
	
	// D1 - pressure convert commands
	// Convert D1 (OSR=256)  0x40
	// Convert D1 (OSR=512)  0x42
	// Convert D1 (OSR=1024) 0x44
	// Convert D1 (OSR=2048) 0x46
	// Convert D1 (OSR=4096) 0x48
	static constexpr uint8_t CMD_CONVERT_PRES = 0x44;

	// D2 - temperature convert commands
	// Convert D2 (OSR=256)  0x50
	// Convert D2 (OSR=512)  0x52
	// Convert D2 (OSR=1024) 0x54
	// Convert D2 (OSR=2048) 0x56
	// Convert D2 (OSR=4096) 0x58
	static constexpr uint8_t CMD_CONVERT_TEMP = 0x54;

	uint8_t _current_cmd { CMD_CONVERT_PRES };

	unsigned _pressure_count { 0 };

	// Qx Coefficients Matrix by Pressure Range
	//  5525DSO-pp001DS (Pmin = -1, Pmax = 1)
	static constexpr uint8_t Q1 = 15;
	static constexpr uint8_t Q2 = 17;
	static constexpr uint8_t Q3 = 7;
	static constexpr uint8_t Q4 = 5;
	static constexpr uint8_t Q5 = 7;
	static constexpr uint8_t Q6 = 21;

	// calibration coefficients from prom
	uint16_t C1 { 0 };
	uint16_t C2 { 0 };
	uint16_t C3 { 0 };
	uint16_t C4 { 0 };
	uint16_t C5 { 0 };
	uint16_t C6 { 0 };

	int64_t Tref { 0 };

	// last readings for D1 (uncompensated pressure) and D2 (uncompensated temperature)
	uint32_t D1 { 0 };
	uint32_t D2 { 0 };

	bool init_ms5525();
	bool _inited { false };

	uint8_t prom_crc4(uint16_t n_prom[]) const;
	
};

#endif /* DRIVERS_MS5525_AIRSPEED_HPP_ */
