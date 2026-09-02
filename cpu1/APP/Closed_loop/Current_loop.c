/* current_loop.c
 *
 * 适用于 YXPHM-MBDL4 驱动板的三相电流采样与电流环控制
 *
 * 电流采样芯片：
 *   CC6903SO-30A
 *
 * 关键换算关系：
 *   Vout = Vcc / 2 + 0.044 * Ip
 *
 * 当 Vcc = 3.3V 时：
 *   零电流点约为 1.65V
 *   1A 对应 44mV
 *   1V 对应约 22.727A
 *
 * 如果 ADC 为 16 位，Vref = 3.3V：
 *   1 LSB ≈ 3.3 / 65535 / 0.044 = 0.001144 A
 *
 * 如果 ADC 为 12 位，Vref = 3.3V：
 *   1 LSB ≈ 3.3 / 4095 / 0.044 = 0.01832 A
 *
 * 注意：
 *   1. 本文件默认 ADC 为 16 位，因为你原来的代码写的是 65535。
 *   2. 如果你的 CubeMX 里 ADC 配置成 12 位，请把 CURRENT_ADC_MAX_COUNT_U 改为 4095U。
 *   3. 原理图中相电流方向如果是 IP- -> IP+，而你希望该方向定义为正电流，
 *      通常需要把 CURRENT_U_SIGN / CURRENT_V_SIGN / CURRENT_W_SIGN 设为 -1。
 *   4. 如果实际测试时“正电流”导致 ADC 电压升高，则把对应相的 SIGN 改成 +1。
 */

#include "current_loop.h"
#include <string.h>
#include <stddef.h>

/* ============================================================
 * 1. 全局变量定义
 * ============================================================ */

PID_Current_Typedef pid_id = {0};    /* Id 轴电流环 PID */
PID_Current_Typedef pid_iq = {0};    /* Iq 轴电流环 PID */
Current_Data_Typedef current = {0};  /* 电流、电压、坐标变换数据 */
PMSM_Params_Typedef pmsm = {0};      /* 电机参数 */

extern float Motor_Electric_Angle;

float theta_elec = 0.0f;             /* 电角度，单位 rad，外部编码器模块需要更新它 */
float theta_mech = 0.0f;             /* 机械角度，单位 rad */
float target_id = 0.0f;              /* d 轴目标电流，通常为 0A */
float target_iq = 0.0f;              /* q 轴目标电流，通常来自速度环输出 */

bool current_loop_enabled = false;   /* 电流环使能标志 */


/* ============================================================
 * 2. 可调参数区
 * ============================================================ */

/* ---------- ADC 与 CC6903 电流传感器参数 ---------- */

/* ADC 参考电压。
 * 如果你的 ADC 参考不是 3.3V，需要改这里。
 */
#define CURRENT_ADC_REF_VOLTAGE      3.3f

/* ADC 满量程计数。
 * 16 位 ADC：65535U
 * 12 位 ADC：4095U
 *
 * 你的旧代码使用 65535，所以这里默认保持 16 位。
 */
#define CURRENT_ADC_MAX_COUNT_U      65535U
#define CURRENT_ADC_MAX_COUNT_F      ((float)CURRENT_ADC_MAX_COUNT_U)

/* 零电流 ADC 理论值。
 * CC6903 零电流输出为 Vcc/2。
 * 如果 ADC 是 16 位，则约为 32768。
 * 如果 ADC 是 12 位，则约为 2048。
 *
 * 实际使用时不建议完全依赖理论值，
 * 应该在电机不上电流时采样平均，然后调用 Current_SetAdcOffsetRaw() 设置。
 */
#define CURRENT_ADC_BIAS_RAW         ((uint16_t)((CURRENT_ADC_MAX_COUNT_U + 1U) / 2U))

/* CC6903SO-30A 灵敏度。
 * 30A 版本：44mV/A = 0.044V/A
 */
#define CC6903_SENS_V_PER_A          0.044f

/* 三相电流符号方向。
 *
 * 原理图上如果实际相电流方向是 IP- -> IP+，
 * 而数据手册公式中的正方向通常按 IP+ -> IP- 理解，
 * 那么软件中要乘以 -1 才能让“实际正电流”对应“程序正电流”。
 *
 * 如果你实测：
 *   正电流时 ADC 值下降：保持 -1
 *   正电流时 ADC 值上升：改成 +1
 */
#define CURRENT_U_SIGN               (-1.0f)
#define CURRENT_V_SIGN               (-1.0f)
#define CURRENT_W_SIGN               (-1.0f)

/* ---------- 电流采样滤波参数 ---------- */

/* 是否启用三点中值滤波。
 * 用于抑制单点毛刺。
 * 电流环要求响应快，如果你发现电流响应变慢，可以改为 0。
 */
#define CURRENT_USE_MEDIAN3          1

/* 是否启用单拍跳变限制。
 * 用于防止 ADC 偶发尖峰直接进入 Park 变换和 PI。
 * 如果电流环调试时感觉动态被限制，可以改为 0。
 */
#define CURRENT_USE_STEP_LIMIT       1

/* 电流一阶低通滤波系数。
 *
 * y = y + alpha * (x - y)
 *
 * alpha 越大，响应越快，但噪声越大。
 * alpha 越小，波形越平滑，但延迟越大。
 *
 * 建议：
 *   10kHz 电流环：0.10 ~ 0.30
 *   20kHz 电流环：0.06 ~ 0.20
 */
#define CURRENT_FILTER_ALPHA         0.15f

/* 单次采样允许的最大电流跳变量，单位 A。
 * 如果某一拍采样突然跳变超过该值，则认为可能是毛刺，限制其变化速度。
 *
 * 注意：
 *   这个值不要太小，否则真正的快速电流变化也会被削弱。
 */
#define CURRENT_SPIKE_LIMIT_A        6.0f


/* ---------- 电流环 PI 参数 ---------- */

/* 电流环控制周期，单位 s。
 * 必须和你调用 PMSM_Current_Control_Update() 的周期一致。
 *
 * 例如：
 *   10kHz 电流环：Ts = 0.0001
 *   20kHz 电流环：Ts = 0.00005
 */
#define CURRENT_CONTROL_TS           0.0001f

/* Id/Iq 电流环 PI 初始参数。
 *
 * 这里的 Ki 是连续形式的积分增益，程序内部使用：
 *   integral += Ki * error * Ts
 *
 * 初始值只是保守可运行值，实际需要根据电机电感、电阻和 PWM 频率调参。
 */
#define ID_KP                        1.0f
#define ID_KI                        300.0f
#define ID_KD                        0.0f

#define IQ_KP                        1.0f
#define IQ_KI                        300.0f
#define IQ_KD                        0.0f

/* 最大电流限幅，单位 A。
 * 这个值不能超过 CC6903SO-30A 的测量范围，也不能超过驱动板和电机允许电流。
 */
#define MAX_CURRENT                  10.0f

/* 电流环输出电压限幅，单位 V。
 *
 * 如果母线是 24V，SVPWM 线性区相电压最大约为：
 *   Udc / sqrt(3) = 13.86V
 *
 * 如果母线是 48V，约为：
 *   27.7V
 *
 * 你原代码里使用 18V，这里保留一个偏保守的默认值。
 * 如果你的母线是 24V，建议先设为 12~13V。
 * 如果你的母线是 48V/60V，可以适当提高。
 */
#define CURRENT_VOLTAGE_LIMIT        10.0f

/* 积分项限幅，单位 V。
 * 积分项限幅最好不要大于总输出电压限幅。
 */
#define CURRENT_INTEGRAL_LIMIT       CURRENT_VOLTAGE_LIMIT


/* ============================================================
 * 3. 私有变量
 * ============================================================ */

/* 三相零电流 ADC 偏置。
 * 默认用理论中点值，但实际必须标定。
 */
static uint16_t current_offset_u_raw = CURRENT_ADC_BIAS_RAW;
static uint16_t current_offset_v_raw = CURRENT_ADC_BIAS_RAW;
static uint16_t current_offset_w_raw = CURRENT_ADC_BIAS_RAW;

/* 三相电流滤波状态 */
static float current_ia_filter = 0.0f;
static float current_ib_filter = 0.0f;
static float current_ic_filter = 0.0f;

/* 三点中值滤波缓存 */
static float current_ia_raw_hist[3] = {0.0f, 0.0f, 0.0f};
static float current_ib_raw_hist[3] = {0.0f, 0.0f, 0.0f};
static float current_ic_raw_hist[3] = {0.0f, 0.0f, 0.0f};

static uint8_t current_raw_hist_index = 0U;
static uint8_t current_raw_hist_count = 0U;
static bool current_filter_ready = false;


/* ============================================================
 * 4. 私有函数声明
 * ============================================================ */

static float Current_Clamp(float x, float min_value, float max_value);
static float Current_Median3(float a, float b, float c);
static float Current_LimitStep(float sample, float last);
static float Current_WrapAngleRad(float angle);
static float Current_AdcToAmp(uint16_t adc_raw, uint16_t offset_raw, float sign);
static void PID_Current_Reset(PID_Current_Typedef *pid);
static float PID_Current_Calculate(PID_Current_Typedef *pid, float measured, float target);
static void Current_PI_Controller(void);
static void Voltage_Vector_Limit(float *ud, float *uq, float max_voltage);


/* ============================================================
 * 5. 基础工具函数
 * ============================================================ */

static float Current_Clamp(float x, float min_value, float max_value)
{
    if (x > max_value)
    {
        return max_value;
    }
    if (x < min_value)
    {
        return min_value;
    }
    return x;
}


static float Current_Median3(float a, float b, float c)
{
    float t;

    if (a > b)
    {
        t = a;
        a = b;
        b = t;
    }

    if (b > c)
    {
        t = b;
        b = c;
        c = t;
    }

    if (a > b)
    {
        t = a;
        a = b;
        b = t;
    }

    return b;
}


static float Current_LimitStep(float sample, float last)
{
    float delta = sample - last;

    if (delta > CURRENT_SPIKE_LIMIT_A)
    {
        return last + CURRENT_SPIKE_LIMIT_A;
    }

    if (delta < -CURRENT_SPIKE_LIMIT_A)
    {
        return last - CURRENT_SPIKE_LIMIT_A;
    }

    return sample;
}


static float Current_WrapAngleRad(float angle)
{
    const float two_pi = 6.2831853071795864769f;

    angle = fmodf(angle, two_pi);

    if (angle < 0.0f)
    {
        angle += two_pi;
    }

    return angle;
}


/* ============================================================
 * 6. ADC 到实际电流换算
 * ============================================================ */

static float Current_AdcToAmp(uint16_t adc_raw, uint16_t offset_raw, float sign)
{
    float delta_count;
    float amp_per_count;
    float current_amp;

    /* ADC 偏离零点的计数值 */
    delta_count = (float)adc_raw - (float)offset_raw;

    /* 每个 ADC 计数对应多少安培。
     *
     * Vadc = adc / ADC_MAX * Vref
     * I = (Vadc - Voffset) / 0.044
     *
     * 所以：
     * I = (adc - offset) * Vref / ADC_MAX / 0.044
     */
    amp_per_count = CURRENT_ADC_REF_VOLTAGE /
                    (CURRENT_ADC_MAX_COUNT_F * CC6903_SENS_V_PER_A);

    current_amp = delta_count * amp_per_count;

    /* 根据 IP+ / IP- 实际走线方向修正符号 */
    current_amp = sign * current_amp;

    return current_amp;
}


/* ============================================================
 * 7. 对外接口：初始化
 * ============================================================ */

void Current_Loop_Init(void)
{
    PMSM_Current_PID_Init();

    /* 这里是电机参数默认值。
     * 实际项目中建议在主程序初始化时调用 PMSM_Set_Motor_Params() 重新设置。
     */
    pmsm.I_bus = 0.0f;
    pmsm.R = 0.5f;
    pmsm.Ld = 0.001f;
    pmsm.Lq = 0.001f;
    pmsm.Psi_f = 0.1f;
    pmsm.PolePairs = 6U;
    pmsm.Kt = 1.5f * (float)pmsm.PolePairs * pmsm.Psi_f;

    memset(&current, 0, sizeof(Current_Data_Typedef));

    theta_elec = 0.0f;
    theta_mech = 0.0f;
    target_id = 0.0f;
    target_iq = 0.0f;

    Current_ResetFilter();
}


void PMSM_Current_PID_Init(void)
{
    pid_id.SetPoint = 0.0f;
    pid_id.Proportion = ID_KP;
    pid_id.Integral = ID_KI;
    pid_id.Derivative = ID_KD;
    pid_id.LastError = 0.0f;
    pid_id.SumError = 0.0f;
    pid_id.Output = 0.0f;

    pid_iq.SetPoint = 0.0f;
    pid_iq.Proportion = IQ_KP;
    pid_iq.Integral = IQ_KI;
    pid_iq.Derivative = IQ_KD;
    pid_iq.LastError = 0.0f;
    pid_iq.SumError = 0.0f;
    pid_iq.Output = 0.0f;

    target_id = 0.0f;
    target_iq = 0.0f;

    current_loop_enabled = false;
}


static void PID_Current_Reset(PID_Current_Typedef *pid)
{
    if (pid == NULL)
    {
        return;
    }

    pid->LastError = 0.0f;
    pid->SumError = 0.0f;
    pid->Output = 0.0f;
}


/* ============================================================
 * 8. 电流采样函数
 * ============================================================ */

void Current_Sample_Update(uint16_t adc_a,
                           uint16_t adc_b,
                           uint16_t adc_c,
                           float *ia,
                           float *ib,
                           float *ic)
{
    float raw_ia;
    float raw_ib;
    float raw_ic;

    float sample_ia;
    float sample_ib;
    float sample_ic;

    if ((ia == NULL) || (ib == NULL) || (ic == NULL))
    {
        return;
    }

    /* 1. ADC 原始值转换为实际电流。
     *
     * adc_a / adc_b / adc_c 的顺序必须和你的 ADC DMA 通道顺序一致。
     * 一般对应：
     *   adc_a -> CH1_IA_ADC
     *   adc_b -> CH1_IB_ADC
     *   adc_c -> CH1_IC_ADC
     */
    raw_ia = Current_AdcToAmp(adc_a, current_offset_u_raw, CURRENT_U_SIGN);
    raw_ib = Current_AdcToAmp(adc_b, current_offset_v_raw, CURRENT_V_SIGN);
    raw_ic = Current_AdcToAmp(adc_c, current_offset_w_raw, CURRENT_W_SIGN);

#if CURRENT_USE_MEDIAN3
    /* 2. 三点中值滤波。
     * 作用：
     *   抑制偶发 ADC 毛刺。
     */
    current_ia_raw_hist[current_raw_hist_index] = raw_ia;
    current_ib_raw_hist[current_raw_hist_index] = raw_ib;
    current_ic_raw_hist[current_raw_hist_index] = raw_ic;

    current_raw_hist_index++;

    if (current_raw_hist_index >= 3U)
    {
        current_raw_hist_index = 0U;
    }

    if (current_raw_hist_count < 3U)
    {
        current_raw_hist_count++;
    }

    if (current_raw_hist_count >= 3U)
    {
        sample_ia = Current_Median3(current_ia_raw_hist[0],
                                    current_ia_raw_hist[1],
                                    current_ia_raw_hist[2]);

        sample_ib = Current_Median3(current_ib_raw_hist[0],
                                    current_ib_raw_hist[1],
                                    current_ib_raw_hist[2]);

        sample_ic = Current_Median3(current_ic_raw_hist[0],
                                    current_ic_raw_hist[1],
                                    current_ic_raw_hist[2]);
    }
    else
    {
        sample_ia = raw_ia;
        sample_ib = raw_ib;
        sample_ic = raw_ic;
    }
#else
    sample_ia = raw_ia;
    sample_ib = raw_ib;
    sample_ic = raw_ic;
#endif

    /* 3. 第一次进入时，直接用当前采样初始化滤波器。
     * 这样可以避免滤波器从 0 慢慢爬升造成初始误差。
     */
    if (!current_filter_ready)
    {
        current_ia_filter = sample_ia;
        current_ib_filter = sample_ib;
        current_ic_filter = sample_ic;
        current_filter_ready = true;
    }
    else
    {
#if CURRENT_USE_STEP_LIMIT
        /* 4. 单拍跳变限制。
         * 防止孤立尖峰直接进入电流环。
         */
        sample_ia = Current_LimitStep(sample_ia, current_ia_filter);
        sample_ib = Current_LimitStep(sample_ib, current_ib_filter);
        sample_ic = Current_LimitStep(sample_ic, current_ic_filter);
#endif

        /* 5. 一阶低通滤波。
         * 电流环里滤波不能太重，否则会引入明显相位滞后。
         */
        current_ia_filter += CURRENT_FILTER_ALPHA * (sample_ia - current_ia_filter);
        current_ib_filter += CURRENT_FILTER_ALPHA * (sample_ib - current_ib_filter);
        current_ic_filter += CURRENT_FILTER_ALPHA * (sample_ic - current_ic_filter);
    }

    *ia = current_ia_filter;
    *ib = current_ib_filter;
    *ic = current_ic_filter;

    current.Ia = current_ia_filter;
    current.Ib = current_ib_filter;
    current.Ic = current_ic_filter;
}


void ADC_Current_Convert(uint16_t adc_a,
                         uint16_t adc_b,
                         uint16_t adc_c,
                         float *ia,
                         float *ib,
                         float *ic)
{
    Current_Sample_Update(adc_a, adc_b, adc_c, ia, ib, ic);
}


void Current_SetAdcOffsetRaw(uint16_t offset_u,
                             uint16_t offset_v,
                             uint16_t offset_w)
{
    current_offset_u_raw = offset_u;
    current_offset_v_raw = offset_v;
    current_offset_w_raw = offset_w;

    /* 零点改变后，滤波器必须重置，否则旧滤波状态会污染新结果。 */
    Current_ResetFilter();
}


void Current_GetAdcOffsetRaw(uint16_t *offset_u,
                             uint16_t *offset_v,
                             uint16_t *offset_w)
{
    if (offset_u != NULL)
    {
        *offset_u = current_offset_u_raw;
    }

    if (offset_v != NULL)
    {
        *offset_v = current_offset_v_raw;
    }

    if (offset_w != NULL)
    {
        *offset_w = current_offset_w_raw;
    }
}


void Current_ResetFilter(void)
{
    current_ia_filter = 0.0f;
    current_ib_filter = 0.0f;
    current_ic_filter = 0.0f;

    current_ia_raw_hist[0] = 0.0f;
    current_ia_raw_hist[1] = 0.0f;
    current_ia_raw_hist[2] = 0.0f;

    current_ib_raw_hist[0] = 0.0f;
    current_ib_raw_hist[1] = 0.0f;
    current_ib_raw_hist[2] = 0.0f;

    current_ic_raw_hist[0] = 0.0f;
    current_ic_raw_hist[1] = 0.0f;
    current_ic_raw_hist[2] = 0.0f;

    current_raw_hist_index = 0U;
    current_raw_hist_count = 0U;

    current_filter_ready = false;
}


float Current_GetFilterAlpha(void)
{
    return CURRENT_FILTER_ALPHA;
}


float Current_GetSpikeLimitA(void)
{
    return CURRENT_SPIKE_LIMIT_A;
}


/* ============================================================
 * 9. Clarke / Park / 反 Park 变换
 * ============================================================ */

void Clark_Transform(float ia,
                     float ib,
                     float ic,
                     float *i_alpha,
                     float *i_beta)
{
    if ((i_alpha == NULL) || (i_beta == NULL))
    {
        return;
    }

    /* 三电阻 / 三传感器 Clarke 变换。
     *
     * 与 arm_clarke_f32(ia, ib) 不同，这里使用 ia、ib、ic 三相都参与计算。
     * 好处：
     *   当三相 offset 有轻微误差时，比只用 ia、ib 更稳一些。
     *
     * 等幅值 Clarke：
     *   Ialpha = 2/3 * (Ia - 0.5*Ib - 0.5*Ic)
     *   Ibeta  = 1/sqrt(3) * (Ib - Ic)
     */
    *i_alpha = 0.6666666667f * (ia - 0.5f * ib - 0.5f * ic);
    *i_beta  = 0.5773502692f * (ib - ic);
}


void Park_Transform(float i_alpha,
                    float i_beta,
                    float theta,
                    float *id,
                    float *iq)
{
    float sin_theta;
    float cos_theta;

    if ((id == NULL) || (iq == NULL))
    {
        return;
    }

    theta = Current_WrapAngleRad(theta);

    sin_theta = arm_sin_f32(theta);
    cos_theta = arm_cos_f32(theta);

    /* Park 变换：
     *   Id =  Ialpha*cos(theta) + Ibeta*sin(theta)
     *   Iq = -Ialpha*sin(theta) + Ibeta*cos(theta)
     */
    *id = i_alpha * cos_theta + i_beta * sin_theta;
    *iq = -i_alpha * sin_theta + i_beta * cos_theta;
}


void Inv_Park_Transform(float ud,
                        float uq,
                        float theta,
                        float *u_alpha,
                        float *u_beta)
{
    float sin_theta;
    float cos_theta;

    if ((u_alpha == NULL) || (u_beta == NULL))
    {
        return;
    }

    theta = Current_WrapAngleRad(theta);

    sin_theta = arm_sin_f32(theta);
    cos_theta = arm_cos_f32(theta);

    /* 反 Park 变换：
     *   Ualpha = Ud*cos(theta) - Uq*sin(theta)
     *   Ubeta  = Ud*sin(theta) + Uq*cos(theta)
     */
    *u_alpha = ud * cos_theta - uq * sin_theta;
    *u_beta  = ud * sin_theta + uq * cos_theta;
}


/* ============================================================
 * 10. 电流环 PI 控制器
 * ============================================================ */

static float PID_Current_Calculate(PID_Current_Typedef *pid,
                                   float measured,
                                   float target)
{
    float error;
    float p_term;
    float i_candidate;
    float d_term;
    float output_unsat;
    float output_sat;

    if (pid == NULL)
    {
        return 0.0f;
    }

    error = target - measured;

    /* 比例项 */
    p_term = pid->Proportion * error;

    /* 积分项。
     *
     * 注意：
     *   这里 pid->SumError 实际保存的是“积分输出电压”，不是单纯误差累加。
     *   这样更方便做电压单位的积分限幅。
     */
    i_candidate = pid->SumError + pid->Integral * error * CURRENT_CONTROL_TS;
    i_candidate = Current_Clamp(i_candidate,
                                -CURRENT_INTEGRAL_LIMIT,
                                CURRENT_INTEGRAL_LIMIT);

    /* 微分项。
     *
     * 电流环一般不建议使用 D 项，因为电流采样噪声会被放大。
     * 默认 Kd = 0。
     */
    d_term = pid->Derivative * (error - pid->LastError) / CURRENT_CONTROL_TS;

    output_unsat = p_term + i_candidate + d_term;
    output_sat = Current_Clamp(output_unsat,
                               -CURRENT_VOLTAGE_LIMIT,
                               CURRENT_VOLTAGE_LIMIT);

    /* 简单抗积分饱和：
     *
     * 如果输出没有饱和，允许积分更新；
     * 如果输出已经饱和，但误差方向有助于退出饱和，也允许积分更新；
     * 否则冻结积分，防止越积越大。
     */
    if ((output_unsat == output_sat) ||
        ((output_unsat > CURRENT_VOLTAGE_LIMIT) && (error < 0.0f)) ||
        ((output_unsat < -CURRENT_VOLTAGE_LIMIT) && (error > 0.0f)))
    {
        pid->SumError = i_candidate;
    }

    output_unsat = p_term + pid->SumError + d_term;
    output_sat = Current_Clamp(output_unsat,
                               -CURRENT_VOLTAGE_LIMIT,
                               CURRENT_VOLTAGE_LIMIT);

    pid->LastError = error;
    pid->Output = output_sat;

    return output_sat;
}


static void Current_PI_Controller(void)
{
    current.Ud = PID_Current_Calculate(&pid_id, current.Id, target_id);
    current.Uq = PID_Current_Calculate(&pid_iq, current.Iq, target_iq);
}


static void Voltage_Vector_Limit(float *ud, float *uq, float max_voltage)
{
    float mag;
    float scale;

    if ((ud == NULL) || (uq == NULL))
    {
        return;
    }

    if (max_voltage <= 0.0f)
    {
        *ud = 0.0f;
        *uq = 0.0f;
        return;
    }

    mag = sqrtf((*ud) * (*ud) + (*uq) * (*uq));

    if (mag > max_voltage)
    {
        scale = max_voltage / mag;
        *ud *= scale;
        *uq *= scale;
    }
}


/* ============================================================
 * 11. 电流环主更新函数
 * ============================================================ */

void PMSM_Current_Control_Update(uint16_t adc_a,
                                 uint16_t adc_b,
                                 uint16_t adc_c,
                                 float id_ref,
                                 float iq_ref)
{
    float ia;
    float ib;
    float ic;

    /* 1. 设置电流参考。
     *
     * 注意：
     *   你的 .h 文件里最后两个参数名字可能还叫 ud、uq。
     *   但从控制逻辑看，它们应该是 id_ref、iq_ref。
     */
    target_id = id_ref;
    target_iq = iq_ref;

    /* 2. 电流参考限幅，防止速度环或外部给定过大。 */
    Current_Limit(&target_id, &target_iq, MAX_CURRENT);

    pid_id.SetPoint = target_id;
    pid_iq.SetPoint = target_iq;

    /* 3. ADC 转换为三相电流。
     *
     * 即使电流环没有使能，也更新 current.Ia/Ib/Ic，
     * 这样你可以在串口或者上位机里观察电流采样是否正常。
     */
    ADC_Current_Convert(adc_a, adc_b, adc_c, &ia, &ib, &ic);

    current.Ia = ia;
    current.Ib = ib;
    current.Ic = ic;

    /* 4. Clarke 变换：abc -> alpha/beta */
    Clark_Transform(current.Ia,
                    current.Ib,
                    current.Ic,
                    &current.I_alpha,
                    &current.I_beta);

    /* 5. Park 变换：alpha/beta -> d/q
     *
     * theta_elec 必须由你的编码器/霍尔/观测器模块实时更新。
     * 单位必须是 rad。
     */
    theta_elec = Current_WrapAngleRad(Motor_Electric_Angle);

    Park_Transform(current.I_alpha,
                   current.I_beta,
                   Motor_Electric_Angle,
                   &current.Id,
                   &current.Iq);

    /* 6. 如果电流环未使能，只采样和变换，不输出电压。 */
    if (!current_loop_enabled)
    {
        current.Ud = 0.0f;
        current.Uq = 0.0f;
        current.U_alpha = 0.0f;
        current.U_beta = 0.0f;

        pid_id.Output = 0.0f;
        pid_iq.Output = 0.0f;

        return;
    }

    /* 7. Id/Iq 电流 PI 控制，输出 Ud/Uq 电压指令。 */
    Current_PI_Controller();

    /* 8. d/q 电压矢量限幅。
     *
     * 这里没有把母线电压 Udc 作为输入，所以使用 CURRENT_VOLTAGE_LIMIT。
     * 如果你希望根据实时母线电压限幅，可以把函数接口扩展为带 Udc。
     */
    Voltage_Vector_Limit(&current.Ud,
                         &current.Uq,
                         CURRENT_VOLTAGE_LIMIT);

    /* 9. 反 Park 变换：d/q 电压 -> alpha/beta 电压。
     *
     * 后续你可以把 current.U_alpha / current.U_beta 送入 SVPWM。
     */
    Inv_Park_Transform(current.Ud,
                       current.Uq,
                       theta_elec,
                       &current.U_alpha,
                       &current.U_beta);
}


/* ============================================================
 * 12. 参数设置接口
 * ============================================================ */

void Set_Id_Reference(float id_ref)
{
    target_id = id_ref;
    pid_id.SetPoint = id_ref;
}


void Set_Iq_Reference(float iq_ref)
{
    target_iq = iq_ref;
    pid_iq.SetPoint = iq_ref;
}


void Set_Current_PID_Params(float kp_d,
                            float ki_d,
                            float kd_d,
                            float kp_q,
                            float ki_q,
                            float kd_q)
{
    pid_id.Proportion = kp_d;
    pid_id.Integral = ki_d;
    pid_id.Derivative = kd_d;

    pid_iq.Proportion = kp_q;
    pid_iq.Integral = ki_q;
    pid_iq.Derivative = kd_q;

    /* 改 PI 参数后，建议清空积分，避免旧积分造成突然冲击。 */
    PID_Current_Reset(&pid_id);
    PID_Current_Reset(&pid_iq);
}


void PMSM_Set_Motor_Params(float R,
                           float Ld,
                           float Lq,
                           float Psi_f,
                           uint8_t PolePairs)
{
    pmsm.R = R;
    pmsm.Ld = Ld;
    pmsm.Lq = Lq;
    pmsm.Psi_f = Psi_f;
    pmsm.PolePairs = PolePairs;

    /* 对旋转电机：
     *   Kt = 1.5 * p * Psi_f
     *
     * 如果你是直线电机，推力常数 Kt 的定义可能不同，
     * 可以在外部单独维护推力常数。
     */
    pmsm.Kt = 1.5f * (float)pmsm.PolePairs * pmsm.Psi_f;
}


float PMSM_Get_Electrical_Angle(float mech_angle)
{
    float elec_angle;

    theta_mech = mech_angle;

    elec_angle = mech_angle * (float)pmsm.PolePairs;
    elec_angle = Current_WrapAngleRad(elec_angle);

    return elec_angle;
}


void PMSM_Current_Control_Enable(bool enable)
{
    current_loop_enabled = enable;

    /* 使能切换时清空积分，防止重新使能瞬间有残余输出。 */
    PID_Current_Reset(&pid_id);
    PID_Current_Reset(&pid_iq);

    if (!enable)
    {
        current.Ud = 0.0f;
        current.Uq = 0.0f;
        current.U_alpha = 0.0f;
        current.U_beta = 0.0f;
    }
}


float PMSM_Calculate_Torque(float iq)
{
    /* 旋转 PMSM 电磁转矩：
     *   Te = 1.5 * p * Psi_f * Iq
     *
     * 如果是永磁同步直线电机，这个函数不能直接当推力公式使用。
     */
    return 1.5f * (float)pmsm.PolePairs * pmsm.Psi_f * iq;
}


void Current_Limit(float *id, float *iq, float max_current)
{
    float mag;
    float scale;

    if ((id == NULL) || (iq == NULL))
    {
        return;
    }

    if (max_current <= 0.0f)
    {
        *id = 0.0f;
        *iq = 0.0f;
        return;
    }

    mag = sqrtf((*id) * (*id) + (*iq) * (*iq));

    if (mag > max_current)
    {
        scale = max_current / mag;
        *id *= scale;
        *iq *= scale;
    }
}








