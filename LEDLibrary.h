#pragma once

#include <stm32f1xx_hal.h>
#include <inttypes.h>

template <uint8_t _leds_max> 
class InfoLeds
{
	enum mode_t : uint8_t { MODE_OFF, MODE_ON, MODE_BLINK };
	
	typedef struct
	{
		GPIO_TypeDef *port = nullptr;
		uint16_t pin_digital;
		
		mode_t mode;
		GPIO_PinState state;
		uint16_t blink_on;
		uint16_t blink_off;
		uint32_t blink_time;
		uint32_t blink_delay;
	} channel_t;
	
	
	public:
		
		void AddLed(channel_t channel, uint8_t led)
		{
			if(led > _leds_max || led == 0) return;
			
			_HW_CFG(channel);
			_channels[led-1] = channel;
			SetOff(led);
			
			return;
		}
		
		void SetOn(uint8_t led)
		{
			if(led > _leds_max || led == 0) return;
			
			channel_t &channel = _channels[led-1];
			_HW_HIGH(channel);
			channel.mode = MODE_ON;
			
			return;
		}
		
		void SetOn(uint8_t led, uint16_t blink_on, uint16_t blink_off)
		{
			if(led > _leds_max || led == 0) return;
			
			channel_t &channel = _channels[led-1];
			SetOn(led);
			channel.blink_on = blink_on;
			channel.blink_off = blink_off;
			channel.mode = MODE_BLINK;
			
			return;
		}
		
		void SetOff(uint8_t led)
		{
			if(led > _leds_max || led == 0) return;
			
			channel_t &channel = _channels[led-1];
			_HW_LOW(channel);
			channel.mode = MODE_OFF;
			
			return;
		}
		
		void SetOff()
		{
			for(uint8_t i = 0; i < _leds_max; ++i)
			{
				channel_t &channel = _channels[i];

				if(channel.port == NULL) continue;

				SetOff(i+1);
			}
			
			return;
		}
		
		void Processing(uint32_t current_time)
		{
			if(current_time - _last_tick_time < 10) return;
			_last_tick_time = current_time;
			
			for(channel_t &channel : _channels)
			{
				if(channel.port == nullptr) continue;
				if(channel.mode == MODE_OFF) continue;
				
				if(channel.mode == MODE_BLINK && current_time - channel.blink_time > channel.blink_delay)
				{
					channel.blink_time = current_time;
					
					if(channel.state == GPIO_PIN_RESET)
					{
						channel.blink_delay = channel.blink_on;
						_HW_HIGH(channel);
					}
					else
					{
						channel.blink_delay = channel.blink_off;
						_HW_LOW(channel);
					}
				}
			}
			
			return;
		}
		
	private:
		
		void _HW_HIGH(channel_t &channel)
		{
			HAL_GPIO_WritePin(channel.port, channel.pin_digital, GPIO_PIN_SET);
			channel.state = GPIO_PIN_SET;
			
			return;
		}
		
		void _HW_LOW(channel_t &channel)
		{
			HAL_GPIO_WritePin(channel.port, channel.pin_digital, GPIO_PIN_RESET);
			channel.state = GPIO_PIN_RESET;
			
			return;
		}

		void _HW_CFG(channel_t &channel)
		{
			GPIO_InitStruct.Pin = channel.pin_digital;
			HAL_GPIO_Init(channel.port, &GPIO_InitStruct);
			channel.state = GPIO_PIN_RESET;
			
			return;
		}
		
		channel_t _channels[_leds_max];
		
		uint32_t _last_tick_time = 0;
		
		GPIO_InitTypeDef GPIO_InitStruct = {GPIO_PIN_RESET, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW};
};
