#include "key.h"

#include "../Closed_loop/speed_pi.h"
#include "driverlib.h"
#include "device.h"

#define KEY_STOP_GPIO         16U                         // 停止按键 GPIO，低电平表示按下
#define KEY_STEP_DOWN_GPIO    17U                         // 减速按键 GPIO，低电平表示按下
#define KEY_PRESET_GPIO       18U                         // 预置速度按键 GPIO，低电平表示按下
#define KEY_STEP_UP_GPIO      19U                         // 加速按键 GPIO，低电平表示按下

#define KEY_DEBOUNCE_US       30000U                      // 按键确认消抖时间
#define KEY_RELEASE_WAIT_US   1000U                       // 等待按键释放时的轮询间隔
#define KEY_SPEED_PRESET_RPS  10.0f                       // 预置目标速度，单位 r/s
#define KEY_SPEED_STEP_RPS    2.0f                        // 单次加减速步长，单位 r/s
#define KEY_SPEED_MIN_RPS    -20.0f                       // 允许的最小目标速度
#define KEY_SPEED_MAX_RPS     20.0f                       // 允许的最大目标速度

static float g_key_command_speed = 0.0f;                  // 按键和 IPC 共用的目标速度缓存

/* 1. 将速度限制在允许范围内 */
static float Key_ClampSpeed(float speed_rps)
{
    if(speed_rps < KEY_SPEED_MIN_RPS)                     // 判断是否低于反向速度下限
    {
        return KEY_SPEED_MIN_RPS;                         // 低于下限时返回下限值
    }

    if(speed_rps > KEY_SPEED_MAX_RPS)                     // 判断是否高于正向速度上限
    {
        return KEY_SPEED_MAX_RPS;                         // 高于上限时返回上限值
    }

    return speed_rps;                                     // 速度正常时直接返回
}

/* 2. 读取低电平有效按键 */
static bool Key_IsPressed(uint32_t pin)
{
    return (GPIO_readPin(pin) == 0U);                     // GPIO 为低电平表示按键已经按下
}

/* 3. 对一次按键按下进行消抖确认 */
static bool Key_CheckPressed(uint32_t pin)
{
    if(!Key_IsPressed(pin))                               // 第一次读取未按下则无需等待
    {
        return false;                                     // 返回未检测到有效按键
    }

    DEVICE_DELAY_US(KEY_DEBOUNCE_US);                     // 延时等待机械抖动结束
    return Key_IsPressed(pin);                            // 第二次读取确认按键状态
}

/* 4. 等待按键释放以避免长按重复触发 */
static void Key_WaitRelease(uint32_t pin)
{
    while(Key_IsPressed(pin))                             // 按键仍保持按下时持续等待
    {
        DEVICE_DELAY_US(KEY_RELEASE_WAIT_US);             // 以短延时降低轮询占用
    }
}

/* 5. 初始化四个按键 GPIO */
void Key_Init(void)
{
    GPIO_setDirectionMode(KEY_STOP_GPIO, GPIO_DIR_MODE_IN);      // 停止按键设为输入
    GPIO_setDirectionMode(KEY_STEP_DOWN_GPIO, GPIO_DIR_MODE_IN); // 减速按键设为输入
    GPIO_setDirectionMode(KEY_PRESET_GPIO, GPIO_DIR_MODE_IN);    // 预置按键设为输入
    GPIO_setDirectionMode(KEY_STEP_UP_GPIO, GPIO_DIR_MODE_IN);   // 加速按键设为输入

    GPIO_setPadConfig(KEY_STOP_GPIO, GPIO_PIN_TYPE_PULLUP);      // 停止按键使用内部上拉
    GPIO_setPadConfig(KEY_STEP_DOWN_GPIO, GPIO_PIN_TYPE_PULLUP); // 减速按键使用内部上拉
    GPIO_setPadConfig(KEY_PRESET_GPIO, GPIO_PIN_TYPE_PULLUP);    // 预置按键使用内部上拉
    GPIO_setPadConfig(KEY_STEP_UP_GPIO, GPIO_PIN_TYPE_PULLUP);   // 加速按键使用内部上拉

    GPIO_setQualificationMode(KEY_STOP_GPIO, GPIO_QUAL_6SAMPLE);      // 停止按键使用六采样滤波
    GPIO_setQualificationMode(KEY_STEP_DOWN_GPIO, GPIO_QUAL_6SAMPLE); // 减速按键使用六采样滤波
    GPIO_setQualificationMode(KEY_PRESET_GPIO, GPIO_QUAL_6SAMPLE);    // 预置按键使用六采样滤波
    GPIO_setQualificationMode(KEY_STEP_UP_GPIO, GPIO_QUAL_6SAMPLE);   // 加速按键使用六采样滤波
}

/* 6. 在主循环中处理按键命令 */
void Key_Process(void)
{
    if(Key_CheckPressed(KEY_STOP_GPIO))                   // 检测停止按键
    {
        g_key_command_speed = 0.0f;                       // 将目标速度直接清零
        PMSM_PID_SetTargetSpeed(g_key_command_speed);     // 将停止目标写入速度 PI
        Key_WaitRelease(KEY_STOP_GPIO);                   // 等待停止按键释放
    }
    else if(Key_CheckPressed(KEY_STEP_DOWN_GPIO))         // 检测减速按键
    {
        g_key_command_speed = Key_ClampSpeed(g_key_command_speed - KEY_SPEED_STEP_RPS); // 按步长降低并限幅速度
        PMSM_PID_SetTargetSpeed(g_key_command_speed);     // 将新目标写入速度 PI
        Key_WaitRelease(KEY_STEP_DOWN_GPIO);              // 等待减速按键释放
    }
    else if(Key_CheckPressed(KEY_PRESET_GPIO))            // 检测预置速度按键
    {
        g_key_command_speed = KEY_SPEED_PRESET_RPS;       // 写入固定预置速度
        PMSM_PID_SetTargetSpeed(g_key_command_speed);     // 将预置速度写入速度 PI
        Key_WaitRelease(KEY_PRESET_GPIO);                 // 等待预置按键释放
    }
    else if(Key_CheckPressed(KEY_STEP_UP_GPIO))           // 检测加速按键
    {
        g_key_command_speed = Key_ClampSpeed(g_key_command_speed + KEY_SPEED_STEP_RPS); // 按步长增加并限幅速度
        PMSM_PID_SetTargetSpeed(g_key_command_speed);     // 将新目标写入速度 PI
        Key_WaitRelease(KEY_STEP_UP_GPIO);                // 等待加速按键释放
    }
}

/* 7. 读取当前目标速度 */
float Key_GetCommandSpeed(void)
{
    return g_key_command_speed;                           // 返回按键或 IPC 最近设置的目标速度
}

/* 8. 由其他模块设置目标速度 */
void Key_SetCommandSpeed(float speed_rps)
{
    g_key_command_speed = Key_ClampSpeed(speed_rps);      // 保存并限幅外部传入的目标速度
    PMSM_PID_SetTargetSpeed(g_key_command_speed);         // 同步更新速度 PI 的目标速度
}
