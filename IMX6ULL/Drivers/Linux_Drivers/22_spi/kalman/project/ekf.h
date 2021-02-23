#ifndef __EKF_H
#define __EKF_H

//#define _USE_MATH_DEFINES


/* 采样周期单位ms */
#define SAMPLE_PERIOD 		(10)
#define DEBUG				1
#if DEBUG
#define		PRINT_VEC(VEC)	_PRINT_VEC(VEC)
#define		PRINT_MAT(MAT)	_PRINT_MAT(MAT)

#define		_PRINT_VEC(VEC)	{printf("# %s = \n", #VEC);v_output(VEC);}
#define		_PRINT_MAT(MAT)	{printf("# %s = \n", #MAT);m_output(MAT);}

#else
#define		PRINT_VEC(VEC)  
#define		PRINT_MAT(MAT)	

#endif

void IMU_init();
void IMU_Update(float gx, float gy, float gz, float ax, float ay, float az, float * roll, float * pitch, float * yaw);



#endif
