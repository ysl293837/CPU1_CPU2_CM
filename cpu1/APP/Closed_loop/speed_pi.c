#include "speed_pi.h"

PID_Typedef speedPID;                                    // 速度 PI 控制器的全部状态和参数
static float g_filtered_speed = 0.0f;                    // 低通滤波后的机械转速

extern float current_speed;                              // 由 ePWM ISR 更新的原始机械转速
extern float target_speed;                               // 由按键或 IPC 设置的目标机械转速

/* 1. 将浮点值限制到指定范围内 */
static float SpeedPI_Clamp(float value, float min_value, float max_value)
{
    if(value < min_value)                                // 判断输入是否低于下限
    {
        return min_value;                                 // 低于下限时返回下限
    }

    if(value > max_value)                                // 判断输入是否高于上限
    {
        return max_value;                                 // 高于上限时返回上限
    }

    return value;                                         // 输入处于范围内时直接返回
}

/* 2. 初始化速度 PI 参数和内部状态 */
void PMSM_PID_Init(void)
{
    speedPID.SetPoint = 0.0f;                             // 清零目标速度
    speedPID.LastError = 0.0f;                            // 清零上次速度误差
    speedPID.SumError = 0.0f;                             // 清零积分误差累加值
    speedPID.Proportion = 800.0f;                         // 设置默认比例系数
    speedPID.Integral = 1.0f;                             // 设置默认积分系数
    speedPID.Derivative = 0.0f;                           // 设置默认微分系数
    speedPID.Output = 0.0f;                               // 清零当前 PI 输出
    speedPID.OutputMin = -30000.0f;                       // 设置 PI 输出下限
    speedPID.OutputMax = 30000.0f;                        // 设置 PI 输出上限
    speedPID.FilterAlpha = 1.0f;                          // 默认不对速度进行额外平滑
    speedPID.UpdatePrescaler = 10U;                       // 默认每 10 个 PWM 周期更新一次 PI
    speedPID.UpdateCounter = 0U;                          // 清零 PI 分频计数器
    speedPID.Enabled = false;                             // 初始化后保持 PI 关闭
    g_filtered_speed = 0.0f;                              // 清零滤波后的速度
}

/* 3. 设置速度 PI 的 Kp、Ki、Kd */
void PMSM_SetPIDParams(float kp, float ki, float kd)
{
    speedPID.Proportion = kp;                             // 写入比例系数 Kp
    speedPID.Integral = ki;                               // 写入积分系数 Ki
    speedPID.Derivative = kd;                             // 写入微分系数 Kd
}

/* 4. 设置 PI 输出限幅 */
void PMSM_PID_SetOutputLimit(float min_output, float max_output)
{
    if(min_output > max_output)                           // 检查调用者是否将上下限顺序写反
    {
        float temp = min_output;                          // 临时保存错误顺序中的下限值
        min_output = max_output;                          // 将较小值放回下限
        max_output = temp;                                // 将较大值放回上限
    }

    speedPID.OutputMin = min_output;                      // 保存有效的 PI 输出下限
    speedPID.OutputMax = max_output;                      // 保存有效的 PI 输出上限
    speedPID.Output = SpeedPI_Clamp(speedPID.Output,      // 同时将已有输出限制到新范围内
                                    speedPID.OutputMin,
                                    speedPID.OutputMax);
}

/* 5. 设置速度反馈低通滤波系数 */
void PMSM_PID_SetFilterAlpha(float alpha)
{
    speedPID.FilterAlpha = SpeedPI_Clamp(alpha, 0.0f, 1.0f); // 将滤波系数限制在 0 至 1
}

/* 6. 设置速度环更新分频 */
void PMSM_PID_SetUpdatePrescaler(uint16_t prescaler)
{
    speedPID.UpdatePrescaler = (prescaler == 0U) ? 1U : prescaler; // 禁止零分频以避免控制器失效
}

/* 7. 设置目标机械转速 */
void PMSM_PID_SetTargetSpeed(float target_speed_rps)
{
    target_speed = target_speed_rps;                      // 同步更新主程序可观察的目标速度
    speedPID.SetPoint = target_speed_rps;                 // 写入 PI 控制器的目标速度
}

/* 8. 读取当前目标机械转速 */
float PMSM_PID_GetTargetSpeed(void)
{
    return speedPID.SetPoint;                             // 返回 PI 控制器当前目标值
}

/* 9. 使能或关闭速度 PI */
void PMSM_PID_Enable(bool enable)
{
    speedPID.Enabled = enable;                            // 保存新的使能状态

    if(!enable)                                           // 关闭 PI 时需要清除历史状态
    {
        PMSM_PID_Reset();                                 // 清除积分、误差和输出以避免下次突变
    }
}

/* 10. 按分频计算一次速度 PI 输出 */
void PMSM_PID_Update(void)
{
    float error;                                          // 保存当前目标与反馈的速度误差
    float p_term;                                         // 保存比例项输出
    float i_term;                                         // 保存积分项输出
    float d_term;                                         // 保存微分项输出

    if(!speedPID.Enabled)                                 // 未使能时不执行任何 PI 计算
    {
        return;                                           // 保持上次状态并直接退出
    }

    speedPID.UpdateCounter++;                             // 累计一个 PWM 控制周期
    if(speedPID.UpdateCounter < speedPID.UpdatePrescaler) // 未到分频更新点时直接退出
    {
        return;                                           // 将计算负载降到设定的速度环频率
    }

    speedPID.UpdateCounter = 0U;                          // 到达分频更新点后重新计数
    g_filtered_speed =                                   // 按一阶低通公式更新速度反馈
        (speedPID.FilterAlpha * current_speed) +
        ((1.0f - speedPID.FilterAlpha) * g_filtered_speed);

    error = speedPID.SetPoint - g_filtered_speed;         // 计算目标速度与滤波反馈的误差
    speedPID.SumError += error;                           // 将当前误差累加到积分项
    speedPID.SumError = SpeedPI_Clamp(speedPID.SumError, -12000.0f, 12000.0f); // 限制积分防止积分饱和

    p_term = speedPID.Proportion * error;                 // 计算比例项 Kp×error
    i_term = speedPID.Integral * speedPID.SumError;       // 计算积分项 Ki×Σerror
    d_term = speedPID.Derivative * (error - speedPID.LastError); // 计算微分项 Kd×Δerror

    speedPID.Output = SpeedPI_Clamp(p_term + i_term + d_term, // 求和后限制为允许的 q 轴电压范围
                                    speedPID.OutputMin,
                                    speedPID.OutputMax);
    speedPID.LastError = error;                           // 保存当前误差供下次微分计算使用
}

/* 11. 读取当前速度 PI 输出 */
float PMSM_PID_GetOutput(void)
{
    return speedPID.Output;                               // 返回限幅后的 PI 输出值
}

/* 12. 清零速度 PI 的内部状态 */
void PMSM_PID_Reset(void)
{
    speedPID.LastError = 0.0f;                            // 清除微分项的历史误差
    speedPID.SumError = 0.0f;                             // 清除积分误差累加值
    speedPID.Output = 0.0f;                               // 清除 PI 输出
    speedPID.UpdateCounter = 0U;                          // 清除更新分频计数器
    g_filtered_speed = 0.0f;                              // 清除速度滤波器历史状态
}

/* 13. 读取低通滤波后的机械转速 */
float PMSM_GetFilteredSpeed(void)
{
    return g_filtered_speed;                              // 返回一阶低通滤波的速度结果
}

/* 14. 读取当前积分项输出 */
float PMSM_PID_GetIntegralTerm(void)
{
    return speedPID.Integral * speedPID.SumError;         // 返回 Ki 乘以积分误差累加值
}
