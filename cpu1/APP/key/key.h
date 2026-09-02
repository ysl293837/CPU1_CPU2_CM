#ifndef APP_KEY_KEY_H
#define APP_KEY_KEY_H

#include <stdint.h>

void Key_Init(void);                                      // 初始化四个低电平有效按键
void Key_Process(void);                                   // 在主循环轮询并处理按键
float Key_GetCommandSpeed(void);                          // 读取当前限幅后的目标速度
void Key_SetCommandSpeed(float speed_rps);                // 设置并限幅目标速度，同时更新速度 PI

#endif /* APP_KEY_KEY_H */
