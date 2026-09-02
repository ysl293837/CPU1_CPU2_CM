#include "foc_svpwm.h"

PWM_Duty_CCR_Typedef PWM_Channel = {0};                   // 保存 U、V、W 三相 PWM 比较值
static Volt_Typedef V_alpha_beta = {0};                   // 保存逆 Park 变换后的 αβ 电压
static Time_Typedef Tabc = {0};                           // 保存三相 SVPWM 作用时间

float UA = 0.0f;                                          // A 相参考电压
float UB = 0.0f;                                          // B 相参考电压
float UC = 0.0f;                                          // C 相参考电压

static float sinx = 0.0f;                                 // 当前电角度的正弦值
static float cosx = 0.0f;                                 // 当前电角度的余弦值
uint16_t UA1 = 0U;                                        // 预留的 A 相调试变量

/* 1. 将 PWM 比较值限制到允许范围 */
static uint16_t FOC_LimitCompareValue(uint16_t value,
                                      uint16_t min_value,
                                      uint16_t max_value)
{
    if(value < min_value)                                 // 判断比较值是否低于最小安全值
    {
        return min_value;                                  // 低于下限时返回最小安全值
    }
    else if(value > max_value)                            // 判断比较值是否超过最大安全值
    {
        return max_value;                                  // 超过上限时返回最大安全值
    }

    return value;                                          // 比较值正常时直接返回
}

/* 2. 将浮点作用时间限制到指定范围 */
static float FOC_LimitFloat(float value, float min_value, float max_value)
{
    if(value < min_value)                                 // 判断输入是否低于下限
    {
        return min_value;                                  // 低于下限时返回下限
    }
    else if(value > max_value)                            // 判断输入是否高于上限
    {
        return max_value;                                  // 高于上限时返回上限
    }

    return value;                                          // 输入正常时直接返回
}

/* 3. 执行 dq 到 αβ 的逆 Park 变换 */
static void FOC_InvPark(float ud, float uq, float sin_theta,
                        float cos_theta, float *alpha, float *beta)
{
    *alpha = ud * cos_theta - uq * sin_theta;             // 计算 α 轴电压
    *beta = ud * sin_theta + uq * cos_theta;              // 计算 β 轴电压
}

/* 4. 执行 αβ 到 AB 的逆 Clarke 变换 */
static void FOC_InvClarke(float alpha, float beta, float *ua, float *ub)
{
    *ua = alpha;                                           // A 相电压等于 α 轴电压
    *ub = -0.5f * alpha + 0.8660254039f * beta;           // 根据 Clarke 公式计算 B 相电压
}

/* 5. 初始化 FOC 和 PWM 变量 */
void FOC_InitVariables(void)
{
    V_alpha_beta.V_alpha = 0.0f;                          // 清零 α 轴电压
    V_alpha_beta.V_beta = 0.0f;                           // 清零 β 轴电压
    Tabc.Ta = 0.0f;                                       // 清零 A 相作用时间
    Tabc.Tb = 0.0f;                                       // 清零 B 相作用时间
    Tabc.Tc = 0.0f;                                       // 清零 C 相作用时间
    UA = 0.0f;                                             // 清零 A 相参考电压
    UB = 0.0f;                                             // 清零 B 相参考电压
    UC = 0.0f;                                             // 清零 C 相参考电压
    sinx = 0.0f;                                           // 清零电角度正弦缓存
    cosx = 0.0f;                                           // 清零电角度余弦缓存
    PWM_Channel.CCR1 = PWM_PERIOD_CYCLES / 2U;            // U 相设置为 50% 初始占空比
    PWM_Channel.CCR2 = PWM_PERIOD_CYCLES / 2U;            // V 相设置为 50% 初始占空比
    PWM_Channel.CCR3 = PWM_PERIOD_CYCLES / 2U;            // W 相设置为 50% 初始占空比
}

/* 6. 计算当前电角度的正弦和余弦 */
void FOC_CalculateElectricalAngle(void)
{
    sinx = sinf(Motor_Electric_Angle);                    // 计算当前电角度正弦值
    cosx = cosf(Motor_Electric_Angle);                    // 计算当前电角度余弦值
}

/* 7. 将 dq 电压变换为三相参考电压 */
void FOC_VoltageCalculation(float ud, float uq)
{
    FOC_InvPark(ud, uq, sinx, cosx,                       // 先完成 dq 到 αβ 的逆 Park 变换
                &V_alpha_beta.V_alpha, &V_alpha_beta.V_beta);
    FOC_InvClarke(V_alpha_beta.V_alpha, V_alpha_beta.V_beta, // 再完成 αβ 到 AB 的逆 Clarke 变换
                  &UA, &UB);
    UC = -0.5f * V_alpha_beta.V_alpha -                   // 根据三相平衡关系计算 C 相电压
         0.8660254039f * V_alpha_beta.V_beta;
}

/* 8. 根据三相参考电压确定 SVPWM 扇区 */
void FOC_GetSector(uint8_t *sector, float UA_in, float UB_in, float UC_in)
{
    int16_t A = (UA_in > 0.0f) ? 1 : 0;                   // 记录 A 相电压的正负状态
    int16_t B = (UB_in > 0.0f) ? 1 : 0;                   // 记录 B 相电压的正负状态
    int16_t C = (UC_in > 0.0f) ? 1 : 0;                   // 记录 C 相电压的正负状态

    *sector = (uint8_t)(4 * C + 2 * B + A);               // 使用 4C+2B+A 编码得到 SVPWM 扇区号
}

/* 9. 按扇区计算三相 SVPWM 作用时间 */
void FOC_CalculateSectorTimes(uint8_t sector, float UA_in, float UB_in,
                              float UC_in, float *T1, float *T2, float *T3)
{
    float Ts = 1.0f / PWM_FREQUENCY * 2.0f;               // 计算中心对齐 PWM 的完整开关周期
    float k = SQRT3 * Ts / UREF;                          // 计算电压到作用时间的比例系数
    float Tx = 0.0f;                                      // 保存当前扇区的第一个有效矢量时间
    float Ty = 0.0f;                                      // 保存当前扇区的第二个有效矢量时间

    *T1 = Ts * 0.5f;                                      // 默认 A 相为 50% 占空比
    *T2 = Ts * 0.5f;                                      // 默认 B 相为 50% 占空比
    *T3 = Ts * 0.5f;                                      // 默认 C 相为 50% 占空比

    switch(sector)                                        // 根据扇区选择对应的有效矢量组合
    {
        case 3:                                           // 扇区 1
            Tx = k * UB_in;                               // 计算第一个有效矢量时间
            Ty = k * UA_in;                               // 计算第二个有效矢量时间
            *T3 = (Ts - Tx - Ty) / 2.0f;                  // 分配零矢量时间到 C 相基准
            *T2 = *T3 + Ty;                               // 叠加第二个有效矢量得到 B 相时间
            *T1 = *T2 + Tx;                               // 叠加第一个有效矢量得到 A 相时间
            break;                                        // 结束扇区 1 计算

        case 1:                                           // 扇区 2
            Tx = -k * UB_in;                              // 计算第一个有效矢量时间
            Ty = -k * UC_in;                              // 计算第二个有效矢量时间
            *T3 = (Ts - Tx - Ty) / 2.0f;                  // 分配零矢量时间到 C 相基准
            *T1 = *T3 + Ty;                               // 叠加第二个有效矢量得到 A 相时间
            *T2 = *T1 + Tx;                               // 叠加第一个有效矢量得到 B 相时间
            break;                                        // 结束扇区 2 计算

        case 5:                                           // 扇区 3
            Tx = k * UA_in;                               // 计算第一个有效矢量时间
            Ty = k * UC_in;                               // 计算第二个有效矢量时间
            *T1 = (Ts - Tx - Ty) / 2.0f;                  // 分配零矢量时间到 A 相基准
            *T3 = *T1 + Ty;                               // 叠加第二个有效矢量得到 C 相时间
            *T2 = *T3 + Tx;                               // 叠加第一个有效矢量得到 B 相时间
            break;                                        // 结束扇区 3 计算

        case 4:                                           // 扇区 4
            Tx = -k * UA_in;                              // 计算第一个有效矢量时间
            Ty = -k * UB_in;                              // 计算第二个有效矢量时间
            *T1 = (Ts - Tx - Ty) / 2.0f;                  // 分配零矢量时间到 A 相基准
            *T2 = *T1 + Ty;                               // 叠加第二个有效矢量得到 B 相时间
            *T3 = *T2 + Tx;                               // 叠加第一个有效矢量得到 C 相时间
            break;                                        // 结束扇区 4 计算

        case 6:                                           // 扇区 5
            Tx = k * UC_in;                               // 计算第一个有效矢量时间
            Ty = k * UB_in;                               // 计算第二个有效矢量时间
            *T2 = (Ts - Tx - Ty) / 2.0f;                  // 分配零矢量时间到 B 相基准
            *T1 = *T2 + Ty;                               // 叠加第二个有效矢量得到 A 相时间
            *T3 = *T1 + Tx;                               // 叠加第一个有效矢量得到 C 相时间
            break;                                        // 结束扇区 5 计算

        case 2:                                           // 扇区 6
            Tx = -k * UC_in;                              // 计算第一个有效矢量时间
            Ty = -k * UA_in;                              // 计算第二个有效矢量时间
            *T2 = (Ts - Tx - Ty) / 2.0f;                  // 分配零矢量时间到 B 相基准
            *T3 = *T2 + Ty;                               // 叠加第二个有效矢量得到 C 相时间
            *T1 = *T3 + Tx;                               // 叠加第一个有效矢量得到 A 相时间
            break;                                        // 结束扇区 6 计算

        default:                                          // 扇区 0 或 7 表示零矢量或异常边界
            *T1 = Ts * 0.5f;                              // 异常时输出 A 相 50% 占空比
            *T2 = Ts * 0.5f;                              // 异常时输出 B 相 50% 占空比
            *T3 = Ts * 0.5f;                              // 异常时输出 C 相 50% 占空比
            break;                                        // 结束默认安全输出
    }

    *T1 = FOC_LimitFloat(*T1, 0.0f, Ts);                 // 限制 A 相作用时间防止溢出
    *T2 = FOC_LimitFloat(*T2, 0.0f, Ts);                 // 限制 B 相作用时间防止溢出
    *T3 = FOC_LimitFloat(*T3, 0.0f, Ts);                 // 限制 C 相作用时间防止溢出
}

/* 10. 保留兼容接口，复用标准扇区时间计算 */
void FOC_CalculateSectorTimes2(uint8_t sector, float UA_in, float UB_in,
                               float UC_in, float *T1, float *T2, float *T3)
{
    FOC_CalculateSectorTimes(sector, UA_in, UB_in, UC_in, // 调用标准函数计算三相作用时间
                             T1, T2, T3);
}

/* 11. 将作用时间换算为 ePWM 比较值 */
void FOC_CalculatePWMValues(float T1, float T2, float T3, float Ts)
{
    uint16_t ccr1;                                        // 保存 U 相计算得到的比较值
    uint16_t ccr2;                                        // 保存 V 相计算得到的比较值
    uint16_t ccr3;                                        // 保存 W 相计算得到的比较值

    if(Ts <= 0.0f)                                        // 检查 PWM 周期是否有效
    {
        PWM_Channel.CCR1 = PWM_PERIOD_CYCLES / 2U;        // 无效周期时输出 U 相 50% 占空比
        PWM_Channel.CCR2 = PWM_PERIOD_CYCLES / 2U;        // 无效周期时输出 V 相 50% 占空比
        PWM_Channel.CCR3 = PWM_PERIOD_CYCLES / 2U;        // 无效周期时输出 W 相 50% 占空比
        return;                                           // 避免发生除零计算
    }

    ccr1 = (uint16_t)(T1 * (float)PWM_PERIOD_CYCLES / Ts); // 将 A 相作用时间换算为比较值
    ccr2 = (uint16_t)(T2 * (float)PWM_PERIOD_CYCLES / Ts); // 将 B 相作用时间换算为比较值
    ccr3 = (uint16_t)(T3 * (float)PWM_PERIOD_CYCLES / Ts); // 将 C 相作用时间换算为比较值

    PWM_Channel.CCR1 = FOC_LimitCompareValue(ccr1, PWM_CMP_MIN, PWM_CMP_MAX); // 限制并保存 U 相比较值
    PWM_Channel.CCR2 = FOC_LimitCompareValue(ccr2, PWM_CMP_MIN, PWM_CMP_MAX); // 限制并保存 V 相比较值
    PWM_Channel.CCR3 = FOC_LimitCompareValue(ccr3, PWM_CMP_MIN, PWM_CMP_MAX); // 限制并保存 W 相比较值
}

/* 12. 完成一次 FOC 到 SVPWM 的计算流程 */
void FOC_SVPWM_Update(float ud, float uq, float Electric_angle)
{
    uint8_t sector = 0U;                                  // 保存本次电压矢量所在扇区
    float Ts = 1.0f / PWM_FREQUENCY * 2.0f;               // 计算中心对齐 PWM 的完整周期

    Motor_Electric_Angle = Electric_angle;                // 保存输入的当前电角度
    FOC_CalculateElectricalAngle();                       // 计算电角度对应的正弦和余弦
    FOC_VoltageCalculation(ud, uq);                       // 将 dq 电压变换为三相参考电压
    FOC_GetSector(&sector, UA, UB, UC);                   // 根据三相参考电压确定 SVPWM 扇区
    FOC_CalculateSectorTimes(sector, UA, UB, UC,          // 计算三相 SVPWM 作用时间
                             &Tabc.Ta, &Tabc.Tb, &Tabc.Tc);
    FOC_CalculatePWMValues(Tabc.Ta, Tabc.Tb, Tabc.Tc, Ts); // 将作用时间换算为三相比较值
}

/* 13. 将计算结果写入三路 ePWM 硬件 */
void FOC_ApplyPWM(void)
{
    EPWM_setCounterCompareValue(FOC_EPWM_U_BASE,          // 选择 U 相 ePWM 模块
                                EPWM_COUNTER_COMPARE_A,   // 使用 CMPA 比较寄存器
                                PWM_Channel.CCR1);        // 写入 U 相比较值
    EPWM_setCounterCompareValue(FOC_EPWM_V_BASE,          // 选择 V 相 ePWM 模块
                                EPWM_COUNTER_COMPARE_A,   // 使用 CMPA 比较寄存器
                                PWM_Channel.CCR2);        // 写入 V 相比较值
    EPWM_setCounterCompareValue(FOC_EPWM_W_BASE,          // 选择 W 相 ePWM 模块
                                EPWM_COUNTER_COMPARE_A,   // 使用 CMPA 比较寄存器
                                PWM_Channel.CCR3);        // 写入 W 相比较值
}
