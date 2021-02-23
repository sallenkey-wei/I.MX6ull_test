#include <stdio.h>
#include "ekf.h"
#include <math.h>
#include "matrix2.h"

static float dt = SAMPLE_PERIOD * 0.001f;

void IMU_Update(float gx, float gy, float gz, float ax, float ay, float az){
	float norm;
	norm = sqrt(ax*ax + ay*ay + az*az);
	VEC	* Z;
	Z = v_get(3);
	/* 加速度读取及单位化 */
	Z->ve[0] = ax/norm;
	Z->ve[1] = ay/norm;
	Z->ve[2] = az/norm;
	PRINT_VEC(Z);

	MAT  *F, *F_t;
	/* 雅克比矩阵F */
	F = m_get(4, 4);
	F->me[0][0] = 1; 		  F->me[0][1] = -gx*dt*0.5f; F->me[0][2] = -gy*dt*0.5f; F->me[0][3] = -gz*dt*0.5f;
	F->me[1][0] = gx*dt*0.5f; F->me[1][1] = 1;           F->me[1][2] =  gz*dt*0.5f; F->me[1][3] = -gy*dt*0.5f; 
	F->me[2][0] = gy*dt*0.5f; F->me[2][1] = -gz*dt*0.5f; F->me[2][2] = 1;           F->me[2][3] =  gx*dt*0.5f;
	F->me[3][0] = gz*dt*0.5f; F->me[3][1] =  gy*dt*0.5f; F->me[3][2] = -gx*dt*0.5f; F->me[3][3] = 1;

	PRINT_MAT(F);

	/* 雅克比矩阵F的转置F_t */
	F_t = m_transp(F, NULL);
	PRINT_MAT(F_t);

	

}
