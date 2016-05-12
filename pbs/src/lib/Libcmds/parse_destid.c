/*
 * Copyright (C) 1994-2016 Altair Engineering, Inc.
 * For more information, contact Altair at www.altair.com.
 *  
 * This file is part of the PBS Professional ("PBS Pro") software.
 * 
 * Open Source License Information:
 *  
 * PBS Pro is free software. You can redistribute it and/or modify it under the
 * terms of the GNU Affero General Public License as published by the Free 
 * Software Foundation, either version 3 of the License, or (at your option) any 
 * later version.
 *  
 * PBS Pro is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License along 
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *  
 * Commercial License Information: 
 * 
 * The PBS Pro software is licensed under the terms of the GNU Affero General 
 * Public License agreement ("AGPL"), except where a separate commercial license 
 * agreement for PBS Pro version 14 or later has been executed in writing with Altair.
 *  
 * Altair’s dual-license business model allows companies, individuals, and 
 * organizations to create proprietary derivative works of PBS Pro and distribute 
 * them - whether embedded or bundled with other software - under a commercial 
 * license agreement.
 * 
 * Use of Altair’s trademarks, including but not limited to "PBS™", 
 * "PBS Professional®", and "PBS Pro™" and Altair’s logos is subject to Altair's 
 * trademark licensing policies.
 *
 */
/*
 *
 * parse_destination_id
 *
 * full syntax permitted;
 *
 * queue_name[@server_name[:port_number]]
 * @server_name[:port_number]
 *
 */

#include <pbs_config.h>   /* the master config generated by configure */
#include "cmds.h"

#define ISNAMECHAR(x) ((isgraph(x)) && ((x) != '#') && ((x) != '@') )


/*
 * note the queue_name_out and server_name_out is now allocated on heap
 * so caller should free them after using
 */

/**
 * @brief
 *	parse destination id
 *
 * @param[in] destination_in - destination string
 * @param[out] queue_name_out - queue name
 * @param[out] server_name_out - server name
 *
 * @return      int
 * @retval      0       success
 * @retval      1       failure
 *
 * NOTE: the queue_name_out and server_name_out is now allocated on heap
 * so caller should free them after using
 *
 */

int
parse_destination_id(char *destination_in, char **queue_name_out, char **server_name_out)
{
	char *c;
	/* moved following static vars to stack */
	char *queue_name = NULL;
	int   q_pos = 0;
	char *server_name = NULL;
	int   c_pos = 0;

	queue_name = calloc(PBS_MAXQUEUENAME+1, 1);
	if (queue_name == NULL)
		goto err;

	server_name = calloc(MAXSERVERNAME, 1);
	if (server_name == NULL)
		goto err;

	/* Begin the parse */
	c = destination_in;
	while (isspace(*c)) c++;

	/* Looking for a queue */
	while (*c != '\0') {
		if (ISNAMECHAR(*c)) {
			if (q_pos >= PBS_MAXQUEUENAME) goto err;
			queue_name[q_pos++]=*c;
		} else
			break;
		c++;
	}

	/* Looking for a server */
	if (*c == '@') {
		c++;
		while (*c != '\0') {
			if (ISNAMECHAR(*c)) {
				if (c_pos >= MAXSERVERNAME) goto err;
				server_name[c_pos++]=*c;
			} else
				break;
			c++;
		}
		if (c_pos == 0) goto err;
	}

	if (*c != '\0') goto err;

	/* set char * pointers to static data, to arguments */
	if (queue_name_out != NULL) *queue_name_out = queue_name;
	if (server_name_out != NULL) *server_name_out = server_name;

	return 0;

err:
	if (queue_name) free(queue_name);
	if (server_name) free(server_name);
	return 1;
}
