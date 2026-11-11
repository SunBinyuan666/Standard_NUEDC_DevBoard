/***********************************************************************************
 * @file      i2c.h
 * @author    Sunbinyuan（EEE2409036@xmu.edu.my）
 * @version   V1.0.0
 * @date      2026-11-11
 * @brief     I2C外设初始化接口(I2C2 硬件IIC, PB10=SCL/PB11=SDA)
 * @history   V1.0.0 首版: 句柄extern与初始化函数声明          ---2026-11-11
 **********************************************************************************/
#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

extern I2C_HandleTypeDef hi2c2;

void MX_I2C2_Init(void);

#ifdef __cplusplus
}
#endif
#endif /* __I2C_H__ */
