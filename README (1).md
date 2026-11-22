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

| Component | Purpose |
|---|---|
| STM32L476 (Nucleo board) | Microcontroller |
| HC-SR04 | Ultrasonic distance sensor (input) |
| SG90 micro servo | Rotates the sensor |
| Buzzer | Audible alert |
| 16x2 character LCD + shift register | Status display (needs only 3 pins) |
| External 5 V supply | Powers the servo |

## Pin connections

| STM32 pin | Connected to | Function |
|---|---|---|
| PB3 | SG90 signal | TIM2_CH2 PWM (50 Hz, 1–2 ms pulse) |
| PC7 | HC-SR04 TRIG | GPIO output, 10 µs trigger pulse |
| PB6 | HC-SR04 ECHO | GPIO input (5 V tolerant, no pull-up/down) |
| PC9 | Buzzer | TIM3_CH4 PWM (~3.5 kHz) |
| PB5 | Shift register data | GPIO output |
| PA5 | Shift register clock | GPIO output |
| PA10 | Shift register latch | GPIO output |

### Power notes

- HC-SR04: VCC to 5 V, GND to STM32 GND.
- SG90: power it from a separate regulated 5 V supply, not the board. Connect the supply's GND to the STM32 GND.
- Don't connect the external +5 V to the STM32's 5 V or 3.3 V rails.

## How it works

1. **Sweep:** The main loop moves the servo 1° at a time. Each step sets the TIM2 compare value to a pulse between 1000 µs (0°) and 2000 µs (180°).
2. **Measure:** At most once every 60 ms, the STM32 sends a 10 µs pulse on TRIG. The HC-SR04 sends out a 40 kHz burst and holds ECHO high until the echo returns. TIM5 counts in microseconds to time the ECHO pulse.
3. **Convert:** Distance (cm) = echo time (µs) × 17 / 1000. Sound travels about 0.034 cm/µs, divided by 2 for the round trip. If no echo arrives within 20 ms, the reading counts as "no object."
4. **React:** If the distance is between 1 and 29 cm, the buzzer turns on (50% duty cycle on TIM3) and the LCD shows the distance. Otherwise the buzzer turns off and the LCD shows "Radar Clear."

## Timer setup

All timers run from the 4 MHz MSI clock, with a prescaler of 3 to get 1 MHz ticks.

| Timer | Use | Settings |
|---|---|---|
| TIM2 | Servo PWM | ARR = 19999 → 20 ms period |
| TIM3 | Buzzer PWM | ARR = 285 → ~3.5 kHz |
| TIM5 | Microsecond counter | 32-bit, free-running |
| SysTick | `HAL_Delay()` | 1 ms ticks |

## Building

1. Create an STM32CubeIDE project for the STM32L476 (for example, a NUCLEO-L476RG).
2. Replace the generated `main.c` with the one from this repository.
3. Build and flash to the board.

## Files

- `main.c` – All of the program code: LCD driver, servo, sensor, buzzer and the main loop
