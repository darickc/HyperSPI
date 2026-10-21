/* arduino_mock.h
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

// Minimal Arduino/NeoPixelBus/FreeRTOS replacement for the host (native) unit tests

#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <vector>

typedef uint8_t byte;
using std::min;

#define LOW 0x0
#define HIGH 0x1
#define OUTPUT 0x03
#define DEC 10

typedef void* TaskHandle_t;
typedef void* xSemaphoreHandle;
#define uxTaskGetStackHighWaterMark(x) 0

struct
{
	uint32_t getFreeHeap() { return 0; }
} ESP;

struct RgbColor
{
	uint8_t R = 0;
	uint8_t G = 0;
	uint8_t B = 0;
};

struct RgbwColor
{
	uint8_t R = 0;
	uint8_t G = 0;
	uint8_t B = 0;
	uint8_t W = 0;
};

/**
 * @brief Controllable clock and recorded GPIO calls
 *
 */
namespace mock
{
	struct PinCall
	{
		bool isMode;
		uint8_t pin;
		uint8_t value;
	};

	uint32_t now = 0;
	std::vector<PinCall> pinCalls;
	std::vector<std::string> log;

	void reset(uint32_t time = 0)
	{
		now = time;
		pinCalls.clear();
		log.clear();
	}

	int countWrites()
	{
		int count = 0;
		for (auto& call : pinCalls)
			if (!call.isMode)
				count++;
		return count;
	}

	int lastWrittenLevel(uint8_t pin)
	{
		for (auto it = pinCalls.rbegin(); it != pinCalls.rend(); ++it)
			if (!it->isMode && it->pin == pin)
				return it->value;
		return -1;
	}
}

inline unsigned long millis()
{
	return mock::now;
}

inline void delay(unsigned long ms)
{
	mock::now += ms;
}

inline void yield()
{
}

inline long random(long max)
{
	return rand() % max;
}

inline void pinMode(uint8_t pin, uint8_t mode)
{
	mock::pinCalls.push_back({true, pin, mode});
}

inline void digitalWrite(uint8_t pin, uint8_t value)
{
	mock::pinCalls.push_back({false, pin, value});
}

/**
 * @brief Serial port mockup: debug output is captured, SPI-like data is injected directly into base.buffer
 *
 */
class
{
	public:
		int available()
		{
			return 0;
		}

		size_t read(uint8_t*, size_t)
		{
			return 0;
		}

		size_t print(const char* s)
		{
			mock::log.push_back(s);
			return strlen(s);
		}

		size_t println(const char* s)
		{
			return print(s);
		}

		size_t write(const char* s)
		{
			return print(s);
		}

		size_t print(unsigned char, int = DEC)
		{
			return 0;
		}

		size_t println(int)
		{
			return 0;
		}
} SerialPort;

#endif
