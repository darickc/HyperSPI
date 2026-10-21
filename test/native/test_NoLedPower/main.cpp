/* test_NoLedPower/main.cpp
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

// Build without LED_POWER_PIN: the original behavior, frames are rendered immediately and no GPIO is touched

#define NEOPIXEL_RGB

#include "../support/protocol_mock.h"

#define TEST_LEDS 300

#if defined(LED_POWER_PIN)
	#error "Unexpected LED power configuration"
#endif

void setUp()
{
	resetProtocol(1000);
}

void tearDown()
{
}

void NoPowerTest_FramesRenderedImmediately()
{
	feedSpi(createFrame(solidColors(TEST_LEDS, 0, 0, 0)));
	TEST_ASSERT_EQUAL_INT_MESSAGE(1, totalShows(), "Black frame must be rendered immediately");

	feedSpi(createFrame(solidColors(TEST_LEDS, 1, 2, 3)));
	TEST_ASSERT_EQUAL_INT(2, totalShows());
	TEST_ASSERT_EQUAL_UINT8(3, base.getLedStrip1()->shown[TEST_LEDS - 1].B);

	auto frame = createFrame(solidColors(TEST_LEDS, 9, 9, 9));
	frame[10] ^= 0xff;
	feedSpi(frame);
	TEST_ASSERT_EQUAL_INT_MESSAGE(2, totalShows(), "Damaged frame must not be rendered");

	for (int i = 0; i < 100; i++)
		idleLoop(1000);
	TEST_ASSERT_EQUAL_INT(2, totalShows());
}

void NoPowerTest_NoGpioActivity()
{
	for (int i = 0; i < 100; i++)
	{
		mock::now += 20;
		feedSpi(createFrame(solidColors(TEST_LEDS, i % 2, 0, 0)));
	}
	TEST_ASSERT_EQUAL_INT(0, (int)mock::pinCalls.size());
	TEST_ASSERT_EQUAL_INT(0, (int)mock::log.size());
}

int main(int argc, char **argv)
{
	UNITY_BEGIN();
	RUN_TEST(NoPowerTest_FramesRenderedImmediately);
	RUN_TEST(NoPowerTest_NoGpioActivity);
	return UNITY_END();
}
