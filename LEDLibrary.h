#pragma once
#include <inttypes.h>

template <uint8_t _leds_max, uint16_t _tick_time = 10> 
class InfoLeds
{
	enum mode_t : uint8_t { MODE_OFF, MODE_ON, MODE_BLINK, MODE_OFF_DELAY };
	
	public:
		
		typedef struct
		{
			GPIO_TypeDef *Port;
			uint16_t Pin;
		} pin_t;
		
		InfoLeds()
		{
			memset(_channels, 0x00, sizeof(_channels));
			
			return;
		}
		
		void AddLed(pin_t pin, uint8_t led)
		{
			if(led > _leds_max || led == 0) return;
			
			_HW_PinInit(pin, GPIO_MODE_OUTPUT_PP);
			_channels[led-1].pin = pin;
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

		void SetOn(uint8_t led, uint32_t delay)
		{
			SetOn(led);
			SetOff(led, delay);
			
			return;
		}
		
		void SetOn(uint8_t led, uint16_t blink_on, uint16_t blink_off)
		{
			if(led > _leds_max || led == 0) return;
			
			channel_t &channel = _channels[led-1];
			SetOn(led);
			channel.mode = MODE_BLINK;
			channel.blink_on = blink_on;
			channel.blink_off = blink_off;

			// Исправляем 'промигивание' при включении. Костыли, но как иначе? :'(
			channel.blink_delay = blink_on;
			channel.init_time = HAL_GetTick();

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

		void SetOff(uint8_t led, uint32_t delay)
		{
			if(led > _leds_max || led == 0) return;

			channel_t &channel = _channels[led-1];
			channel.mode = MODE_OFF_DELAY;
			channel.blink_delay = delay;
			channel.init_time = HAL_GetTick();
			
			return;
		}
		
		void SetOff()
		{
			for(uint8_t i = 0; i < _leds_max; ++i)
			{
				channel_t &channel = _channels[i];

				if(channel.pin.Port == NULL) continue;

				SetOff(i+1);
			}
			
			return;
		}
		
		void Processing(uint32_t current_time)
		{
			if(current_time - _last_tick_time < _tick_time) return;
			_last_tick_time = current_time;
			
			for(channel_t &channel : _channels)
			{
				if(channel.pin.Port == nullptr) continue;
				if(channel.mode == MODE_OFF) continue;
				
				if(channel.mode == MODE_BLINK && current_time - channel.init_time > channel.blink_delay)
				{
					channel.init_time = current_time;
					
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
				else if(channel.mode == MODE_OFF_DELAY && current_time - channel.init_time > channel.blink_delay)
				{
					channel.mode = MODE_OFF;
					_HW_LOW(channel);
				}
			}
			
			return;
		}
		
	private:
		
		typedef struct
		{
			pin_t pin;
			
			mode_t mode;
			GPIO_PinState state;
			uint16_t blink_on;
			uint16_t blink_off;
			uint32_t blink_delay;
			uint32_t init_time;
		} channel_t;
		
		void _HW_HIGH(channel_t &channel)
		{
			HAL_GPIO_WritePin(channel.pin.Port, channel.pin.Pin, GPIO_PIN_SET);
			channel.state = GPIO_PIN_SET;
			
			return;
		}
		
		void _HW_LOW(channel_t &channel)
		{
			HAL_GPIO_WritePin(channel.pin.Port, channel.pin.Pin, GPIO_PIN_RESET);
			channel.state = GPIO_PIN_RESET;
			
			return;
		}
		
		void _HW_PinInit(pin_t pin, uint32_t mode)
		{
			HAL_GPIO_WritePin(pin.Port, pin.Pin, GPIO_PIN_RESET);
			
			_pin_config.Pin = pin.Pin;
			_pin_config.Mode = mode;
			HAL_GPIO_Init(pin.Port, &_pin_config);
			
			return;
		}
		
		channel_t _channels[_leds_max];
		
		uint32_t _last_tick_time = 0;

		GPIO_InitTypeDef _pin_config = { GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_LOW };
};
