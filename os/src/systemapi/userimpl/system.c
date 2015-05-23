/*
 * system.c
 *
 *  Created on: 22.05.2015
 *      Author: Nicolaj Hoess
 */

#include "../includes/system.h"
#include "../includes/systemcall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * \brief This function executes the given command. The command may contain additional arguments.
 * 		  The function returns SYSTEM_FAILURE if the command has not been executed or a positive number
 * 		  which corresponds to the newly created process's pid.
 */
int execute(char* command)
{
	int returnArg = 0;
	// Package the command into a message_t struct
	systemCallMessage_t message;
	message.systemCallNumber = SYSTEM_CALL_EXEC;
	message.messageArgs.callArg = strlen(command);
	message.messageArgs.callBuf = command;

	message.messageArgs.returnArg = &returnArg;

	SystemCall(&message);

	return returnArg;
}