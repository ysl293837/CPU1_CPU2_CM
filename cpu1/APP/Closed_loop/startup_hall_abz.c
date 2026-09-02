#include "startup_hall_abz.h"



#include "foc_svpwm.h"
#include "usart.h"
#include <stdint.h>
#include "stdio.h"

/* =========================================================
 *                     �ⲿ����
 * ========================================================= */
extern TIM_HandleTypeDef htim4;
extern int32_t OverflowCount;

/* =========================================================
 *                     �û�����
 * ========================================================= */
/* ������ÿ��еȦ�ܼ��������� 2500��AB�ı�Ƶ����10000 */
#define ENC_CPR         10000.0f

/* ����������ǰ�湤������ 5 */


/* 2�� */
#define TWO_PI          6.28318530718f

/* �ǶȻ��� */
#define DEG_TO_RAD(x)   ((x) * 0.01745329252f)

/* �������ڣ�������û����ǰ����һ����С�ĵ���ٶ�ǰ��
 * ���ֵ��Ҫ̫�󣬷��򷴶���ƽ��
 */
#define STARTUP_ELEC_SPEED   (0.5f * 3.1415926f)   /* 1���������/�� */

/* �����׶ε� q ���޷�������ƽ̨��� */
#define STARTUP_UQ_LIMIT     1000.0f

/* =========================================================
 *                     ����״̬
 * ========================================================= */
typedef enum
{
    STARTUP_HALL_ONLY = 0,   /* ֻ�л����ֽ� */
    STARTUP_HALL_AB,         /* �������� + AB��ֵ */
    STARTUP_ABZ_READY        /* Z �Ѳ���������� */
} StartupMode_t;

/* =========================================================
 *                     �������ƽṹ��
 * ========================================================= */
typedef struct
{
    StartupMode_t mode;

    /* ��ǰ����״̬����һ�λ���״̬ */
    uint8_t hall_now;
    uint8_t hall_last;

    /* ��ǰ������ */
    float elec_angle;

    /* ���һ�λ������ض�Ӧ�ľ��Ե�� */
    float hall_edge_angle;

    /* ���һ�λ�������ʱ�ı������ܼ��� */
    int32_t hall_edge_count;

    /* ���� ABZ ƫ�� */
    float encoder_offset;

    /* �Ƿ��Ѿ����� Z */
    uint8_t z_ready;
} StartupCtrl_t;

/* ȫ���������� */
static StartupCtrl_t g_startup;

/* =========================================================
 *                     �ڲ���������
 * ========================================================= */

/* �Ƕ����Ƶ� 0~2�� */
static float wrap_0_2pi(float x)
{
    while (x >= TWO_PI) x -= TWO_PI;
    while (x < 0.0f)    x += TWO_PI;
    return x;
}

/* ��ȡ�������ܼ�����16λTIM���� + ����ۼ� */
static int32_t get_encoder_total_count(void)
{
    return (OverflowCount * 65536) + (int32_t)__HAL_TIM_GET_COUNTER(&htim4);
}

/* ������������ -> ��ǶȲ� */
static float count_to_elec_angle(int32_t delta_cnt)
{
    return ((float)delta_cnt / ENC_CPR) * TWO_PI * POLE_PAIRS;
}

/* ��ȡ����ABC״̬
 *
 * ע�⣺
 * ������Ȼ���������� HALLU / HALLV / HALLW��
 * �����Ѿ���ȷ˵�ˣ�
 *   A ��Ӧ U��
 *   B ��Ӧ V��
 *   C ��Ӧ W��
 *
 * ���������߼��Ͼ��ǣ�
 *   bit2 = A
 *   bit1 = B
 *   bit0 = C
 */
static uint8_t read_hall_abc(void)
{
    uint8_t A = HAL_GPIO_ReadPin(HALLU_GPIO_Port, HALLU_Pin) ? 1 : 0;
    uint8_t B = HAL_GPIO_ReadPin(HALLV_GPIO_Port, HALLV_Pin) ? 1 : 0;
    uint8_t C = HAL_GPIO_ReadPin(HALLW_GPIO_Port, HALLW_Pin) ? 1 : 0;

    return (uint8_t)((A << 2) | (B << 1) | C);
}

/* ���ݻ���״̬�����ظ����������ĵ��
 *
 * �����ϸ���ͼƬ��Ŀռ��ϵ��
 *
 *   ״̬   λ��    ���Ľ�
 *   001    ��      270��
 *   011    ����    330��
 *   010    ����    30��
 *   110    ��      90��
 *   100    ����    150��
 *   101    ����    210��
 *
 * ������ӷ�����ʱ�� CCW
 *
 * ����ǵĲο���׼�ǣ�
 * ��A�ᣨҲ����U���ᣩ��Ϊ�ռ�ο���
 */
static float hall_to_center_angle(uint8_t hall)
{
    switch (hall)
    {
        case 0x1: return DEG_TO_RAD(150.0f);  /* 001 */
        case 0x3: return DEG_TO_RAD(210.0f);  /* 011 */
        case 0x2: return DEG_TO_RAD(270.0f);   /* 010 */
        case 0x6: return DEG_TO_RAD(330.0f);   /* 110 */
        case 0x4: return DEG_TO_RAD(30.0f);  /* 100 */
        case 0x5: return DEG_TO_RAD(90.0f);  /* 101 */
        default:  return 0.0f;                /* �Ƿ�״̬ */
    }
}

/* ���ݡ��½���Ļ���״̬���������û������صľ��Ե��
 *
 * ԭ��
 * һ������״̬����60���Ƿ�Χ
 * �������Ľ� ��30�� ���Ǳ߽�
 *
 * ���ﰴ CCW �������壺
 * ������һ��������ʱ������Ӧ�������������ǰ�߽�
 * ���ԣ�
 *   ���ؽ� = �������Ľ� - 30��
 */
static float hall_to_edge_angle(uint8_t hall)
{
    float center = hall_to_center_angle(hall);
//	printf("center - DEG_TO_RAD:%.2f\r\n",  center - DEG_TO_RAD(30.0f));
	
    return wrap_0_2pi(center - DEG_TO_RAD(30.0f));
}

/* =========================================================
 *                     ����ӿ�
 * ========================================================= */

/* ������ʼ��
 *
 * ���ܣ�
 * 1. ��ȡ��ǰ����״̬
 * 2. �õ�ǰ�����������Ľǣ���Ϊ��ʼ�ֵ��
 * 3. ��ʱ����d��ǿ����λ�����Բ����ȶ�һ��
 */
void Startup_HallABZ_Init(void)
{
    g_startup.mode = STARTUP_HALL_ONLY;

    g_startup.hall_now  = read_hall_abc();
    g_startup.hall_last = g_startup.hall_now;

    g_startup.elec_angle = hall_to_center_angle(g_startup.hall_now);

    g_startup.hall_edge_angle = 0.0f;
    g_startup.hall_edge_count = get_encoder_total_count();

    g_startup.encoder_offset = 0.0f;
    g_startup.z_ready = 0;
}

/* �����׶νǶȸ���
 *
 * ���飺ÿ��PWM���ڶ�����һ��
 *
 * ���������
 *
 * 1) ��û������������
 *    �á������������Ľ� + С��ǰ�ơ���ƽ����
 *
 * 2) һ����������
 *    ��¼�������صľ��Ե��
 *    ͬʱ��¼�˿̱���������
 *
 * 3) Ȼ���ã�
 *      ��ǰ��� = �������ؽ� + ���������λ�ƻ����
 *
 * 4) ���Z�Ѿ�����
 *    ��ֱ����ABZ���սǶ�
 */
void Startup_HallABZ_Update(void)
{
    uint8_t hall_new;
    int32_t enc_now;
    float delta_angle;

    hall_new = read_hall_abc();
	
//printf("hall_new:%d\r\n",  hall_new);
    enc_now  = get_encoder_total_count();

    /* ���1��Z�Ѿ�������ֱ��������ABZ */
    if (g_startup.mode == STARTUP_ABZ_READY)
    {
        g_startup.elec_angle =
            wrap_0_2pi(count_to_elec_angle(enc_now) + g_startup.encoder_offset);
        return;
    }

    /* -----------------------------
     * �׶�A����û������һ����������
     * ֻ������׶β�ȥ��顰��һ����Ч�������ء�
     * һ��ץ������ִֻ��һ�Σ�Ȼ���е� STARTUP_HALL_AB
     * ----------------------------- */
    if (g_startup.mode == STARTUP_HALL_ONLY)
    {
        /* ���жϵ�ǰ����״̬�ǲ��ǺϷ�״̬ */
        if ((hall_new == 0x1) || (hall_new == 0x3) || (hall_new == 0x2) ||
            (hall_new == 0x6) || (hall_new == 0x4) || (hall_new == 0x5))
        {
            /* �������һ�β�ͬ��˵�������˵�һ���������� */
            if (hall_new != g_startup.hall_last)
            {
                g_startup.hall_last = hall_new;
                g_startup.hall_now  = hall_new;

                /* ���桰��һ���������ء��ľ��Ե�� */
                g_startup.hall_edge_angle = hall_to_edge_angle(hall_new);
//							printf("hall_edge_angle:%.2f\r\n",  g_startup.hall_edge_angle/0.01745329252f);

                /* ������һ�̵ı��������� */
                g_startup.hall_edge_count = enc_now;

                /* �������+AB��ֵ�׶�
                 * ע�⣺�����Ͳ����ظ�ץ������������
                 */
                g_startup.mode = STARTUP_HALL_AB;
            }
        }

        /* �����ûץ����һ���������أ��ͼ����û����ֽ� + Сǰ�� */
        if (g_startup.mode == STARTUP_HALL_ONLY)
        {
            g_startup.elec_angle += STARTUP_ELEC_SPEED * (1.0f / PWM_FREQUENCY);
            g_startup.elec_angle = wrap_0_2pi(g_startup.elec_angle);
        }

        return;
    }
    

    /* -----------------------------
     * �׶�B���Ѿ�ץ����һ����������
     * ���治�ٴ����������
     * ֻ�á���һ���������ؽ� + ������λ�ơ�������ֵ
     * �������ÿ�����ڶ�ִ�У�����ִֻ��һ��
     * ----------------------------- */
    if (g_startup.mode == STARTUP_HALL_AB)
    {
        delta_angle = count_to_elec_angle(enc_now - g_startup.hall_edge_count);
        g_startup.elec_angle = wrap_0_2pi(g_startup.hall_edge_angle + delta_angle);
//			printf("elec_angle:%.2f\r\n",  g_startup.elec_angle*360);
			
        return;
    }
}

/* ��ȡ��ǰ���������� */
float Startup_HallABZ_GetElectricalAngle(void)
{
    return g_startup.elec_angle;
}

/* �����׶ζ� q ���޷�
 *
 * û�õ�����Zƫ��ǰ����Ҫ������̫��
 */
float Startup_HallABZ_LimitUq(float uq_in)
{
    if (g_startup.mode != STARTUP_HALL_AB)
    {
//        if (uq_in > STARTUP_UQ_LIMIT)  uq_in = STARTUP_UQ_LIMIT;
//        if (uq_in < -STARTUP_UQ_LIMIT) uq_in = -STARTUP_UQ_LIMIT;
			uq_in = 3000.0;
    }
    return uq_in;
}

/* Z�����嵽��ʱ����
 *
 * ԭ��
 * ��ʱ�����Ѿ���һ���Ƚ�׼ȷ�ġ���ǰ��ǹ���ֵ��
 * ��֪����ǰ��������е���ο���Zʱ�̵ı�����������
 * ���Կ���������չ̶�ƫ�ã�
 *
 *   encoder_offset = ��ǰ������ - ��ǰ������������
 *
 * �Ժ������ȫ��ABZ����
 */
void Startup_HallABZ_OnZPulse(void)
{
    int32_t enc_now;
    float enc_angle;

    if (g_startup.z_ready)
    return;
    enc_now = get_encoder_total_count();
    enc_angle = count_to_elec_angle(enc_now);

    g_startup.encoder_offset = wrap_0_2pi(g_startup.elec_angle - enc_angle);

    g_startup.z_ready = 1;
    g_startup.mode = STARTUP_ABZ_READY;
}

/* �ж��Ƿ��Ѿ��������ABZ���� */
uint8_t Startup_HallABZ_IsReady(void)
{
    return (g_startup.mode == STARTUP_ABZ_READY) ? 1 : 0;
}


float Startup_GetUqCmd(float uq_pid)
{
    /* ֻ�и��ϵ硢��û������һ����������ʱ��������ʱ�޷� */
    if (g_startup.mode == STARTUP_HALL_ONLY)
    {
        if (uq_pid > 3000.0f)  uq_pid = 3000.0f;
        if (uq_pid < -3000.0f) uq_pid = -3000.0f;
    }

    /* һ������ HALL_AB �� ABZ_READY������ȫ�ſ� */
    return uq_pid;
}


//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//    if (GPIO_Pin == ENCZ_Pin)
//    {
//        Startup_HallABZ_OnZPulse();
//    }
//}






