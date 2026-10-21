/* protocol_mock.h
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

// Mocked LED strip + Awa frame builder + SPI chunk delivery for the native protocol tests.
// Include after the configuration defines (NEOPIXEL_RGB, LED_POWER_*, SECOND_SEGMENT_*).

#ifndef PROTOCOL_MOCK_H
#define PROTOCOL_MOCK_H

#include "arduino_mock.h"
#include <unity.h>

/**
 * @brief Mockup LED driver that records the shown frames
 *
 */
class MockStrip
{
	public:
		int pin = -1;
		int showCount = 0;
		std::vector<RgbColor> pixels;
		std::vector<RgbColor> shown;

		MockStrip(int count, int _pin) : MockStrip(count)
		{
			pin = _pin;
		}

		MockStrip(int count) : pixels(count)
		{
		}

		void Begin()
		{
		}

		void Begin(int, int, int, int)
		{
		}

		bool CanShow()
		{
			return true;
		}

		void Show(bool = true)
		{
			showCount++;
			shown = pixels;
		}

		void SetPixelColor(uint16_t index, RgbColor color)
		{
			TEST_ASSERT_LESS_THAN_MESSAGE(pixels.size(), index, "LED index out of scope");
			pixels[index] = color;
		}
};

#define LED_DRIVER MockStrip
#define LED_DRIVER2 MockStrip
#include "main.h"

// same chunk format as HyperHDR ProviderSpi::writeBytesEsp32 and readSpi() in src/main.cpp
static const uint32_t SPI_CHUNK = 1536;

/**
 * @brief Build Awa protocol frame (version 1, RGB)
 *
 */
std::vector<uint8_t> createFrame(const std::vector<RgbColor>& colors)
{
	std::vector<uint8_t> frame;
	uint16_t count = colors.size() - 1;

	frame.push_back('A');
	frame.push_back('w');
	frame.push_back('a');
	frame.push_back((count >> 8) & 0xff);
	frame.push_back(count & 0xff);
	frame.push_back(frame[3] ^ frame[4] ^ 0x55);

	uint16_t fletcher1 = 0, fletcher2 = 0, fletcherExt = 0;
	uint8_t position = 0;

	for (auto& c : colors)
	{
		for (uint8_t v : {c.R, c.G, c.B})
		{
			frame.push_back(v);
			fletcherExt = (fletcherExt + (v ^ (position++))) % 255;
			fletcher1 = (fletcher1 + v) % 255;
			fletcher2 = (fletcher2 + fletcher1) % 255;
		}
	}

	frame.push_back((uint8_t)fletcher1);
	frame.push_back((uint8_t)fletcher2);
	frame.push_back((uint8_t)((fletcherExt != 0x41) ? fletcherExt : 0xaa));
	return frame;
}

std::vector<RgbColor> solidColors(int count, uint8_t r, uint8_t g, uint8_t b)
{
	RgbColor c;
	c.R = r;
	c.G = g;
	c.B = b;
	return std::vector<RgbColor>(count, c);
}

/**
 * @brief Deliver the data in zero padded SPI chunks and process each one like the firmware loop does
 *
 */
void feedSpi(const std::vector<uint8_t>& data)
{
	for (size_t offset = 0; offset < data.size(); offset += SPI_CHUNK)
	{
		uint8_t chunk[SPI_CHUNK] = {0};
		memcpy(chunk, data.data() + offset, std::min((size_t)SPI_CHUNK, data.size() - offset));

		if (base.queueEnd + SPI_CHUNK < MAX_BUFFER)
		{
			memcpy(&(base.buffer[base.queueEnd]), chunk, SPI_CHUNK);
			base.queueEnd += SPI_CHUNK;
		}
		else
		{
			int left = MAX_BUFFER - base.queueEnd;
			memcpy(&(base.buffer[base.queueEnd]), chunk, left);
			memcpy(&(base.buffer[0]), chunk + left, SPI_CHUNK - left);
			base.queueEnd = SPI_CHUNK - left;
		}

		processData();
	}
}

/**
 * @brief One iteration of the firmware loop() without new data
 *
 */
void idleLoop(uint32_t advanceMs)
{
	mock::now += advanceMs;
	processData();
}

int totalShows()
{
	int count = (base.getLedStrip1() != nullptr) ? base.getLedStrip1()->showCount : 0;
	if (base.getLedStrip2() != nullptr)
		count += base.getLedStrip2()->showCount;
	return count;
}

void resetProtocol(uint32_t time = 0)
{
	mock::reset(time);
	base.queueCurrent = 0;
	base.queueEnd = 0;
	frameState.setState(AwaProtocol::HEADER_A);
	statistics.reset(time);
}

#endif
