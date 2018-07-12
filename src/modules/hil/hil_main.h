/*
 * hil_main.h
 *
 *  Created on: Jun 19, 2018
 *      Author: lidq
 */

#ifndef SRC_MODULES_HIL_HIL_MAIN_H_
#define SRC_MODULES_HIL_HIL_MAIN_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <sys/time.h>
#include <px4_tasks.h>
#include <drivers/drv_hrt.h>
#include <systemlib/mavlink_log.h>
#include <lib/geo/geo.h>
#include <uORB/uORB.h>
#include <uORB/topics/actuator_controls.h>
#include <uORB/topics/sensor_combined.h>
#include <uORB/topics/actuator_armed.h>
#include <uORB/topics/vehicle_attitude.h>
#include <uORB/topics/vehicle_gps_position.h>
#include <uORB/topics/vehicle_global_position.h>
#include <uORB/topics/vehicle_local_position.h>

#include "matrix.h"

#ifdef __PX4_POSIX
#define CONFIG_PTHREAD_STACK_DEFAULT	(2048)
#endif

#define MAX_ANGLE_RATE		(90.0 * M_PI / 180.0)	//角速度限幅 弧度/s^2
#define MAX_ANGLE_RATE_Z	(5.0 * M_PI / 180.0)	//角速度限幅 弧度/s^2
#define MAX_ANGLE		(45.0 * M_PI / 180.0)	//角度限幅 弧度/s
#define MAX_ACC_BODY	(90)					//机体加速度限幅 m/s^2
#define MAX_VEL_BODY_XY	(10)					//机体速度限幅水平方向 m/s
#define MAX_VEL_BODY_Z	(10)

#define MAX_F	(62.0)

#define MAX_ACC_GLABOL		(15)
#define MAX_ACC_GLABOL_Z	(15)//机体加速度限幅 m/s^2
#define MAX_VEL_GLABOL_XY	(15)					//机体速度限幅水平方向 m/s
#define MAX_VEL_GLABOL_Z	(15)

//机体速度限幅垂直方向 m/s
#define MIN_MID_ZERO	(0.001)					//中位归0限幅
#define ACC_MID			(0.50)					//控制量升力中位值(0.0~1.0)

#define Kv_x			(3.0)					//旋转角加速度系数x
#define Kv_y			(3.0)					//旋转角加速度系数y
#define Kv_z			(0.02)					//旋转角加速度系数z

#define Aair			(3)						//空气阻尼加速度xy

#define Kacc_x			(65.0)					//机体加速度系数x
#define Kacc_y			(65.0)					//机体加速度系数y
#define Kacc_z			(65.0)					//机体加速度系数z

#define M 3
#define N 3
#define AT(i, j, n) (i * n + j)

struct quat
{
	float w;
	float x;
	float y;
	float z;
};

static void TransMatrix_R_vb_set_value(s_Matrix *R_vb, double angle_x, double angle_y, double angle_z);

static void TransMatrix_R_Q_set_value(s_Matrix *R_vb, double w, double x, double y, double z);

static void AngularVel_body_from_omega(double *omega, double *a0, double *a1, double *a2);

static void F_body_from_omega(double *omega_val, double *f_body_x, double *f_body_y, double *f_body_z);

static void Acc_global_from_F(double *f_global, double *acc_x, double *acc_y, double *acc_z);

static void hil_init(void);

static void hil_zero(void);

static void hil_maxmin(double *val, double max, double min);

static void hil_cal(double theta_t);

static void hil_angle2q(double *xyz, struct quat *q);

static void hil_pub_att(void);

static void hil_pub_local_pos(void);

static void hil_pub_global_pos(void);

static void hil_pub_gps(void);

static int hil_task_main(int argc, char *argv[]);

static int start(int argc, char *argv[]);

int hil_main(int argc, char *argv[]);

#endif /* SRC_MODULES_HIL_HIL_MAIN_H_ */
