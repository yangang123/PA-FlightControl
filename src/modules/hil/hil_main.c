/*
 * hil_main.c
 *
 *  Created on: Jun 19, 2018
 *      Author: lidq
 */

#include "hil_main.h"

//static orb_advert_t _mavlink_log_pub = NULL;

static orb_advert_t _att_pub = NULL;
static orb_advert_t _local_pos_pub = NULL;
static orb_advert_t _global_pos_pub = NULL;
static orb_advert_t _gps_pub = NULL;

static struct quat q_reset = { 0 };
static struct map_projection_reference_s _ref_pos;

static s_Matrix control;			//控制量
static s_Matrix output;				//混控输出
static s_Matrix omega;				//电机转数
static s_Matrix R_trans_matrix;		//变换矩阵

static s_Matrix F_body;				//机体系拉力
static s_Matrix F_global;			//惯性系拉力
static s_Matrix Acc_body;			//机体加速度

static s_Matrix Acc_global;			//惯性系速度
static s_Matrix Vel_global;			//惯性系速度
static s_Matrix Pos_global;			//惯性系位移

static s_Matrix AngularVel_body;	//机体角速度
static s_Matrix Angular_body;		//姿态角度

//欧拉角变换矩阵
void TransMatrix_R_vb_set_value(s_Matrix *R_vb, double angle_x, double angle_y, double angle_z)
{
	R_vb->v[AT(0, 0, R_vb->n)] = cos(angle_z) * cos(angle_y);
	R_vb->v[AT(0, 1, R_vb->n)] = sin(angle_z) * cos(angle_y);
	R_vb->v[AT(0, 2, R_vb->n)] = -sin(angle_y);

	R_vb->v[AT(1, 0, R_vb->n)] = -sin(angle_z) * cos(angle_x) + cos(angle_z) * sin(angle_y) * sin(angle_x);
	R_vb->v[AT(1, 1, R_vb->n)] = cos(angle_z) * cos(angle_x) + sin(angle_z) * sin(angle_y) * sin(angle_x);
	R_vb->v[AT(1, 2, R_vb->n)] = cos(angle_y) * sin(angle_x);

	R_vb->v[AT(2, 0, R_vb->n)] = sin(angle_z) * sin(angle_x) + cos(angle_z) * sin(angle_y) * cos(angle_x);
	R_vb->v[AT(2, 1, R_vb->n)] = -cos(angle_z) * sin(angle_x) + sin(angle_z) * sin(angle_y) * cos(angle_x);
	R_vb->v[AT(2, 2, R_vb->n)] = cos(angle_y) * cos(angle_x);
}

//四元数据变换矩阵
void TransMatrix_R_Q_set_value(s_Matrix *R_vb, double w, double x, double y, double z)
{
	R_vb->v[AT(0, 0, R_vb->n)] = 1.0 - 2.0 * y * y - 2.0 * z * z;
	R_vb->v[AT(0, 1, R_vb->n)] = 2.0 * x * y - 2.0 * w * z;
	R_vb->v[AT(0, 2, R_vb->n)] = 2.0 * x * z + 2.0 * w * y;

	R_vb->v[AT(1, 0, R_vb->n)] = 2.0 * x * y + 2.0 * w * z;
	R_vb->v[AT(1, 1, R_vb->n)] = 1.0 - 2.0 * x * x - 2.0 * z * z;
	R_vb->v[AT(1, 2, R_vb->n)] = 2.0 * y * z - 2.0 * w * x;

	R_vb->v[AT(2, 0, R_vb->n)] = 2.0 * x * z - 2.0 * w * y;
	R_vb->v[AT(2, 1, R_vb->n)] = 2.0 * y * z + 2.0 * w * x;
	R_vb->v[AT(2, 2, R_vb->n)] = 1.0 - 2.0 * x * x - 2.0 * y * y;
}

void F_body_from_omega(double *omega_val, double *f_body_x, double *f_body_y, double *f_body_z)
{
	*f_body_x = 0.0;
	*f_body_y = 0.0;
	*f_body_z = (omega_val[0] * omega_val[0] + omega_val[1] * omega_val[1] + omega_val[2] * omega_val[2] + omega_val[3] * omega_val[3]) * F_OMEGA;
}

void Acc_global_from_F(double *f_global, double *acc_x, double *acc_y, double *acc_z)
{
	*acc_x = f_global[0] / M_KG;
	*acc_y = f_global[1] / M_KG;
	*acc_z = f_global[2] / M_KG;
}

void hil_init(void)
{
	matrix_init(&control, 4, 1);
	matrix_init(&output, 4, 1);
	matrix_init(&omega, 4, 1);
	matrix_init(&R_trans_matrix, 3, 3);

	matrix_init(&F_body, 3, 1);
	matrix_init(&F_global, 3, 1);
	matrix_init(&Acc_body, 3, 1);

	matrix_init(&Acc_global, 3, 1);
	matrix_init(&Vel_global, 3, 1);
	matrix_init(&Pos_global, 3, 1);

	matrix_init(&AngularVel_body, 3, 1);
	matrix_init(&Angular_body, 3, 1);
}

void hil_zero(void)
{
	matrix_zero(&control);
	matrix_zero(&output);
	matrix_zero(&omega);
	matrix_zero(&R_trans_matrix);

	matrix_zero(&F_body);
	matrix_zero(&F_global);
	matrix_zero(&Acc_body);

	matrix_zero(&Acc_global);
	matrix_zero(&Vel_global);
	matrix_zero(&Pos_global);

	matrix_zero(&AngularVel_body);
	matrix_zero(&Angular_body);
}

void hil_maxmin(double *val, double max, double min)
{
	if (*val < min)
	{
		*val = min;
	}
	if (*val > max)
	{
		*val = max;
	}
}

void hil_cal(double theta_t)
{
#ifdef __PX4_POSIX
	//混控后的桨转数
	omega.v[0] = (output.v[0] + 1.0) / 2.0;
	omega.v[1] = (output.v[1] + 1.0) / 2.0;
	omega.v[2] = (output.v[2] + 1.0) / 2.0;
	omega.v[3] = (output.v[3] + 1.0) / 2.0;
#else
	//混控后的桨转数
	omega.v[0] = control.v[3];
	omega.v[1] = control.v[3];
	omega.v[2] = control.v[3];
	omega.v[3] = control.v[3];
#endif
	//根据转动惯量计算角速度
	AngularVel_body.v[0] = control.v[0] / I_X;
	AngularVel_body.v[1] = control.v[1] / I_Y;
	AngularVel_body.v[2] = control.v[2] / I_Z;

	//根据角速度积分得到角度
	Angular_body.v[0] += AngularVel_body.v[0] * theta_t;
	Angular_body.v[1] += AngularVel_body.v[1] * theta_t;
	Angular_body.v[2] += AngularVel_body.v[2] * theta_t;

	//角度限幅
	hil_maxmin(&Angular_body.v[0], MAX_ANGLE, -MAX_ANGLE);
	hil_maxmin(&Angular_body.v[1], MAX_ANGLE, -MAX_ANGLE);

	//根据欧拉角计算四元数
	struct quat q_value = { 0 };
	double angle[3];
	angle[0] = -Angular_body.v[0];
	angle[1] = -Angular_body.v[1];
	angle[2] = Angular_body.v[2];
	hil_angle2q(angle, &q_value);
	//设置四元数变换矩阵的值
	TransMatrix_R_Q_set_value(&R_trans_matrix, q_value.w, q_value.x, q_value.y, q_value.z);

	//计算机体系下的拉力
	F_body_from_omega(omega.v, &F_body.v[0], &F_body.v[1], &F_body.v[2]);

	//变换矩阵左乘机体系拉力得到惯性系下的拉力
	matrix_mult(&F_global, &R_trans_matrix, &F_body);

	//惯性系下减去垂直方向自身的重力
	F_global.v[2] -= M_KG * G_MS2;

	//根据惯性系下的拉力计算惯性系下的加速度
	Acc_global_from_F(F_global.v, &Acc_global.v[0], &Acc_global.v[1], &Acc_global.v[2]);

	//根据加速度积分得到速度
	Vel_global.v[0] += Acc_global.v[0] * theta_t;
	Vel_global.v[1] += Acc_global.v[1] * theta_t;
	Vel_global.v[2] += Acc_global.v[2] * theta_t;
	hil_maxmin(&Vel_global.v[0], MAX_VEL_XY, -MAX_VEL_XY);
	hil_maxmin(&Vel_global.v[1], MAX_VEL_XY, -MAX_VEL_XY);
	hil_maxmin(&Vel_global.v[2], MAX_VEL_Z, -MAX_VEL_Z);

	//根据速度积分得到位置
	Pos_global.v[0] += Vel_global.v[0] * theta_t;
	Pos_global.v[1] += Vel_global.v[1] * theta_t;
	Pos_global.v[2] += Vel_global.v[2] * theta_t;
}

void hil_angle2q(double *xyz, struct quat *q)
{
	double x = xyz[0] / 2;
	double y = xyz[1] / 2;
	double z = xyz[2] / 2;
	q->w = cos(x) * cos(y) * cos(z) + sin(x) * sin(y) * sin(z);
	q->x = sin(x) * cos(y) * cos(z) - cos(x) * sin(y) * sin(z);
	q->y = cos(x) * sin(y) * cos(z) + sin(x) * cos(y) * sin(z);
	q->z = cos(x) * cos(y) * sin(z) - sin(x) * sin(y) * cos(z);
}

void hil_pub_att(void)
{
	struct quat q_value = { 0 };
	double angle[3];
	angle[0] = Angular_body.v[0];
	angle[1] = Angular_body.v[1];
	angle[2] = Angular_body.v[2];
	hil_angle2q(angle, &q_value);

	struct vehicle_attitude_s s_att_hil;
	s_att_hil.timestamp = hrt_absolute_time();
	s_att_hil.rollspeed = AngularVel_body.v[0];
	s_att_hil.pitchspeed = AngularVel_body.v[1];
	s_att_hil.yawspeed = AngularVel_body.v[2];

	s_att_hil.q[0] = q_value.w;
	s_att_hil.q[1] = q_value.x;
	s_att_hil.q[2] = q_value.y;
	s_att_hil.q[3] = q_value.z;

	s_att_hil.delta_q_reset[0] = q_reset.w;
	s_att_hil.delta_q_reset[1] = q_reset.x;
	s_att_hil.delta_q_reset[2] = q_reset.y;
	s_att_hil.delta_q_reset[3] = q_reset.z;

	if (_att_pub == NULL)
	{
		_att_pub = orb_advertise(ORB_ID(vehicle_attitude), &s_att_hil);
	}

	orb_publish(ORB_ID(vehicle_attitude), _att_pub, &s_att_hil);

	q_reset.w = q_value.w;
	q_reset.x = q_value.x;
	q_reset.y = q_value.y;
	q_reset.z = q_value.z;
}

void hil_pub_local_pos(void)
{
	struct vehicle_local_position_s s_local_pos_hil = { 0 };
	s_local_pos_hil.timestamp = hrt_absolute_time();
	s_local_pos_hil.xy_valid = true;
	s_local_pos_hil.z_valid = true;
	s_local_pos_hil.v_xy_valid = true;
	s_local_pos_hil.v_z_valid = true;
	s_local_pos_hil.x = Pos_global.v[0];
	s_local_pos_hil.y = Pos_global.v[1];
	s_local_pos_hil.z = -Pos_global.v[2];
	s_local_pos_hil.vx = Vel_global.v[0];
	s_local_pos_hil.vy = Vel_global.v[1];
	s_local_pos_hil.vz = -Vel_global.v[2];
	s_local_pos_hil.yaw = Angular_body.v[2];
	s_local_pos_hil.xy_global = true;
	s_local_pos_hil.z_global = true;
	s_local_pos_hil.ref_timestamp = hrt_absolute_time();
	s_local_pos_hil.ref_lat = home_lat;
	s_local_pos_hil.ref_lon = home_lon;
	s_local_pos_hil.ref_alt = home_alt;
	s_local_pos_hil.dist_bottom = 0.0f;
	s_local_pos_hil.dist_bottom_rate = 0.0f;
	s_local_pos_hil.dist_bottom_valid = false;
	s_local_pos_hil.eph = 0.01;
	s_local_pos_hil.epv = 0.01;

	if (_local_pos_pub == NULL)
	{
		_local_pos_pub = orb_advertise(ORB_ID(vehicle_local_position), &s_local_pos_hil);
	}

	orb_publish(ORB_ID(vehicle_local_position), _local_pos_pub, &s_local_pos_hil);
}

void hil_pub_global_pos(void)
{
	double x = Pos_global.v[0];
	double y = Pos_global.v[1];
	double z = Pos_global.v[2];

	double lat = home_lat;
	double lon = home_lon;
	double alt = home_alt + z;

	map_projection_reproject(&_ref_pos, x, y, &lat, &lon);

	struct vehicle_global_position_s s_global_pos_hil = { 0 };
	s_global_pos_hil.timestamp = hrt_absolute_time();
	s_global_pos_hil.lat = lat;
	s_global_pos_hil.lon = lon;
	s_global_pos_hil.alt = alt;
	s_global_pos_hil.vel_n = Vel_global.v[0];
	s_global_pos_hil.vel_e = Vel_global.v[1];
	s_global_pos_hil.vel_d = -Vel_global.v[2];
	s_global_pos_hil.yaw = Angular_body.v[2];
	s_global_pos_hil.eph = 0.01;
	s_global_pos_hil.epv = 0.01;
	s_global_pos_hil.terrain_alt = 0;
	s_global_pos_hil.terrain_alt_valid = false;
	s_global_pos_hil.dead_reckoning = false;

	if (_global_pos_pub == NULL)
	{
		_global_pos_pub = orb_advertise(ORB_ID(vehicle_global_position), &s_global_pos_hil);
	}

	orb_publish(ORB_ID(vehicle_global_position), _global_pos_pub, &s_global_pos_hil);
}

void hil_pub_gps(void)
{
	double x = Pos_global.v[0];
	double y = Pos_global.v[1];
	double z = Pos_global.v[2];

	double lat = home_lat;
	double lon = home_lon;
	double alt = home_alt + z;

	map_projection_reproject(&_ref_pos, x, y, &lat, &lon);

	struct vehicle_gps_position_s s_gps_hil;
	s_gps_hil.timestamp = hrt_absolute_time();
	s_gps_hil.lat = (int32_t) (lat * 10000000);
	s_gps_hil.lon = (int32_t) (lon * 10000000);
	s_gps_hil.alt = (int32_t) (alt * 1000);
	s_gps_hil.satellites_used = 23;
	s_gps_hil.time_utc_usec = hrt_absolute_time();
	s_gps_hil.s_variance_m_s = 0.0f;
	s_gps_hil.c_variance_rad = 0.0f;
	s_gps_hil.fix_type = 3;
	s_gps_hil.eph = 0.01f;
	s_gps_hil.epv = 0.01f;
	s_gps_hil.vel_n_m_s = Vel_global.v[0];
	s_gps_hil.vel_e_m_s = Vel_global.v[1];
	s_gps_hil.vel_d_m_s = -Vel_global.v[2];
	s_gps_hil.vel_m_s = sqrtf(s_gps_hil.vel_n_m_s * s_gps_hil.vel_n_m_s + s_gps_hil.vel_e_m_s * s_gps_hil.vel_e_m_s + s_gps_hil.vel_d_m_s * s_gps_hil.vel_d_m_s);
	s_gps_hil.cog_rad = Angular_body.v[AT(2, 0, Angular_body.n)];
	s_gps_hil.vel_ned_valid = true;

	if (_gps_pub == NULL)
	{
		_gps_pub = orb_advertise(ORB_ID(vehicle_gps_position), &s_gps_hil);
	}
	orb_publish(ORB_ID(vehicle_gps_position), _gps_pub, &s_gps_hil);
}

int hil_task_main(int argc, char *argv[])
{
	int control_sub = orb_subscribe(ORB_ID(actuator_controls_0));
	int armed_sub = orb_subscribe(ORB_ID(actuator_armed));
	int output_sub = orb_subscribe(ORB_ID(actuator_outputs));

	struct actuator_controls_s s_actuators = { 0 };
	struct actuator_armed_s s_armed = { 0 };
	struct actuator_outputs_s s_output = { 0 };

	bool updated = false;

	map_projection_init(&_ref_pos, home_lat, home_lon);

	hil_init();

	hil_zero();

	float theta_t = 0.0f;

	long curr_time = 0;
	long last_time = 0;

	last_time = hrt_absolute_time();

	while (1)
	{
		usleep(10 * 1000);

		curr_time = hrt_absolute_time();
		theta_t = (float) (curr_time - last_time) / (float) (1000.0 * 1000.0);
		last_time = curr_time;

		orb_check(control_sub, &updated);
		if (updated)
		{
			orb_copy(ORB_ID(actuator_controls), control_sub, &s_actuators);
		}

		orb_check(output_sub, &updated);
		if (updated)
		{
			orb_copy(ORB_ID(actuator_outputs), output_sub, &s_output);
		}

		orb_check(armed_sub, &updated);
		if (updated)
		{
			orb_copy(ORB_ID(actuator_armed), armed_sub, &s_armed);
		}

		control.v[0] = s_actuators.control[0];
		control.v[1] = s_actuators.control[1];
		control.v[2] = s_actuators.control[2];
		control.v[3] = s_actuators.control[3];

		output.v[0] = s_output.output[0];
		output.v[1] = s_output.output[1];
		output.v[2] = s_output.output[2];
		output.v[3] = s_output.output[3];

		hil_cal(theta_t);

		if (!s_armed.armed)
		{
			hil_zero();
		}

		if (Pos_global.v[AT(2, 0, Pos_global.n)] <= 0)
		{
			hil_zero();
		}

		hil_pub_att();

		hil_pub_local_pos();

		hil_pub_global_pos();

		hil_pub_gps();

		//mavlink_log_info(&_mavlink_log_pub, "%+4.3f %+4.3f %+4.3f %+4.3f", control.v[0], control.v[1], control.v[2], control.v[3]);
	}

	return 0;
}

int start(int argc, char *argv[])
{
	return px4_task_spawn_cmd("hil", SCHED_DEFAULT, SCHED_PRIORITY_FAST_DRIVER, CONFIG_PTHREAD_STACK_DEFAULT, (px4_main_t) hil_task_main, (char * const *) argv);
}

int hil_main(int argc, char *argv[])
{
	if (strcmp(argv[1], "start") == 0)
	{
		start(argc, argv);
	}

	return 0;
}

