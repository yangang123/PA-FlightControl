/*
 * extctl_typedef.h
 *
 *  Created on: Jun 7, 2018
 *      Author: lidq
 */

#ifndef SRC_DRIVERS_IOBOARD_IOBOARD_TYPEDEF_H_
#define SRC_DRIVERS_IOBOARD_IOBOARD_TYPEDEF_H_

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <px4_config.h>
#include <px4_getopt.h>
#include <px4_module.h>
#include <px4_tasks.h>
#include <px4_time.h>
#include <arch/board/board.h>
#include <drivers/drv_hrt.h>
#include <mathlib/mathlib.h>
#include <systemlib/systemlib.h>
#include <systemlib/err.h>
#include <termios.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <setjmp.h>
#include <sys/syscall.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <poll.h>
#include <px4_getopt.h>

#include <uORB/topics/pwm_output.h>
#include <uORB/topics/input_rc.h>
#include <uORB/topics/battery_status.h>

#ifdef __PX4_POSIX
#define CONFIG_PTHREAD_STACK_DEFAULT	(2048)
#endif

#define DEV_NAME			"/dev/ttyAMA0"

#define DEV_BAUDRATE		(B115200)
#define DEV_RATE_BASE		(1000 * 1000)
#define DEV_RATE_W			(DEV_RATE_BASE / 100)
#define DEV_RATE_R			(DEV_RATE_BASE / 100)

#define VCC_SOFT			(0.004f)

typedef struct pwm_out_s
{
	uint16_t num_outputs;
	uint16_t pwm[16];

} pwm_out_s;

typedef struct rc_input_s
{
	bool rc_failsafe;
	bool rc_lost;
	uint32_t channel_count;
	uint16_t values[18];
} rc_input_s;

typedef struct battery_s
{
	float vcc;
} battery_s;

enum data_type
{
	DATA_TYPE_PWM_OUTPUT = 0,
	DATA_TYPE_RC_INPUT,
	DATA_TYPE_BATTERY,
	DATA_TYPE_END,
};

#endif /* SRC_MODULES_EXTCTL_EXTCTL_TYPEDEF_H_ */
