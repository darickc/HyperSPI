/* power_control_tests.h
*
*  MIT License
*
*  Copyright (c) 2021-2026 awawa-dev
*
*  https://github.com/awawa-dev/HyperSPI
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.

*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
 */

// Shared LED power state machine tests, the including suite selects the configuration
// (LED_POWER_PIN, LED_POWER_INVERT, LED_POWER_IDLE_TIMEOUT...) before including this file.

#ifndef POWER_CONTROL_TESTS_H
#define POWER_CONTROL_TESTS_H

#include "arduino_mock.h"
#include <unity.h>
#include "powercontrol.h"

#if defined(LED_POWER_INVERT)
	#define RELAY_ACTIVE LOW
	#define RELAY_INACTIVE HIGH
#else
	#define RELAY_ACTIVE HIGH
	#define RELAY_INACTIVE LOW
#endif

#define ASSERT_RELAY(active) \
	do { \
		TEST_ASSERT_EQUAL_MESSAGE(active, powerControl.isRelayActive(), "Unexpected relay state"); \
		TEST_ASSERT_EQUAL_INT_MESSAGE((active) ? RELAY_ACTIVE : RELAY_INACTIVE, mock::lastWrittenLevel(LED_POWER_PIN), "Unexpected relay GPIO level"); \
	} while (0)

#define ASSERT_STATE(expected) TEST_ASSERT_EQUAL_INT_MESSAGE((int)(expected), (int)powerControl.getState(), "Unexpected power state")

void boot(uint32_t time)
{
	mock::reset(time);
	powerControl.init();
}

/**
 * @brief Relay on and power stabilized at the given time
 *
 */
void bootAndPowerOn(uint32_t time)
{
	boot(time);
	powerControl.onValidFrame(time, false);
	powerControl.update(time + LED_POWER_ON_DELAY);
	ASSERT_STATE(PowerState::POWER_ON);
}

void PowerTest_BootRelayOff()
{
	boot(1000);

	TEST_ASSERT_GREATER_OR_EQUAL_INT(2, (int)mock::pinCalls.size());
	TEST_ASSERT_FALSE_MESSAGE(mock::pinCalls[0].isMode, "Inactive level must be written before the pin becomes an output");
	TEST_ASSERT_EQUAL_INT(RELAY_INACTIVE, mock::pinCalls[0].value);
	TEST_ASSERT_TRUE(mock::pinCalls[1].isMode);
	TEST_ASSERT_EQUAL_INT(OUTPUT, mock::pinCalls[1].value);
	TEST_ASSERT_EQUAL_INT(LED_POWER_PIN, mock::pinCalls[1].pin);

	ASSERT_RELAY(false);
	ASSERT_STATE(PowerState::POWER_OFF);
	TEST_ASSERT_FALSE(powerControl.isOutputReady());

	// nothing happens without frames
	for (uint32_t t = 1000; t < 60000; t += 100)
		powerControl.update(t);
	ASSERT_RELAY(false);
}

void PowerTest_FirstNonBlackFrameEnablesRelay()
{
	boot(1000);
	powerControl.onValidFrame(1000, false);
	ASSERT_RELAY(true);
	ASSERT_STATE(PowerState::POWER_ON_WAIT);
	TEST_ASSERT_FALSE_MESSAGE(powerControl.isOutputReady(), "Output must wait for the power to stabilize");

	powerControl.update(1000 + LED_POWER_ON_DELAY - 1);
	TEST_ASSERT_FALSE(powerControl.isOutputReady());

	powerControl.update(1000 + LED_POWER_ON_DELAY);
	ASSERT_STATE(PowerState::POWER_ON);
	TEST_ASSERT_TRUE(powerControl.isOutputReady());
	ASSERT_RELAY(true);
}

void PowerTest_BlackFramesNeverEnableRelay()
{
	boot(0);
	int writes = mock::countWrites();

	for (uint32_t t = 0; t < 30000; t += 20)
	{
		powerControl.onValidFrame(t, true);
		powerControl.update(t);
	}

	ASSERT_STATE(PowerState::POWER_OFF);
	TEST_ASSERT_EQUAL_INT(writes, mock::countWrites());
}

void PowerTest_ShortBlackSceneKeepsRelayOn()
{
	bootAndPowerOn(0);
	const uint32_t start = 10000;

	for (uint32_t t = start; t < start + LED_POWER_OFF_DELAY - 1; t += 20)
	{
		powerControl.onValidFrame(t, true);
		powerControl.update(t);
	}
	powerControl.update(start + LED_POWER_OFF_DELAY - 1);

	ASSERT_RELAY(true);
	ASSERT_STATE(PowerState::POWER_OFF_PENDING);
	TEST_ASSERT_TRUE_MESSAGE(powerControl.isOutputReady(), "Black frames must still be rendered");

	// scene is bright again: the black timer is cancelled
	powerControl.onValidFrame(start + LED_POWER_OFF_DELAY - 1, false);
	ASSERT_STATE(PowerState::POWER_ON);

	for (uint32_t t = start + LED_POWER_OFF_DELAY; t < start + 3 * LED_POWER_OFF_DELAY; t += 20)
	{
		powerControl.onValidFrame(t, (t / 1000) % 2);
		powerControl.update(t);
	}
	ASSERT_RELAY(true);
}

void PowerTest_ProlongedBlackTurnsRelayOff()
{
	bootAndPowerOn(0);
	const uint32_t start = 10000;

	for (uint32_t t = start; t < start + LED_POWER_OFF_DELAY; t += 20)
	{
		powerControl.onValidFrame(t, true);
		powerControl.update(t);
	}
	ASSERT_RELAY(true);

	powerControl.onValidFrame(start + LED_POWER_OFF_DELAY, true);
	powerControl.update(start + LED_POWER_OFF_DELAY);
	ASSERT_RELAY(false);
	ASSERT_STATE(PowerState::POWER_OFF);
	TEST_ASSERT_FALSE(powerControl.isOutputReady());
}

void PowerTest_BlackFramesThenSilenceTurnsRelayOff()
{
	// HyperHDR sends 2 black frames when the LED device is disabled and closes the SPI connection
	bootAndPowerOn(0);
	powerControl.onValidFrame(1000, true);
	powerControl.onValidFrame(1001, true);

	powerControl.update(1000 + LED_POWER_OFF_DELAY - 1);
	ASSERT_RELAY(true);
	powerControl.update(1000 + LED_POWER_OFF_DELAY);
	ASSERT_RELAY(false);
}

void PowerTest_NonBlackFrameReactivatesRelay()
{
	bootAndPowerOn(0);
	powerControl.onValidFrame(1000, true);
	powerControl.update(1000 + LED_POWER_OFF_DELAY);
	ASSERT_RELAY(false);

	powerControl.onValidFrame(20000, true);
	ASSERT_RELAY(false);

	powerControl.onValidFrame(20010, false);
	ASSERT_RELAY(true);
	ASSERT_STATE(PowerState::POWER_ON_WAIT);
	powerControl.update(20010 + LED_POWER_ON_DELAY);
	ASSERT_STATE(PowerState::POWER_ON);
}

void PowerTest_BlackFrameDuringStabilization()
{
	boot(0);
	powerControl.onValidFrame(100, false);
	powerControl.onValidFrame(150, true);
	ASSERT_STATE(PowerState::POWER_ON_WAIT);

	powerControl.update(100 + LED_POWER_ON_DELAY);
	ASSERT_STATE(PowerState::POWER_OFF_PENDING);
	TEST_ASSERT_TRUE(powerControl.isOutputReady());

	powerControl.update(150 + LED_POWER_OFF_DELAY - 1);
	ASSERT_RELAY(true);
	powerControl.update(150 + LED_POWER_OFF_DELAY);
	ASSERT_RELAY(false);
}

void PowerTest_InactivityTimeout()
{
	bootAndPowerOn(0);
	powerControl.onValidFrame(1000, false);

	#if LED_POWER_IDLE_TIMEOUT > 0
		powerControl.update(1000 + LED_POWER_IDLE_TIMEOUT - 1);
		ASSERT_RELAY(true);
		powerControl.update(1000 + LED_POWER_IDLE_TIMEOUT);
		ASSERT_RELAY(false);
		ASSERT_STATE(PowerState::POWER_OFF);
	#else
		for (uint32_t t = 1000; t < 3600000; t += 1000)
			powerControl.update(t);
		ASSERT_RELAY(true);
		ASSERT_STATE(PowerState::POWER_ON);
	#endif
}

void PowerTest_StaticColorKeepAlive()
{
	// HyperHDR smoothing resends a static image about once per second
	bootAndPowerOn(0);
	int writes = mock::countWrites();

	for (uint32_t t = 1000; t < 600000; t += 1000)
	{
		powerControl.onValidFrame(t, false);
		for (uint32_t i = 0; i < 1000; i += 50)
			powerControl.update(t + i);
	}

	ASSERT_RELAY(true);
	TEST_ASSERT_EQUAL_INT_MESSAGE(writes, mock::countWrites(), "Relay GPIO must not be toggled");
}

void PowerTest_NoRedundantGpioWrites()
{
	boot(0);
	int writes = mock::countWrites();

	for (int i = 0; i < 1000; i++)
	{
		powerControl.onValidFrame(i * 20, false);
		powerControl.update(i * 20);
	}
	TEST_ASSERT_EQUAL_INT_MESSAGE(writes + 1, mock::countWrites(), "Only one write expected to switch on");

	powerControl.onValidFrame(30000, true);
	for (uint32_t t = 30000; t <= 30000 + 3 * LED_POWER_OFF_DELAY; t += 20)
		powerControl.update(t);
	TEST_ASSERT_EQUAL_INT_MESSAGE(writes + 2, mock::countWrites(), "Only one write expected to switch off");
	ASSERT_RELAY(false);
}

void PowerTest_MillisRollover()
{
	// stabilization across the rollover
	const uint32_t start = 0xFFFFFFFFu - 50;
	boot(start);
	powerControl.onValidFrame(start, false);
	powerControl.update(start + 10);
	ASSERT_STATE(PowerState::POWER_ON_WAIT);
	powerControl.update((uint32_t)(start + LED_POWER_ON_DELAY - 1));
	ASSERT_STATE(PowerState::POWER_ON_WAIT);
	powerControl.update((uint32_t)(start + LED_POWER_ON_DELAY));
	ASSERT_STATE(PowerState::POWER_ON);

	// black timeout across the rollover
	const uint32_t blackStart = 0xFFFFFFFFu - 1000;
	bootAndPowerOn(blackStart - 2000);
	for (uint32_t t = blackStart - 2000; t != blackStart; t += 10)
		powerControl.onValidFrame(t, false);
	powerControl.onValidFrame(blackStart, true);
	powerControl.update(blackStart + 500);
	powerControl.update((uint32_t)(blackStart + 2000));
	powerControl.update((uint32_t)(blackStart + LED_POWER_OFF_DELAY - 1));
	ASSERT_RELAY(true);
	powerControl.update((uint32_t)(blackStart + LED_POWER_OFF_DELAY));
	ASSERT_RELAY(false);

	#if LED_POWER_IDLE_TIMEOUT > 0
		// inactivity timeout across the rollover
		bootAndPowerOn(blackStart - 2000);
		powerControl.onValidFrame(blackStart, false);
		powerControl.update((uint32_t)(blackStart + LED_POWER_IDLE_TIMEOUT - 1));
		ASSERT_RELAY(true);
		powerControl.update((uint32_t)(blackStart + LED_POWER_IDLE_TIMEOUT));
		ASSERT_RELAY(false);
	#endif
}

void PowerTest_DebugOutputOnlyOnStateChange()
{
	#if defined(LED_POWER_DEBUG)
		bootAndPowerOn(0);
		size_t lines = mock::log.size();
		TEST_ASSERT_GREATER_THAN(0, (int)lines);

		for (uint32_t t = 1000; t < 3000; t += 20)
		{
			powerControl.onValidFrame(t, false);
			powerControl.update(t);
		}
		TEST_ASSERT_EQUAL_INT_MESSAGE(lines, mock::log.size(), "No debug output expected for regular frames");

		powerControl.onValidFrame(3000, true);
		powerControl.update(3000 + LED_POWER_OFF_DELAY);
		TEST_ASSERT_EQUAL_STRING("[LED POWER] Relay OFF", mock::log.back().c_str());
	#else
		TEST_IGNORE_MESSAGE("LED_POWER_DEBUG is not enabled");
	#endif
}

void runPowerControlTests()
{
	RUN_TEST(PowerTest_BootRelayOff);
	RUN_TEST(PowerTest_FirstNonBlackFrameEnablesRelay);
	RUN_TEST(PowerTest_BlackFramesNeverEnableRelay);
	RUN_TEST(PowerTest_ShortBlackSceneKeepsRelayOn);
	RUN_TEST(PowerTest_ProlongedBlackTurnsRelayOff);
	RUN_TEST(PowerTest_BlackFramesThenSilenceTurnsRelayOff);
	RUN_TEST(PowerTest_NonBlackFrameReactivatesRelay);
	RUN_TEST(PowerTest_BlackFrameDuringStabilization);
	RUN_TEST(PowerTest_InactivityTimeout);
	RUN_TEST(PowerTest_StaticColorKeepAlive);
	RUN_TEST(PowerTest_NoRedundantGpioWrites);
	RUN_TEST(PowerTest_MillisRollover);
	RUN_TEST(PowerTest_DebugOutputOnlyOnStateChange);
}

#endif
