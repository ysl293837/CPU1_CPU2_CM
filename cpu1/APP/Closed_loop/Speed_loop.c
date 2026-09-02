#include "Encoder.h"
#include "speed_loop.h"

// 私有变量
PID_Typedef speedPID;              // 速度环PID结构体
static float current_speed = 0.0f;        // 当前速度 (r/s)
static uint8_t pid_update_count = 0;      // PID更新计数器
static float filtered_speed = 0.0f;       // 滤波后的速度
static bool pid_enabled = false;          // PID使能状态
static float target_speed = 0.0f;         // 目标速度

// 私有宏定义
#define PID_UPDATE_INTERVAL    SPEED_CALC_INTERVAL         // PID更新间隔 (中断次数)
#define SPEED_FILTER_ALPHA     1.0f       // 速度滤波系数 (0-1)
#define PID_INTEGRAL_LIMIT     50000.0f     // 积分限幅

// 私有函数声明
static float ApplyLimit(float value, float min, float max);
static float FirstOrderFilter(float input, float last_output, float alpha);

/**
  * @brief  PMSM PID控制器初始化
  */
void PMSM_PID_Init(void)
{
  // 初始化PID参数
  speedPID.SetPoint = 0.0f;           // 目标速度
  speedPID.Proportion = 1.0f;         // Kp系数
  speedPID.Integral = 0.1f;           // Ki系数
  speedPID.Derivative = 0.0f;         // Kd系数
  speedPID.LastError = 0.0f;
  speedPID.SumError = 0.0f;
  speedPID.Output = 0.0f;
  
  // 初始化其他变量
  pid_update_count = 0;
  filtered_speed = 0.0f;
  pid_enabled = false;
  target_speed = 0.0f;
  current_speed = 0.0f;
}

/**
  * @brief  位置式PID速度环计算
  * @param  current_speed: 当前速度 (r/s)
  * @param  target_speed: 目标速度 (r/s)
  * @retval PID输出值
  */
static float PID_Calculate(float current_speed, float target_speed)
{
  float error = 0.0f;
  float output = 0.0f;
  
  // 计算误差
  error = target_speed - current_speed;
  
  // PID计算
  // 比例项
  float p_term = speedPID.Proportion * error;
  
  // 积分项（带积分限幅）
  speedPID.SumError += error;
  speedPID.SumError = ApplyLimit(speedPID.SumError, -PID_INTEGRAL_LIMIT, PID_INTEGRAL_LIMIT);
  float i_term = speedPID.Integral * speedPID.SumError;
  
  // 微分项
  float d_term = speedPID.Derivative * (error - speedPID.LastError);
  
  // 计算PID输出
	output = ApplyLimit(p_term + i_term + d_term, -32768, 32768);
  
  // 保存误差历史
  speedPID.LastError = error;
  speedPID.Output = output;
  
  return output;
}

/**
  * @brief  PID更新函数（需要在定时中断中调用）
  */
void PMSM_PID_Update(void)
{
  // 检查PID是否使能
  if (!pid_enabled)
  {
    return;
  }
  
  // 更新计数器
  pid_update_count++;
  
  // 每10次中断更新一次PID
  if (pid_update_count >= PID_UPDATE_INTERVAL)
  {
    // 获取当前速度
    float raw_speed = Encoder_CalculateSpeed();
    
    // 应用一阶低通滤波
    // Y(n) = α * X(n) + (1-α) * Y(n-1)
    filtered_speed = FirstOrderFilter(raw_speed, filtered_speed, SPEED_FILTER_ALPHA);
    current_speed = filtered_speed;
    
    // 计算PID输出
    float pid_output = PID_Calculate(current_speed, target_speed);
    speedPID.Output = pid_output;
    
    // 重置计数器
    pid_update_count = 0;
  }
}

/**
  * @brief  设置目标速度
  * @param  target_speed_rps: 目标速度 (r/s)
  */
void PMSM_PID_SetTargetSpeed(float target_speed_rps)
{
  target_speed = target_speed_rps;
  speedPID.SetPoint = target_speed_rps;
}

/**
  * @brief  获取目标速度
  * @retval 目标速度 (r/s)
  */
float PMSM_PID_GetTargetSpeed(void)
{
  return target_speed;
}

/**
  * @brief  启用/禁用PID控制
  * @param  enable: true=启用, false=禁用
  */
void PMSM_PID_Enable(bool enable)
{
  pid_enabled = enable;
  
  if (!enable)
  {
    // 禁用PID时，重置PID状态
    PMSM_PID_Reset();
  }
}

/**
  * @brief  获取当前PID输出
  * @retval PID输出值
  */
float PMSM_PID_GetOutput(void)
{
  return speedPID.Output;
}

/**
  * @brief  重置PID控制器状态
  */
void PMSM_PID_Reset(void)
{
  speedPID.LastError = 0.0f;
  speedPID.SumError = 0.0f;
  speedPID.Output = 0.0f;
  
  current_speed = 0.0f;
  filtered_speed = 0.0f;
  pid_update_count = 0;
}

/**
  * @brief  设置PID参数
  * @param  kp: 比例系数
  * @param  ki: 积分系数
  * @param  kd: 微分系数
  */
void PMSM_SetPIDParams(float kp, float ki, float kd)
{
  speedPID.Proportion = kp;
  speedPID.Integral = ki;
  speedPID.Derivative = kd;
}

/**
  * @brief  获取当前PID参数
  */
float PMSM_PID_GetKp(void) { return speedPID.Proportion; }
float PMSM_PID_GetKi(void) { return speedPID.Integral; }
float PMSM_PID_GetKd(void) { return speedPID.Derivative; }

/**
  * @brief  获取当前速度
  * @retval 当前速度 (r/s)
  */
float PMSM_GetCurrentSpeed(void)
{
  return current_speed;
}

/**
  * @brief  应用限幅
  */
static float ApplyLimit(float value, float min, float max)
{
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

/**
  * @brief  一阶低通滤波
  */
static float FirstOrderFilter(float input, float last_output, float alpha)
{
  return (alpha * input) + ((1.0f - alpha) * last_output);
}







