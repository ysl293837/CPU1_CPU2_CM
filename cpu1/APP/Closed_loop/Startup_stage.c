#include "Startup_stage.h"

#include "device.h"
#include "foc_svpwm.h"

/* 1. 使用固定 d 轴电压完成转子预定位 */
void Rotor_Positioning2(float UD, float UQ, uint32_t positioning_time_us)
{
    uint16_t i;                                           // 预定位前刷新 PWM 的循环计数器

    /* 2. 先反复写入固定电压矢量稳定逆变器输出 */
    for(i = 0U; i < 60000U; i++)                          // 连续刷新 PWM，保证预定位矢量已经稳定
    {
        FOC_SVPWM_Update(UD, UQ, 0.0f);                   // 在零电角度计算固定 dq 电压对应的三相 PWM
        FOC_ApplyPWM();                                   // 将本次三相 PWM 占空比写入硬件
    }

    /* 3. 保持零电角度直到预定位时间结束 */
    DEVICE_DELAY_US(positioning_time_us);                 // 保持转子对准 d 轴指定的微秒时间
}
