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


/**
 * @file    db_postgres_que.c
 *
 * @brief
 *      Implementation of the queue data access functions for postgres
 */

#include <pbs_config.h>   /* the master config generated by configure */
#include "pbs_db.h"
#include "db_postgres.h"

/**
 * @brief
 *	Prepare all the queue related sqls. Typically called after connect
 *	and before any other sql exeuction
 *
 * @param[in]	conn - Database connection handle
 *
 * @return      Error code
 * @retval	-1 - Failure
 * @retval	 0 - Success
 *
 */
int
pg_db_prepare_que_sqls(pbs_db_conn_t *conn)
{
	sprintf(conn->conn_sql, "insert into pbs.queue("
		"qu_name, "
		"qu_sv_name, "
		"qu_type, "
		"qu_ctime, "
		"qu_mtime "
		") "
		"values "
		"($1, $2, $3, localtimestamp, localtimestamp)");

	if (pg_prepare_stmt(conn, STMT_INSERT_QUE, conn->conn_sql, 3) != 0)
		return -1;

	sprintf(conn->conn_sql, "update pbs.queue set "
		"qu_sv_name = $2, "
		"qu_type = $3, "
		"qu_mtime = localtimestamp "
		" where qu_name = $1");
	if (pg_prepare_stmt(conn, STMT_UPDATE_QUE, conn->conn_sql, 3) != 0)
		return -1;

	sprintf(conn->conn_sql, "select qu_name, "
		"qu_sv_name, "
		"qu_type, "
		"extract(epoch from qu_ctime)::bigint as qu_ctime, "
		"extract(epoch from qu_mtime)::bigint as qu_mtime "
		"from pbs.queue "
		"where qu_name = $1");
	if (pg_prepare_stmt(conn, STMT_SELECT_QUE, conn->conn_sql, 1) != 0)
		return -1;

	sprintf(conn->conn_sql, "insert into "
		"pbs.queue_attr "
		"(qu_name, attr_name, "
		"attr_resource, "
		"attr_value, "
		"attr_flags) "
		"values"
		"($1, $2, $3, $4, $5)");
	if (pg_prepare_stmt(conn, STMT_INSERT_QUEATTR, conn->conn_sql, 5) != 0)
		return -1;

	sprintf(conn->conn_sql, "update pbs.queue_attr set "
		"attr_resource = $3, "
		"attr_value = $4, "
		"attr_flags = $5 "
		"where qu_name = $1 "
		"and attr_name = $2");
	if (pg_prepare_stmt(conn, STMT_UPDATE_QUEATTR, conn->conn_sql, 5) != 0)
		return -1;

	sprintf(conn->conn_sql, "update pbs.queue_attr set "
		"attr_value = $4, "
		"attr_flags = $5 "
		"where qu_name = $1 "
		"and attr_name = $2 "
		"and attr_resource = $3");
	if (pg_prepare_stmt(conn, STMT_UPDATE_QUEATTR_RESC, conn->conn_sql, 5) != 0)
		return -1;

	sprintf(conn->conn_sql, "delete from pbs.queue_attr "
		"where qu_name = $1 "
		"and attr_name = $2");
	if (pg_prepare_stmt(conn, STMT_DELETE_QUEATTR, conn->conn_sql, 2) != 0)
		return -1;

	sprintf(conn->conn_sql, "delete from pbs.queue_attr "
		"where qu_name = $1 "
		"and attr_name = $2 "
		"and attr_resource = $3");
	if (pg_prepare_stmt(conn, STMT_DELETE_QUEATTR_RESC, conn->conn_sql, 3) != 0)
		return -1;

	sprintf(conn->conn_sql, "select "
		"attr_name, "
		"attr_resource, "
		"attr_value, "
		"attr_flags "
		"from pbs.queue_attr "
		"where qu_name = $1");
	if (pg_prepare_stmt(conn, STMT_SELECT_QUEATTR, conn->conn_sql, 1) != 0)
		return -1;

	sprintf(conn->conn_sql, "select "
		"qu_name, "
		"qu_sv_name, "
		"qu_type, "
		"extract(epoch from qu_ctime)::bigint as qu_ctime, "
		"extract(epoch from qu_mtime)::bigint as qu_mtime "
		"from pbs.queue order by qu_ctime");
	if (pg_prepare_stmt(conn, STMT_FIND_QUES_ORDBY_CREATTM,
		conn->conn_sql, 0) != 0)
		return -1;

	sprintf(conn->conn_sql, "delete from pbs.queue where qu_name = $1");
	if (pg_prepare_stmt(conn, STMT_DELETE_QUE, conn->conn_sql, 1) != 0)
		return -1;

	return 0;
}

/**
 * @brief
 *	Load queue data from the row into the queue object
 *
 * @param[in]	res - Resultset from a earlier query
 * @param[in]	pnd  - Queue object to load data into
 * @param[in]	row - The current row to load within the resultset
 *
 */
static void
load_que(PGresult *res, pbs_db_que_info_t *pq, int row)
{
	/* get the other fields */
	strcpy(pq->qu_name, PQgetvalue(res, row,
		PQfnumber(res, "qu_name")));
	strcpy(pq->qu_sv_name, PQgetvalue(res, row,
		PQfnumber(res, "qu_sv_name")));
	pq->qu_type = strtol(PQgetvalue(res, row,
		PQfnumber(res, "qu_type")), NULL, 10);
	pq->qu_ctime = strtoll(PQgetvalue(res, row,
		PQfnumber(res, "qu_ctime")), NULL, 10);
	pq->qu_mtime = strtoll(PQgetvalue(res, row,
		PQfnumber(res, "qu_mtime")), NULL, 10);
}

/**
 * @brief
 *	Insert queue data into the database
 *
 * @param[in]	conn - Connection handle
 * @param[in]	obj  - Information of queue to be inserted
 *
 * @return      Error code
 * @retval	-1 - Failure
 * @retval	 0 - Success
 *
 */
int
pg_db_insert_que(pbs_db_conn_t *conn, pbs_db_obj_info_t *obj)
{
	pbs_db_que_info_t *pq = obj->pbs_db_un.pbs_db_que;
	LOAD_STR(conn, pq->qu_name, 0);
	LOAD_STR(conn, pq->qu_sv_name, 1);
	LOAD_INTEGER(conn, pq->qu_type, 2);

	if (pg_db_cmd(conn, STMT_INSERT_QUE, 3) != 0)
		return -1;

	return 0;
}

/**
 * @brief
 *	Update queue data into the database
 *
 * @param[in]	conn - Connection handle
 * @param[in]	obj  - Information of queue to be updated
 *
 * @return      Error code
 * @retval	-1 - Failure
 * @retval	 0 - Success
 * @retval	 1 - Success but no rows updated
 *
 */
int
pg_db_update_que(pbs_db_conn_t *conn, pbs_db_obj_info_t *obj)
{
	pbs_db_que_info_t *pq = obj->pbs_db_un.pbs_db_que;
	LOAD_STR(conn, pq->qu_name, 0);
	LOAD_STR(conn, pq->qu_sv_name, 1);
	LOAD_INTEGER(conn, pq->qu_type, 2);

	return (pg_db_cmd(conn, STMT_UPDATE_QUE, 3));
}

/**
 * @brief
 *	Load queue data from the database
 *
 * @param[in]	conn - Connection handle
 * @param[in]	obj  - Load queue information into this object
 *
 * @return      Error code
 * @retval	-1 - Failure
 * @retval	 0 - Success
 * @retval	 1 -  Success but no rows loaded
 *
 */
int
pg_db_load_que(pbs_db_conn_t *conn, pbs_db_obj_info_t *obj)
{
	PGresult *res;
	int rc;
	pbs_db_que_info_t *pq = obj->pbs_db_un.pbs_db_que;

	LOAD_STR(conn, pq->qu_name, 0);

	if ((rc = pg_db_query(conn, STMT_SELECT_QUE, 1, &res)) != 0)
		return rc;

	load_que(res, pq, 0);

	PQclear(res);
	return 0;
}

/**
 * @brief
 *	Find queues
 *
 * @param[in]	conn - Connection handle
 * @param[in]	st   - The cursor state variable updated by this query
 * @param[in]	obj  - Information of queue to be found
 * @param[in]	opts - Any other options (like flags, timestamp)
 *
 * @return      Error code
 * @retval	-1 - Failure
 * @retval	 0 - Success
 * @retval	 1 - Success, but no rows found
 *
 */
int
pg_db_find_que(pbs_db_conn_t *conn, void *st, pbs_db_obj_info_t *obj,
	pbs_db_query_options_t *opts)
{
	PGresult *res;
	int rc;
	pg_query_state_t *state = (pg_query_state_t *) st;

	if (!state)
		return -1;

	strcpy(conn->conn_sql, STMT_FIND_QUES_ORDBY_CREATTM);
	if ((rc = pg_db_query(conn, conn->conn_sql, 0, &res)) != 0)
		return rc;

	state->row = 0;
	state->res = res;
	state->count = PQntuples(res);

	return 0;
}

/**
 * @brief
 *	Get the next queue from the cursor
 *
 * @param[in]	conn - Connection handle
 * @param[in]	st   - The cursor state
 * @param[in]	obj  - queue information is loaded into this object
 *
 * @return      Error code
 *		(Even though this returns only 0 now, keeping it as int
 *			to support future change to return a failure)
 * @retval	 0 - Success
 *
 */
int
pg_db_next_que(pbs_db_conn_t* conn, void *st, pbs_db_obj_info_t* obj)
{
	pg_query_state_t *state = (pg_query_state_t *) st;

	load_que(state->res, obj->pbs_db_un.pbs_db_que, state->row);
	return 0;
}

/**
 * @brief
 *	Delete the queue from the database
 *
 * @param[in]	conn - Connection handle
 * @param[in]	obj  - queue information
 *
 * @return      Error code
 * @retval	-1 - Failure
 * @retval	 0 - Success
 * @retval	 1 - Success but no rows deleted
 *
 */
int
pg_db_delete_que(pbs_db_conn_t *conn, pbs_db_obj_info_t *obj)
{
	pbs_db_que_info_t *pq = obj->pbs_db_un.pbs_db_que;
	LOAD_STR(conn, pq->qu_name, 0);
	return (pg_db_cmd(conn, STMT_DELETE_QUE, 1));
}
