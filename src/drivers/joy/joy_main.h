/*
 * joy_main.h
 *
 *  Created on: Jul 4, 2018
 *      Author: lidq
 */

#ifndef SRC_DRIVERS_JOY_JOY_MAIN_H_
#define SRC_DRIVERS_JOY_JOY_MAIN_H_


#include "joy_typedef.h"

int joy_main(int argc, char *argv[]);

static int start(int argc, char *argv[]);

static int stop(void);

static int joy_run(int argc, char *argv[]);

static void parse_joy(uint8_t *buff, int len);

static void publish_rc(void);

#endif /* SRC_DRIVERS_JOY_JOY_MAIN_H_ */
