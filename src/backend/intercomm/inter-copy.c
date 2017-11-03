/*-------------------------------------------------------------------------
 *
 * inter-copy.c
 *	  Internode copy routines by NodeHandle
 *
 *
 * Portions Copyright (c) 2016-2017, ADB Development Group
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/intercomm/inter-copy.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "miscadmin.h"

#include "access/xact.h"
#include "agtm/agtm.h"
#include "commands/prepare.h"
#include "executor/clusterReceiver.h"
#include "intercomm/inter-comm.h"
#include "libpq/libpq.h"
#include "libpq/libpq-int.h"
#include "libpq/libpq-node.h"
#include "nodes/execnodes.h"
#include "pgxc/pgxc.h"
#include "pgxc/copyops.h"
#include "utils/lsyscache.h"
#include "utils/snapmgr.h"

typedef struct RemoteCopyContext
{
	RemoteCopyState	   *node;
	TupleTableSlot	   *slot;
} RemoteCopyContext;

static NodeHandle*LookupNodeHandle(List *handle_list, Oid node_id);
static void HandleCopyOutRow(RemoteCopyState *node, char *buf, int len);
static int HandleStartRemoteCopy(NodeHandle *handle, CommandId cmid, Snapshot snap, const char *copy_query);
static bool HandleRecvCopyResult(NodeHandle *handle);
static void FetchRemoteCopyRow(RemoteCopyState *node, StringInfo row);
static bool FetchCopyRowHook(void *context, struct pg_conn *conn, PQNHookFuncType type, ...);

/*
 * StartRemoteCopy
 *
 * Send begin and copy query to involved nodes.
 */
void
StartRemoteCopy(RemoteCopyState *node)
{
	InterXactState		state;
	NodeMixHandle	   *mix_handle;
	NodeHandle		   *handle;
	ListCell		   *lc_handle;
	const char		   *copy_query;
	const List		   *node_list;
	bool				is_from;
	bool				already_begin;
	Snapshot			snap;
	CommandId			cmid;
	TimestampTz			timestamp;
	GlobalTransactionId gxid;

	if (!node)
		return ;

	copy_query = node->query_buf.data;
	node_list = node->exec_nodes->nodeids;
	is_from = node->is_from;

	/* sanity check */
	if (!copy_query || !list_length(node_list))
		return ;

	/* Must use the command ID to mark COPY FROM tuples */
	cmid = GetCurrentCommandId(is_from);
	snap = GetActiveSnapshot();
	timestamp = GetCurrentTransactionStartTimestamp();
	state = MakeInterXactState2(GetTopInterXactState(), node_list);
	/* It is no need to send BEGIN when COPY TO */
	state->need_xact_block = is_from;
	mix_handle = state->mix_handle;

	agtm_BeginTransaction();
	gxid = GetCurrentTransactionId();

	PG_TRY();
	{
		foreach (lc_handle, mix_handle->handles)
		{
			handle = (NodeHandle *) lfirst(lc_handle);
			already_begin = false;

			if (!HandleBegin(state, handle, gxid, timestamp, is_from, &already_begin) ||
				!HandleStartRemoteCopy(handle, cmid, snap, copy_query))
			{
				state->block_state |= IBLOCK_ABORT;
				ereport(ERROR,
						(errcode(ERRCODE_INTERNAL_ERROR),
						 errmsg("Fail to start remote COPY %s", is_from ? "FROM" : "TO"),
						 errnode(NameStr(handle->node_name)),
						 errhint("%s", HandleGetError(handle, false))));
			}
		}
	} PG_CATCH();
	{
		InterXactGC(state);
		PG_RE_THROW();
	} PG_END_TRY();

	node->copy_handles = mix_handle->handles;
}

/*
 * EndRemoteCopy
 *
 * Send copy end message to involved handles.
 */
void
EndRemoteCopy(RemoteCopyState *node)
{
	NodeHandle	   *handle;
	ListCell	   *lc_handle;

	Assert(node);
	PG_TRY();
	{
		if (PrHandle && list_member_ptr(node->copy_handles, PrHandle))
		{
			if (PQputCopyEnd(PrHandle->node_conn, NULL) <= 0 ||
				!HandleFinishCommand(PrHandle, NULL))
				ereport(ERROR,
						(errmsg("Fail to end COPY %s", node->is_from ? "FROM" : "TO"),
						 errnode(NameStr(PrHandle->node_name)),
						 errhint("%s", HandleGetError(PrHandle, false))));
		}

		foreach (lc_handle, node->copy_handles)
		{
			handle = (NodeHandle *) lfirst(lc_handle);
			/* Primary handle has been copy end already, see above */
			if (PrHandle && handle == PrHandle)
				continue;
			if (PQputCopyEnd(handle->node_conn, NULL) <= 0 ||
				!HandleFinishCommand(handle, NULL))
				ereport(ERROR,
						(errmsg("Fail to end COPY %s", node->is_from ? "FROM" : "TO"),
						 errnode(NameStr(PrHandle->node_name)),
						 errhint("%s", HandleGetError(handle, false))));
		}
	} PG_CATCH();
	{
		HandleListGC(node->copy_handles);
		PG_RE_THROW();
	} PG_END_TRY();
}

/*
 * SendCopyFromHeader
 *
 * Send PG_HEADER for a COPY FROM in binary mode to all involved nodes.
 */
void
SendCopyFromHeader(RemoteCopyState *node, const StringInfo header)
{
	ListCell	   *lc_handle;
	NodeHandle	   *handle;

	Assert(node && header);
	PG_TRY();
	{
		foreach (lc_handle, node->copy_handles)
		{
			handle = (NodeHandle *) lfirst(lc_handle);
			if (PQputCopyData(handle->node_conn, header->data, header->len) <= 0)
				ereport(ERROR,
						(errcode(ERRCODE_BAD_COPY_FILE_FORMAT),
						 errmsg("Fail to send COPY FROM header in binary mode."),
						 errnode(NameStr(handle->node_name)),
						 errhint("%s", HandleGetError(handle, false))));
		}
	} PG_CATCH();
	{
		HandleListGC(node->copy_handles);
		PG_RE_THROW();
	} PG_END_TRY();
}

/*
 * DoRemoteCopyFrom
 *
 * Send copy line buffer for a COPY FROM to the node list.
 */
void
DoRemoteCopyFrom(RemoteCopyState *node, const StringInfo line_buf, const List *node_list)
{
	NodeHandle	   *handle;
	ListCell	   *lc_node;
	Oid				node_id;

	Assert(node && line_buf);
	PG_TRY();
	{
		/* Primary handle should be sent first */
		if (PrHandle && list_member_oid(node_list, PrHandle->node_id))
		{
			Assert(list_member_ptr(node->copy_handles, PrHandle));
			if (PQputCopyData(PrHandle->node_conn, line_buf->data, line_buf->len) <= 0)
				ereport(ERROR,
						(errmsg("Fail to send to COPY FROM data."),
						 errnode(NameStr(PrHandle->node_name)),
						 errhint("%s", HandleGetError(PrHandle, false))));
		}

		foreach (lc_node, node_list)
		{
			node_id = lfirst_oid(lc_node);
			/* Primary handle has been sent already, see above */
			if (PrHandle && node_id == PrHandle->node_id)
				continue;
			handle = LookupNodeHandle(node->copy_handles, node_id);
			Assert(handle);
			if (PQputCopyData(handle->node_conn, line_buf->data, line_buf->len) <= 0)
				ereport(ERROR,
						(errmsg("Fail to send to COPY FROM data."),
						 errnode(NameStr(handle->node_name)),
						 errhint("%s", HandleGetError(handle, false))));
		}
	} PG_CATCH();
	{
		HandleListGC(node->copy_handles);
		PG_RE_THROW();
	} PG_END_TRY();
}

/*
 * DoRemoteCopyTo
 *
 * Fetch each copy out row and handle them
 *
 * return the count of copy row
 */
uint64
DoRemoteCopyTo(RemoteCopyState *node)
{
	StringInfoData row = {NULL, 0, 0, 0};

	Assert(node);

	node->processed = 0;
	for (;;)
	{
		FetchRemoteCopyRow(node, &row);
		if (!row.data)
			break;
		node->processed++;
		HandleCopyOutRow(node, row.data, row.len);
	}

	return node->processed;
}

/*
 * HandleCopyOutRow
 *
 * handle the row buffer to the specified destination.
 */
static void
HandleCopyOutRow(RemoteCopyState *node, char *buf, int len)
{
	Assert(node);
	switch (node->remoteCopyType)
	{
		case REMOTE_COPY_FILE:
			Assert(node->copy_file);
			/* Write data directly to file */
			fwrite(buf, 1, len, node->copy_file);
			break;
		case REMOTE_COPY_STDOUT:
			/* Send back data to client */
			pq_putmessage('d', buf, len);
			break;
		case REMOTE_COPY_TUPLESTORE:
			{
				TupleDesc			tupdesc = node->tuple_desc;
				Form_pg_attribute  *attr = tupdesc->attrs;
				Datum			   *values;
				bool			   *nulls;
				Oid				   *typioparams;
				FmgrInfo		   *in_functions;
				char			  **fields;
				int					i, dropped;

				values = (Datum *) palloc(tupdesc->natts * sizeof(Datum));
				nulls = (bool *) palloc0(tupdesc->natts * sizeof(bool));
				in_functions = (FmgrInfo *) palloc(tupdesc->natts * sizeof(FmgrInfo));
				typioparams = (Oid *) palloc(tupdesc->natts * sizeof(Oid));

				/* Calculate the Oids of input functions */
				for (i = 0; i < tupdesc->natts; i++)
				{
					Oid in_func_oid;

					/* Do not need any information for dropped attributes */
					if (attr[i]->attisdropped)
						continue;

					getTypeInputInfo(attr[i]->atttypid, &in_func_oid, &typioparams[i]);
					fmgr_info(in_func_oid, &in_functions[i]);
				}

				/*
				 * Convert message into an array of fields.
				 * Last \n is not included in converted message.
				 */
				fields = CopyOps_RawDataToArrayField(tupdesc, buf, len - 1);

				/* Fill in the array values */
				dropped = 0;
				for (i = 0; i < tupdesc->natts; i++)
				{
					char *string = fields[i - dropped];
					/* Do not need any information for dropped attributes */
					if (attr[i]->attisdropped)
					{
						dropped++;
						nulls[i] = true; /* Consider dropped parameter as NULL */
						continue;
					}

					/* Find value */
					values[i] = InputFunctionCall(&in_functions[i],
												  string,
												  typioparams[i],
												  attr[i]->atttypmod);
					/* Setup value with NULL flag if necessary */
					if (string == NULL)
						nulls[i] = true;
				}

				/* Then insert the values into tuplestore */
				tuplestore_putvalues(node->tuplestorestate,
									 node->tuple_desc,
									 values,
									 nulls);

				/* Clean up everything */
				if (*fields)
					pfree(*fields);
				pfree(fields);
				pfree(values);
				pfree(nulls);
				pfree(in_functions);
				pfree(typioparams);
			}
			break;
		case REMOTE_COPY_NONE:
		default:
			Assert(0); /* Should not happen */
	}
}

/*
 * HandleStartRemoteCopy
 *
 * Send copy query to remote and wait for receiving response.
 *
 * return 0 if any trouble.
 * return 1 if OK.
 */
static int
HandleStartRemoteCopy(NodeHandle *handle, CommandId cmid, Snapshot snap, const char *copy_query)
{
	if (!HandleSendQueryTree(handle, cmid, snap, copy_query, NULL) ||
		!HandleRecvCopyResult(handle))
		return 0;

	return 1;
}

/*
 * HandleRecvCopyResult
 *
 * Wait for receiving correct COPY response synchronously
 *
 * return true if OK
 * return false if trouble
 */
static bool
HandleRecvCopyResult(NodeHandle *handle)
{
	bool copy_start_ok = false;
	PGresult *res = NULL;

	Assert(handle && handle->node_conn);
	res = PQgetResult(handle->node_conn);
	switch (PQresultStatus(res))
	{
		case PGRES_COPY_OUT:
		case PGRES_COPY_IN:
		case PGRES_COPY_BOTH:
			copy_start_ok = true;
			break;
		default:
			break;
	}
	PQclear(res);

	return copy_start_ok;
}

/*
 * FetchRemoteCopyRow
 *
 * Fetch any copy row from remote handles. keep them save
 * in StringInfo row.
 */
static void
FetchRemoteCopyRow(RemoteCopyState *node, StringInfo row)
{
	List *handle_list = NIL;

	Assert(node && row);
	handle_list = node->copy_handles;
	row->data = NULL;
	row->len = 0;
	row->maxlen = 0;
	row->cursor = 0;

	PQNListExecFinish(handle_list, HandleGetPGconn, FetchCopyRowHook, row, true);
}

static bool
FetchCopyRowHook(void *context, struct pg_conn *conn, PQNHookFuncType type, ...)
{
	va_list args;

	switch(type)
	{
		case PQNHFT_ERROR:
			return PQNEFHNormal(NULL, conn, type);
		case PQNHFT_COPY_OUT_DATA:
			{
				StringInfo	row = (StringInfo) context;

				va_start(args, type);
				row->data = (char *) va_arg(args, const char*);
				row->len = row->maxlen = va_arg(args, int);
				va_end(args);

				return true;
			}
			break;
		case PQNHFT_COPY_IN_ONLY:
			PQputCopyEnd(conn, NULL);
			break;
		case PQNHFT_RESULT:
			{
				PGresult	   *res;
				ExecStatusType	status;

				va_start(args, type);
				res = va_arg(args, PGresult*);
				if(res)
				{
					status = PQresultStatus(res);
					if(status == PGRES_FATAL_ERROR)
						PQNReportResultError(res, conn, ERROR, true);
					else if(status == PGRES_COPY_IN)
						PQputCopyEnd(conn, NULL);
				}
				va_end(args);
			}
			break;
		default:
			break;
	}

	return false;
}

/*
 * LookupNodeHandle
 *
 * Find hanlde from handle list by node ID.
 *
 * return NULL if not found
 * return handle if found
 */
static NodeHandle*
LookupNodeHandle(List *handle_list, Oid node_id)
{
	ListCell	   *lc_handle;
	NodeHandle	   *handle;

	if (!OidIsValid(node_id))
		return NULL;

	foreach (lc_handle, handle_list)
	{
		handle = (NodeHandle *) lfirst(lc_handle);
		if (handle->node_id == node_id)
			return handle;
	}

	return NULL;
}
