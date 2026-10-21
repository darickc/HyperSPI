/* powercontrol.h
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

#ifndef POWERCONTROL_H
#define POWERCONTROL_H

#if defined(LED_POWER_PIN)

#include <stdint.h>

// continuous black frames time before the LED power is switched off
#ifndef LED_POWER_OFF_DELAY
	#define LED_POWER_OFF_DELAY 5000
#endif

// time for the LED power supply to stabilize before the first frame is rendered
#ifndef LED_POWER_ON_DELAY
	#define LED_POWER_ON_DELAY 200
#endif

// no valid frames time before the LED power is switched off (0 = disabled)
#ifndef LED_POWER_IDLE_TIMEOUT
	#define LED_POWER_IDLE_TIMEOUT 5000
#endif

#if defined(LED_POWER_DEBUG)
	#define LED_POWER_LOG(msg) SerialPort.println("[LED POWER] " msg)
#else
	#define LED_POWER_LOG(msg)
#endif

/**
 * @brief LED power relay state
 *
 */
enum class PowerState
{
	POWER_OFF,
	POWER_ON_WAIT,
	POWER_ON,
	POWER_OFF_PENDING
};

/**
 * @brief Controls the relay that switches the shared LED strip power supply
 *
 */
class PowerControl
{
	PowerState state = PowerState::POWER_OFF;
	bool relayActive = false;
	bool lastFrameBlack = true;
	uint32_t poweredAt = 0;
	uint32_t lastValidFrame = 0;
	uint32_t blackSince = 0;

	/**
	 * @brief Rollover-safe check if the period has elapsed
	 *
	 */
	static inline bool elapsed(uint32_t now, uint32_t since, uint32_t period)
	{
		return (uint32_t)(now - since) >= period;
	}

	/**
	 * @brief Drive the relay pin, only if the state is changed
	 *
	 * @param active
	 */
	inline void setRelay(bool active)
	{
		if (relayActive == active)
			return;

		relayActive = active;
		writeRelayPin();
	}

	inline void writeRelayPin()
	{
		#if defined(LED_POWER_INVERT)
			digitalWrite(LED_POWER_PIN, relayActive ? LOW : HIGH);
		#else
			digitalWrite(LED_POWER_PIN, relayActive ? HIGH : LOW);
		#endif
	}

	inline void powerOff()
	{
		setRelay(false);
		state = PowerState::POWER_OFF;
		LED_POWER_LOG("Relay OFF");
	}

	public:
		/**
		 * @brief Apply the inactive level before enabling the output to avoid the glitch
		 *
		 */
		void init()
		{
			state = PowerState::POWER_OFF;
			relayActive = false;
			lastFrameBlack = true;
			writeRelayPin();
			pinMode(LED_POWER_PIN, OUTPUT);
			writeRelayPin();
		}

		/**
		 * @brief Must be called only for a frame that passed all integrity checks
		 *
		 * @param now
		 * @param isBlack
		 */
		void onValidFrame(uint32_t now, bool isBlack)
		{
			lastValidFrame = now;

			if (isBlack)
			{
				if (!lastFrameBlack)
					blackSince = now;
				lastFrameBlack = true;

				if (state == PowerState::POWER_ON)
					state = PowerState::POWER_OFF_PENDING;
				return;
			}

			lastFrameBlack = false;

			if (state == PowerState::POWER_OFF)
			{
				LED_POWER_LOG("Valid LED data received");
				setRelay(true);
				poweredAt = now;
				LED_POWER_LOG("Relay ON");

				if (LED_POWER_ON_DELAY == 0)
				{
					state = PowerState::POWER_ON;
					LED_POWER_LOG("Power stabilized");
				}
				else
					state = PowerState::POWER_ON_WAIT;
			}
			else if (state == PowerState::POWER_OFF_PENDING)
				state = PowerState::POWER_ON;
		}

		/**
		 * @brief Non-blocking timers, call it periodically
		 *
		 * @param now
		 */
		void update(uint32_t now)
		{
			switch (state)
			{
			case PowerState::POWER_OFF:
				break;

			case PowerState::POWER_ON_WAIT:
				if (elapsed(now, poweredAt, LED_POWER_ON_DELAY))
				{
					state = (lastFrameBlack) ? PowerState::POWER_OFF_PENDING : PowerState::POWER_ON;
					LED_POWER_LOG("Power stabilized");
				}
				break;

			case PowerState::POWER_OFF_PENDING:
				if (elapsed(now, blackSince, LED_POWER_OFF_DELAY))
				{
					LED_POWER_LOG("Black-frame timeout reached");
					powerOff();
					break;
				}
				// fall through

			case PowerState::POWER_ON:
				if (LED_POWER_IDLE_TIMEOUT > 0 && elapsed(now, lastValidFrame, LED_POWER_IDLE_TIMEOUT))
				{
					LED_POWER_LOG("SPI inactivity timeout reached");
					powerOff();
				}
				break;
			}
		}

		/**
		 * @brief Check if the LED power is stable and the frame can be rendered
		 *
		 */
		inline bool isOutputReady() const
		{
			return state == PowerState::POWER_ON || state == PowerState::POWER_OFF_PENDING;
		}

		inline bool isRelayActive() const
		{
			return relayActive;
		}

		inline PowerState getState() const
		{
			return state;
		}
} powerControl;

#endif

#endif
