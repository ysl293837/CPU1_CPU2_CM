#ifndef __CURRENT_LOOP_H
#define __CURRENT_LOOP_H

#ifdef __cplusplus
extern "C" {
#endif

/* ������ͷ�ļ� */

#include "math.h"
#include <stdbool.h>
#include "arm_math.h"   // STM32 DSP
/* ������PID�ṹ�� */
typedef struct
{
    float SetPoint;     // Ŀ�����
    float Proportion;   // ����ϵ�� Kp
    float Integral;      // ����ϵ�� Ki
    float Derivative;   // ΢��ϵ�� Kd
    float LastError;    // �ϴ����
    float SumError;     // ������
    float Output;       // PID���
} PID_Current_Typedef;

/* ���������ṹ�� */
typedef struct
{
    float Ia;           // A����� (A)
    float Ib;           // B����� (A)
    float Ic;           // C����� (A)
    float I_alpha;      // ������� (A)
    float I_beta;       // ������� (A)
    float Id;           // d����� (A) - ��������
    float Iq;           // q����� (A) - ת�ط���
    float Ud;           // d���ѹ (V)
    float Uq;           // q���ѹ (V)
    float U_alpha;      // �����ѹ (V)
    float U_beta;       // �����ѹ (V)
} Current_Data_Typedef;

/* ���������Ʋ����ṹ�� */
typedef struct
{
    float I_bus;        // ĸ�ߵ���
    float Kt;           // ת�س���
    float R;            // ���ӵ��� (��)
    float Ld;           // d���� (H)
    float Lq;           // q���� (H)
    float Psi_f;        // ��������� (Wb)
    uint8_t PolePairs;  // ������
} PMSM_Params_Typedef;

/* ȫ�ֱ������� */
extern PID_Current_Typedef pid_id;   // Id�������PID
extern PID_Current_Typedef pid_iq;   // Iq�������PID
extern Current_Data_Typedef current; // ��������
extern PMSM_Params_Typedef pmsm;     // PMSM����
extern float theta_elec;             // ��Ƕ� (rad)
extern float theta_mech;             // ��е�Ƕ� (rad)
extern float target_id;              // d��Ŀ����� (ͨ��Ϊ0)
extern float target_iq;              // q��Ŀ����� (���ٶȻ����)


/* �������� */
void Current_Loop_Init(void);
void PMSM_Current_PID_Init(void);
void PMSM_Current_PID_Update(void);
void Current_Sample_Update(uint16_t adc_a, uint16_t adc_b, uint16_t adc_c, float *ia, float *ib, float *ic);
void ADC_Current_Convert(uint16_t adc_a, uint16_t adc_b, uint16_t adc_c, float *ia, float *ib, float *ic);
void Current_SetAdcOffsetRaw(uint16_t offset_u, uint16_t offset_v, uint16_t offset_w);
void Current_ResetFilter(void);
void Current_GetAdcOffsetRaw(uint16_t *offset_u, uint16_t *offset_v, uint16_t *offset_w);
float Current_GetFilterAlpha(void);
float Current_GetSpikeLimitA(void);
void Clark_Transform(float ia, float ib, float ic, float *i_alpha, float *i_beta);
void Park_Transform(float i_alpha, float i_beta, float theta, float *id, float *iq);
void Inv_Park_Transform(float ud, float uq, float theta, float *u_alpha, float *u_beta);
//void SVPWM_Generate(float u_alpha, float u_beta, float Udc, uint16_t *ta, uint16_t *tb, uint16_t *tc);
void PMSM_Current_Control_Update(uint16_t adc_a, uint16_t adc_b, uint16_t adc_c,float ud ,float uq);
void Set_Id_Reference(float id_ref);
void Set_Iq_Reference(float iq_ref);
void Set_Current_PID_Params(float kp_d, float ki_d, float kd_d, float kp_q, float ki_q, float kd_q);
void PMSM_Set_Motor_Params(float R, float Ld, float Lq, float Psi_f, uint8_t PolePairs);
float PMSM_Get_Electrical_Angle(float mech_angle);
void PMSM_Current_Control_Enable(bool enable);
float PMSM_Calculate_Torque(float iq);
void Current_Limit(float *id, float *iq, float max_current);

#ifdef __cplusplus
}
#endif

#endif /* __CURRENT_LOOP_H */












