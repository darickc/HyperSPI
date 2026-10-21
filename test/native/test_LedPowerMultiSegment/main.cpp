/* test_LedPowerMultiSegment/main.cpp
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

// LED power relay with two segments: one shared relay for the whole LED power supply

#define NEOPIXEL_RGB
#define DATA_PIN 2
#define SECOND_SEGMENT_START_INDEX 100
#define SECOND_SEGMENT_DATA_PIN 4
#define LED_POWER_PIN 16
#define LED_POWER_ON_DELAY 200

#include "../support/protocol_mock.h"

#define TEST_LEDS 250

void setUp()
{
	resetProtocol(1000);
	powerControl.init();
}

void tearDown()
{
}

void MultiSegmentTest_SecondSegmentColorEnablesSharedRelay()
{
	auto colors = solidColors(TEST_LEDS, 0, 0, 0);
	colors[SECOND_SEGMENT_START_INDEX + 20].G = 77;
	feedSpi(createFrame(colors));

	TEST_ASSERT_NOT_NULL(base.getLedStrip2());
	TEST_ASSERT_TRUE_MESSAGE(powerControl.isRelayActive(), "Color in the second segment must enable the shared relay");
	TEST_ASSERT_EQUAL_INT(HIGH, mock::lastWrittenLevel(LED_POWER_PIN));
	TEST_ASSERT_EQUAL_INT(0, base.getLedStrip1()->showCount);
	TEST_ASSERT_EQUAL_INT(0, base.getLedStrip2()->showCount);

	idleLoop(LED_POWER_ON_DELAY);
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, base.getLedStrip1()->showCount, "Segment 1 must be rendered after stabilization");
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, base.getLedStrip2()->showCount, "Segment 2 must be rendered after stabilization");
	TEST_ASSERT_EQUAL_UINT8(77, base.getLedStrip2()->shown[20].G);
}

void MultiSegmentTest_BlackInBothSegmentsTurnsRelayOff()
{
	auto colors = solidColors(TEST_LEDS, 0, 0, 0);
	colors[5].R = 1;
	feedSpi(createFrame(colors));
	idleLoop(LED_POWER_ON_DELAY);
	TEST_ASSERT_TRUE(powerControl.isRelayActive());

	// only the first segment goes black: still a non-black frame
	colors = solidColors(TEST_LEDS, 0, 0, 0);
	colors[TEST_LEDS - 1].R = 1;
	for (int i = 0; i < 400; i++)
	{
		mock::now += 20;
		feedSpi(createFrame(colors));
	}
	TEST_ASSERT_TRUE(powerControl.isRelayActive());

	// both segments black
	colors = solidColors(TEST_LEDS, 0, 0, 0);
	for (int i = 0; i < LED_POWER_OFF_DELAY / 20; i++)
	{
		mock::now += 20;
		feedSpi(createFrame(colors));
	}
	idleLoop(20);
	TEST_ASSERT_FALSE(powerControl.isRelayActive());
}

int main(int argc, char **argv)
{
	UNITY_BEGIN();
	RUN_TEST(MultiSegmentTest_SecondSegmentColorEnablesSharedRelay);
	RUN_TEST(MultiSegmentTest_BlackInBothSegmentsTurnsRelayOff);
	return UNITY_END();
}
