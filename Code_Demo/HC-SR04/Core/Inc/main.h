/***********************************************************************************
 * @file      main.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     工程公共引脚定义与全局错误处理接口
 *
 * @note      引脚分配(设计要求§4表):
 *           PA6=TIM3_CH1舵机PWM / PB0=TRIG / PB9=TIM4_CH4 ECHO
 *           PB6=LED3 / PB7=LED4 / PD8=LED5(均低电平点亮)
 *           PE13=SW2 / PE14=SW3 / PE15=SW4(输入上拉低有效)
 * @history   V1.0.0 首版: 引脚宏集中定义                      ---2026-11-11
 **********************************************************************************/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* ==========================================================================
 *  引脚定义(设计要求§4)
 * ========================================================================== */
#define HCSR04_TRIG_Pin        GPIO_PIN_0
#define HCSR04_TRIG_GPIO_Port  GPIOB     /* TRIG: PB0推挽输出 */

#define LED3_Pin               GPIO_PIN_6
#define LED3_GPIO_Port         GPIOB     /* LED3: PB6低亮 */
#define LED4_Pin               GPIO_PIN_7
#define LED4_GPIO_Port         GPIOB     /* LED4: PB7低亮 */
#define LED5_Pin               GPIO_PIN_8
#define LED5_GPIO_Port         GPIOD     /* LED5: PD8低亮 */

#define SW2_Pin                GPIO_PIN_13
#define SW2_GPIO_Port          GPIOE     /* SW2: PE13输入上拉低有效 */
#define SW3_Pin                GPIO_PIN_14
#define SW3_GPIO_Port          GPIOE     /* SW3: PE14输入上拉低有效 */
#define SW4_Pin                GPIO_PIN_15
#define SW4_GPIO_Port          GPIOE     /* SW4: PE15输入上拉低有效 */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
