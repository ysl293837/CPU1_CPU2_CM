
#include "Encoder.h"
#include "TFTLCD.h"
#include "tim.h"
extern int32_t  TIM2_OVFLOW_COUNT;
// 全局变量定义

float s_speed_r_per_sec = 0.0f;   				// 当前转速
uint64_t diff_ticks = 0;
uint16_t call_count = 0;          			  // 调用次数计数器-->
int32_t saved_encoder_count = 0;   			  // 保存的编码器计数值-->上一次的计数值
int32_t current_count = 0;      					// 当前编码器计数值-->这一次的计数值
int32_t pulse_diff = 0;         					// 脉冲变化量			-->变化量
uint64_t saved_timestamp = 0;    			    // 保存的时间戳		-->上一次的时间值
uint64_t current_timestamp = 0; 					// 当前时间戳			-->这一次的时间值
float time_diff = 0.0;          				  // 时间差					-->秒
int32_t time_diff_int = 0.0;							// 时间差					-->微妙

uint64_t timestamp = 0;													//-->这一次的时间值
/**
  * @brief 获取高精度定时器值（基于TIM2）
  * @return 定时器计数值（32位主计数器 + 32位溢出计数）
  * @note 返回64位的时间戳
  */
uint64_t Get_Timer2_TimeStamp(void)
{
    uint32_t count = 0;
    uint32_t overflow = 0;
    
    // 读取当前计数值
    count = __HAL_TIM_GET_COUNTER(&htim2);
    overflow = TIM2_OVFLOW_COUNT;
    
    // 组合为64位时间戳
    timestamp = ((uint64_t)overflow << 32) | count;
    
    return timestamp;
}

/**
  * @brief 计算时间差（单位：秒）
  * @param start 开始时间戳
  * @param end 结束时间戳
  * @return 时间差（秒）
  */
float Calculate_Time_Diff(uint64_t start, uint64_t end)
{

    float time_sec = 0.0f;
    
    // 计算时钟滴答差
    if (end >= start)
    {
        diff_ticks = end - start;	
    }
    else
    {
        // 处理回绕
//        diff_ticks = (UINT64_MAX - start) + end;
    }
    
    // TIM2时钟频率 = APB1时钟 * 2 = 84MHz
    // 转换为秒
    time_sec = (float)diff_ticks / 240000000.0f;  // 84MHz
    
    return time_sec;
}

/**
  * @brief 计算电机转速（使用TIM2定时器，更高精度）
  * @return 当前转速（r/s）
  * @note 每10次中断计算一次速度
  */
float Encoder_CalculateSpeed(void)
{
		
    // 每次调用都增加计数器
    call_count++;
    
    // 只有每SPEED_CALC_INTERVAL次才计算一次
    if (call_count >= SPEED_CALC_INTERVAL)
    {
        // 获取当前编码器计数值和时间戳
        current_count = Encoder_GetCounting();
        current_timestamp = Get_Timer2_TimeStamp();
        
        // 计算脉冲变化量
        pulse_diff = current_count - saved_encoder_count;
        
        // 计算时间间隔（秒）
        time_diff = Calculate_Time_Diff(saved_timestamp, current_timestamp);
				

			
        
        // 更新保存的计数值和时间戳
        saved_encoder_count = current_count;
        saved_timestamp = current_timestamp;
        
        // 重置计数器
        call_count = 0;
        
        // 计算转速（r/s）
				s_speed_r_per_sec = (float)pulse_diff / ENCODER_PULSES_PER_REV / time_diff;

				
				// 保存时间差用于显示
				time_diff_int = (uint32_t)(time_diff * 1000000.0f);  // 转换为微秒
        

    }
    
    return s_speed_r_per_sec;
}

/**
  * @brief 获取当前电机转速
  * @return 当前转速（r/s）
  */
float Encoder_GetSpeed(void)
{
    return s_speed_r_per_sec;
}

/**
  * @brief 保存定时器2当前时间
  */
void Save_current_T2(void)
{
    saved_encoder_count = 0;
    s_speed_r_per_sec = 0.0f;
    saved_timestamp = Get_Timer2_TimeStamp();
}




