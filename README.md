# STM32 Ultrasonic Radar

A simple radar system built on the STM32L476 microcontroller. An SG90 servo sweeps an HC-SR04 ultrasonic sensor back and forth from 0° to 180°. When an object comes within 30 cm, a buzzer sounds and a 16x2 LCD shows "Object Detected" and how far away it is.

The project uses register-level programming for the GPIO pins, timers and PWM. The STM32 HAL is used only for clock setup and millisecond delays.

## Features

- Servo sweeps the sensor 0° → 180° → 0° continuously
- Ultrasonic distance measurement accurate to 1 µs, timed with TIM5
- Buzzer turns on at about 3.5 kHz when an object is closer than 30 cm
- LCD shows "Radar Clear" or "Object Detected" with the distance in cm
- LCD only redraws when something changes, which avoids flicker
- Hardware PWM drives the servo and buzzer, so the CPU doesn't have to toggle pins

## Hardware

- **STM32L476** (Nucleo board): the microcontroller that runs everything
- **HC-SR04:** ultrasonic distance sensor, the system's input
- **SG90 micro servo:** rotates the sensor across the scan area
- **Buzzer:** gives an audible alert
- **16x2 character LCD with a shift register:** displays the status using only 3 pins
- **External 5 V supply:** powers the servo

## Pin connections

- **PB3 → SG90 signal:** TIM2 channel 2 PWM (50 Hz, 1–2 ms pulse)
- **PC7 → HC-SR04 TRIG:** GPIO output that sends a 10 µs trigger pulse
- **PB6 ← HC-SR04 ECHO:** GPIO input (5 V tolerant, no pull-up/down)
- **PC9 → Buzzer:** TIM3 channel 4 PWM (about 3.5 kHz)
- **PB5 → Shift register data**
- **PA5 → Shift register clock**
- **PA10 → Shift register latch**

The HC-SR04 runs on 5 V, with its GND connected to the STM32 GND. Power the SG90 from a separate regulated 5 V supply instead of the board, because the servo can draw more current than the board can safely provide. Connect the external supply's ground to the STM32 ground so all the signals share the same reference. Don't connect the external +5 V to the STM32's 5 V or 3.3 V rails.

## How it works

The main loop moves the servo 1° at a time, sweeping from 0° to 180° and back. Each step sets the TIM2 compare value to a pulse between 1000 µs (0°) and 2000 µs (180°). TIM2 repeats that pulse every 20 ms with no extra work from the CPU.

The system takes a distance reading during the sweep, at most once every 60 ms. The STM32 sends a 10 µs pulse on TRIG, and the HC-SR04 answers by sending out a 40 kHz burst of ultrasound. The sensor holds ECHO high until the sound bounces back. TIM5 counts in microseconds, so the program times exactly how long ECHO stays high.

To get the distance, the program multiplies the echo time in microseconds by 17 and divides by 1000. Sound travels about 0.034 cm per microsecond, and dividing by 2 accounts for the trip out and back. Using whole numbers keeps the math fast and avoids floating point. If no echo arrives within 20 ms, the reading counts as "no object."

If the distance is between 1 and 29 cm, TIM3 turns the buzzer on at a 50% duty cycle, and the LCD shows "Object Detected" with the distance. Otherwise the buzzer turns off and the LCD shows "Radar Clear."

## Timer setup

All timers run from the 4 MHz MSI clock. A prescaler of 3 divides it down to 1 MHz, so each timer tick is 1 µs.

- **TIM2 (servo PWM):** ARR = 19999, for a 20 ms period
- **TIM3 (buzzer PWM):** ARR = 285, for about 3.5 kHz
- **TIM5 (microsecond counter):** 32-bit, free-running
- **SysTick:** 1 ms ticks for `HAL_Delay()`

## Building

Create an STM32CubeIDE project for the STM32L476 (for example, a NUCLEO-L476RG). Replace the generated `main.c` with the one from this repository, then build and flash it to the board.
