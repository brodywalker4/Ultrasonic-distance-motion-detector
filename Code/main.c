/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "main.h"

// Sets up the system clock
void SystemClock_Config(void);

// Sets of the GPIO pins
static void MX_GPIO_Init(void);

//LCD function intializations//
 void LCD_Init(void);
 void Write_SR_LCD(uint8_t data);
 void LCD_nibble_write(uint8_t data, uint8_t rs);
 void Write_Char_LCD(uint8_t letter);
 void Write_Instr_LCD(uint8_t instruction);
 void Write_String_LCD(char *text);
 void Write_Number_LCD(int number);
 void LCD_SetCursor(int line, int column);
 void Update_LCD(int detected, int cm);

//radar function intializations//
void Timer_Init(void);
void Servo_Init(void);
void Servo_Write(int angle);
void Radar_Init(void);
int Read_Distance(void);
void scan(void);
void Buzzer_Init(void);
void Delay_us(uint32_t microseconds);

//variables used by radar//
int distance = 0;
uint32_t lastScan = 0;

 // send one byte to shift register//
void Write_SR_LCD(uint8_t data)
{
	uint8_t mask = 0x80;
	int i;
	
 for(i = 0; i < 8; i++)
	{
		//PB5 for serial data//
		if((data & mask) == 0)
			GPIOB->ODR &= ~(1 << 5);
		else
			GPIOB->ODR |= (1 << 5);
		
		//Pulse PA5 to shift iin one bit//
		GPIOA->ODR &= ~(1 << 5);
		GPIOA->ODR |= (1 << 5);
		
		mask = mask >> 1;
	}
	//Pulse PA10 to under shift register//
		GPIOA->ODR &= ~(1 << 10);
		GPIOA->ODR |= (1 << 10);
}

void Write_Instr_LCD(uint8_t instruction)
{
	LCD_nibble_write(instruction & 0xF0, 0);
	LCD_nibble_write(instruction << 4, 0);
}

//send 4 bits to LCD with RS and Enable//
void LCD_nibble_write(uint8_t data, uint8_t rs)
{
	uint8_t temp;
	temp = data & 0xF0;
	
	if(rs == 1) //RS = 1 for a character//
			temp |= 0x01;
	
	temp |= 0x02; // enable = 1//
	Write_SR_LCD(temp);
	HAL_Delay(1);
	
	temp &= ~0x02; //enable = 0//
	Write_SR_LCD(temp);
	HAL_Delay(1);
}

//Display One character on the LCD//
void Write_Char_LCD(uint8_t letter)
{
	LCD_nibble_write(letter & 0xF0, 1);
	LCD_nibble_write(letter << 4, 1);
}
 //display a string on the LCD//
void Write_String_LCD(char *text)
{
	int i = 0;
	
		while(text[i] != '\0')
		{
			Write_Char_LCD(text[i]);
			i++;
		}
}

//Display an integer using divide by 10//
void Write_Number_LCD(int number)
{
	char digit[10];
	int count = 0;
	
	if(number < 0)
	{
		Write_Char_LCD('-');
		number = -number;
	}
	
	do
	{
		digit[count] = (number % 10) + '0';
		number = number / 10;
		count++;
	}while(number > 0);
	
	while(count > 0)
	{
		count--;
		Write_Char_LCD(digit[count]);
	}
}

void LCD_SetCursor(int line, int column)
{
	if(line == 0)
	{
		Write_Instr_LCD(0x80 + column);
	}
	else
	{
		Write_Instr_LCD(0xC0 + column);
	}
}

//configure GPIO pins for LCD//
void LCD_Init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	
	// PA5 = shift clock, PA10 = Latch//
	GPIOA->MODER &= ~(3 << (2*5));
	GPIOA->MODER |= (1 << (2*5));
	GPIOA->MODER &= ~(3 << (2*10));
	GPIOA->MODER |= (1 << (2*10));
	GPIOA->OTYPER &= ~((1 << 5) | (1 << 10));
	GPIOA->PUPDR &= ~((3 << (2*5)) | (3 << (2*10)));
	
	//PB5 = serial data//
	GPIOB->MODER &= ~(3 << (2*5));
	GPIOB->MODER |= (1 <<(2*5));
	GPIOB->OTYPER &= ~(1 << 5);
	GPIOB->PUPDR &= ~(3 << (2*5));
	
	GPIOA->ODR &= ~((1 << 5) | (1 << 10));
	GPIOB->ODR &= ~(1 << 5);
	
	//LCD Startup CODE//
	HAL_Delay(20);
	LCD_nibble_write(0x30, 0);
	HAL_Delay(5);
	LCD_nibble_write(0x30, 0);
	HAL_Delay(1);
	LCD_nibble_write(0x30, 0);
	HAL_Delay(1);
	LCD_nibble_write(0x20, 0); //Sets 4 Bit mode//
	HAL_Delay(1);
	
	Write_Instr_LCD(0x28);
	Write_Instr_LCD(0x0C);
	Write_Instr_LCD(0x01);
	Write_Instr_LCD(0x06);
}

//show weather an object is detected and how far away it is//
void Update_LCD(int detected, int cm)
{
	static int previousState = -1;
	static int previousDistance = -1;
	
	//clear and change the first line only when detection occurs.//
	if(detected != previousState)
	{
		Write_Instr_LCD(0x01); //clear LCD
		HAL_Delay(2);
		LCD_SetCursor(0,0);
		
		if(detected == 1)
		{
			Write_String_LCD("Object Detected");
		}
		else
		{
			Write_String_LCD("Radar Clear");
		}
		
		previousState = detected;
		previousDistance = -1;
	}
	//update second line if distance has changed//
	if(detected == 1 && cm != previousDistance)
	{
		LCD_SetCursor(1, 0);
		Write_Number_LCD(cm);
		Write_String_LCD(" cm away   ");
		previousDistance = cm;
	}
}


//Radar Functions//

//TIM5 counts microseconds for HC-SR04 sensor//
void Timer_Init(void)
{
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
	
	TIM5->PSC = 3; //4 MHz / (3+1) = 1 MHz
	TIM5->ARR = 0xFFFFFFFF; //32 bit timer//
	TIM5->EGR = 1;
	TIM5->CNT = 0;
	TIM5->CR1 |= 1; // start timer//
}

// wait for small number of microseconds//
void Delay_us(uint32_t microseconds)
{
	uint32_t start = TIM5->CNT;
	
	while((uint32_t)(TIM5->CNT - start) < microseconds)
	{
	}
}

//PB3 uses TIM2 Channel 2 to make servo pulse automatically//
void Servo_Init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
  RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	
	//PB3 = Alternate function mode (AF1= TIM2_CH2)
	GPIOB->MODER &= ~(3 << (2*3));
	GPIOB->MODER |=(2 << (2*3));
	GPIOB->AFR[0] &= ~ ( 15 << (4*3));
	GPIOB->AFR[0] |= (1 << (4*3));
	GPIOB->PUPDR &= ~(3 << (2*3));
	
	//1 timer tick = 1 microsecond; period = 20 ms//
	TIM2->PSC = 3;
	TIM2->ARR = 19999; 
	TIM2->CCR2 = 1500; // servo starts at center//
	
	//PWM mode 1 for channel 1//
	TIM2->CCMR1 &= ~(7 << 12);
	TIM2->CCMR1 |= (6 << 12);
	TIM2->CCMR1 |= ( 1 << 11); // compare preload//
	TIM2->CCER |= (1 << 4);  // enable channel 2 output//
	TIM2->CR1 |= (1 << 7);	//auto reload preload//
	TIM2->EGR = 1;
	TIM2->CR1 |= 1;	//start timer//
}

//send angle 0 to 180 degrees to the servo//
void Servo_Write(int angle)
{
	if(angle < 0)
	{
		angle = 0;
	}
	if(angle > 180)
	{
		angle = 180;
	}
	
	// 0 degrees = 1000 us; 180 = 2000 us
	TIM2->CCR2 = 1000 + (angle *1000) / 180;
}

//Set up the HC-SR04 sensor with ECHO = PB6, TRIG = PC7
void Radar_Init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
	
	//PC7 as output//
	GPIOC->MODER &= ~(3 << (2*7));
	GPIOC->MODER |= (1 << (2*7));
	GPIOC->OTYPER &= ~(1 << 7);
	GPIOC->PUPDR &= ~(3 << (2*7));
	GPIOC->ODR &= ~(1 << 7);
	
	//PB6 set to digital input with analog functions disabled so it is 5v tolerant//
	GPIOB->MODER &= ~(3 << (2*6));
	GPIOB->PUPDR &= ~(3 << (2*6));
}

//Read Distance measurement in cm//
//Return 0 if no echo return is received//
int Read_Distance(void)
{
	uint32_t start;
	uint32_t echoStart;
	uint32_t duration;
	
	//send 10us pulse to TRIG//
	GPIOC->ODR &= ~(1 << 7);
	Delay_us(3);
	GPIOC->ODR |= (1 << 7);
	Delay_us(10);
	GPIOC->ODR &= ~(1 << 7);
	
	//wait for ECHO to go high//
	start = TIM5->CNT;
	while((GPIOB->IDR & (1 << 6)) == 0)
	{
		if((uint32_t)(TIM5->CNT - start) >= 20000)
		{
			return 0;
		}
	}
	
	//measure how long ECHO stays high//
	echoStart = TIM5->CNT;
	while((GPIOB->IDR & (1 << 6)) != 0)
	{
		if((uint32_t)(TIM5->CNT - start) >= 20000)
		{
			return 0;
		}
	}
	
	duration = (uint32_t)(TIM5->CNT - echoStart);
	return (int)((duration * 17) / 1000); // cm = us*0.034 / 2//
}

//PC9 TIM3 channel 4 for a 3500 Hz buzzer tone//
void Buzzer_Init(void)
{
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
  RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN;
	
	//PC9 = Alternate function mode (AF2 = TIM3_CH4)
	GPIOC->MODER &= ~(3 << (2*9));
	GPIOC->MODER |= (2 << (2*9));
	GPIOC->AFR[1] &= ~(15 << (4 *(9-8)));
	GPIOC->AFR[1] |=  (2 << (4 *(9-8)));
	GPIOC->PUPDR &= ~(3 << (2*9));
	
	TIM3->PSC = 3;
	TIM3->ARR = 285; //3500 Hz
	TIM3->CCR4 = 0;// Start buzzer off//
	
	//PWM mode 1 for channel 1
	TIM3->CCMR2 &= ~(7 << 12);
  TIM3->CCMR2 |= (6 << 12);
  TIM3->CCMR2 |= (1 << 11);
  TIM3->CCER |= (1 << 12);
  TIM3->CR1 |= (1 << 7);
  TIM3->EGR = 1;
  TIM3->CR1 |= 1;
}

//Measure the Distance and control the LCD/Buzzer//
void scan(void)
{
	//HC-SR04 should not trigger more often than 60ms//
	if(HAL_GetTick() - lastScan < 60)
	{
		return;
	}
	
	lastScan = HAL_GetTick();
	distance = Read_Distance();
	
	if(distance > 0 && distance < 30)
	{
		// the object is in range play buzzer until out of range//
		TIM3->CCR4 = 143;
		Update_LCD(1, distance);
	}
	else
	{
		//no objects in range turn buzzer off
		TIM3->CCR4 = 0;
		Update_LCD(0,0);
	}
}


// The main function, and the entry point of the program
int main(void)
{
	int pos;
	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	
	LCD_Init();
	Timer_Init();
	Servo_Init();
	Radar_Init();
	Buzzer_Init();
	
	//start with radar clear//
	Update_LCD(0,0);
	
	while(1)
	{
		//sweep servo 0 to 180 degrees//
		for(pos = 0; pos <= 180; pos++)
		{
			Servo_Write(pos);
			scan();
			HAL_Delay(5);
		}
		
		//sweep from 180 to 0//
		for(pos = 180; pos >= 0; pos--)
		{
			Servo_Write(pos);
			scan();
			HAL_Delay(5);
		}
	}
}



// Set up the GPIO pins
static void MX_GPIO_Init(void)
{
  // TODO: Initialize any input or output pins you'll use here
	// Unused due to initializing my pins in the keypad and LCD Initalization//
}



// Set up the system clock
// ! IMPORTANT ! This function is generated by STM32CubeMX and should not be modified
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  // Configure the main internal regulator output voltage
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  // Initializes the RCC Oscillators according to the specified parameters
  //  in the RCC_OscInitTypeDef structure.
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  // Initializes the CPU, AHB and APB buses clocks
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}



// This function is called whenever there is an error, such as illegal memory access
void Error_Handler(void)
{
  // We don't really need to do anything for errors, so just stay here forever
  __disable_irq();
  while (1)
  {
  }
}



// ! IMPORTANT ! All code below this line is generated by STM32CubeMX and should not be modified
#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
