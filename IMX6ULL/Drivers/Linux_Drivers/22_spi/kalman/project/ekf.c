#include <stdio.h>
#include "ekf.h"
#include <math.h>
#include "matrix2.h"

static float dt = SAMPLE_PERIOD * 0.001f;
/* 观测值，这里是重力加速度 */
static VEC * Z = NULL;
static VEC * Z_p = NULL;
static VEC * Z_e = NULL;
/* 状态转移方程的雅可比矩阵 */
static MAT * F = NULL;
static MAT * F_t = NULL;
/* 隐状态，这里是四元数 */
static VEC * q = NULL;
static VEC * q_p = NULL;
static VEC * q_e = NULL;

/* 观测方程的雅可比矩阵 */
static MAT * H = NULL;
static MAT * H_t = NULL;

/* 角速度计观测噪声协方差矩阵 */
static MAT * Q = NULL;
/* 加速度计观测噪声协方差矩阵 */
static MAT * R = NULL;

/* 观测值协方差矩阵 */
static MAT * P = NULL;
static MAT * P_tmp1 = NULL;
static MAT * P_tmp2 = NULL;
static MAT * P_p = NULL;

static MAT * K = NULL;
static MAT * K_tmp1 = NULL;
static MAT * K_tmp2 = NULL;
static MAT * K_tmp3 = NULL;

static MAT * I = NULL;




void IMU_init(){
	Z = v_get(3);
	Z_p = v_get(3);
	Z_e = v_get(3);
	
	F = m_get(4, 4);
	F_t = m_get(4, 4);
	
	q = v_get(4);
	q_p = v_get(4);
	q_e = v_get(4);
	q->ve[0] = 0;
	q->ve[1] = 0;
	q->ve[2] = 0;
	q->ve[3] = -1;
	
	H = m_get(3, 4);
	H_t = m_get(4, 3);

	Q = m_get(4, 4);
	R = m_get(3, 3);
	m_zero(Q);
	m_zero(R);
	Q->me[0][0] = 1e-6;
	Q->me[1][1] = 1e-6;
	Q->me[2][2] = 1e-6;
	Q->me[3][3] = 1e-6;

	/* 加速度计噪声大，所以协方差矩阵数值取大点 */
	R->me[0][0] = 1e-1;
	R->me[1][1] = 1e-1;
	R->me[2][2] = 1e-1;

	P = m_get(4, 4);
	P_tmp1 = m_get(4, 4);
	P_tmp2 = m_get(4, 4);
	P_p = m_get(4, 4);
	m_zero(P);

	K = m_get(4, 3);
	K_tmp1 = m_get(4, 3);
	K_tmp2 = m_get(3, 4);
	K_tmp3 = m_get(3, 3);

	I = m_get(4, 4);
	m_zero(I);
	I->me[0][0] = 1;
	I->me[1][1] = 1;
	I->me[2][2] = 1;
	I->me[3][3] = 1;
}

void IMU_Update(float gx, float gy, float gz, float ax, float ay, float az,
					float * roll, float * pitch, float * yaw){
	float norm;
	norm = sqrt(ax*ax + ay*ay + az*az);
	
	/* 加速度读取及单位化 */
	Z->ve[0] = ax/norm;
	Z->ve[1] = ay/norm;
	Z->ve[2] = az/norm;
	PRINT_VEC(Z);


	/* 雅克比矩阵F */	
	F->me[0][0] = 1; 		  F->me[0][1] = -gx*dt*0.5f; F->me[0][2] = -gy*dt*0.5f; F->me[0][3] = -gz*dt*0.5f;
	F->me[1][0] = gx*dt*0.5f; F->me[1][1] = 1;           F->me[1][2] =  gz*dt*0.5f; F->me[1][3] = -gy*dt*0.5f; 
	F->me[2][0] = gy*dt*0.5f; F->me[2][1] = -gz*dt*0.5f; F->me[2][2] = 1;           F->me[2][3] =  gx*dt*0.5f;
	F->me[3][0] = gz*dt*0.5f; F->me[3][1] =  gy*dt*0.5f; F->me[3][2] = -gx*dt*0.5f; F->me[3][3] = 1;
	PRINT_MAT(F);

	/* 状态转移方程的雅克比矩阵F的转置F_t */
	m_transp(F, F_t);
	PRINT_MAT(F_t);

	/* update */
	mv_mlt(F, q, q_p);
	/* predict四元数的单位化 */
	norm = v_norm2(q_p);
	q_p->ve[0] /= norm;
	q_p->ve[1] /= norm;
	q_p->ve[2] /= norm;
	q_p->ve[3] /= norm;
	PRINT_VEC(q_p);

	m_mlt(F, P, P_tmp1);
	m_mlt(P_tmp1, F_t, P_tmp2);
	m_add(P_tmp2, Q, P_p);/* 更新协方差矩阵P */
	PRINT_MAT(P_p);
	

	/* predict */
	Z_p->ve[0] = 2 * (q_p->ve[0]*q_p->ve[2] - q_p->ve[1]*q_p->ve[3]);
	Z_p->ve[1] = -2 * (q_p->ve[0]*q_p->ve[1] + q_p->ve[2]*q_p->ve[3]);
	Z_p->ve[2] = q_p->ve[1]*q_p->ve[1] + q_p->ve[2]*q_p->ve[2] - q_p->ve[0]*q_p->ve[0] - q_p->ve[3]*q_p->ve[3];
	norm = v_norm2(Z_p);
	Z_p->ve[0] /= norm;
	Z_p->ve[1] /= norm;
	Z_p->ve[2] /= norm;
	PRINT_VEC(Z_p);

	/* 观测方程的雅克比矩阵H */	
	H->me[0][0] =  2*q_p->ve[2]; H->me[0][1] = -2*q_p->ve[3]; H->me[0][2] =  2*q_p->ve[0]; H->me[0][3] = -2*q_p->ve[1];
	H->me[1][0] = -2*q_p->ve[1]; H->me[1][1] = -2*q_p->ve[0]; H->me[1][2] = -2*q_p->ve[3]; H->me[1][3] = -2*q_p->ve[2]; 
	H->me[2][0] = -2*q_p->ve[0]; H->me[2][1] =  2*q_p->ve[1]; H->me[2][2] =  2*q_p->ve[2]; H->me[2][3] = -2*q_p->ve[3];
	PRINT_MAT(H);
	m_transp(H, H_t);
	PRINT_MAT(H_t);

	m_mlt(P_p, H_t, K_tmp1);
	m_mlt(H, P_p, K_tmp2);
	m_mlt(K_tmp2, H_t, K_tmp3);
	m_add(K_tmp3, R, K_tmp3);
	m_inverse(K_tmp3, K_tmp3);
	m_mlt(K_tmp1, K_tmp3, K);
	PRINT_MAT(K);

	m_mlt(K, H, P_tmp1);
	m_sub(I, P_tmp1, P_tmp2);
	m_mlt(P_tmp2, P_p, P);
	PRINT_MAT(P);

	v_sub(Z, Z_p, Z_e);
	mv_mlt(K, Z_e, q_e);
	v_add(q_p, q_e, q);
	
	norm = v_norm2(q);
	q->ve[0] /= norm;
	q->ve[1] /= norm;
	q->ve[2] /= norm;
	q->ve[3] /= norm;
	PRINT_VEC(q);

	*roll = atan2(2*q->ve[2]*q->ve[3]+2*q->ve[0]*q->ve[1], -2*q->ve[1]*q->ve[1]-2*q->ve[2]*q->ve[2] + 1) * 57.3;
	*pitch = asin(-2*q->ve[1]*q->ve[3]+2*q->ve[0]*q->ve[2])*57.3;
	*yaw = atan2(2*q->ve[1]*q->ve[2]+2*q->ve[0]*q->ve[3], q->ve[0]*q->ve[0] + q->ve[1]*q->ve[1] - q->ve[2]*q->ve[2] - q->ve[3]*q->ve[3]) * 57.3;
}
