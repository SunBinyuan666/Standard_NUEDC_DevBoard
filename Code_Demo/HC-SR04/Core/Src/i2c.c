/***********************************************************************************
 * @file      i2c.c
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     I2C外设初始化: I2C2标准模式100kHz(PB10=SCL/PB11=SDA开漏复用)
 *
 * @note      板级接线(所有元器件的连接方式.md §4.5 P1): PB10/PB11引至P1四针
 *           接口(1=5V/2=PB10/3=PB11/4=GND), SW5滑动开关可接入R21/R22
 *           4.7k上拉; 超声波模块(2020款HC-SR04 IIC模式)接P1即可。
 *           本文件结构与 CubeMX 生成布局一致, 应用逻辑在 app_* 模块。
 * @history   V1.0.0 首版: I2C2初始化+MSP引脚配置                    ---2026-11-11
 **********************************************************************************/
#include "i2c.h"

/* ==========================================================================
 *  公共变量
 * ========================================================================== */
I2C_HandleTypeDef hi2c2;

/* ==========================================================================
 *  公共函数
 * ========================================================================== */

/*************************************
 * 函数名称 ： MX_I2C2_Init
 * 描述     ： I2C2初始化: 标准模式100kHz, 7位寻址,
 *             APB1=36MHz作I2C内核时钟
 * 输入     ： 无
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void MX_I2C2_Init(void)
{
    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = 100000U;
    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = 0U;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0U;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK)
    {
        Error_Handler();
    }
}

/* ==========================================================================
 *  MSP初始化(覆盖HAL弱符号, 不可加static)
 * ========================================================================== */

/*************************************
 * 函数名称 ： HAL_I2C_MspInit
 * 描述     ： I2C底层初始化: 使能I2C2时钟, PB10/PB11配置为
 *             开漏复用输出(50MHz), 依赖板上SW5上拉或模块自带上拉
 * 输入     ： i2cHandle - I2C句柄
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_I2C_MspInit(I2C_HandleTypeDef *i2cHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (i2cHandle->Instance == I2C2)
    {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_I2C2_CLK_ENABLE();

        /* I2C2_SCL -> PB10, I2C2_SDA -> PB11: 开漏复用, 外部上拉 */
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
    else
    {
        /* 其他I2C实例未使用 */
    }
}

/*************************************
 * 函数名称 ： HAL_I2C_MspDeInit
 * 描述     ： I2C底层反初始化: 关I2C2时钟, 引脚恢复默认
 * 输入     ： i2cHandle - I2C句柄
 * 输出     ： 无
 * 返回     ： 无
 **************************************/
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *i2cHandle)
{
    if (i2cHandle->Instance == I2C2)
    {
        __HAL_RCC_I2C2_CLK_DISABLE();

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_11);
    }
    else
    {
        /* 其他I2C实例未使用 */
    }
}
