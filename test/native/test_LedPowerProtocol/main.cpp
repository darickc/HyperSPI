/* test_LedPowerProtocol/main.cpp
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

// LED power relay driven by the real Awa protocol parser (processData) with SPI-like chunks

#define NEOPIXEL_RGB
#define LED_POWER_PIN 16
#define LED_POWER_INVERT
#define LED_POWER_OFF_DELAY 5000
#define LED_POWER_ON_DELAY 200
#define LED_POWER_IDLE_TIMEOUT 5000

#include "../support/protocol_mock.h"

#define TEST_LEDS 300

void setUp()
{
	resetProtocol(1000);
	powerControl.init();
	if (base.getLedStrip1() != nullptr)
	{
		base.getLedStrip1()->showCount = 0;
		base.dropLateFrame();
	}
}

void tearDown()
{
}

/**
 * @brief Power on the relay and wait for the stabilization period
 *
 */
void powerOnWithFrame(const std::vector<RgbColor>& colors)
{
	feedSpi(createFrame(colors));
	TEST_ASSERT_TRUE(powerControl.isRelayActive());
	idleLoop(LED_POWER_ON_DELAY);
	TEST_ASSERT_TRUE(powerControl.isOutputReady());
}

void ProtocolTest_InvalidFramesDoNotActivateRelay()
{
	int writes = mock::countWrites();

	// broken Fletcher checksums
	for (int i = 0; i < 50; i++)
	{
		auto frame = createFrame(solidColors(TEST_LEDS, 200, 128, 64));
		int index = 6 + random(frame.size() - 6);
		frame[index] ^= 0xff;
		feedSpi(frame);
		idleLoop(20);
	}

	// broken header CRC
	auto frame = createFrame(solidColors(TEST_LEDS, 200, 128, 64));
	frame[5] ^= 0x01;
	feedSpi(frame);

	// truncated frame
	frame = createFrame(solidColors(TEST_LEDS, 200, 128, 64));
	frame.resize(frame.size() - 2);
	feedSpi(frame);

	TEST_ASSERT_FALSE(powerControl.isRelayActive());
	TEST_ASSERT_EQUAL_INT(writes, mock::countWrites());
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, totalShows(), "Nothing can be rendered while LED power is off");
}

void ProtocolTest_BlackFrameWhileOffIsNotRendered()
{
	feedSpi(createFrame(solidColors(TEST_LEDS, 0, 0, 0)));
	for (int i = 0; i < 100; i++)
		idleLoop(100);

	TEST_ASSERT_FALSE(powerControl.isRelayActive());
	TEST_ASSERT_EQUAL_INT(0, totalShows());
}

void ProtocolTest_SinglePixelMakesFrameNonBlack()
{
	auto colors = solidColors(TEST_LEDS, 0, 0, 0);
	colors[TEST_LEDS - 1].B = 1;
	feedSpi(createFrame(colors));

	TEST_ASSERT_TRUE(powerControl.isRelayActive());
	TEST_ASSERT_EQUAL_INT(LOW, mock::lastWrittenLevel(LED_POWER_PIN));
}

void ProtocolTest_FrameDuringStabilizationIsRendered()
{
	auto colors = solidColors(TEST_LEDS, 10, 20, 30);
	feedSpi(createFrame(colors));

	TEST_ASSERT_TRUE(powerControl.isRelayActive());
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, totalShows(), "Frame must wait for the stabilized power");
	TEST_ASSERT_TRUE(base.hasLateFrameToRender());

	for (int i = 0; i < LED_POWER_ON_DELAY - 1; i++)
		idleLoop(1);
	TEST_ASSERT_EQUAL_INT(0, totalShows());

	// no new data from HyperHDR, the pending frame must be shown anyway
	idleLoop(1);
	TEST_ASSERT_EQUAL_INT(1, totalShows());
	TEST_ASSERT_EQUAL_UINT8(10, base.getLedStrip1()->shown[0].R);
	TEST_ASSERT_EQUAL_UINT8(20, base.getLedStrip1()->shown[TEST_LEDS / 2].G);
	TEST_ASSERT_EQUAL_UINT8(30, base.getLedStrip1()->shown[TEST_LEDS - 1].B);

	for (int i = 0; i < 10; i++)
		idleLoop(10);
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, totalShows(), "The frame must be rendered only once");
}

void ProtocolTest_NewestFrameDuringStabilizationWins()
{
	feedSpi(createFrame(solidColors(TEST_LEDS, 10, 10, 10)));
	idleLoop(50);
	feedSpi(createFrame(solidColors(TEST_LEDS, 0, 0, 0)));
	idleLoop(50);
	feedSpi(createFrame(solidColors(TEST_LEDS, 99, 98, 97)));
	idleLoop(LED_POWER_ON_DELAY);

	TEST_ASSERT_EQUAL_INT(1, totalShows());
	TEST_ASSERT_EQUAL_UINT8(99, base.getLedStrip1()->shown[0].R);
	TEST_ASSERT_EQUAL_UINT8(97, base.getLedStrip1()->shown[TEST_LEDS - 1].B);
}

void ProtocolTest_DarkSceneThenShutdownThenWakeUp()
{
	powerOnWithFrame(solidColors(TEST_LEDS, 200, 100, 50));
	int shows = totalShows();

	// dark scene, 50 FPS, shorter than the off delay: rendered and still powered
	for (int t = 0; t < LED_POWER_OFF_DELAY - 100; t += 20)
	{
		mock::now += 20;
		feedSpi(createFrame(solidColors(TEST_LEDS, 0, 0, 0)));
	}
	TEST_ASSERT_TRUE(powerControl.isRelayActive());
	TEST_ASSERT_GREATER_THAN(shows, totalShows());

	// bright again
	mock::now += 20;
	feedSpi(createFrame(solidColors(TEST_LEDS, 1, 2, 3)));
	TEST_ASSERT_EQUAL_INT((int)PowerState::POWER_ON, (int)powerControl.getState());

	// HyperHDR disables the LED device: 2 black frames and the connection is closed
	feedSpi(createFrame(solidColors(TEST_LEDS, 0, 0, 0)));
	feedSpi(createFrame(solidColors(TEST_LEDS, 0, 0, 0)));
	shows = totalShows();
	for (int i = 0; i < LED_POWER_OFF_DELAY / 100 - 1; i++)
		idleLoop(100);
	TEST_ASSERT_TRUE(powerControl.isRelayActive());
	idleLoop(100);
	TEST_ASSERT_FALSE(powerControl.isRelayActive());
	TEST_ASSERT_EQUAL_INT(HIGH, mock::lastWrittenLevel(LED_POWER_PIN));

	// HyperHDR is back
	mock::now += 60000;
	feedSpi(createFrame(solidColors(TEST_LEDS, 50, 60, 70)));
	TEST_ASSERT_TRUE(powerControl.isRelayActive());
	TEST_ASSERT_EQUAL_INT(shows, totalShows());
	idleLoop(LED_POWER_ON_DELAY);
	TEST_ASSERT_EQUAL_INT(shows + 1, totalShows());
	TEST_ASSERT_EQUAL_UINT8(70, base.getLedStrip1()->shown[0].B);
}

void ProtocolTest_InactivityTurnsRelayOff()
{
	// the inactivity time is counted from the last valid frame, including the stabilization period
	powerOnWithFrame(solidColors(TEST_LEDS, 200, 100, 50));
	for (int i = 0; i < (LED_POWER_IDLE_TIMEOUT - LED_POWER_ON_DELAY) / 100 - 1; i++)
		idleLoop(100);
	TEST_ASSERT_TRUE(powerControl.isRelayActive());
	idleLoop(100);
	TEST_ASSERT_FALSE(powerControl.isRelayActive());
}

int main(int argc, char **argv)
{
	UNITY_BEGIN();
	RUN_TEST(ProtocolTest_InvalidFramesDoNotActivateRelay);
	RUN_TEST(ProtocolTest_BlackFrameWhileOffIsNotRendered);
	RUN_TEST(ProtocolTest_SinglePixelMakesFrameNonBlack);
	RUN_TEST(ProtocolTest_FrameDuringStabilizationIsRendered);
	RUN_TEST(ProtocolTest_NewestFrameDuringStabilizationWins);
	RUN_TEST(ProtocolTest_DarkSceneThenShutdownThenWakeUp);
	RUN_TEST(ProtocolTest_InactivityTurnsRelayOff);
	return UNITY_END();
}
