/*--------------------------------------------------------------------------
 *
 * Copyright (c) 2018-2019, Asiainfo Database Innovation Lab
 *
 * Switching is not allowed to go back, always going forward.
 * 
 * -------------------------------------------------------------------------
 */

#include "postgres.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "mgr/mgr_switcher.h"
#include "mgr/mgr_msg_type.h"
#include "mgr/mgr_cmds.h"
#include "mgr/mgr_helper.h"
#include "access/xlog.h"
#include "access/htup_details.h"
#include "executor/spi.h"
#include "utils/memutils.h"
#include "utils/builtins.h"
#include "../../src/interfaces/libpq/libpq-fe.h"
#include "../../src/interfaces/libpq/libpq-int.h"
#include "catalog/pgxc_node.h"


static SwitcherNodeWrapper *checkGetOldMaster(char *oldMasterName,
												char nodeType,	
												int connectTimeout,
												MemoryContext spiContext);
static SwitcherNodeWrapper *checkGetOldMasterForZoneCoord(MemoryContext spiContext,
															char nodeType,
															char *oldMasterName);			
static SwitcherNodeWrapper *checkGetGtmCoordOldMaster(char *oldMasterName,
													  int connectTimeout,
													  MemoryContext spiContext);
static SwitcherNodeWrapper *checkGetSwitchoverNewMaster(char *newMasterName,
														char nodeType,
														bool forceSwitch,
														MemoryContext spiContext);
static SwitcherNodeWrapper *checkGetSwitchoverOldMaster(Oid oldMasterOid,
														char nodetype,	
														MemoryContext spiContext);
static void checkGetAllDataNodes(dlist_head *dataNodes,
								 MemoryContext spiContext);
static void checkGetSlaveNodesRunningStatus(SwitcherNodeWrapper *masterNode,
											MemoryContext spiContext,
											Oid excludeSlaveOid,
											char *zone,
											dlist_head *failedSlaves,
											dlist_head *runningSlaves);
static void checkGetSlaveNodesRunningSecondStatus(SwitcherNodeWrapper *oldMaster,
													MemoryContext spiContext,
													Oid excludeSlaveOid,
													dlist_head *excludeRunningSlaves,
													dlist_head *excludeFailedSlaves,
													dlist_head *runningSlavesSecond,
													dlist_head *failedSlavesSecond);
static void DeleteFromMgrNodeListByOid(dlist_head *switcherNodes, Oid excludeSlaveOid);		
static void DeleteFromMgrNodeListByList(dlist_head *switcherNodes, dlist_head *excludeSlaveList);									
static void checkGetRunningSlaveNodesInZone(SwitcherNodeWrapper *masterNode,
											MemoryContext spiContext,
											char slaveNodetype,
											char *zone,
											dlist_head *runningSlaves);											
static void precheckPromotionNode(dlist_head *runningSlaves,
									bool forceSwitch);
static SwitcherNodeWrapper *getBestWalLsnSlaveNode(dlist_head *runningSlaves,
												   dlist_head *failedSlaves,
												   char *masterNodeZone,
												   char *zone);
static void sortNodesByWalLsnDesc(dlist_head *nodes);
static bool checkIfSyncSlaveNodeIsRunning(MemoryContext spiContext,
										  MgrNodeWrapper *masterNode);
static void validateFailedSlavesForSwitch(MgrNodeWrapper *oldMaster,
										  MgrNodeWrapper *newMaster,
										  dlist_head *failedSlaves,
										  bool forceSwitch);
static void validateNewMasterCandidateForSwitch(MgrNodeWrapper *oldMaster,
												SwitcherNodeWrapper *candidate,
												bool forceSwitch);
static void restoreCoordinatorSetting(SwitcherNodeWrapper *coordinator);
static bool tryConnectNode(SwitcherNodeWrapper *node, int connectTimeout);
static void classifyNodesForSwitch(dlist_head *nodes,
								   dlist_head *runningNodes,
								   dlist_head *failedNodes);
static void runningSlavesFollowNewMaster(SwitcherNodeWrapper *newMaster,
										 SwitcherNodeWrapper *oldMaster,
										 dlist_head *runningSlaves,
										 MgrNodeWrapper *gtmMaster,
										 MemoryContext spiContext,
										 char *operType,
										 char *zone);
static int walLsnDesc(const void *node1, const void *node2);
static void checkSet_pool_release_to_idle_timeout(SwitcherNodeWrapper *node);
static void promoteNewMasterStartReign(SwitcherNodeWrapper *oldMaster,
									   SwitcherNodeWrapper *newMaster);
static void refreshMgrNodeBeforeSwitch(SwitcherNodeWrapper *node,
									   MemoryContext spiContext);
static void refreshMgrNodeListBeforeSwitch(MemoryContext spiContext, dlist_head *nodes);
static void refreshMgrNodeListAfterFailoverGtm(MemoryContext spiContext, 
													dlist_head *nodes);
static void refreshOldMasterBeforeSwitch(SwitcherNodeWrapper *oldMaster,
										 MemoryContext spiContext);
static void refreshSlaveNodesBeforeSwitch(SwitcherNodeWrapper *newMaster,
										  dlist_head *runningSlaves,
										  dlist_head *failedSlaves,
										  dlist_head *runningSlavesSecond,
										  dlist_head *failedSlavesSecond,
										  MemoryContext spiContext);
static void refreshSlaveNodesAfterSwitch(SwitcherNodeWrapper *newMaster,
										 SwitcherNodeWrapper *oldMaster, 
										 dlist_head *runningSlaves,
										 dlist_head *failedSlaves,
										 dlist_head *runningSlavesSecond,
										 dlist_head *failedSlavesSecond,
										 MemoryContext spiContext,
										 char *operType,
										 char *zone);
static void refreshOldMasterAfterSwitch(SwitcherNodeWrapper *oldMaster,
										SwitcherNodeWrapper *newMaster,
										MemoryContext spiContext,
										bool kickOutOldMaster);
static void refreshOtherNodeAfterSwitchGtmCoord(SwitcherNodeWrapper *node,
												MemoryContext spiContext);
static void refreshOldMasterAfterSwitchover(SwitcherNodeWrapper *oldMaster,
											SwitcherNodeWrapper *newMaster,
											MemoryContext spiContext);
static void refreshNewMasterAfterSwitchover(SwitcherNodeWrapper *newMaster,	
											MemoryContext spiContext);											
static void refreshMgrUpdateparmAfterSwitch(MgrNodeWrapper *oldMaster,
											MgrNodeWrapper *newMaster,
											MemoryContext spiContext,
											bool kickOutOldMaster);
static void refreshPgxcNodesOfCoordinators(SwitcherNodeWrapper *holdLockNode,
										   dlist_head *coordinators,
										   SwitcherNodeWrapper *oldMaster,
										   SwitcherNodeWrapper *newMaster);
static void refreshPgxcNodesOfNewDataNodeMaster(SwitcherNodeWrapper *holdLockNode,
												SwitcherNodeWrapper *oldMaster,
												SwitcherNodeWrapper *newMaster,
												bool complain);
static void checkCreateDataNodeSlaveOnPgxcNodeOfMaster(PGconn *activeConn,
													   char *masterNodeName,
													   bool localExecute,
													   MgrNodeWrapper *dataNodeSlave,
													   bool complain);
static void deleteMgrUpdateparmByNodenameType(char *updateparmnodename,
											  char updateparmnodetype,
											  MemoryContext spiContext);
static void updateMgrUpdateparmNodetype(char *nodename, char nodetype,
										MemoryContext spiContext);
static void refreshPgxcNodeBeforeSwitchDataNode(dlist_head *coordinators);
static bool deletePgxcNodeDataNodeSlaves(SwitcherNodeWrapper *coordinator,
										 bool complain);
static bool updatePgxcNodeForSwitch(SwitcherNodeWrapper *holdLockNode,
									SwitcherNodeWrapper *executeOnNode,
									SwitcherNodeWrapper *oldNode,
									SwitcherNodeWrapper *newNode,
									bool complain);
static bool pgxcPoolReloadOnNode(SwitcherNodeWrapper *holdLockNode,
								 SwitcherNodeWrapper *executeOnNode,
								 bool complain);
static SwitcherNodeWrapper *getGtmCoordMaster(dlist_head *coordinators);
static void batchSetGtmInfoOnNodes(MgrNodeWrapper *gtmMaster,
								   dlist_head *nodes,
								   SwitcherNodeWrapper *ignoreNode,
								   bool complain);
static void batchCheckGtmInfoOnNodes(MgrNodeWrapper *gtmMaster,
									 dlist_head *nodes,
									 SwitcherNodeWrapper *ignoreNode,
									 int checkSeconds,
									 bool complain);
static void batchSetCheckGtmInfoOnNodes(MgrNodeWrapper *gtmMaster,
										dlist_head *nodes,
										SwitcherNodeWrapper *ignoreNode,
										bool complain);
static bool isCurestatusForRunningOk(char *curestatus);
static void checkTrackActivitiesForSwitchover(dlist_head *coordinators,
											  SwitcherNodeWrapper *oldMaster);
static void checkActiveConnectionsForSwitchover(dlist_head *coordinators,
												SwitcherNodeWrapper *oldMaster,
												int maxTrys);
static bool checkActiveConnections(PGconn *activePGcoon,
								   bool localExecute,
								   char *nodename);
static void checkActiveLocksForSwitchover(dlist_head *coordinators, 
										  SwitcherNodeWrapper *holdLockCoordIn,
										  SwitcherNodeWrapper *oldMaster,
										  int maxTrys);
static bool checkActiveLocks(PGconn *activePGcoon,
							 bool localExecute,
							 MgrNodeWrapper *mgrNode);
static void checkXlogDiffForSwitchover(SwitcherNodeWrapper *oldMaster,
									   SwitcherNodeWrapper *newMaster,
									   int maxTrys);
static void checkXlogDiffForSwitchoverCoord(dlist_head *coordinators, SwitcherNodeWrapper *holdLockCoordIn,
											SwitcherNodeWrapper *oldMaster,
											SwitcherNodeWrapper *newMaster,
											int maxTrys);	
static bool RevertFailoverDataOfPgxcNode(SwitcherNodeWrapper *holdLockCoordinator,
										dlist_head *coordinators,
										SwitcherNodeWrapper *oldMaster,
										SwitcherNodeWrapper *newMaster);													
static void RevertFailOverData(MemoryContext spiContext,
								bool beginFailOver,	
								dlist_head *coordinators,
								SwitcherNodeWrapper *oldMaster,
								SwitcherNodeWrapper *newMaster,
								dlist_head *runningSlaves);
static void revertGtmInfoSetting(SwitcherNodeWrapper *oldGtmMaster,
								 SwitcherNodeWrapper *newGtmMaster,
								 dlist_head *coordinators,
								 dlist_head *coordinatorSlaves,
								 dlist_head *dataNodes);
static void RevertPgxcNodeCoord(dlist_head *coordinators,
								 SwitcherNodeWrapper *oldMaster,
								 SwitcherNodeWrapper *newMaster,
								 bool unLoc);								 
static MgrNodeWrapper *checkGetMasterNodeBySlaveNodename(char *slaveNodename,
														 char slaveNodetype,
														 MemoryContext spiContext,
														 bool complain);
static SwitcherNodeWrapper *getNewMasterNodeByNodename(dlist_head *runningSlaves,
													   dlist_head *failedSlaves,
													   char *newMasterName);
static void ShutdownRunningNotZone(dlist_head *runningSlaves, char *zone);
static void RefreshMgrNodesBeforeSwitchGtmCoord(MemoryContext spiContext, 
											SwitcherNodeWrapper *newMaster,
											SwitcherNodeWrapper *oldMaster,
											dlist_head *runningSlaves,
											dlist_head *failedSlaves,
											dlist_head *runningSlavesSecond,
											dlist_head *failedSlavesSecond,
											dlist_head *coordinators,
											dlist_head *coordinatorSlaves,
											dlist_head *dataNodes);
static void RefreshMgrNodesAfterSwitchGtmCoord(MemoryContext spiContext, 
											dlist_head *coordinators, 
											dlist_head *coordinatorSlaves,
											dlist_head *dataNodes,
											SwitcherNodeWrapper *newMaster);	
static void RefreshPgxcNodeName(SwitcherNodeWrapper *node, char *nodeName);
static void selectActiveMgrNodeChild(MemoryContext spiContext, 
									MgrNodeWrapper *node,
									char slaveNodetype,
									dlist_head *slaveNodes);
static void PrintMgrNode(MemoryContext spiContext, 
						dlist_head *mgrNodes);								
static void RevertZoneSwitchoverDataNode(MemoryContext spiContext, 
										ZoneOverGtm *zoGtm, 
										ZoneOverDN	*zoDN);										

static void SetGtmInfoSettingToList(dlist_head *nodes,
									SwitcherNodeWrapper *gtmMaster);
static void switchoverGtmCoordForZone(MemoryContext spiContext, 
										char *newMasterName, 
										bool forceSwitch, 
										char *curZone,
										int maxTrys, 
										ZoneOverGtm *zoGtm);
static void SwitchoverGtmFreeMalloc(ZoneOverGtm *zoGtm);
static void SwitchoverCoordFreeMalloc(dlist_head *zoCoordList);
static void SwitchoverDataNodeFreeMalloc(dlist_head *zoDNList);

static bool RevertRefreshPgxcNodeList(SwitcherNodeWrapper *holdLockCoordinator,
										dlist_head *nodeList,
										SwitcherNodeWrapper *oldMaster,
										SwitcherNodeWrapper *newMaster, 
										bool complain);																																				
static void batchSetCheckGtmInfoOnAllNodes(ZoneOverGtm *zoGtm, 
											SwitcherNodeWrapper *gtmMaster);
static void switchoverCoordForZone(MemoryContext spiContext,
									char 	*newMasterName,									
									char 	*curZone, 
									bool 	forceSwitch, 
									int     maxTrys,   	
									ZoneOverGtm *zoGtm,
									ZoneOverCoord *zoCoord);									
static void switchoverDataNodeForZone(MemoryContext spiContext,
										char *newMasterName, 
										bool forceSwitch, 
										int maxTrys,
										char *curZone,
										ZoneOverGtm *zoGtm, 
										ZoneOverDN *zoDN);										
static void FailOverGtmCoordMasterForZone(MemoryContext spiContext,
											char 	*oldMasterName,
											char  	*currentZone,
											bool 	forceSwitch,
											int 	maxTrys,
											bool 	kickOutOldMaster,
											ZoneOverGtm *zoGtm);
static void FailOverCoordMasterForZone(MemoryContext spiContext,
										char 	*oldMasterName,
										char 	*curZone,
										bool 	forceSwitch,
										bool 	kickOutOldMaster,
										int 	maxTrys,
										ZoneOverGtm *zoGtm,
										ZoneOverCoord *zoCoord);										
static void FailOverDataNodeMasterForZone(MemoryContext spiContext,
										char *oldMasterName,
										char *curZone,
										bool forceSwitch,
										bool kickOutOldMaster,
										int 	maxTrys,
										ZoneOverGtm *zoGtm,
										ZoneOverDN *zoDN);	
static void InitZoneOverCoord(ZoneOverCoord *zoCoord);																				
static void InitZoneoverDN(ZoneOverDN *zoDN);
static void GetSwitchSlaveByMaster(MemoryContext spiContext, 
									MgrNodeWrapper *mgrNode, 
									char *zone, 
									char type,
									NameData *newDataName);
static NodeRunningMode getNodeRunningModeEx(SwitcherNodeWrapper *holdLockNode, 
											SwitcherNodeWrapper *executeOnNode);	
static void SetXcMaintenanceModeOn(PGconn *pgConn);
static void CheckpointCoord(PGconn *pgConn,
							SwitcherNodeWrapper *node);
static void DelNodeFromSwitcherNodeWrappers(dlist_head  *nodes, 
											SwitcherNodeWrapper *delNode);
static void RevertZoneSwitchoverCoord(MemoryContext spiContext,
									ZoneOverGtm *zoGtm,
									SwitcherNodeWrapper *oldMaster,
									SwitcherNodeWrapper *newMaster);
static void RevertZoneOverDataNodes(MemoryContext spiContext, 
									ZoneOverGtm *zoGtm, 
									dlist_head *zoDNList,
									char *overType);
static void RevertZoneOverCoords(MemoryContext spiContext, 
								ZoneOverGtm *zoGtm,
								dlist_head *zoCoordList,
								char *overType);									
static void RevertZoneSwitchOverGtm(MemoryContext spiContext, 
									ZoneOverGtm *zoGtm);
static void RevertZoneFailOverDataNode(MemoryContext spiContext, 
										ZoneOverGtm *zoGtm, 
										ZoneOverDN	*zoDN);																		
static void RevertZoneFailOverCoord(MemoryContext spiContext,
									ZoneOverGtm *zoGtm,
									SwitcherNodeWrapper *oldMaster,
									SwitcherNodeWrapper *newMaster);
static void RevertZoneFailOverGtm(MemoryContext spiContext,
								ZoneOverGtm *zoGtm,
								dlist_head *zoCoordList);
static void RefreshOldNodeWrapperConfig(SwitcherNodeWrapper *oldMaster,
										SwitcherNodeWrapper *newMaster);
static void SetGtmInfoToList(dlist_head *nodes, SwitcherNodeWrapper *gtmMaster);
static void RevertFailOverShutdownCoords(MemoryContext spiContext, 
										 dlist_head *zoCoordList);
static void checkGetCoordinatorsForZone(MemoryContext spiContext,
											SwitcherNodeWrapper *oldMaster,
											char *zone,
											char nodeType,
											dlist_head *coordinators);
static void refreshReplicationSlots(SwitcherNodeWrapper *oldMaster,
									SwitcherNodeWrapper *newMaster);
static PGconn *GetNodeConn(Form_mgr_node nodeIn,
							int newPort,
							int connectTimeout,
							MemoryContext spiContext);
static bool DeletePgxcNodeDataNodeByName(PGconn *pgConn, 
										char *nodeName, 
										bool complain);
static void PrintReplicationInfo(SwitcherNodeWrapper *masterNode);
static void PrintCoordReplicationInfo(dlist_head  *coordinators);
static void RewindSlaveToMaster(MemoryContext spiContext,
								SwitcherNodeWrapper *masterNode, 
								SwitcherNodeWrapper *slaveNode);
static void getRunningSlaveOfNewMaster(MemoryContext spiContext,
										MgrNodeWrapper *masterNode,
										dlist_head *runningSlaveOfNewMaster);
static void refreshSyncToAsync(MemoryContext spiContext, dlist_head *nodes);
static void restartNodes(dlist_head *nodes, bool complain);								
static void RestartCurzoneNodes(dlist_head *dataNodes,
								dlist_head *coordinators, 
								dlist_head *coordinatorSlaves, 
								dlist_head *runningSlaves, 
								bool complain);

#define CheckNodeInZone(mgrNode, curZone)															\
{																									\
	if (pg_strcasecmp(NameStr(mgrNode->form.nodezone), curZone) != 0)							    \
	{																								\
		ereport(ERROR, (errmsg("the node(%s) is not in current zone(%s), can't operator it.",	\
				NameStr(mgrNode->form.nodename), curZone)));									    \
	}																								\
}
								
/**
 * system function of failover datanode
 */
Datum mgr_failover_one_dn(PG_FUNCTION_ARGS)
{
	HeapTuple tup_result;
	char *nodename;
	bool force;
	NameData newMasterName = {{0}};

	nodename = PG_GETARG_CSTRING(0);
	force = PG_GETARG_BOOL(1);
	namestrcpy(&newMasterName, PG_GETARG_CSTRING(2));

	if (RecoveryInProgress())
		ereport(ERROR, (errmsg("cannot assign TransactionIds during recovery")));

	FailOverDataNodeMaster(nodename, force, true, &newMasterName, mgr_zone);
	tup_result = build_common_command_tuple(&newMasterName,
											true,
											"failover datanode success");
	return HeapTupleGetDatum(tup_result);
}

/**
 * system function of failover gtm node
 */
Datum mgr_failover_gtm(PG_FUNCTION_ARGS)
{
	HeapTuple tup_result;
	char *nodename;
	bool force;
	NameData newMasterName = {{0}};

	nodename = PG_GETARG_CSTRING(0);
	force = PG_GETARG_BOOL(1);
	namestrcpy(&newMasterName, PG_GETARG_CSTRING(2));

	if (RecoveryInProgress())
		ereport(ERROR,
				(errmsg("cannot assign TransactionIds during recovery")));

	FailOverGtmCoordMaster(nodename, force, true, &newMasterName, mgr_zone);

	tup_result = build_common_command_tuple(&newMasterName,
											true,
											"failover gtmcoord success");
	return HeapTupleGetDatum(tup_result);
}

Datum mgr_switchover_func(PG_FUNCTION_ARGS)
{
	HeapTuple tup_result;
	char nodetype;
	NameData nodeNameData;
	bool force;
	int maxTrys = 0;
	char message[NAMEDATALEN] = {0}; 

	/* get the input variable */
	nodetype = PG_GETARG_INT32(0);
	namestrcpy(&nodeNameData, PG_GETARG_CSTRING(1));
	force = PG_GETARG_INT32(2);
	maxTrys = PG_GETARG_INT32(3);

	if (maxTrys < 1)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("the value of maxTrys must be positive")));
	if (maxTrys < 10)
		maxTrys = 10;			 
	/* check the type */
	if (CNDN_TYPE_DATANODE_MASTER == nodetype ||
		CNDN_TYPE_GTM_COOR_MASTER == nodetype)
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("it is the %s, no need switchover",
						CNDN_TYPE_GTM_COOR_MASTER == nodetype ? "gtm master" : "datanode master")));
	}
	if (CNDN_TYPE_DATANODE_SLAVE == nodetype)
	{
		switchoverDataNode(NameStr(nodeNameData), force, mgr_zone, maxTrys);	
		strcpy(message, "switchover datanode success");		
	}
	else if (CNDN_TYPE_GTM_COOR_SLAVE == nodetype)
	{
		switchoverGtmCoord(NameStr(nodeNameData), force, mgr_zone, maxTrys);
		strcpy(message, "switchover gtmcoord success");
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("unknown node type : %c",
						nodetype)));
	}

	tup_result = build_common_command_tuple(&nodeNameData,
											true,
											message);
	return HeapTupleGetDatum(tup_result);
}
/**
 * If the switch is forced, it means that data loss can be tolerated,
 * Though it may cause data loss, but that is acceptable.
 * If not force, allow switching without losing any data, In other words,
 * all slave nodes must running normally, and then pick the one which 
 * hold the biggest wal lsn as the new master.
 */
void FailOverDataNodeMaster(char *oldMasterName,
						  bool forceSwitch,
						  bool kickOutOldMaster,
						  Name newMasterName,
						  char *curZone)
{
	int spiRes;
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	SwitcherNodeWrapper *gtmMaster = NULL;
	dlist_head coordinators = DLIST_STATIC_INIT(coordinators);
	dlist_head runningSlaves = DLIST_STATIC_INIT(runningSlaves);
	dlist_head failedSlaves = DLIST_STATIC_INIT(failedSlaves);
	dlist_head runningSlavesSecond = DLIST_STATIC_INIT(runningSlavesSecond);
	dlist_head failedSlavesSecond = DLIST_STATIC_INIT(failedSlavesSecond);
	MemoryContext oldContext;
	MemoryContext switchContext;
	MemoryContext spiContext;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	ErrorData *edata = NULL;
	SwitcherNodeWrapper *RevertOldMaster = NULL;
	SwitcherNodeWrapper *RevertNewMaster = NULL;
	bool	beginFailOver = false;

	oldContext = CurrentMemoryContext;
	switchContext = AllocSetContextCreate(oldContext,
										  "FailOverDataNodeMaster",
										  ALLOCSET_DEFAULT_SIZES);
	spiRes = SPI_connect();
	if (spiRes != SPI_OK_CONNECT)
	{
		ereport(ERROR,
				(errmsg("SPI_connect failed, connect return:%d",
						spiRes)));
	}
	spiContext = CurrentMemoryContext;
	MemoryContextSwitchTo(switchContext);

	ereport(LOG, (errmsg("------------- FailOverDataNodeMaster oldMasterName(%s) before -------------", oldMasterName)));				
	PrintMgrNodeList(spiContext);

	PG_TRY();
	{
		oldMaster = checkGetOldMaster(oldMasterName,
										CNDN_TYPE_DATANODE_MASTER,
										2,
										spiContext);
		CheckNodeInZone(oldMaster->mgrNode, curZone);								
		oldMaster->startupAfterException = false;
		PrintReplicationInfo(oldMaster);

		checkGetSlaveNodesRunningStatus(oldMaster,
										spiContext,
										(Oid)0,
										"",
										&failedSlaves,
										&runningSlaves);
		precheckPromotionNode(&runningSlaves, forceSwitch);	
		checkGetMasterCoordinators(spiContext,
								   &coordinators,
								   true, true);
		RevertOldMaster = oldMaster;
		chooseNewMasterNode(oldMaster,
							&newMaster,
							&runningSlaves,
							&failedSlaves,
							spiContext,
							forceSwitch,
							NameStr(*newMasterName),
							curZone);
		checkGetSlaveNodesRunningSecondStatus(oldMaster,
											spiContext,
											(Oid)newMaster->mgrNode->form.oid,
											&runningSlaves,
											&failedSlaves,
											&runningSlavesSecond,
											&failedSlavesSecond);
		validateFailedSlavesForSwitch(oldMaster->mgrNode,
									  newMaster->mgrNode,
									  &failedSlaves,
									  forceSwitch);
		namestrcpy(newMasterName, NameStr(newMaster->mgrNode->form.nodename));
		CHECK_FOR_INTERRUPTS();

		RevertNewMaster = newMaster;
		/**
		 * Switch datanode oldMaster to the standby running mode, 
		 * and switch datanode newMaster to the master running mode, 
		 * The runningSlaves follow this newMaster, and the other failedSlaves
		 * will update curestatus to followfail, then the operation of following
		 * new master will handed to node doctor(see adb_doctor_node_monitor.c). 
		 * If any error occurred, will complain.
		 */

		/* Prevent other doctor processes from manipulating these nodes simultaneously */
		refreshSlaveNodesBeforeSwitch(newMaster,
									  &runningSlaves,
									  &failedSlaves,
									  &runningSlavesSecond,
									  &failedSlavesSecond,
									  spiContext);

		gtmMaster = getGtmCoordMaster(&coordinators);
		Assert(gtmMaster);
		/* ensure gtm info is correct */
		setCheckGtmInfoInPGSqlConf(gtmMaster->mgrNode,
								   newMaster->mgrNode,
								   newMaster->pgConn,
								   true,
								   CHECK_GTM_INFO_SECONDS,
								   true);
		beginFailOver = true;

		refreshPgxcNodeBeforeSwitchDataNode(&coordinators);

		tryLockCluster(&coordinators);

		promoteNewMasterStartReign(oldMaster, newMaster);

		/* The better slave node is in front of the list */
		sortNodesByWalLsnDesc(&runningSlaves);
		runningSlavesFollowNewMaster(newMaster,
									 NULL,
									 &runningSlaves,
									 NULL,
									 spiContext,
									 OVERTYPE_FAILOVER,
									 curZone);
        
		holdLockCoordinator = getHoldLockCoordinator(&coordinators);
		if (!holdLockCoordinator)
			ereport(ERROR, (errmsg("System error, can not find a "
								   "coordinator that hode the cluster lock")));
		refreshPgxcNodesOfCoordinators(holdLockCoordinator,
									   &coordinators,
									   oldMaster,
									   newMaster);
		/* change pgxc_node on datanode master */
		refreshPgxcNodesOfNewDataNodeMaster(holdLockCoordinator,
											oldMaster,
											newMaster,
											true);
		refreshOldMasterAfterSwitch(oldMaster,
									newMaster,
									spiContext,
									kickOutOldMaster);
		refreshSlaveNodesAfterSwitch(newMaster,
		                             oldMaster, 
									 &runningSlaves,
									 &failedSlaves,
									 &runningSlavesSecond,
									 &failedSlavesSecond,
									 spiContext,
									 OVERTYPE_FAILOVER,
									 curZone);		
		refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
										newMaster->mgrNode,
										spiContext,
										kickOutOldMaster);
		tryUnlockCluster(&coordinators, true);

		ereportNoticeLog(errmsg("Switch the datanode master from %s to %s "
								"has been successfully completed",
								NameStr(oldMaster->mgrNode->form.nodename),
								NameStr(newMaster->mgrNode->form.nodename)));
	}
	PG_CATCH();
	{
		/* Save error info in our stmt_mcontext */
		MemoryContextSwitchTo(oldContext);
		edata = CopyErrorData();
		FlushErrorState();
		RevertFailOverData(spiContext,
							beginFailOver,
							&coordinators,
							RevertNewMaster,
							RevertOldMaster,
							&runningSlaves);
	}
	PG_END_TRY();

	ereport(LOG, (errmsg("------------- FailOverDataNodeMaster oldMasterName(%s) after -------------", oldMasterName)));			
	PrintMgrNodeList(spiContext);

	/* pfree data and close PGconn */
	pfreeSwitcherNodeWrapperList(&failedSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&runningSlaves, newMaster);
	pfreeSwitcherNodeWrapperList(&runningSlavesSecond, NULL);
	pfreeSwitcherNodeWrapperList(&failedSlavesSecond, NULL);
	pfreeSwitcherNodeWrapperList(&coordinators, NULL);
	pfreeSwitcherNodeWrapper(oldMaster);
	pfreeSwitcherNodeWrapper(newMaster);

	(void)MemoryContextSwitchTo(oldContext);
	MemoryContextDelete(switchContext);

	SPI_finish();

	if (edata)
		ReThrowError(edata);
}
static bool RevertFailoverDataOfPgxcNode(SwitcherNodeWrapper *holdLockCoordinator,
										dlist_head *coordinators,
										SwitcherNodeWrapper *oldMaster,
										SwitcherNodeWrapper *newMaster)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;
	bool execOk = true;

	if (newMaster != NULL && newMaster->pgxcNodeChanged)
	{
		if (updatePgxcNodeForSwitch(holdLockCoordinator,
									newMaster,
									newMaster,
									oldMaster,
									false))
		{
			newMaster->pgxcNodeChanged = false;
		}
		else
		{
			ereportNoticeLog(errmsg("%s revert pgxc_node failed",
							NameStr(newMaster->mgrNode->form.nodename)));
			execOk = false;
		}
	}

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (node->pgxcNodeChanged)
		{
			if (updatePgxcNodeForSwitch(holdLockCoordinator,
										node,
										oldMaster,
										newMaster,										
										false))
			{
				node->pgxcNodeChanged = false;
			}
			else
			{
				ereportNoticeLog(errmsg("%s revert pgxc_node failed",
								NameStr(node->mgrNode->form.nodename)));
				execOk = false;
			}
			pgxcPoolReloadOnNode(holdLockCoordinator,
									node,
									false);
		}
	}
	
	return execOk;
}								 
static void RevertFailOverData(MemoryContext spiContext,
								bool beginFailOver,
								dlist_head *coordinators,
								SwitcherNodeWrapper *oldMaster,
								SwitcherNodeWrapper *newMaster,
								dlist_head *runningSlaves)
{
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	bool execOk = true;

	CheckNull(coordinators);
	CheckNull(newMaster);
	ereportNoticeLog(errmsg("begin to revert failover"));

	if (oldMaster == NULL)
	{
		if (!makesure_node_is_running(&newMaster->mgrNode->form, newMaster->mgrNode->form.nodeport))
			callAgentStartNode(newMaster->mgrNode, false, false);
		return;
	}

	DelNodeFromSwitcherNodeWrappers(runningSlaves, newMaster);	
	/* stop the old master is avoid the double master in the cluster */
	if (makesure_node_is_running(&oldMaster->mgrNode->form, oldMaster->mgrNode->form.nodeport))
		shutdownNodeWithinSeconds(oldMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								false);
	
	callAgentStartNode(newMaster->mgrNode, false, false);
	if (beginFailOver && 
		makesure_node_is_running(&newMaster->mgrNode->form, newMaster->mgrNode->form.nodeport))
	{
		promoteNewMasterStartReign(oldMaster, newMaster); 

		ereportWarningLog(errmsg("please rewind %s to %s by hand.", 
					NameStr(oldMaster->mgrNode->form.nodename),
					NameStr(newMaster->mgrNode->form.nodename)));

        /* If oldMaster has promote to master, oldMaster follow to newMaster will be failed here, 
		 * because the lsn of oldMaster is bigger than newMaster, so should rewind oldMaster.
		 * but failover datanode willbe deal by doctor, and doctor is not stable.      
		
		if (!makesure_node_is_running(&oldMaster->mgrNode->form, oldMaster->mgrNode->form.nodeport))
			callAgentStartNode(oldMaster->mgrNode, true, false);

		appendSlaveNodeFollowMaster(newMaster->mgrNode,
									oldMaster->mgrNode,
									newMaster->pgConn,
									spiContext);
		*/
		runningSlavesFollowNewMaster(newMaster,
									NULL,
									runningSlaves,
									NULL,
									spiContext,
									NULL,
									NULL);
	}

	holdLockCoordinator = getHoldLockCoordinator(coordinators);
	execOk = RevertFailoverDataOfPgxcNode(holdLockCoordinator,
											coordinators,
											oldMaster,
											newMaster);

	if (!tryUnlockCluster(coordinators, false))
	{
		execOk = false;
	}
	if (execOk)
	{
		ereportNoticeLog(errmsg("revert cluster setting successfully completed"));
	}
	else
	{
		ereport(WARNING,
				(errmsg("revert cluster setting, but some operations failed")));
	}
	ereport(WARNING,
			(errmsg("An exception occurred during the switching operation, "
					"It is recommended to use command such as 'monitor all', "
					"'monitor ha' to check the failure point in the cluster "
					"first, and then retry the switching operation!!!")));
}
static void RevertSwitchverData(MemoryContext spiContext,
								bool beginSwitchOver,
								dlist_head *coordinators,
								SwitcherNodeWrapper *oldMaster,
								SwitcherNodeWrapper *newMaster,
								dlist_head *runningSlaves)
{
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	bool execOk = true;
	StringInfoData infosendmsg;
	GetAgentCmdRst getAgentCmdRst;
	MgrNodeWrapper *slaveMgrNode;

	CheckNull(coordinators);
	CheckNull(newMaster);
	CheckNull(oldMaster);

	initStringInfo(&(getAgentCmdRst.description));
	initStringInfo(&infosendmsg);

	if (beginSwitchOver)
	{
		if (makesure_node_is_running(&oldMaster->mgrNode->form, oldMaster->mgrNode->form.nodeport))
		{
			shutdownNodeWithinSeconds(oldMaster->mgrNode,
									SHUTDOWN_NODE_FAST_SECONDS,
									SHUTDOWN_NODE_IMMEDIATE_SECONDS,
									false);
			ClosePgConn(oldMaster->pgConn);					
		}

		if (!makesure_node_is_running(&newMaster->mgrNode->form, newMaster->mgrNode->form.nodeport))
			callAgentStartNode(newMaster->mgrNode, true, false);

		promoteNewMasterStartReign(oldMaster, newMaster);

		if (!makesure_node_is_running(&oldMaster->mgrNode->form, oldMaster->mgrNode->form.nodeport))
			callAgentStartNode(oldMaster->mgrNode, true, false);
		
		RewindSlaveToMaster(spiContext,
							newMaster,
							oldMaster);

		slaveMgrNode = oldMaster->mgrNode;
		mgr_add_parameters_pgsqlconf(slaveMgrNode->form.oid,
									slaveMgrNode->form.nodetype,
									slaveMgrNode->form.nodeport,
									&infosendmsg);
		mgr_add_parm(NameStr(slaveMgrNode->form.nodename),
							slaveMgrNode->form.nodetype,
							&infosendmsg);
		mgr_append_pgconf_paras_str_quotastr("pgxc_node_name",
												NameStr(slaveMgrNode->form.nodename),
												&infosendmsg);
		mgr_send_conf_parameters(AGT_CMD_CNDN_REFRESH_PGSQLCONF,
									slaveMgrNode->nodepath,
									&infosendmsg,
									slaveMgrNode->form.nodehost,
									&getAgentCmdRst);
		if (!getAgentCmdRst.ret)
		{
			ereport(ERROR,
					(errmsg("set %s parameters failed, %s",
							NameStr(slaveMgrNode->form.nodename),
							getAgentCmdRst.description.data)));
		}
		appendSlaveNodeFollowMaster(newMaster->mgrNode,
									oldMaster->mgrNode,
									newMaster->pgConn,
									spiContext);
		runningSlavesFollowNewMaster(newMaster,
									NULL,
									runningSlaves,
									NULL,
									spiContext,
									NULL,
									NULL);
	}

	holdLockCoordinator = getHoldLockCoordinator(coordinators);
	if (isDataNodeMgrNode(oldMaster->mgrNode->form.nodetype))
	{
		execOk = RevertFailoverDataOfPgxcNode(holdLockCoordinator,
												coordinators,
												oldMaster,
												newMaster);
		refreshReplicationSlots(oldMaster, newMaster);
	}
	else
	{
		DelNodeFromSwitcherNodeWrappers(coordinators, oldMaster);
		execOk = RevertFailoverDataOfPgxcNode(holdLockCoordinator,
											coordinators,
											oldMaster,
											newMaster);
	}

	if (!tryUnlockCluster(coordinators, false))
	{
		execOk = false;
	}
	if (execOk)
	{
		ereportNoticeLog(errmsg("revert cluster setting successfully completed"));
	}
	else
	{
		ereport(WARNING,
				(errmsg("revert cluster setting, but some operations failed")));
	}
	ereport(WARNING,
			(errmsg("An exception occurred during the switching operation, "
					"It is recommended to use command such as 'monitor all', "
					"'monitor ha' to check the failure point in the cluster "
					"first, and then retry the switching operation!!!")));
}

void FailOverCoordMaster(char *oldMasterName,
						  bool forceSwitch,
						  bool kickOutOldMaster,
						  Name newMasterName,
						  char *curZone)
{
	int spiRes;
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	SwitcherNodeWrapper *gtmMaster = NULL;
	dlist_head coordinators = DLIST_STATIC_INIT(coordinators);
	dlist_head runningSlaves = DLIST_STATIC_INIT(runningSlaves);
	dlist_head failedSlaves = DLIST_STATIC_INIT(failedSlaves);
	MemoryContext oldContext;
	MemoryContext switchContext;
	MemoryContext spiContext;
	ErrorData *edata = NULL;
	SwitcherNodeWrapper   *switchNode = NULL;
	dlist_mutable_iter 	    miter;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	
	oldContext = CurrentMemoryContext;
	switchContext = AllocSetContextCreate(oldContext,
										  "FailOverCoordMaster",
										  ALLOCSET_DEFAULT_SIZES);
	spiRes = SPI_connect();
	if (spiRes != SPI_OK_CONNECT)
	{
		ereport(ERROR,
				(errmsg("SPI_connect failed, connect return:%d",
						spiRes)));
	}
	spiContext = CurrentMemoryContext;
	MemoryContextSwitchTo(switchContext);

	ereport(LOG, (errmsg("------------- FailOverCoordMaster oldMasterName(%s) before -------------", oldMasterName)));				
	PrintMgrNodeList(spiContext);

	PG_TRY();
	{
		oldMaster = checkGetOldMaster(oldMasterName,
										CNDN_TYPE_COORDINATOR_MASTER,
										2,
										spiContext);
		oldMaster->startupAfterException = false;
		PrintReplicationInfo(oldMaster);

		checkGetSlaveNodesRunningStatus(oldMaster,
										spiContext,
										(Oid)0,
										"",
										&failedSlaves,
										&runningSlaves);
		checkGetMasterCoordinators(spiContext,
								   &coordinators,
								   true, true);
		chooseNewMasterNode(oldMaster,
							&newMaster,
							&runningSlaves,
							&failedSlaves,
							spiContext,
							forceSwitch,
							NameStr(*newMasterName),
							curZone);
		dlist_foreach_modify(miter, &coordinators)
		{
			switchNode = dlist_container(SwitcherNodeWrapper, link, miter.cur);
			Assert(switchNode);
			if (isSameNodeName(switchNode->mgrNode, oldMaster->mgrNode)){
				dlist_delete(miter.cur);
				break;
			}				
		}
		
		validateFailedSlavesForSwitch(oldMaster->mgrNode,
									  newMaster->mgrNode,
									  &failedSlaves,
									  forceSwitch);
		namestrcpy(newMasterName,  NameStr(newMaster->mgrNode->form.nodename));
		CHECK_FOR_INTERRUPTS();

		/* Prevent other doctor processes from manipulating these nodes simultaneously */
		refreshMgrNodeBeforeSwitch(newMaster, spiContext);
    	refreshMgrNodeListBeforeSwitch(spiContext, &runningSlaves);
		refreshMgrNodeListBeforeSwitch(spiContext, &failedSlaves);							  

		gtmMaster = getGtmCoordMaster(&coordinators);
		Assert(gtmMaster);
		setCheckGtmInfoInPGSqlConf(gtmMaster->mgrNode,
								   newMaster->mgrNode,
								   newMaster->pgConn,
								   true,
								   CHECK_GTM_INFO_SECONDS,
								   true);

		promoteNewMasterStartReign(oldMaster, newMaster);

		newMaster->mgrNode->form.nodetype =	getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
		dlist_push_tail(&coordinators, &newMaster->link);

		sortNodesByWalLsnDesc(&runningSlaves);
		runningSlavesFollowNewMaster(newMaster,
									 oldMaster,
									 &runningSlaves,
									 NULL,
									 spiContext,
									 OVERTYPE_FAILOVER,
									 curZone);

		refreshPgxcNodesOfCoordinators(holdLockCoordinator,
									   &coordinators,
									   oldMaster,
									   newMaster);

		refreshOldMasterAfterSwitch(oldMaster,
									newMaster,
									spiContext,
									kickOutOldMaster);
		refreshSlaveNodesAfterSwitch(newMaster,
								     oldMaster,
									 &runningSlaves,
									 &failedSlaves,
									 NULL,
 									 NULL,
									 spiContext,
									 OVERTYPE_FAILOVER,
									 curZone);
		
		refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
										newMaster->mgrNode,
										spiContext,
										kickOutOldMaster);

		RefreshPgxcNodeName(newMaster, NameStr(newMaster->mgrNode->form.nodename));							

		if (pg_strcasecmp(NameStr(oldMaster->mgrNode->form.nodezone), curZone) != 0)
		{
			ShutdownRunningNotZone(&runningSlaves, curZone);
			ShutdownRunningNotZone(&failedSlaves, curZone);
		}

		ereportNoticeLog((errmsg("Switch the coordinator master from %s to %s "
						"has been successfully completed",
						NameStr(oldMaster->mgrNode->form.nodename),
						NameStr(newMaster->mgrNode->form.nodename))));						
	}
	PG_CATCH();
	{
		MemoryContextSwitchTo(oldContext);
		edata = CopyErrorData();
		FlushErrorState();

		RevertPgxcNodeCoord(&coordinators, oldMaster, newMaster, true);
	}
	PG_END_TRY();

	ereport(LOG, (errmsg("------------- FailOverCoordMaster oldMasterName(%s) after -------------", oldMasterName)));			
	PrintMgrNodeList(spiContext);

	/* pfree data and close PGconn */
	pfreeSwitcherNodeWrapperList(&failedSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&runningSlaves, newMaster);
	pfreeSwitcherNodeWrapperList(&coordinators, newMaster);
	pfreeSwitcherNodeWrapper(oldMaster);
	pfreeSwitcherNodeWrapper(newMaster);

	(void)MemoryContextSwitchTo(oldContext);
	MemoryContextDelete(switchContext);

	SPI_finish();

	if (edata)
		ReThrowError(edata);
}

void FailOverGtmCoordMaster(char *oldMasterName,
						  bool forceSwitch,
						  bool kickOutOldMaster,
						  Name newMasterName,
						  char* curZone)
{
	int spiRes;
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head coordinators = DLIST_STATIC_INIT(coordinators);
	dlist_head coordinatorSlaves = DLIST_STATIC_INIT(coordinatorSlaves);
	dlist_head runningSlaves = DLIST_STATIC_INIT(runningSlaves);
	dlist_head failedSlaves = DLIST_STATIC_INIT(failedSlaves);
	dlist_head runningSlavesSecond = DLIST_STATIC_INIT(runningSlavesSecond);
	dlist_head failedSlavesSecond = DLIST_STATIC_INIT(failedSlavesSecond);
	dlist_head dataNodes = DLIST_STATIC_INIT(dataNodes);
	MemoryContext oldContext;
	MemoryContext switchContext;
	MemoryContext spiContext;
	ErrorData *edata = NULL; 
	dlist_head isolatedNodes = DLIST_STATIC_INIT(isolatedNodes);
	SwitcherNodeWrapper *RevertOldMaster = NULL;
	SwitcherNodeWrapper *RevertNewMaster = NULL;
	bool				beginFailOver = false;
	bool				complain = true;

    oldContext = CurrentMemoryContext;
	switchContext = AllocSetContextCreate(oldContext,
										  "FailOverGtmCoordMaster",
										  ALLOCSET_DEFAULT_SIZES);
    spiRes = SPI_connect();
	if (spiRes != SPI_OK_CONNECT)
	{
		ereport(ERROR,
				(errmsg("SPI_connect failed, connect return:%d",
						spiRes)));
	}
	spiContext = CurrentMemoryContext;
	MemoryContextSwitchTo(switchContext);

	ereport(LOG, (errmsg("------------- FailOverGtmCoordMaster oldMasterName(%s) before -------------", oldMasterName)));			
	PrintMgrNodeList(spiContext);

	PG_TRY();
	{
		oldMaster = checkGetGtmCoordOldMaster(oldMasterName,
											  2,
											  spiContext);
		CheckNodeInZone(oldMaster->mgrNode, curZone);									  
		oldMaster->startupAfterException = false;
		PrintReplicationInfo(oldMaster);

		checkGetSlaveNodesRunningStatus(oldMaster,
										spiContext,
										(Oid)0,
										"",
										&failedSlaves,
										&runningSlaves);
		precheckPromotionNode(&runningSlaves, forceSwitch);								
		checkGetMasterCoordinators(spiContext,
								   &coordinators,
								   false, false);							   
	    checkGetSlaveCoordinators(spiContext,
								&coordinatorSlaves,
								false);
		checkGetAllDataNodes(&dataNodes, spiContext);
		RevertOldMaster = oldMaster;
		chooseNewMasterNode(oldMaster,
							&newMaster,
							&runningSlaves,
							&failedSlaves,
							spiContext,
							forceSwitch,
							NameStr(*newMasterName),
							curZone);
		checkGetSlaveNodesRunningSecondStatus(oldMaster,
											spiContext,
											(Oid)newMaster->mgrNode->form.oid,
											&runningSlaves,
											&failedSlaves,
											&runningSlavesSecond,
											&failedSlavesSecond);					
		validateFailedSlavesForSwitch(oldMaster->mgrNode,
									  newMaster->mgrNode,
									  &failedSlaves,
									  forceSwitch);
		namestrcpy(newMasterName, NameStr(newMaster->mgrNode->form.nodename));        
		CHECK_FOR_INTERRUPTS();

		RevertNewMaster = newMaster;
		/**
		 * Switch gtm coordinator oldMaster to the standby running mode, 
		 * and switch gtm coordinator newMaster to the master running mode, 
		 * The runningSlaves follow this newMaster, and the other failedSlaves
		 * will update curestatus to followfail, then the operation of following 
		 * new master will handed to node doctor(see adb_doctor_node_monitor.c). 
		 * If any error occurred, will complain.
		 */

		/* Prevent other doctor processes from manipulating these nodes simultaneously */
		refreshSlaveNodesBeforeSwitch(newMaster,
									  &runningSlaves,
									  &failedSlaves,
									  &runningSlavesSecond,
 									  &failedSlavesSecond,
									  spiContext);
		refreshMgrNodeListBeforeSwitch(spiContext, &coordinators);
		refreshMgrNodeListBeforeSwitch(spiContext, &coordinatorSlaves);
		refreshMgrNodeListBeforeSwitch(spiContext, &dataNodes);							  
		
		setCheckGtmInfoInPGSqlConf(newMaster->mgrNode,
								   newMaster->mgrNode,
								   newMaster->pgConn,
								   true,
								   CHECK_GTM_INFO_SECONDS,
								   complain);
		newMaster->gtmInfoChanged = true;
		beginFailOver = true;

		RefreshGtmAdbCheckSyncNextid(newMaster->mgrNode, ADB_CHECK_SYNC_NEXTID_OFF);
		promoteNewMasterStartReign(oldMaster, newMaster);

		/* newMaster also is a coordinator */
		newMaster->mgrNode->form.nodetype =	getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
		dlist_push_head(&coordinators, &newMaster->link);

		/* The better slave node is in front of the list */
		sortNodesByWalLsnDesc(&runningSlaves);
		runningSlavesFollowNewMaster(newMaster,
									 NULL,
									 &runningSlaves,
									 NULL,
									 spiContext,
									 OVERTYPE_FAILOVER,
									 curZone);

		ereportNoticeLog(errmsg("set gtmhost and gtmport to every node, please wait for a moment..."));	
		batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,
									&dataNodes,
									newMaster,
									complain);						 
		batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,
									&coordinators,
									newMaster,
									complain);
		batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,
									&coordinatorSlaves,
									newMaster,
									complain);
		refreshPgxcNodesOfCoordinators(NULL,
									   &coordinators,
									   oldMaster,
									   newMaster);

		/* isolated node in pgxc_node would block the cluster */
		selectIsolatedMgrNodes(spiContext, &isolatedNodes);
		cleanMgrNodesOnCoordinator(&isolatedNodes,
								   newMaster->mgrNode,
								   newMaster->pgConn,
								   complain);
		pfreeMgrNodeWrapperList(&isolatedNodes, NULL);

		/* 
		 * Save the cluster status after the switch operation to the mgr_node 
		 * table and correct the gtm information in datanodes. To ensure 
		 * consistency, lock the cluster.
		 */
		tryLockCluster(&coordinators);
		
		refreshOldMasterAfterSwitch(oldMaster,
									newMaster,
									spiContext,
									kickOutOldMaster);
		refreshSlaveNodesAfterSwitch(newMaster,
								     oldMaster,			
									 &runningSlaves,
									 &failedSlaves,
									 &runningSlavesSecond,
 									 &failedSlavesSecond,
									 spiContext,
									 OVERTYPE_FAILOVER,
									 curZone);
		refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
										newMaster->mgrNode,
										spiContext,
										kickOutOldMaster);
		refreshMgrNodeListAfterFailoverGtm(spiContext, &coordinators);
		refreshMgrNodeListAfterFailoverGtm(spiContext, &coordinatorSlaves);
		refreshMgrNodeListAfterFailoverGtm(spiContext, &dataNodes);

		tryUnlockCluster(&coordinators, complain);

		ereportNoticeLog(errmsg("Switch the GTM master from %s to %s "
						"has been successfully completed",
						NameStr(oldMaster->mgrNode->form.nodename),
						NameStr(newMaster->mgrNode->form.nodename)));
	}
	PG_CATCH();
	{
		/* Save error info in our stmt_mcontext */
		MemoryContextSwitchTo(oldContext);
		edata = CopyErrorData();
		FlushErrorState();
		revertGtmInfoSetting(RevertNewMaster,
							RevertOldMaster, 
							&coordinators, 
							&coordinatorSlaves,
							&dataNodes);
		RevertFailOverData(spiContext,
							beginFailOver,
							&coordinators,
							RevertNewMaster,
							RevertOldMaster,
							&runningSlaves);			
	}
	PG_END_TRY();

	ereport(LOG, (errmsg("------------- FailOverGtmCoordMaster oldMasterName(%s) after -------------", oldMasterName)));			
	PrintMgrNodeList(spiContext);

	if (newMaster != NULL)
		RefreshGtmAdbCheckSyncNextid(newMaster->mgrNode, ADB_CHECK_SYNC_NEXTID_ON);
	
	/* pfree data and close PGconn */
	pfreeSwitcherNodeWrapperList(&failedSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&runningSlaves, newMaster);
	pfreeSwitcherNodeWrapperList(&runningSlavesSecond, NULL);
	pfreeSwitcherNodeWrapperList(&failedSlavesSecond, NULL);
	/* newMaster may be added in coordinators */
	pfreeSwitcherNodeWrapperList(&coordinators, newMaster);
	pfreeSwitcherNodeWrapperList(&coordinatorSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&dataNodes, NULL);
	pfreeSwitcherNodeWrapper(oldMaster);
	pfreeSwitcherNodeWrapper(newMaster);

	(void)MemoryContextSwitchTo(oldContext);
	MemoryContextDelete(switchContext);

	SPI_finish();
	if (edata)
		ReThrowError(edata);	
}
void switchoverDataNode(char *newMasterName, bool forceSwitch, char *curZone, int maxTrys)
{
	int spiRes;
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head coordinators = DLIST_STATIC_INIT(coordinators);
	dlist_head runningSlaves = DLIST_STATIC_INIT(runningSlaves);
	dlist_head runningSlavesSecond = DLIST_STATIC_INIT(runningSlavesSecond);
	dlist_head failedSlaves = DLIST_STATIC_INIT(failedSlaves);
	dlist_head failedSlavesSecond = DLIST_STATIC_INIT(failedSlavesSecond);
	dlist_head runningSlaveOfNewMaster = DLIST_STATIC_INIT(runningSlaveOfNewMaster);  /* the running slave of new Master  */
	MemoryContext oldContext;
	MemoryContext switchContext;
	MemoryContext spiContext;
	ErrorData *edata = NULL;
	SwitcherNodeWrapper *gtmMaster = NULL;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	SwitcherNodeWrapper *RevertOldMaster = NULL;
	SwitcherNodeWrapper *RevertNewMaster = NULL;
	bool beginSwitchOver = false;

	oldContext = CurrentMemoryContext;
	switchContext = AllocSetContextCreate(oldContext,
										  "switchoverDataNode",
										  ALLOCSET_DEFAULT_SIZES);
	spiRes = SPI_connect();
	if (spiRes != SPI_OK_CONNECT)
	{
		ereport(ERROR,
				(errmsg("SPI_connect failed, connect return:%d",
						spiRes)));
	}
	spiContext = CurrentMemoryContext;
	MemoryContextSwitchTo(switchContext);

	ereport(LOG, (errmsg("------------- switchoverDataNode newMasterName(%s) before -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);

	PG_TRY();
	{
		newMaster = checkGetSwitchoverNewMaster(newMasterName,
												CNDN_TYPE_DATANODE_SLAVE,
												forceSwitch,
												spiContext);
		CheckNodeInZone(newMaster->mgrNode, curZone);

		oldMaster = checkGetSwitchoverOldMaster(newMaster->mgrNode->form.nodemasternameoid,
												CNDN_TYPE_DATANODE_MASTER,
												spiContext);
		CheckNodeInZone(oldMaster->mgrNode, curZone);

		oldMaster->startupAfterException = true;
		PrintReplicationInfo(oldMaster);

		checkGetSlaveNodesRunningStatus(oldMaster,
										spiContext,
										newMaster->mgrNode->form.oid,
										"",
										&failedSlaves,
										&runningSlaves);		
		checkGetSlaveNodesRunningSecondStatus(oldMaster,
											spiContext,
											newMaster->mgrNode->form.oid,
											&runningSlaves,
											&failedSlaves,
											&runningSlavesSecond,
											&failedSlavesSecond);
		getRunningSlaveOfNewMaster(spiContext,
									newMaster->mgrNode,
						 			&runningSlaveOfNewMaster);											
		validateFailedSlavesForSwitch(oldMaster->mgrNode,
									  newMaster->mgrNode,
									  &failedSlaves,
									  forceSwitch);
		checkGetMasterCoordinators(spiContext,
								   &coordinators,
								   true, true);
	
		checkTrackActivitiesForSwitchover(&coordinators,
										  oldMaster);

		/* check interrupt before lock the cluster */
		CHECK_FOR_INTERRUPTS();

		RevertOldMaster = oldMaster;
		RevertNewMaster = newMaster;

		refreshPgxcNodeBeforeSwitchDataNode(&coordinators);	
		if (forceSwitch)
		{
			tryLockCluster(&coordinators);
			holdLockCoordinator = getHoldLockCoordinator(&coordinators);
			Assert(holdLockCoordinator);
			MgrSendFinishActiveBackendToGtm(holdLockCoordinator->pgConn);
		}
		else
		{
			checkActiveConnectionsForSwitchover(&coordinators,
												oldMaster,
												maxTrys);
		}
		checkActiveLocksForSwitchover(&coordinators, NULL, oldMaster, maxTrys);
		checkXlogDiffForSwitchover(oldMaster,
								   newMaster,
								   maxTrys);
		CHECK_FOR_INTERRUPTS();		
		/* Prevent doctor process from manipulating this node simultaneously. */
		refreshOldMasterBeforeSwitch(oldMaster, spiContext);

		/* Prevent doctor processes from manipulating these nodes simultaneously. */
		refreshSlaveNodesBeforeSwitch(newMaster,
									  &runningSlaves,
									  &failedSlaves,
									  &runningSlavesSecond,
 									  &failedSlavesSecond,
									  spiContext);
		beginSwitchOver = true;	
		oldMaster->startupAfterException = (oldMaster->runningMode == NODE_RUNNING_MODE_MASTER &&
			 								newMaster->runningMode == NODE_RUNNING_MODE_SLAVE);
		shutdownNodeWithinSeconds(oldMaster->mgrNode,
								  SHUTDOWN_NODE_FAST_SECONDS,
								  SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								  true);
		/* ensure gtm info is correct */
		gtmMaster = getGtmCoordMaster(&coordinators);
		Assert(gtmMaster);
		setCheckGtmInfoInPGSqlConf(gtmMaster->mgrNode,
								   newMaster->mgrNode,
								   newMaster->pgConn,
								   true,
								   CHECK_GTM_INFO_SECONDS,
								   true);

		promoteNewMasterStartReign(oldMaster, newMaster);

		appendSlaveNodeFollowMasterForSwitchOver(spiContext,
												newMaster,
												oldMaster, 
												true);
		runningSlavesFollowNewMaster(newMaster, 
									NULL,
									&runningSlaves,
									NULL,
									spiContext,
									OVERTYPE_SWITCHOVER,
									curZone);

		holdLockCoordinator = getHoldLockCoordinator(&coordinators);
		if (!holdLockCoordinator)
			ereport(ERROR, (errmsg("System error, can not find a "
								   "coordinator that hode the cluster lock")));					   
		refreshPgxcNodesOfCoordinators(holdLockCoordinator,
									   &coordinators,
									   oldMaster,
									   newMaster);
		/* change pgxc_node on datanode master */
		refreshPgxcNodesOfNewDataNodeMaster(holdLockCoordinator,
											oldMaster,
											newMaster,
											true);
		refreshOldMasterAfterSwitchover(oldMaster,
										newMaster,
										spiContext);
		refreshSlaveNodesAfterSwitch(newMaster,
								     oldMaster,	
									 &runningSlaves,
									 &failedSlaves,
									 &runningSlavesSecond,
 									 &failedSlavesSecond,
									 spiContext,
									 OVERTYPE_SWITCHOVER,
									 curZone);
		refreshReplicationSlots(oldMaster, 
								newMaster);							 
		refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
										newMaster->mgrNode,
										spiContext,
										false);
		tryUnlockCluster(&coordinators, true);

		ereportNoticeLog(errmsg("Switch the datanode master from %s to %s "
						"has been successfully completed",
						NameStr(oldMaster->mgrNode->form.nodename),
						NameStr(newMaster->mgrNode->form.nodename)));
	}
	PG_CATCH();
	{
		/* Save error info in our stmt_mcontext */
		MemoryContextSwitchTo(oldContext);
		edata = CopyErrorData();
		FlushErrorState();
		ereportNoticeLog(errmsg("revert switchover datanode begin."));
		RevertSwitchverData(spiContext,
							beginSwitchOver,
							&coordinators,
							RevertNewMaster,
							RevertOldMaster,
							&runningSlaves);
		ereportNoticeLog(errmsg("revert switchover datanode end."));	
	}
	PG_END_TRY();

	ereport(LOG, (errmsg("------------- switchoverDataNode newMasterName(%s) after -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);

	/* pfree data and close PGconn */
	pfreeSwitcherNodeWrapperList(&failedSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&runningSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&runningSlavesSecond, NULL);
	pfreeSwitcherNodeWrapperList(&failedSlavesSecond, NULL);
	pfreeSwitcherNodeWrapperList(&coordinators, NULL);
	pfreeSwitcherNodeWrapper(oldMaster);
	pfreeSwitcherNodeWrapper(newMaster);

	(void)MemoryContextSwitchTo(oldContext);
	MemoryContextDelete(switchContext);

	SPI_finish();

	if (edata)
		ReThrowError(edata);
}

void switchoverGtmCoord(char *newMasterName, bool forceSwitch, char *curZone, int maxTrys)
{
	int spiRes;
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head coordinators = DLIST_STATIC_INIT(coordinators);
	dlist_head coordinatorSlaves = DLIST_STATIC_INIT(coordinatorSlaves);
	dlist_head runningSlaves = DLIST_STATIC_INIT(runningSlaves);
	dlist_head runningSlavesSecond = DLIST_STATIC_INIT(runningSlavesSecond);
	dlist_head failedSlaves = DLIST_STATIC_INIT(failedSlaves);
	dlist_head failedSlavesSecond = DLIST_STATIC_INIT(failedSlavesSecond);
	dlist_head dataNodes = DLIST_STATIC_INIT(dataNodes);
	dlist_head isolatedNodes = DLIST_STATIC_INIT(isolatedNodes);
	MemoryContext oldContext;
	MemoryContext switchContext;
	MemoryContext spiContext;
	ErrorData *edata = NULL;
	dlist_mutable_iter miter;
	SwitcherNodeWrapper *node;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	SwitcherNodeWrapper *RevertOldMaster = NULL;
	SwitcherNodeWrapper *RevertNewMaster = NULL;
	bool beginSwitchOver = false;

	oldContext = CurrentMemoryContext;
	switchContext = AllocSetContextCreate(oldContext,
										  "switchoverGtmCoord",
										  ALLOCSET_DEFAULT_SIZES);
	spiRes = SPI_connect();
	if (spiRes != SPI_OK_CONNECT)
	{
		ereport(ERROR,
				(errmsg("SPI_connect failed, connect return:%d",
						spiRes)));
	}
	spiContext = CurrentMemoryContext;
	MemoryContextSwitchTo(switchContext);

	ereport(LOG, (errmsg("------------- switchoverGtmCoord newMasterName(%s) before -------------", newMasterName)));				
	PrintMgrNodeList(spiContext);

	PG_TRY();
	{
		newMaster = checkGetSwitchoverNewMaster(newMasterName,
												CNDN_TYPE_GTM_COOR_SLAVE,
												forceSwitch,
												spiContext);
		CheckNodeInZone(newMaster->mgrNode, curZone);										
											
		oldMaster = checkGetSwitchoverOldMaster(newMaster->mgrNode->form.nodemasternameoid,
												CNDN_TYPE_GTM_COOR_MASTER,
												spiContext);
		CheckNodeInZone(oldMaster->mgrNode, curZone);

		oldMaster->startupAfterException = false;
		PrintReplicationInfo(oldMaster);

		checkGetSlaveNodesRunningStatus(oldMaster,
										spiContext,
										newMaster->mgrNode->form.oid,
										"",
										&failedSlaves,
										&runningSlaves);
		checkGetSlaveNodesRunningSecondStatus(oldMaster,
											spiContext,
											newMaster->mgrNode->form.oid,
											&runningSlaves,
											&failedSlaves,
											&runningSlavesSecond,
											&failedSlavesSecond);
		validateFailedSlavesForSwitch(oldMaster->mgrNode,
									  newMaster->mgrNode,
									  &failedSlaves,
									  forceSwitch);
		checkGetMasterCoordinators(spiContext,
								   &coordinators,
								   false, false);
		checkGetSlaveCoordinators(spiContext,
									&coordinatorSlaves,
									false);						   
		checkGetAllDataNodes(&dataNodes, spiContext);
		checkTrackActivitiesForSwitchover(&coordinators, oldMaster);
		checkTrackActivitiesForSwitchover(&coordinatorSlaves, oldMaster);								  
		/* oldMaster also is a coordinator */
		dlist_push_head(&coordinators, &oldMaster->link);

		/* check interrupt before lock the cluster */
		CHECK_FOR_INTERRUPTS();
		RevertOldMaster = oldMaster;
		RevertNewMaster = newMaster;

		if (forceSwitch)
		{
			tryLockCluster(&coordinators);
		}
		else
		{
			checkActiveConnectionsForSwitchover(&coordinators,
												oldMaster,
												maxTrys);												
		}
		checkActiveLocksForSwitchover(&coordinators, NULL,
									  oldMaster,
									  maxTrys);
		checkXlogDiffForSwitchover(oldMaster,
								   newMaster,
								   maxTrys);
		CHECK_FOR_INTERRUPTS();

		/* Prevent doctor process from manipulating this node simultaneously. */
		refreshOldMasterBeforeSwitch(oldMaster,
									 spiContext);
		RefreshMgrNodesBeforeSwitchGtmCoord(spiContext, 
											newMaster,
											oldMaster,
											&runningSlaves,
											&failedSlaves,
											&runningSlavesSecond,
											&failedSlavesSecond,
											&coordinators,
											&coordinatorSlaves,
											&dataNodes);
		beginSwitchOver = true;									
		oldMaster->startupAfterException = (oldMaster->runningMode == NODE_RUNNING_MODE_MASTER &&
			 								newMaster->runningMode == NODE_RUNNING_MODE_SLAVE);
		shutdownNodeWithinSeconds(oldMaster->mgrNode,
								  SHUTDOWN_NODE_FAST_SECONDS,
								  SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								  true);
		if (oldMaster->holdClusterLock)
		{
			/* I am already dead. If I hold a cluster lock, I will automatically give up. */
			oldMaster->holdClusterLock = false;
			ereportNoticeLog(errmsg("%s has been shut down and the cluster is unlocked",
							NameStr(oldMaster->mgrNode->form.nodename)));
		}
		ClosePgConn(oldMaster->pgConn); // test error
		/* Delete the oldMaster, it is not a coordinator now. */
		dlist_foreach_modify(miter, &coordinators) 
		{
			node = dlist_container(SwitcherNodeWrapper, link, miter.cur);
			if (node == oldMaster)
			{
				dlist_delete(miter.cur);
				break;
			}
		}
		/* 
		 * It is impossible for a node other than the old gtm coord master to 
		 * have a cluster lock, but try to unlock the cluster anyway... 
		 */
		tryUnlockCluster(&coordinators, true);

		setCheckGtmInfoInPGSqlConf(newMaster->mgrNode,   // test error
								   newMaster->mgrNode,
								   newMaster->pgConn,
								   true,
								   CHECK_GTM_INFO_SECONDS,
								   true);
		newMaster->gtmInfoChanged = true;

		RefreshGtmAdbCheckSyncNextid(newMaster->mgrNode, ADB_CHECK_SYNC_NEXTID_OFF);

		promoteNewMasterStartReign(oldMaster, newMaster);

		/* newMaster also is a coordinator */
		newMaster->mgrNode->form.nodetype =	getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
		dlist_push_head(&coordinators, &newMaster->link);
		runningSlavesFollowNewMaster(newMaster,
									NULL,
									&runningSlaves,
									newMaster->mgrNode,
									spiContext,
									OVERTYPE_SWITCHOVER,
									curZone);	

		ereportNoticeLog(errmsg("set gtmhost, gtmport to every node, please wait for a moment."));
		batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,	&dataNodes,	NULL, false);
		batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, &coordinators, newMaster, true);
		batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, &coordinatorSlaves, NULL, false);
		batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, &runningSlavesSecond, NULL, false);
		batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, &failedSlavesSecond, NULL, false);

		/* isolated node in pgxc_node would block the cluster */
		selectIsolatedMgrNodes(spiContext, &isolatedNodes);
		cleanMgrNodesOnCoordinator(&isolatedNodes,
								   newMaster->mgrNode,
								   newMaster->pgConn,
								   true);

		/* 
		 * Save the cluster status after the switch operation to the mgr_node 
		 * table and correct the gtm information in datanodes. To ensure 
		 * consistency, lock the cluster.
		 */
		tryLockCluster(&coordinators);
		holdLockCoordinator = getHoldLockCoordinator(&coordinators);

		refreshPgxcNodesOfCoordinators(holdLockCoordinator,
									   &coordinators,
									   oldMaster,
									   newMaster);
		appendSlaveNodeFollowMaster(newMaster->mgrNode,
									oldMaster->mgrNode,
									newMaster->pgConn,
									spiContext);
		refreshOldMasterAfterSwitchover(oldMaster,
										newMaster,
										spiContext);
		refreshSlaveNodesAfterSwitch(newMaster,
									 oldMaster, 	
									 &runningSlaves,
									 &failedSlaves,
									 &runningSlavesSecond,
 									 &failedSlavesSecond,
									 spiContext,
									 OVERTYPE_SWITCHOVER,
									 curZone);
		RefreshMgrNodesAfterSwitchGtmCoord(spiContext, 
											&coordinators,
											&coordinatorSlaves, 
											&dataNodes,
											newMaster);
		refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
										newMaster->mgrNode,
										spiContext,
										false);
		tryUnlockCluster(&coordinators, true);

		ereportNoticeLog((errmsg("Switchover the GTM master from %s to %s "
						"has been successfully completed",
						NameStr(oldMaster->mgrNode->form.nodename),
						NameStr(newMaster->mgrNode->form.nodename))));
	}
	PG_CATCH();
	{
		/* Save error info in our stmt_mcontext */
		MemoryContextSwitchTo(oldContext);
		edata = CopyErrorData();
		FlushErrorState();
		ereportNoticeLog(errmsg("revert switchover gtm coordinator begin"));
		revertGtmInfoSetting(RevertNewMaster, 
							RevertOldMaster, 
							&coordinators,
							&coordinatorSlaves, 
							&dataNodes);
		RevertSwitchverData(spiContext,
							beginSwitchOver,
							&coordinators,
							RevertNewMaster,
							RevertOldMaster,
							&runningSlaves);
		ereportNoticeLog(errmsg("revert switchover gtm coordinator end"));							
	}
	PG_END_TRY();

	ereport(LOG, (errmsg("------------- switchoverGtmCoord newMasterName(%s) after -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);

	if (newMaster != NULL)
		RefreshGtmAdbCheckSyncNextid(newMaster->mgrNode, ADB_CHECK_SYNC_NEXTID_ON);
	pfreeSwitcherNodeWrapperList(&failedSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&failedSlavesSecond, NULL);
	pfreeSwitcherNodeWrapperList(&runningSlavesSecond, NULL);
	pfreeSwitcherNodeWrapperList(&runningSlaves, newMaster);	
	/* oldMaster or newMaster may be added in coordinators */
	dlist_foreach_modify(miter, &coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, miter.cur);
		dlist_delete(miter.cur);
		if (node != oldMaster && node != newMaster)
		{
			pfreeSwitcherNodeWrapper(node);
		}
	}
	pfreeSwitcherNodeWrapperList(&coordinatorSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&dataNodes, NULL);
	pfreeSwitcherNodeWrapper(oldMaster);
	pfreeSwitcherNodeWrapper(newMaster);
	pfreeMgrNodeWrapperList(&isolatedNodes, NULL);

	(void)MemoryContextSwitchTo(oldContext);
	MemoryContextDelete(switchContext);

	SPI_finish();

	if (edata)
		ReThrowError(edata);
}
void switchoverCoord(char *newMasterName, bool forceSwitch, char *curZone)
{
	int spiRes;
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head coordinators = DLIST_STATIC_INIT(coordinators);
	dlist_head runningSlaves = DLIST_STATIC_INIT(runningSlaves);
	dlist_head failedSlaves = DLIST_STATIC_INIT(failedSlaves);
	MemoryContext oldContext;
	MemoryContext switchContext;
	MemoryContext spiContext;
	ErrorData *edata = NULL;
	dlist_mutable_iter miter;
	SwitcherNodeWrapper *node;
	SwitcherNodeWrapper *gtmMaster = NULL;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	int maxTrys = 10;

	oldContext = CurrentMemoryContext;
	switchContext = AllocSetContextCreate(oldContext,
										  "switchoverCoord",
										  ALLOCSET_DEFAULT_SIZES);
	spiRes = SPI_connect();
	if (spiRes != SPI_OK_CONNECT)
	{
		ereport(ERROR,
				(errmsg("SPI_connect failed, connect return:%d",
						spiRes)));
	}
	spiContext = CurrentMemoryContext;
	MemoryContextSwitchTo(switchContext);

	ereport(LOG, (errmsg("------------- switchoverCoord newMasterName(%s) before -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);

	PG_TRY();
	{
		newMaster = checkGetSwitchoverNewMaster(newMasterName,
												CNDN_TYPE_COORDINATOR_SLAVE,
												forceSwitch,
												spiContext);
		if (pg_strcasecmp(NameStr(newMaster->mgrNode->form.nodezone), curZone) != 0)
		{
			ereport(ERROR, (errmsg("the new coord(%s) is not in current zone(%s), can't switchover it.",	
					NameStr(newMaster->mgrNode->form.nodename), curZone)));
		}												
		oldMaster = checkGetSwitchoverOldMaster(newMaster->mgrNode->form.nodemasternameoid,
												CNDN_TYPE_COORDINATOR_MASTER,
												spiContext);
		oldMaster->startupAfterException = false;
		PrintReplicationInfo(oldMaster);

		checkGetSlaveNodesRunningStatus(oldMaster,
										spiContext,
										newMaster->mgrNode->form.oid,
										"",
										&failedSlaves,
										&runningSlaves);	
		validateFailedSlavesForSwitch(oldMaster->mgrNode,
									  newMaster->mgrNode,
									  &failedSlaves,
									  forceSwitch);													   
		checkGetMasterCoordinators(spiContext,
								   &coordinators,
								   true, true);
		if (forceSwitch){
			tryLockCluster(&coordinators);
		}
		else{
			checkActiveConnectionsForSwitchover(&coordinators, oldMaster, maxTrys);
		}		
		holdLockCoordinator = getHoldLockCoordinator(&coordinators);
		Assert(holdLockCoordinator);

		checkActiveLocksForSwitchover(&coordinators, NULL, oldMaster, maxTrys);
		checkXlogDiffForSwitchoverCoord(&coordinators, NULL, oldMaster, newMaster, maxTrys);
		CHECK_FOR_INTERRUPTS();

		refreshMgrNodeBeforeSwitch(oldMaster, spiContext);
		refreshMgrNodeBeforeSwitch(newMaster, spiContext);
		refreshMgrNodeListBeforeSwitch(spiContext, &runningSlaves);
		refreshMgrNodeListBeforeSwitch(spiContext, &failedSlaves);

		oldMaster->startupAfterException = (oldMaster->runningMode == NODE_RUNNING_MODE_MASTER &&
			 								newMaster->runningMode == NODE_RUNNING_MODE_SLAVE);
		shutdownNodeWithinSeconds(oldMaster->mgrNode,
								  SHUTDOWN_NODE_FAST_SECONDS,
								  SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								  true);		
		ClosePgConn(oldMaster->pgConn);
		dlist_foreach_modify(miter, &coordinators) 
		{
			node = dlist_container(SwitcherNodeWrapper, link, miter.cur);
			if (isSameNodeName(node->mgrNode, oldMaster->mgrNode))
			{
				dlist_delete(miter.cur);
				break;
			}
		}
		
		gtmMaster = getGtmCoordMaster(&coordinators);
		Assert(gtmMaster);
		setCheckGtmInfoInPGSqlConf(gtmMaster->mgrNode,
								   newMaster->mgrNode,
								   newMaster->pgConn,
								   true,
								   CHECK_GTM_INFO_SECONDS,
								   true);

		promoteNewMasterStartReign(oldMaster, newMaster);

		newMaster->mgrNode->form.nodetype =	getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
		dlist_push_tail(&coordinators, &newMaster->link);
        appendSlaveNodeFollowMaster(newMaster->mgrNode,
									oldMaster->mgrNode,
									newMaster->pgConn,
									spiContext);
		if (pg_strcasecmp(NameStr(oldMaster->mgrNode->form.nodezone), curZone) == 0)
		{
			runningSlavesFollowNewMaster(newMaster, 
										oldMaster,
										&runningSlaves,
										NULL,
										spiContext,
										OVERTYPE_SWITCHOVER,
										curZone);
		}							

		refreshPgxcNodesOfCoordinators(holdLockCoordinator,           	
									   &coordinators,
									   oldMaster,
									   newMaster);

		refreshOldMasterAfterSwitchover(oldMaster, newMaster, spiContext);
		refreshSlaveNodesAfterSwitch(newMaster,
								     oldMaster,
									 &runningSlaves,
									 &failedSlaves,
									 NULL,
 									 NULL,
									 spiContext,
									 OVERTYPE_SWITCHOVER,
									 curZone);

		refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode, newMaster->mgrNode, spiContext, false);
		
		tryUnlockCluster(&coordinators, true);

		RefreshPgxcNodeName(newMaster, NameStr(newMaster->mgrNode->form.nodename));

		ereportNoticeLog((errmsg("Switchover the coordinator master from %s to %s "
						"has been successfully completed",
						NameStr(oldMaster->mgrNode->form.nodename),
						NameStr(newMaster->mgrNode->form.nodename))));
	}
	PG_CATCH();
	{
		/* Save error info in our stmt_mcontext */
		MemoryContextSwitchTo(oldContext);
		edata = CopyErrorData();
		FlushErrorState();

		RevertPgxcNodeCoord(&coordinators, oldMaster, newMaster, true);
	}
	PG_END_TRY();

	ereport(LOG, (errmsg("------------- switchoverCoord newMasterName(%s) after -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);

	pfreeSwitcherNodeWrapperList(&coordinators, newMaster);
	pfreeSwitcherNodeWrapperList(&runningSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&failedSlaves, NULL);
	pfreeSwitcherNodeWrapper(oldMaster);
	pfreeSwitcherNodeWrapper(newMaster);

	(void)MemoryContextSwitchTo(oldContext);
	MemoryContextDelete(switchContext);

	SPI_finish();

	if (edata)
		ReThrowError(edata);
}
static void precheckPromotionNode(dlist_head *runningSlaves, bool forceSwitch)
{
	SwitcherNodeWrapper *node = NULL;
	SwitcherNodeWrapper *syncNode = NULL;
	dlist_mutable_iter miter;

	if (dlist_is_empty(runningSlaves))
	{
		ereport(ERROR,
				(errmsg("Can't find any slave node that can be promoted")));
	}
	if (!forceSwitch)
	{
		/* check sync slave node */
		dlist_foreach_modify(miter, runningSlaves)
		{
			node = dlist_container(SwitcherNodeWrapper, link, miter.cur);
			if (strcmp(NameStr(node->mgrNode->form.nodesync),
					   getMgrNodeSyncStateValue(SYNC_STATE_SYNC)) == 0)
			{
				syncNode = node;
				break;
			}
			else
			{
				continue;
			}
		}
		if (syncNode == NULL)
		{
			ereport(ERROR,
					(errmsg("Can't find a Synchronous standby node, "
							"Abort switching to avoid data loss")));
		}
	}
}

void chooseNewMasterNode(SwitcherNodeWrapper *oldMaster,
						 SwitcherNodeWrapper **newMasterP,
						 dlist_head *runningSlaves,
						 dlist_head *failedSlaves,
						 MemoryContext spiContext,
						 bool forceSwitch,
						 char *newMasterName,
						 char *zone)
{
	SwitcherNodeWrapper *node;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_mutable_iter miter;

	if(dlist_is_empty(runningSlaves))
		ereportErrorLog(errmsg("Error there is no running slave to choose as a candidate for promotion"));

	/* Prevent other doctor processe from manipulating this node simultaneously */
	refreshOldMasterBeforeSwitch(oldMaster, spiContext);

	/* Sentinel, ensure to shut down old master */
	if(mgr_check_agent_running(oldMaster->mgrNode->form.nodehost))
		shutdownNodeWithinSeconds(oldMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								false);
	/* given new master node? */
	if(newMasterName && strlen(newMasterName) > 0)
	{
		newMaster = getNewMasterNodeByNodename(runningSlaves,
											   failedSlaves,
											   newMasterName);			  
	}
	else
	{
		newMaster = getBestWalLsnSlaveNode(runningSlaves,
										   failedSlaves,
										   NameStr(oldMaster->mgrNode->form.nodezone),
										   zone);
	}
	if (NULL == newMaster)
		ereportErrorLog(errmsg("Error there is no new %s for promotion",
				mgr_get_nodetype_desc(getMgrMasterNodetype(oldMaster->mgrNode->form.nodetype))));

	*newMasterP = newMaster;
	if (newMaster)
	{
		oldMaster->startupAfterException =
			(oldMaster->runningMode == NODE_RUNNING_MODE_MASTER &&
			 newMaster->runningMode == NODE_RUNNING_MODE_SLAVE);

		dlist_foreach_modify(miter, runningSlaves)
		{
			node = dlist_container(SwitcherNodeWrapper, link, miter.cur);
			if (node == newMaster)
			{
				dlist_delete(miter.cur);
			}
		}
	}
	else
	{
		oldMaster->startupAfterException =
			oldMaster->runningMode == NODE_RUNNING_MODE_MASTER;
	}

	CheckNodeInZone(newMaster->mgrNode, zone);
	validateNewMasterCandidateForSwitch(oldMaster->mgrNode,
										newMaster,
										forceSwitch);
	ereportNoticeLog(errmsg("%s have the best wal lsn, "
					"choose it as a candidate for promotion",
					NameStr(newMaster->mgrNode->form.nodename)));
}

void chooseNewMasterNodeForZone(SwitcherNodeWrapper *oldMaster,
								SwitcherNodeWrapper **newMasterP,
								dlist_head *runningSlaves,
								bool forceSwitch,
								char *zone)
{
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head failedSlaves = DLIST_STATIC_INIT(failedSlaves); 

	newMaster = getBestWalLsnSlaveNode(runningSlaves,
										&failedSlaves,
										NameStr(oldMaster->mgrNode->form.nodezone),
										zone);
	if (NULL == newMaster)
		ereportErrorLog(errmsg("Error there is no new %s for promotion",
				 mgr_get_nodetype_desc(getMgrMasterNodetype(oldMaster->mgrNode->form.nodetype))));
		
	*newMasterP = newMaster;

	validateNewMasterCandidateForSwitch(oldMaster->mgrNode, newMaster, forceSwitch);
	pfreeSwitcherNodeWrapperList(&failedSlaves, NULL);

	ereportNoticeLog(errmsg("%s have the best wal lsn, "
						"choose it as a candidate for promotion",
						NameStr(newMaster->mgrNode->form.nodename)));
						
}						 
static void RevertPgxcNodeCoord(dlist_head *coordinators,
								 SwitcherNodeWrapper *oldMaster,
								 SwitcherNodeWrapper *newMaster,
								 bool unLoc)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	bool execOk = true;
	
	CheckNull(oldMaster);
	CheckNull(newMaster);	
	CheckNull(coordinators);

	holdLockCoordinator = getHoldLockCoordinator(coordinators);

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (node->pgxcNodeChanged)
		{
			if (updatePgxcNodeForSwitch(holdLockCoordinator,
										node,
										newMaster,
										oldMaster,
										false))
			{
				node->pgxcNodeChanged = false;
			}
			else
			{
				ereportNoticeLog(errmsg("%s revert pgxc_node failed", NameStr(node->mgrNode->form.nodename)));
				execOk = false;
			}

			pgxcPoolReloadOnNode(holdLockCoordinator, node, false);
		}
	}

	if (oldMaster && oldMaster->startupAfterException)
	{
		callAgentStartNode(oldMaster->mgrNode, false, false);
	}
	if (unLoc){
		if (!tryUnlockCluster(coordinators, false))
		{
			execOk = false;
		}
	}

	if (execOk){
		ereportNoticeLog(errmsg("revert cluster setting successfully completed"));
	}
	else{
		ereport(WARNING,(errmsg("revert cluster setting, but some operations failed")));
	}
	ereport(WARNING,
			(errmsg("An exception occurred during the switching coordinator operation, "
					"It is recommended to use command such as 'monitor all', "
					"'monitor ha' to check the failure point in the cluster "
					"first, and then retry the switching operation!!!")));
}

/*
 * Executes an "pause cluster" operation on master coordinator. 
 * If any of the sub-operations fails, the execution of the remaining 
 * sub-operations is abandoned, and the function returns false. 
 * Finally, if operation failed, may should call to 
 * revert the settings that was changed during execution.
 * When cluster is locked, the connection which  execute the lock command 
 * "SELECT PG_PAUSE_CLUSTER();" is the only active connection.
 */
void tryLockCluster(dlist_head *coordinators)
{
	dlist_iter iter;
	SwitcherNodeWrapper *coordinator;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;

	if (dlist_is_empty(coordinators))
	{
		ereport(ERROR,
				(errmsg("There is no master coordinator, lock cluster failed")));
	}

	dlist_foreach(iter, coordinators)
	{
		coordinator = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		checkSet_pool_release_to_idle_timeout(coordinator);
	}

	/* gtm_coord is the first priority */
	holdLockCoordinator = getGtmCoordMaster(coordinators);

	/* When cluster is locked, the connection which 
	 * executed the lock command is the only active connection */
	if (!holdLockCoordinator)
	{
		dlist_foreach(iter, coordinators)
		{
			holdLockCoordinator =
				dlist_container(SwitcherNodeWrapper, link, iter.cur);
			break;
		}
	}
	holdLockCoordinator->holdClusterLock =
		exec_pg_pause_cluster(holdLockCoordinator->pgConn, false);
	if (holdLockCoordinator->holdClusterLock)
	{
		ereportNoticeLog((errmsg("%s try lock cluster successfully",
						NameStr(holdLockCoordinator->mgrNode->form.nodename))));
	}
	else
	{
		ereport(ERROR,
				(errmsg("%s try lock cluster failed",
						NameStr(holdLockCoordinator->mgrNode->form.nodename))));
	}
}

bool tryUnlockCluster(dlist_head *coordinators, bool complain)
{
	dlist_iter iter;
	SwitcherNodeWrapper *coordinator;
	bool execOk = true;
	bool triedUnlock = false;

	dlist_foreach(iter, coordinators)
	{
		coordinator = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (coordinator->holdClusterLock)
		{
			triedUnlock = true;
			if (exec_pg_unpause_cluster(coordinator->pgConn, complain))
			{
				coordinator->holdClusterLock = false;
			}
			else
			{
				execOk = false;
			}
			if (execOk)
			{
				ereport(NOTICE,
						(errmsg("%s try unlock cluster successfully",
								NameStr(coordinator->mgrNode->form.nodename))));
				ereport(LOG,
						(errmsg("%s try unlock cluster successfully",
								NameStr(coordinator->mgrNode->form.nodename))));
			}
			else
			{
				ereport(complain ? ERROR : WARNING,
						(errmsg("%s try unlock cluster failed",
								NameStr(coordinator->mgrNode->form.nodename))));
			}
			break;
		}
		else
		{
			continue;
		}
	}
	if (execOk && triedUnlock)
	{
		dlist_foreach(iter, coordinators)
		{
			coordinator = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			restoreCoordinatorSetting(coordinator);
		}
	}
	return execOk;
}

void mgrNodesToSwitcherNodes(dlist_head *mgrNodes,
							 dlist_head *switcherNodes)
{
	SwitcherNodeWrapper *switcherNode;
	MgrNodeWrapper *mgrNode;
	dlist_iter iter;

	if (mgrNodes == NULL || dlist_is_empty(mgrNodes))
	{
		return;
	}
	dlist_foreach(iter, mgrNodes)
	{
		mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
		switcherNode = palloc0(sizeof(SwitcherNodeWrapper));
		switcherNode->mgrNode = mgrNode;
		dlist_push_tail(switcherNodes, &switcherNode->link);
	}
}

void switcherNodesToMgrNodes(dlist_head *switcherNodes,
							 dlist_head *mgrNodes)
{
	SwitcherNodeWrapper *switcherNode;
	MgrNodeWrapper *mgrNode;
	dlist_iter iter;

	if (switcherNodes == NULL || dlist_is_empty(switcherNodes))
	{
		return;
	}
	dlist_foreach(iter, switcherNodes)
	{
		switcherNode = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		mgrNode = switcherNode->mgrNode;
		dlist_push_tail(mgrNodes, &mgrNode->link);
	}
}
void appendSlaveNodeFollowMaster(MgrNodeWrapper *masterNode,
								 MgrNodeWrapper *slaveNode,
								 PGconn *masterPGconn,
								 MemoryContext spiContext)
{
	setPGHbaTrustSlaveReplication(masterNode, slaveNode, true);

	setSynchronousStandbyNames(slaveNode, "");

	setSlaveNodeRecoveryConf(masterNode, slaveNode);

	shutdownNodeWithinSeconds(slaveNode,
							  SHUTDOWN_NODE_FAST_SECONDS,
							  SHUTDOWN_NODE_IMMEDIATE_SECONDS,
							  true);

	callAgentStartNode(slaveNode, false, false);

	waitForNodeRunningOk(slaveNode, false, NULL, NULL);

	appendToSyncStandbyNames(masterNode,
							 slaveNode,
							 masterPGconn,
							 spiContext);

	ereport(LOG,
			(errmsg("%s has followed master %s",
					NameStr(slaveNode->form.nodename),
					NameStr(masterNode->form.nodename))));
}
void appendSlaveNodeFollowMasterForSwitchOver(MemoryContext spiContext,
												SwitcherNodeWrapper *master,
												SwitcherNodeWrapper *slave,
												bool complain)
{
	MgrNodeWrapper *masterNode = master->mgrNode;
	MgrNodeWrapper *slaveNode  = slave->mgrNode;
	PGconn 	*masterPGconn 	  = master->pgConn;

	setPGHbaTrustSlaveReplication(masterNode, slaveNode, complain);

	setSynchronousStandbyNames(slaveNode, "");

	setSlaveNodeRecoveryConf(masterNode, slaveNode);

	shutdownNodeWithinSeconds(slaveNode,
							  SHUTDOWN_NODE_FAST_SECONDS,
							  SHUTDOWN_NODE_IMMEDIATE_SECONDS,
							  complain);

	callAgentStartNode(slaveNode, false, false);

	waitForNodeRunningOk(slaveNode, false, &slave->pgConn, &slave->runningMode);

    appendToSyncStandbyNames(masterNode,
							 slaveNode,
							 masterPGconn,
							 spiContext);

	ereportNoticeLog(errmsg("%s has followed %s success",
					NameStr(slaveNode->form.nodename),
					NameStr(masterNode->form.nodename)));
}

void appendSlaveNodeFollowMasterEx(MemoryContext spiContext,
								SwitcherNodeWrapper *master,
								SwitcherNodeWrapper *slave,
								bool complain)
{
	MgrNodeWrapper *masterNode = master->mgrNode;
	MgrNodeWrapper *slaveNode  = slave->mgrNode;
	PGconn 	*masterPGconn 	  = master->pgConn;

	setPGHbaTrustSlaveReplication(masterNode, slaveNode, complain);

	setSynchronousStandbyNames(slaveNode, "");

	setSlaveNodeRecoveryConf(masterNode, slaveNode);

	shutdownNodeWithinSeconds(slaveNode,
							  SHUTDOWN_NODE_FAST_SECONDS,
							  SHUTDOWN_NODE_IMMEDIATE_SECONDS,
							  complain);

	callAgentStartNode(slaveNode, false, false);

	waitForNodeRunningOk(slaveNode, false, &slave->pgConn, &slave->runningMode);

    if (master->runningMode != NODE_RUNNING_MODE_UNKNOW)
		appendToSyncStandbyNamesForZone(masterNode,
										slaveNode,
										masterPGconn,
										slave->pgConn,
										spiContext);

	ereportNoticeLog(errmsg("%s has followed %s success",
					NameStr(slaveNode->form.nodename),
					NameStr(masterNode->form.nodename)));
}
/**
 * Compare all nodes's wal lsn, find a node which has the best wal lsn, 
 * called it "bestNode", we think this bestNode can be promoted, also 
 * this node will be deleted in runningSlaves. In  subsequent operations, 
 * If the bestNode is promoted as new master node, the remaining nodes in 
 * runningSlaves should follow this bestNode.
 */
static SwitcherNodeWrapper *getBestWalLsnSlaveNode(dlist_head *runningSlaves,
												   dlist_head *failedSlaves,
												   char *masterNodeZone,
												   char *zone)
{
	SwitcherNodeWrapper *node;
	SwitcherNodeWrapper *bestNode = NULL;
	dlist_mutable_iter miter;

	dlist_foreach_modify(miter, runningSlaves)
	{
		node = dlist_container(SwitcherNodeWrapper, link, miter.cur);

        if (strlen(zone) == 0){
			if (pg_strcasecmp(masterNodeZone, NameStr(node->mgrNode->form.nodezone)) != 0)
				continue;
		}
		else{
			if (pg_strcasecmp(zone, NameStr(node->mgrNode->form.nodezone)) != 0)
				continue;
		}

		node->walLsn = getNodeWalLsn(node->pgConn, node->runningMode);
		if (node->walLsn <= InvalidXLogRecPtr)
		{
			dlist_delete(miter.cur);
			dlist_push_tail(failedSlaves, &node->link);
			ereport(WARNING,
					(errmsg("%s get wal lsn failed",
							NameStr(node->mgrNode->form.nodename))));
		}
		else
		{
			if (bestNode == NULL)
			{
				bestNode = node;
			}
			else
			{
				if (node->walLsn > bestNode->walLsn)
				{
					bestNode = node;
				}
				else if (node->walLsn == bestNode->walLsn)
				{
					if (strcmp(NameStr(bestNode->mgrNode->form.nodesync),
							   getMgrNodeSyncStateValue(SYNC_STATE_SYNC)) == 0)
					{
						continue;
					}
					else
					{
						bestNode = node;
					}
				}
				else
				{
					continue;
				}
			}
		}
	}
	return bestNode;
}

/*
 * If any of the sub-operations failed to execute, the funtion returns 
 * false, but ignores any execution failures and continues to execute 
 * the remaining sub-operations.
 */
static void restoreCoordinatorSetting(SwitcherNodeWrapper *coordinator)
{
	if (coordinator->temporaryHbaItems)
	{
		if (callAgentDeletePGHbaConf(coordinator->mgrNode,
									 coordinator->temporaryHbaItems,
									 false))
		{
			pfreePGHbaItem(coordinator->temporaryHbaItems);
			coordinator->temporaryHbaItems = NULL;
		}
		else
		{
			ereport(NOTICE,
					(errmsg("%s restore pg_hba.conf failed",
							NameStr(coordinator->mgrNode->form.nodename))));
			ereport(LOG,
					(errmsg("%s restore pg_hba.conf failed",
							NameStr(coordinator->mgrNode->form.nodename))));
		}
	}
	if (coordinator->originalParameterItems)
	{
		if (callAgentRefreshPGSqlConfReload(coordinator->mgrNode,
											coordinator->originalParameterItems,
											false))
		{
			pfreePGConfParameterItem(coordinator->originalParameterItems);
			coordinator->originalParameterItems = NULL;
		}
		else
		{
			ereport(NOTICE,
					(errmsg("%s restore postgresql.conf failed",
							NameStr(coordinator->mgrNode->form.nodename))));
			ereport(LOG,
					(errmsg("%s restore postgresql.conf failed",
							NameStr(coordinator->mgrNode->form.nodename))));
		}
	}

	callAgentReloadNode(coordinator->mgrNode, false);

	if (!exec_pool_close_idle_conn(coordinator->pgConn, false))
	{
		ereport(LOG,
				(errmsg("%s exec pool_close_idle_conn failed",
						NameStr(coordinator->mgrNode->form.nodename))));
	}
}

static void checkGetSlaveNodesRunningStatus(SwitcherNodeWrapper *masterNode,
											MemoryContext spiContext,
											Oid excludeSlaveOid,
											char *zone,
											dlist_head *failedSlaves,
											dlist_head *runningSlaves)
{
	char slaveNodetype;
	dlist_head mgrNodes = DLIST_STATIC_INIT(mgrNodes);
	dlist_head slaveNodes = DLIST_STATIC_INIT(slaveNodes);
	dlist_mutable_iter iter;
	MgrNodeWrapper *mgrNode;

	slaveNodetype = getMgrSlaveNodetype(masterNode->mgrNode->form.nodetype);
	if (strlen(zone) > 0){		
		selectMgrSlaveNodesByOidTypeInZone(masterNode->mgrNode->form.oid,
										slaveNodetype,
										zone,
										spiContext,
										&mgrNodes);
	}
	else{
		selectMgrSlaveNodesByOidType(masterNode->mgrNode->form.oid,
									slaveNodetype,
									spiContext,
									&mgrNodes);
	}

	if (excludeSlaveOid > 0)
	{
		dlist_foreach_modify(iter, &mgrNodes)
		{
			mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
			if (mgrNode->form.oid == excludeSlaveOid)
			{
				dlist_delete(iter.cur);
				pfreeMgrNodeWrapper(mgrNode);
			}
		}
	}
	mgrNodesToSwitcherNodes(&mgrNodes,
							&slaveNodes);

	classifyNodesForSwitch(&slaveNodes,
						   runningSlaves,
						   failedSlaves);
	/* add isolated slave node to failedSlaves */
	dlist_init(&mgrNodes);
	selectIsolatedMgrSlaveNodes(masterNode->mgrNode->form.oid,
								slaveNodetype,
								spiContext,
								&mgrNodes);
	if (excludeSlaveOid > 0)
	{
		dlist_foreach_modify(iter, &mgrNodes)
		{
			mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
			if (mgrNode->form.oid == excludeSlaveOid)
			{
				dlist_delete(iter.cur);
				pfreeMgrNodeWrapper(mgrNode);
			}
		}
	}
	mgrNodesToSwitcherNodes(&mgrNodes,
							failedSlaves);
}
static void checkGetSlaveNodesRunningSecondStatus(SwitcherNodeWrapper *oldMaster,	
													MemoryContext spiContext,
													Oid excludeSlaveOid,
													dlist_head *excludeRunningSlaves,
													dlist_head *excludeFailedSlaves,													
													dlist_head *runningSlavesSecond,
													dlist_head *failedSlavesSecond)
{
	char slaveNodetype;
	dlist_head mgrNodes = DLIST_STATIC_INIT(mgrNodes);
	dlist_head slaveNodes = DLIST_STATIC_INIT(slaveNodes);

	slaveNodetype = getMgrSlaveNodetype(oldMaster->mgrNode->form.nodetype);
	selectActiveMgrNodeChild(spiContext,
							oldMaster->mgrNode,
							slaveNodetype,
							&slaveNodes);	
	DeleteFromMgrNodeListByOid(&slaveNodes, excludeSlaveOid);	
	DeleteFromMgrNodeListByList(&slaveNodes, excludeRunningSlaves);
	classifyNodesForSwitch(&slaveNodes, runningSlavesSecond, failedSlavesSecond);

	dlist_init(&mgrNodes);
	selectIsolatedMgrSlaveNodesByNodeType(slaveNodetype, spiContext, &mgrNodes);
	mgrNodesToSwitcherNodes(&mgrNodes, failedSlavesSecond);
	DeleteFromMgrNodeListByOid(failedSlavesSecond, excludeSlaveOid);	
	DeleteFromMgrNodeListByList(failedSlavesSecond, excludeFailedSlaves);
}
static void
getRunningSlaveOfNewMaster(MemoryContext spiContext,
							MgrNodeWrapper *masterNode,
							dlist_head *runningSlaveOfNewMaster)
{
	dlist_head 	mgrNodes = DLIST_STATIC_INIT(mgrNodes);

	selectMgrSlaveNodesByOidType(masterNode->form.oid,
								getMgrSlaveNodetype(masterNode->form.nodetype),
								spiContext,
								&mgrNodes);
	mgrNodesToSwitcherNodes(&mgrNodes,
							runningSlaveOfNewMaster);
}

static void DeleteFromMgrNodeListByOid(dlist_head *switcherNodes, Oid excludeSlaveOid)
{
	dlist_mutable_iter 	mutable_iter;
	SwitcherNodeWrapper	*switcherNode;
	Assert(switcherNodes);

	if (excludeSlaveOid > 0)
	{
		dlist_foreach_modify(mutable_iter, switcherNodes)
		{
			switcherNode = dlist_container(SwitcherNodeWrapper, link, mutable_iter.cur);
			if (switcherNode->mgrNode->form.oid == excludeSlaveOid)
			{
				dlist_delete(mutable_iter.cur);
				pfreeSwitcherNodeWrapper(switcherNode);
			}
		}
	}
}
static void DeleteFromMgrNodeListByList(dlist_head *switcherNodes, dlist_head *excludeSlaveList)
{
	dlist_mutable_iter 	mutable_iter;
	dlist_iter 			iter;
	SwitcherNodeWrapper *switcherNodeFrom;
	SwitcherNodeWrapper	*switcherNodeExclude;
	
	Assert(switcherNodes);
	Assert(excludeSlaveList);
	
	dlist_foreach(iter, excludeSlaveList)
	{
		switcherNodeExclude = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		Assert(switcherNodeExclude);
		dlist_foreach_modify(mutable_iter, switcherNodes)
		{
			switcherNodeFrom = dlist_container(SwitcherNodeWrapper, link, mutable_iter.cur);
			if (isSameNodeName(switcherNodeFrom->mgrNode, switcherNodeExclude->mgrNode))
			{
				dlist_delete(mutable_iter.cur);
				pfreeSwitcherNodeWrapper(switcherNodeFrom);
			}
		}
	}
}
static void checkGetRunningSlaveNodesInZone(SwitcherNodeWrapper *masterNode,
											MemoryContext spiContext,
											char slaveNodetype,
											char *zone,
											dlist_head *runningSlaves)
{
	dlist_head 	mgrNodes 	= DLIST_STATIC_INIT(mgrNodes);
	dlist_head 	slaveNodes 	= DLIST_STATIC_INIT(slaveNodes);	
	dlist_head 	failedSlaves= DLIST_STATIC_INIT(failedSlaves);
	dlist_mutable_iter iter;
	MgrNodeWrapper *mgrNode;

	Assert(masterNode);
	Assert(spiContext);
	Assert(zone);
	Assert(runningSlaves);

	selectActiveMgrSlaveNodes(masterNode->mgrNode->form.oid, slaveNodetype, spiContext, &mgrNodes);
	dlist_foreach_modify(iter, &mgrNodes)
	{
		mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
		if (pg_strcasecmp(NameStr(mgrNode->form.nodezone), zone) != 0)
		{
			dlist_delete(iter.cur);
			pfreeMgrNodeWrapper(mgrNode);
		}
	}

	mgrNodesToSwitcherNodes(&mgrNodes, &slaveNodes);
	classifyNodesForSwitch(&slaveNodes, runningSlaves, &failedSlaves);
	pfreeSwitcherNodeWrapperList(&failedSlaves, NULL);
}
static void classifyNodesForSwitch(dlist_head *nodes,
								   dlist_head *runningNodes,
								   dlist_head *failedNodes)
{
	SwitcherNodeWrapper *node;
	dlist_mutable_iter miter;
	bool nodeOk;

	dlist_foreach_modify(miter, nodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, miter.cur);

		if (tryConnectNode(node, 10))
		{
			node->runningMode = getNodeRunningMode(node->pgConn);
			if (node->runningMode == NODE_RUNNING_MODE_UNKNOW)
			{
				nodeOk = false;
				ereport(WARNING,
						(errmsg("%s running status unknow, you can manually fix it, "
								"or enable doctor to fix it automatically",
								NameStr(node->mgrNode->form.nodename))));
			}
			else
			{
				nodeOk = true;
			}
		}
		else
		{
			nodeOk = false;
			ereport(WARNING,
					(errmsg("connect to %s failed, you can manually fix it, "
							"or enable doctor to fix it automatically",
							NameStr(node->mgrNode->form.nodename))));
		}
		dlist_delete(miter.cur);
		if (nodeOk)
		{
			dlist_push_tail(runningNodes, &node->link);
		}
		else
		{
			dlist_push_tail(failedNodes, &node->link);
		}
	}
}

/**
 * Sort new slave nodes (exclude new master node) by walReceiveLsn.
 * The larger the walReceiveLsn, the more in front of the dlist.
 */
static void sortNodesByWalLsnDesc(dlist_head *nodes)
{
	dlist_mutable_iter miter;
	SwitcherNodeWrapper *node;
	SwitcherNodeWrapper **sortItems;
	int i, numOfNodes;

	numOfNodes = 0;
	/* Exclude new master node from old slave nodes */
	dlist_foreach_modify(miter, nodes)
	{
		numOfNodes++;
	}
	if (numOfNodes > 1)
	{
		sortItems = palloc(sizeof(SwitcherNodeWrapper *) * numOfNodes);
		i = 0;
		dlist_foreach_modify(miter, nodes)
		{
			node = dlist_container(SwitcherNodeWrapper, link, miter.cur);
			sortItems[i] = node;
			i++;
		}
		/* order by wal lsn desc */
		qsort(sortItems, numOfNodes, sizeof(SwitcherNodeWrapper *), walLsnDesc);
		/* add to dlist in order */
		dlist_init(nodes);
		for (i = 0; i < numOfNodes; i++)
		{
			dlist_push_tail(nodes, &sortItems[i]->link);
		}
		pfree(sortItems);
	}
	else
	{
		/* No need to sort */
	}
}

static bool checkIfSyncSlaveNodeIsRunning(MemoryContext spiContext,
										  MgrNodeWrapper *masterNode)
{
	dlist_head mgrNodes = DLIST_STATIC_INIT(mgrNodes);
	dlist_iter iter;
	MgrNodeWrapper *mgrNode;
	bool standbySyncOk;

	selectAllMgrSlaveNodes(masterNode->form.oid,
						   getMgrSlaveNodetype(masterNode->form.nodetype),
						   spiContext,
						   &mgrNodes);
	standbySyncOk = true;
	dlist_foreach(iter, &mgrNodes)
	{
		mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
		if (strcmp(NameStr(mgrNode->form.nodesync),
				   getMgrNodeSyncStateValue(SYNC_STATE_SYNC)) == 0)
		{
			if (PQPING_OK == pingNodeDefaultDB(mgrNode, 10))
			{
				standbySyncOk = true;
				break;
			}
			else
			{
				standbySyncOk = false;
				/* may be there are more than one sync slaves */
				ereport(WARNING,
						(errmsg("%s is a Synchronous standby node of %s, but it is not running properly",
								NameStr(mgrNode->form.nodename),
								NameStr(masterNode->form.nodename))));
			}
		}
		else
		{
			continue;
		}
	}
	pfreeMgrNodeWrapperList(&mgrNodes, NULL);
	return standbySyncOk;
}

void checkGetMasterCoordinators(MemoryContext spiContext,
								dlist_head *coordinators,
								bool includeGtmCoord,
								bool checkRunningMode)
{
	dlist_head masterMgrNodes = DLIST_STATIC_INIT(masterMgrNodes);
	SwitcherNodeWrapper *node;
	dlist_iter iter;

	if (includeGtmCoord)
	{
		selectActiveMasterCoordinators(spiContext, &masterMgrNodes);
		if (dlist_is_empty(&masterMgrNodes))
		{
			ereport(ERROR,
					(errmsg("can't find any master coordinator")));
		}
	}
	else
	{
		selectActiveMgrNodeByNodetype(spiContext,
									  CNDN_TYPE_COORDINATOR_MASTER,
									  &masterMgrNodes);
	}
	mgrNodesToSwitcherNodes(&masterMgrNodes, coordinators);

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (!tryConnectNode(node, 10))
		{
			ereport(ERROR,
					(errmsg("connect to %s failed",
							NameStr(node->mgrNode->form.nodename))));
		}
		if (checkRunningMode)
		{
			node->runningMode = getNodeRunningMode(node->pgConn);
			if (node->runningMode != NODE_RUNNING_MODE_MASTER)
			{
				ereport(ERROR,
						(errmsg("%s configured as master, "
								"but actually did not running on that status",
								NameStr(node->mgrNode->form.nodename))));
			}
			/* 
			 * The data of the PGXC_NODE table needs to be modified during the 
			 * switching process. If the synchronization node of the coordinator 
			 * fails, the cluster will hang. 
			 */
			if (!checkIfSyncSlaveNodeIsRunning(spiContext, node->mgrNode))
			{
				ereport(ERROR,
						(errmsg("%s Synchronous standby node Streaming Replication failure",
								NameStr(node->mgrNode->form.nodename))));
			}
		}
		else
		{
			node->runningMode = NODE_RUNNING_MODE_UNKNOW;
		}
	}
}
static void checkGetCoordinatorsForZone(MemoryContext spiContext,
											SwitcherNodeWrapper *oldMaster,
											char *zone,
											char nodeType,
											dlist_head *coordinators)
{
	dlist_head masterMgrNodes = DLIST_STATIC_INIT(masterMgrNodes);
	SwitcherNodeWrapper *node;
	dlist_iter iter;

	selectActiveNodeInZone(spiContext, 
							zone, 
							nodeType, 
							&masterMgrNodes);
	mgrNodesToSwitcherNodes(&masterMgrNodes, coordinators);

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (!tryConnectNode(node, 10))
		{
			ereport(LOG, (errmsg("connect to %s failed",
							NameStr(node->mgrNode->form.nodename))));
		}
	}

	if (dlist_is_empty(coordinators))
	{
		ereport(LOG, (errmsg("can't find any coordinator")));
	}
}

void checkGetSlaveCoordinators(MemoryContext spiContext,
								dlist_head *coordinators,
								bool checkRunningMode)
{
	dlist_head masterMgrNodes = DLIST_STATIC_INIT(masterMgrNodes);
	SwitcherNodeWrapper *node;
	dlist_iter iter;

	selectActiveMgrNodeByNodetype(spiContext,
									CNDN_TYPE_COORDINATOR_SLAVE,
									&masterMgrNodes);

	mgrNodesToSwitcherNodes(&masterMgrNodes, coordinators);

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (!tryConnectNode(node, 10))
		{
			ereport(ERROR,
					(errmsg("connect to %s failed",
							NameStr(node->mgrNode->form.nodename))));
		}
		if (checkRunningMode)
		{
			node->runningMode = getNodeRunningMode(node->pgConn);
			if (node->runningMode != NODE_RUNNING_MODE_SLAVE)
			{
				ereport(ERROR,
						(errmsg("%s configured as slave, "
								"but actually did not running on that status",
								NameStr(node->mgrNode->form.nodename))));
			}			
		}
		else
		{
			node->runningMode = NODE_RUNNING_MODE_UNKNOW;
		}
	}
}
void checkGetMasterDataNodes(MemoryContext spiContext,
								dlist_head *dataNodes,
								bool checkRunningMode)
{
	dlist_head masterMgrNodes = DLIST_STATIC_INIT(masterMgrNodes);
	SwitcherNodeWrapper *node;
	dlist_iter iter;

	selectActiveMgrNodeByNodetype(spiContext, CNDN_TYPE_DATANODE_MASTER, &masterMgrNodes);
	if (dlist_is_empty(&masterMgrNodes)){
		ereport(ERROR,(errmsg("can't find any master datanodes.")));
	}
	mgrNodesToSwitcherNodes(&masterMgrNodes, dataNodes);

	dlist_foreach(iter, dataNodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (!tryConnectNode(node, 10)){
			ereport(ERROR,(errmsg("connect to %s failed", NameStr(node->mgrNode->form.nodename))));
		}

		if (checkRunningMode)
		{
			node->runningMode = getNodeRunningMode(node->pgConn);
			if (node->runningMode != NODE_RUNNING_MODE_MASTER)
			{
				ereport(ERROR, (errmsg("%s configured as master, but actually did not running on that status",
						NameStr(node->mgrNode->form.nodename))));
			}
			/* 
			 * The data of the PGXC_NODE table needs to be modified during the 
			 * switching process. If the synchronization node of the coordinator 
			 * fails, the cluster will hang. 
			 */
			if (!checkIfSyncSlaveNodeIsRunning(spiContext, node->mgrNode))
			{
				ereport(ERROR, (errmsg("%s Synchronous standby node Streaming Replication failure",
						NameStr(node->mgrNode->form.nodename))));
			}
		}
		else
		{
			node->runningMode = NODE_RUNNING_MODE_UNKNOW;
		}
	}
}

/**
 * There is a situation, If a slave node have been promoted, 
 * and then suddenly crashed, on this time, switching may cause data loss.
 */
static void validateFailedSlavesForSwitch(MgrNodeWrapper *oldMaster,
										  MgrNodeWrapper *newMaster,
										  dlist_head *failedSlaves,
										  bool forceSwitch)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	if (forceSwitch)
	{
		if (!dlist_is_empty(failedSlaves))
			ereport(WARNING,
					(errmsg("There are some slave nodes failed, force switch may cause data loss")));
	}
	else
	{
		if ((pg_strcasecmp(NameStr(newMaster->form.nodesync), getMgrNodeSyncStateValue(SYNC_STATE_SYNC)) != 0) &&
		    isSameNodeZone(newMaster, oldMaster))
		{
			dlist_foreach(iter, failedSlaves)
			{
				node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
				if (strcmp(NameStr(node->mgrNode->form.nodesync),
						   getMgrNodeSyncStateValue(SYNC_STATE_SYNC)) == 0)
				{
					ereport(ERROR,
							(errmsg("%s failed, but it is a Synchronous standby node of old master %s, "
									"abort switching to avoid data loss",
									NameStr(node->mgrNode->form.nodename),
									NameStr(oldMaster->form.nodename))));
				}
			}
		}
	}
}

static void validateNewMasterCandidateForSwitch(MgrNodeWrapper *oldMaster,
												SwitcherNodeWrapper *candidate,
												bool forceSwitch)
{
	if (candidate == NULL)
	{
		ereport(ERROR,
				(errmsg("can't find a qualified slave node that can be promoted")));
	}
	if (candidate->walLsn <= InvalidXLogRecPtr)
	{
		ereport(ERROR,
				(errmsg("invalid wal lsn %ld of candidate %s",
						candidate->walLsn,
						NameStr(candidate->mgrNode->form.nodename))));
	}
	if ((!forceSwitch) &&
		(pg_strcasecmp(NameStr(oldMaster->form.nodezone), NameStr(candidate->mgrNode->form.nodezone)) == 0))
	{
		if (strcmp(NameStr(candidate->mgrNode->form.nodesync),
				   getMgrNodeSyncStateValue(SYNC_STATE_SYNC)) != 0)
		{
			ereport(ERROR,
					(errmsg("candidate %s is not a Synchronous standby node of old master %s, "
							"abort switching to avoid data loss",
							NameStr(candidate->mgrNode->form.nodename),
							NameStr(oldMaster->form.nodename))));
		}
		if (strcmp(NameStr(oldMaster->form.nodezone),
				   NameStr(candidate->mgrNode->form.nodezone)) != 0)
		{
			ereport(ERROR,
					(errmsg("candidate %s is not in the same zone with old master %s, "
							"abort switching to avoid data loss",
							NameStr(candidate->mgrNode->form.nodename),
							NameStr(oldMaster->form.nodename))));
		}
	}
}

static void checkGetAllDataNodes(dlist_head *dataNodes,
								 MemoryContext spiContext)
{
	dlist_head mgrNodes = DLIST_STATIC_INIT(mgrNodes);
	SwitcherNodeWrapper *node;
	dlist_iter iter;

	selectMgrAllDataNodes(spiContext, &mgrNodes);
	mgrNodesToSwitcherNodes(&mgrNodes, dataNodes);

	dlist_foreach(iter, dataNodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (!tryConnectNode(node, 10))
		{
			ereport(LOG, (errmsg("connect to %s failed",
							NameStr(node->mgrNode->form.nodename))));
		}
	}

	if (dlist_is_empty(dataNodes))
	{
		ereport(LOG, (errmsg("can't find any datanode")));
	}
}
static void checkGetAllDataNodesForZone(dlist_head *dataNodes,
										char *zone,
								 		MemoryContext spiContext)
{
	dlist_head mgrNodes = DLIST_STATIC_INIT(mgrNodes);
	SwitcherNodeWrapper *node;
	dlist_iter iter;

	selectMgrAllDataNodesInZone(spiContext, zone, &mgrNodes);
	mgrNodesToSwitcherNodes(&mgrNodes, dataNodes);

	dlist_foreach(iter, dataNodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (!tryConnectNode(node, 10))
		{
			ereport(LOG, (errmsg("connect to %s failed",
							NameStr(node->mgrNode->form.nodename))));
		}
	}

	if (dlist_is_empty(dataNodes))
	{
		ereport(LOG, (errmsg("can't find any datanode")));
	}
}

/**
 * connectTimeout: Maximum wait for connection, in seconds 
 * (write as a decimal integer, e.g. 10). 
 * Zero, negative, or not specified means wait indefinitely. 
 * If get connection successfully, the PGconn is saved in node->pgConn.
 */
static bool tryConnectNode(SwitcherNodeWrapper *node, int connectTimeout)
{
	/* ensure to close obtained connection */
	pfreeSwitcherNodeWrapperPGconn(node);
	node->pgConn = getNodeDefaultDBConnection(node->mgrNode, connectTimeout);
	return node->pgConn != NULL;
}

static SwitcherNodeWrapper *checkGetOldMaster(char *oldMasterName,
												char nodeType,
												int connectTimeout,
												MemoryContext spiContext)
{
	MgrNodeWrapper *mgrNode;
	SwitcherNodeWrapper *oldMaster;

	mgrNode = selectMgrNodeByNodenameType(oldMasterName,
										  nodeType,
										  spiContext);
	if (!mgrNode)
	{
		ereport(ERROR,
				(errmsg("%s does not exist or is not a %s node",
						oldMasterName, mgr_get_nodetype_desc(nodeType))));
	}
	if (mgrNode->form.nodetype != nodeType)
	{
		ereport(ERROR,
				(errmsg("%s is not a %s node",
						oldMasterName, mgr_get_nodetype_desc(nodeType))));
	}
	if (!mgrNode->form.nodeinited)
	{
		ereport(ERROR,
				(errmsg("%s has not be initialized",
						oldMasterName)));
	}
	if (!mgrNode->form.nodeincluster)
	{
		ereport(ERROR,
				(errmsg("%s has been kicked out of the cluster",
						oldMasterName)));
	}
	oldMaster = palloc0(sizeof(SwitcherNodeWrapper));
	oldMaster->mgrNode = mgrNode;
	if (tryConnectNode(oldMaster, connectTimeout))
	{
		oldMaster->runningMode = getNodeRunningMode(oldMaster->pgConn);
	}
	else
	{
		oldMaster->runningMode = NODE_RUNNING_MODE_UNKNOW;
	}
	return oldMaster;
}

static SwitcherNodeWrapper *checkGetOldMasterForZoneCoord(MemoryContext spiContext,
															char nodeType,
															char *oldMasterName)
{
	MgrNodeWrapper *mgrNode;
	SwitcherNodeWrapper *oldMaster;

	mgrNode = selectMgrNodeByNodenameType(oldMasterName,
										  nodeType,
										  spiContext);
	if (!mgrNode)
	{
		ereport(ERROR,
				(errmsg("%s does not exist or is not a %s node",
						oldMasterName, mgr_get_nodetype_desc(nodeType))));
	}
	if (mgrNode->form.nodetype != nodeType)
	{
		ereport(ERROR,
				(errmsg("%s is not a %s node",
						oldMasterName, mgr_get_nodetype_desc(nodeType))));
	}
	oldMaster = palloc0(sizeof(SwitcherNodeWrapper));
	oldMaster->mgrNode = mgrNode;
	oldMaster->runningMode = NODE_RUNNING_MODE_UNKNOW;

	return oldMaster;
}

void checkGetSiblingMasterNodes(MemoryContext spiContext,
								SwitcherNodeWrapper *masterNode,
								dlist_head *siblingMasters)
{
	dlist_head mgrNodes = DLIST_STATIC_INIT(mgrNodes);
	MgrNodeWrapper *mgrNode;
	SwitcherNodeWrapper *switcherNode;
	dlist_mutable_iter iter;

	selectActiveMgrNodeByNodetype(spiContext,
								  getMgrMasterNodetype(masterNode->mgrNode->form.nodetype),
								  &mgrNodes);
	dlist_foreach_modify(iter, &mgrNodes)
	{
		mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
		if (mgrNode->form.oid == masterNode->mgrNode->form.oid)
		{
			dlist_delete(iter.cur);
			pfreeMgrNodeWrapper(mgrNode);
		}
	}
	mgrNodesToSwitcherNodes(&mgrNodes, siblingMasters);
	dlist_foreach_modify(iter, siblingMasters)
	{
		switcherNode = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		/*
		 * One "waitswitch" datanode master may block the operation of "alter node"
		 * in subsequent processes, If there is such a node, temporarily bypass it.
		 * When that "waitswitch" datanode master back to normal,
		 * It must compare the data in its pgxc_node table with the data in mgr_node,
		 * and then update the difference datanode master data to its own pgxc_node table.
		 */
		if (pg_strcasecmp(NameStr(switcherNode->mgrNode->form.curestatus),
						  CURE_STATUS_NORMAL) != 0 &&
			pg_strcasecmp(NameStr(switcherNode->mgrNode->form.curestatus),
						  CURE_STATUS_SWITCHED) != 0)
		{
			switcherNode->runningMode = NODE_RUNNING_MODE_UNKNOW;
			continue;
		}
		else
		{

			if (!tryConnectNode(switcherNode, 10))
			{
				ereport(ERROR,
						(errmsg("connect to sibling master %s failed",
								NameStr(switcherNode->mgrNode->form.nodename))));
			}
			switcherNode->runningMode = getNodeRunningMode(switcherNode->pgConn);
			if (switcherNode->runningMode != NODE_RUNNING_MODE_MASTER)
			{
				ereport(ERROR,
						(errmsg("sibling master %s configured as master, "
								"but actually did not running on that status",
								NameStr(switcherNode->mgrNode->form.nodename))));
			}
			if (!checkIfSyncSlaveNodeIsRunning(spiContext, switcherNode->mgrNode))
			{
				ereport(ERROR,
						(errmsg("sibling master %s Synchronous standby node Streaming Replication failure",
								NameStr(switcherNode->mgrNode->form.nodename))));
			}
		}
	}
}

static SwitcherNodeWrapper *checkGetGtmCoordOldMaster(char *oldMasterName,
													  int connectTimeout,
													  MemoryContext spiContext)
{
	MgrNodeWrapper *mgrNode;
	SwitcherNodeWrapper *oldMaster;

	mgrNode = selectMgrNodeByNodenameType(oldMasterName,
										  CNDN_TYPE_GTM_COOR_MASTER,
										  spiContext);
	if (!mgrNode)
	{
		ereport(ERROR,
				(errmsg("%s does not exist or is not a master gtm coordinator",
						oldMasterName)));
	}
	if (mgrNode->form.nodetype != CNDN_TYPE_GTM_COOR_MASTER)
	{
		ereport(ERROR,
				(errmsg("%s is not a master gtm coordinator",
						oldMasterName)));
	}
	if (!mgrNode->form.nodeinited)
	{
		ereport(ERROR,
				(errmsg("%s has not be initialized",
						oldMasterName)));
	}
	if (!mgrNode->form.nodeincluster)
	{
		ereport(ERROR,
				(errmsg("%s has been kicked out of the cluster",
						oldMasterName)));
	}
	oldMaster = palloc0(sizeof(SwitcherNodeWrapper));
	oldMaster->mgrNode = mgrNode;
	if (tryConnectNode(oldMaster, connectTimeout))
	{
		oldMaster->runningMode = getNodeRunningMode(oldMaster->pgConn);
	}
	else
	{
		oldMaster->runningMode = NODE_RUNNING_MODE_UNKNOW;
	}
	return oldMaster;
}

static SwitcherNodeWrapper *checkGetSwitchoverNewMaster(char *newMasterName,
														char nodeType,
														bool forceSwitch,
														MemoryContext spiContext)
{
	MgrNodeWrapper *mgrNode;
	SwitcherNodeWrapper *newMaster;

	mgrNode = selectMgrNodeByNodenameType(newMasterName,
										  nodeType,
										  spiContext);
	if (!mgrNode)
	{			
		ereport(ERROR,
				(errmsg("%s does not exist or is not a %s node",
						newMasterName,
						mgr_get_nodetype_desc(nodeType))));
	}
	if (mgrNode->form.nodetype != nodeType)
	{
		ereport(ERROR,
				(errmsg("%s is not a slave node",
						newMasterName)));
	}
	if (!mgrNode->form.nodeinited)
	{
		ereport(ERROR,
				(errmsg("%s has not be initialized",
						newMasterName)));
	}
	if (!mgrNode->form.nodeincluster)
	{
		ereport(ERROR,
				(errmsg("%s has been kicked out of the cluster",
						newMasterName)));
	}
	if (!isCurestatusForRunningOk(NameStr(mgrNode->form.curestatus)))
	{
		ereport(forceSwitch ? WARNING : ERROR,
				(errmsg("%s illegal curestatus:%s, "
						"switching is not recommended in this situation",
						NameStr(mgrNode->form.nodename),
						NameStr(mgrNode->form.curestatus))));
	}
	newMaster = palloc0(sizeof(SwitcherNodeWrapper));
	newMaster->mgrNode = mgrNode;
	if (tryConnectNode(newMaster, 10))
	{
		newMaster->runningMode = getNodeRunningMode(newMaster->pgConn);
		if (newMaster->runningMode != NODE_RUNNING_MODE_SLAVE)
		{
			pfreeSwitcherNodeWrapperPGconn(newMaster);
			ereport(ERROR,
					(errmsg("%s expected running status is slave mode, but actually is not",
							NameStr(mgrNode->form.nodename))));
		}
	}
	else
	{
		ereport(ERROR,
				(errmsg("%s connection failed",
						NameStr(mgrNode->form.nodename))));
	}
	return newMaster;
}

static SwitcherNodeWrapper *checkGetSwitchoverOldMaster(Oid oldMasterOid,
														char nodetype,
														MemoryContext spiContext)
{
	MgrNodeWrapper *mgrNode;
	SwitcherNodeWrapper *oldMaster;

	mgrNode = selectMgrNodeByOid(oldMasterOid, spiContext);
	if (!mgrNode)
	{
		ereport(ERROR,
				(errmsg("node oid(%d) does not exist", oldMasterOid)));
	}
	if (mgrNode->form.nodetype != nodetype)
	{
		ereport(ERROR,
				(errmsg("%s type(%s) is not a %s node",
						NameStr(mgrNode->form.nodename), mgr_get_nodetype_desc(mgrNode->form.nodetype), mgr_get_nodetype_desc(nodetype))));
	}
	if (!mgrNode->form.nodeinited)
	{
		ereport(ERROR,
				(errmsg("%s has not be initialized",
						NameStr(mgrNode->form.nodename))));
	}
	if (!mgrNode->form.nodeincluster)
	{
		ereport(ERROR,
				(errmsg("%s is not incluster",
						NameStr(mgrNode->form.nodename))));
	}
	oldMaster = palloc0(sizeof(SwitcherNodeWrapper));
	oldMaster->mgrNode = mgrNode;
	if (tryConnectNode(oldMaster, 10))
	{
		oldMaster->runningMode = getNodeRunningMode(oldMaster->pgConn);
		if (oldMaster->runningMode != NODE_RUNNING_MODE_MASTER && isMasterNode(nodetype, true))
		{
			pfreeSwitcherNodeWrapperPGconn(oldMaster);
			ereport(ERROR,
					(errmsg("%s expected running status is master mode, but actually is not",
							NameStr(mgrNode->form.nodename))));
		}
	}
	else
	{
		ereport(ERROR,
				(errmsg("%s connection failed",
						NameStr(mgrNode->form.nodename))));
	}
	return oldMaster;
}
static SwitcherNodeWrapper *checkGetSwitchoverOldMasterForZone(SwitcherNodeWrapper *holdLockCoordinator,
																Oid oldMasterOid,
																char nodetype,
																MemoryContext spiContext)
{
	MgrNodeWrapper *mgrNode;
	SwitcherNodeWrapper *oldMaster;

	mgrNode = selectMgrNodeByOid(oldMasterOid, spiContext);
	if (!mgrNode)
	{
		ereport(ERROR,
				(errmsg("node oid(%d) does not exist", oldMasterOid)));
	}
	if (mgrNode->form.nodetype != nodetype)
	{
		ereport(ERROR,
				(errmsg("%s type(%s) is not a %s node",
						NameStr(mgrNode->form.nodename), mgr_get_nodetype_desc(mgrNode->form.nodetype), mgr_get_nodetype_desc(nodetype))));
	}
	if (!mgrNode->form.nodeinited)
	{
		ereport(ERROR,
				(errmsg("%s has not be initialized",
						NameStr(mgrNode->form.nodename))));
	}
	if (!mgrNode->form.nodeincluster)
	{
		ereport(ERROR,
				(errmsg("%s is not incluster",
						NameStr(mgrNode->form.nodename))));
	}
	oldMaster = palloc0(sizeof(SwitcherNodeWrapper));
	oldMaster->mgrNode = mgrNode;
	oldMaster->runningMode = getNodeRunningModeEx(holdLockCoordinator, oldMaster);
	if (oldMaster->runningMode != NODE_RUNNING_MODE_MASTER && isMasterNode(nodetype, true))
	{
		pfreeSwitcherNodeWrapperPGconn(oldMaster);
		ereport(ERROR,
				(errmsg("%s expected running status is master mode, but actually is not",
						NameStr(mgrNode->form.nodename))));
	}
	return oldMaster;
}

static void runningSlavesFollowNewMaster(SwitcherNodeWrapper *newMaster,
										 SwitcherNodeWrapper *oldMaster,
										 dlist_head *runningSlaves,
										 MgrNodeWrapper *gtmMaster,
										 MemoryContext spiContext,
										 char *overType,
										 char *zone)
{
	dlist_mutable_iter iter;
	SwitcherNodeWrapper *slaveNode;
	dlist_head mgrNodes = DLIST_STATIC_INIT(mgrNodes);

	if (dlist_is_empty(runningSlaves))
	{
		appendToSyncStandbyNames(newMaster->mgrNode,
								 NULL,
								 newMaster->pgConn,
								 spiContext);
		return;
	}

	/* config these slave node to follow new master and restart them */
	dlist_foreach_modify(iter, runningSlaves)
	{
		slaveNode = dlist_container(SwitcherNodeWrapper,
									link, iter.cur);
		setPGHbaTrustSlaveReplication(newMaster->mgrNode,
									  slaveNode->mgrNode,
									  true);
		setSynchronousStandbyNames(slaveNode->mgrNode, "");
		setSlaveNodeRecoveryConf(newMaster->mgrNode,
								 slaveNode->mgrNode);
		pfreeSwitcherNodeWrapperPGconn(slaveNode);
	}

	if (gtmMaster)
		batchSetGtmInfoOnNodes(gtmMaster, runningSlaves, NULL, true);

	switcherNodesToMgrNodes(runningSlaves, &mgrNodes);

	batchShutdownNodesWithinSeconds(&mgrNodes,
									SHUTDOWN_NODE_FAST_SECONDS,
									SHUTDOWN_NODE_IMMEDIATE_SECONDS,
									true);

	batchStartupNodesWithinSeconds(&mgrNodes,
								   STARTUP_NODE_SECONDS,
								   false);

	dlist_foreach_modify(iter, runningSlaves)
	{
		slaveNode = dlist_container(SwitcherNodeWrapper,
									link, iter.cur);
		waitForNodeRunningOk(slaveNode->mgrNode,
							 false,
							 &slaveNode->pgConn,
							 &slaveNode->runningMode);
		
		appendToSyncStandbyNames(newMaster->mgrNode,
								 slaveNode->mgrNode,
								 newMaster->pgConn,
								 spiContext);
		ereport(LOG,
				(errmsg("%s has followed master %s",
						NameStr(slaveNode->mgrNode->form.nodename),
						NameStr(newMaster->mgrNode->form.nodename))));
	}
	if (gtmMaster)
		batchCheckGtmInfoOnNodes(gtmMaster,
								 runningSlaves,
								 NULL,
								 CHECK_GTM_INFO_SECONDS,
								 true);
}

static int walLsnDesc(const void *node1, const void *node2)
{
	return (*(SwitcherNodeWrapper **)node2)->walLsn -
		   (*(SwitcherNodeWrapper **)node1)->walLsn;
}

static void checkSet_pool_release_to_idle_timeout(SwitcherNodeWrapper *node)
{
	char *parameterName;
	char *expectValue;
	char *originalValue;
	PGConfParameterItem *originalItems;
	PGConfParameterItem *expectItems;
	bool execOk = false;
	int nTrys;

	CheckNull(node);
	CheckNull(node->pgConn);

	parameterName = "pool_release_to_idle_timeout";
	expectValue = "-1";
	originalValue = showNodeParameter(NameStr(node->mgrNode->form.nodename), 
										node->pgConn, 
										parameterName, 
										false);
	if (strcmp(originalValue, expectValue) == 0)
	{
		ereport(LOG, (errmsg("parameter %s on node %s already is %s,"
							" no need to set",
							NameStr(node->mgrNode->form.nodename),
							parameterName, expectValue)));
		pfree(originalValue);
		return;
	}

	originalItems = newPGConfParameterItem(parameterName,
										   originalValue, true);
	pfree(originalValue);

	/* We will revert these settings after switching is complete. */
	node->originalParameterItems = originalItems;
	expectItems = newPGConfParameterItem(parameterName,
										 expectValue, true);
	execOk = callAgentRefreshPGSqlConfReload(node->mgrNode,
											 expectItems, false);
	pfreePGConfParameterItem(expectItems);
	if (execOk)
	{
		for (nTrys = 0; nTrys < 10; nTrys++)
		{
			/*sleep 0.1s*/
			pg_usleep(100000L);
			/* check the param */
			execOk = equalsNodeParameter(NameStr(node->mgrNode->form.nodename),
										 node->pgConn,
										 parameterName,
										 expectValue);
			if (execOk)
				break;
		}
	}
	if (execOk)
		ereport(LOG, (errmsg("%s set %s = %s successful",
							 NameStr(node->mgrNode->form.nodename),
							 parameterName, expectValue)));
	else
		ereport(ERROR, (errmsg("%s set %s = %s failed",
							   NameStr(node->mgrNode->form.nodename),
							   parameterName, expectValue)));
}

void waitForNodeRunningOk(MgrNodeWrapper *mgrNode,
							bool isMaster,
							PGconn **pgConnP,
							NodeRunningMode *runningModeP)
{
	NodeConnectionStatus connStatus;
	NodeRunningMode runningMode;
	NodeRunningMode expectedRunningMode;
	PGconn *pgConn = NULL;
	int networkFailures = 0;
	int maxNetworkFailures = 60;
	int runningModeFailures = 0;
	int maxRunningModeFailures = 60;
 
	while (networkFailures < maxNetworkFailures)
	{
		connStatus = connectNodeDefaultDB(mgrNode, 10, &pgConn);
		if (connStatus == NODE_CONNECTION_STATUS_SUCCESS ||
			connStatus == NODE_CONNECTION_STATUS_CANNOT_SHUTTING_DOWN)
		{
			break;
		}
		else if (connStatus == NODE_CONNECTION_STATUS_BUSY ||
				 connStatus == NODE_CONNECTION_STATUS_CANNOT_CONNECT_NOW)
		{
			/* wait for start up */
			networkFailures = 0;
		}
		else
		{
			setPGHbaTrustMyself(mgrNode);
			networkFailures++;
		}
		fputs(_("."), stdout);
		fflush(stdout);
		pg_usleep(1 * 1000000L);
	}
	if (connStatus != NODE_CONNECTION_STATUS_SUCCESS)
	{
		ereport(ERROR,
				(errmsg("%s connection failed, may crashed",
						NameStr(mgrNode->form.nodename))));
	}

	expectedRunningMode = getExpectedNodeRunningMode(isMaster);
	while (runningModeFailures < maxRunningModeFailures)
	{
		runningMode = getNodeRunningMode(pgConn);
		if (runningMode == expectedRunningMode)
		{
			runningModeFailures = 0;
			break;
		}
		else if (runningMode == NODE_RUNNING_MODE_UNKNOW)
		{
			runningModeFailures++;
		}
		else
		{
			break;
		}
		fputs(_("."), stdout);
		fflush(stdout);
		pg_usleep(1 * 1000000L);
	}

	if (pgConnP)
	{
		if (*pgConnP)
		{
			PQfinish(*pgConnP);
			*pgConnP = NULL;
		}
		*pgConnP = pgConn;
	}
	else
	{
		if (pgConn)
		{
			PQfinish(pgConn);
			pgConn = NULL;
		}
	}
	if (runningModeP)
	{
		*runningModeP = runningMode;
	}
	if (runningMode == expectedRunningMode)
	{
		ereportNoticeLog(errmsg("%s running on correct status of %s mode",
						NameStr(mgrNode->form.nodename),
						isMaster ? "master" : "slave"));
	}
	else
	{
		ereport(LOG,
				(errmsg("%s recovery status error, runningMode(%d) != expectedRunningMode(%d).",
						NameStr(mgrNode->form.nodename), runningMode, expectedRunningMode)));
		ereport(ERROR,
				(errmsg("%s recovery status error",
						NameStr(mgrNode->form.nodename))));
	}
}

static void promoteNewMasterStartReign(SwitcherNodeWrapper *oldMaster,
									   SwitcherNodeWrapper *newMaster)
{
	/* If the lastest switch failed, it is possible that a standby node 
	 * has been promoted to the master, so it's not need to promote again. */
	if (newMaster->runningMode != NODE_RUNNING_MODE_MASTER)
	{
		/* The old connection may mistake the judgment of whether new master 
		 * is running normally. So close it first, we will reconnect this node 
		 * after promotion completed. */
		pfreeSwitcherNodeWrapperPGconn(newMaster);
		callAgentPromoteNode(newMaster->mgrNode, true);
	}
	/* 
	 * the slave has been  promoted to master, prohibit switching back to  
	 * the oldMaster to avoid data loss. 
	 */
	oldMaster->startupAfterException = false;		
	waitForNodeRunningOk(newMaster->mgrNode,
						 true,
						 &newMaster->pgConn,
						 &newMaster->runningMode);
	setCheckSynchronousStandbyNames(newMaster->mgrNode,
									newMaster->pgConn,
									"",
									CHECK_SYNC_STANDBY_NAMES_SECONDS);
}

static void refreshMgrNodeBeforeSwitch(SwitcherNodeWrapper *node,
									   MemoryContext spiContext)
{
	char *newCurestatus;
	/* Use CURE_STATUS_SWITCHING as a tag to prevent the node doctor from 
	 * operating on this node simultaneously. */
	newCurestatus = CURE_STATUS_SWITCHING;
	/* backup curestatus */
	memcpy(&node->oldCurestatus,
		   &node->mgrNode->form.curestatus, sizeof(NameData));
	updateCureStatusForSwitch(node->mgrNode,
							  newCurestatus,
							  spiContext);
}
static void refreshMgrNodeListBeforeSwitch(MemoryContext spiContext, dlist_head *nodes)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	dlist_foreach(iter, nodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		refreshMgrNodeBeforeSwitch(node, spiContext);
	}
}
static void
refreshSyncToAsync(MemoryContext spiContext, dlist_head *nodes)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	dlist_foreach(iter, nodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		Assert(node);
		if ((is_equal_string(NameStr(node->mgrNode->form.nodesync), getMgrNodeSyncStateValue(SYNC_STATE_SYNC))) ||
			(is_equal_string(NameStr(node->mgrNode->form.nodesync), getMgrNodeSyncStateValue(SYNC_STATE_POTENTIAL))))
		{
			namestrcpy(&node->oldNodeSync, NameStr(node->mgrNode->form.nodesync));
			node->syncStateChanged = true;

			char *newSyncState = getMgrNodeSyncStateValue(SYNC_STATE_ASYNC);
			updateSyncStatusForSwitch(node->mgrNode, newSyncState, spiContext);
		}
	}
}
void
refreshAsyncToSync(MemoryContext spiContext, dlist_head *nodes)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	dlist_foreach(iter, nodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		Assert(node);
		if (node->syncStateChanged)
		{
			updateSyncStatusForSwitch(node->mgrNode, NameStr(node->oldNodeSync), spiContext);
			node->syncStateChanged = false;
		}
	}
}

static void refreshMgrNodeListAfterFailoverGtm(MemoryContext spiContext, dlist_head *nodes)
{
	dlist_iter 			iter;
	SwitcherNodeWrapper *node;

	dlist_foreach(iter, nodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		Assert(node);
		refreshOtherNodeAfterSwitchGtmCoord(node, spiContext);
	}
}
static void refreshOldMasterBeforeSwitch(SwitcherNodeWrapper *oldMaster,
										 MemoryContext spiContext)
{
	refreshMgrNodeBeforeSwitch(oldMaster, spiContext);
}

static void refreshSlaveNodesBeforeSwitch(SwitcherNodeWrapper *newMaster,
										  dlist_head *runningSlaves,
										  dlist_head *failedSlaves,
										  dlist_head *runningSlavesSecond,
										  dlist_head *failedSlavesSecond,
										  MemoryContext spiContext)
{
	refreshMgrNodeBeforeSwitch(newMaster, spiContext);
    refreshMgrNodeListBeforeSwitch(spiContext, runningSlaves);
	refreshMgrNodeListBeforeSwitch(spiContext, failedSlaves);
	refreshMgrNodeListBeforeSwitch(spiContext, runningSlavesSecond);
	refreshMgrNodeListBeforeSwitch(spiContext, failedSlavesSecond);
}

static void refreshSlaveNodesAfterSwitch(SwitcherNodeWrapper *newMaster,
										 SwitcherNodeWrapper *oldMaster,
										 dlist_head *runningSlaves,
										 dlist_head *failedSlaves,
										 dlist_head *runningSlavesSecond,
										 dlist_head *failedSlavesSecond,
										 MemoryContext spiContext,
										 char *operType,
										 char *curZone)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	refreshNewMasterAfterSwitchover(newMaster, spiContext);

	dlist_foreach(iter, runningSlaves)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);

		if (pg_strcasecmp(NameStr(oldMaster->mgrNode->form.nodezone), curZone) == 0)
		{
			node->mgrNode->form.nodemasternameoid = newMaster->mgrNode->form.oid;
		}

		if ((pg_strcasecmp(NameStr(oldMaster->mgrNode->form.nodezone), curZone) != 0) &&
			(pg_strcasecmp(operType, OVERTYPE_FAILOVER) == 0) &&
		    (pg_strcasecmp(NameStr(node->mgrNode->form.nodezone), curZone) != 0))
		{
			node->mgrNode->form.nodeinited    = false;
			node->mgrNode->form.nodeincluster = false;
			ereport(LOG, (errmsg("failover node(%s) is set to not inited, not incluster. nodezone(%s),oldMaster nodezone(%s), curZone(%s)", 
				NameStr(node->mgrNode->form.nodename), NameStr(node->mgrNode->form.nodezone), NameStr(oldMaster->mgrNode->form.nodezone), curZone)));	
		}
		updateMgrNodeAfterSwitch(node->mgrNode, CURE_STATUS_SWITCHED, spiContext);
	}

	if (runningSlavesSecond != NULL)
	{
		dlist_foreach(iter, runningSlavesSecond)
		{
			node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			if ((pg_strcasecmp(NameStr(oldMaster->mgrNode->form.nodezone), curZone) != 0) &&
				(pg_strcasecmp(operType, OVERTYPE_FAILOVER) == 0) &&
				(pg_strcasecmp(NameStr(node->mgrNode->form.nodezone), curZone) != 0))
			{
				node->mgrNode->form.nodeinited    = false;
				node->mgrNode->form.nodeincluster = false;
				ereport(LOG, (errmsg("failover node(%s) is set to not inited, not incluster. nodezone(%s),oldMaster nodezone(%s), curZone(%s)", 
					NameStr(node->mgrNode->form.nodename), NameStr(node->mgrNode->form.nodezone), NameStr(oldMaster->mgrNode->form.nodezone), curZone)));	
			}
			updateMgrNodeAfterSwitch(node->mgrNode, CURE_STATUS_SWITCHED, spiContext);
		}
	}

	dlist_foreach(iter, failedSlaves)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (pg_strcasecmp(NameStr(oldMaster->mgrNode->form.nodezone), curZone) == 0)
		{
			node->mgrNode->form.nodemasternameoid = newMaster->mgrNode->form.oid;
		}
		if ((pg_strcasecmp(NameStr(oldMaster->mgrNode->form.nodezone), curZone) != 0) &&
			(pg_strcasecmp(operType, OVERTYPE_FAILOVER) == 0) &&
			(pg_strcasecmp(NameStr(node->mgrNode->form.nodezone), curZone) != 0))
		{	
			node->mgrNode->form.nodeinited    = false;
			node->mgrNode->form.nodeincluster = false;	
			ereport(LOG, (errmsg("failover node(%s) is set to not inited, not incluster. nodezone(%s),oldMaster nodezone(%s), curZone(%s)", 
				NameStr(node->mgrNode->form.nodename), NameStr(node->mgrNode->form.nodezone), NameStr(oldMaster->mgrNode->form.nodezone), curZone)));
			
		}
		/* Update other failure slave node follow the new master, 
		 * Then, The node "follow the new master node" task is handed over 
		 * to the node doctor. */
		if (pg_strcasecmp(NameStr(node->oldCurestatus),
						  CURE_STATUS_OLD_MASTER) == 0 ||
			pg_strcasecmp(NameStr(node->oldCurestatus),
						  CURE_STATUS_ISOLATED) == 0)
		{
			updateMgrNodeAfterSwitch(node->mgrNode,
									 NameStr(node->oldCurestatus),
									 spiContext);
		}
		else
		{
			updateMgrNodeAfterSwitch(node->mgrNode,
									 CURE_STATUS_FOLLOW_FAIL,
									 spiContext);
		}
	}

	if (failedSlavesSecond != NULL)
	{
		dlist_foreach(iter, failedSlavesSecond)
		{
			node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			if ((pg_strcasecmp(NameStr(oldMaster->mgrNode->form.nodezone), curZone) != 0) &&
				(pg_strcasecmp(operType, OVERTYPE_FAILOVER) == 0) &&
				(pg_strcasecmp(NameStr(node->mgrNode->form.nodezone), curZone) != 0))
			{
				node->mgrNode->form.nodeinited    = false;
				node->mgrNode->form.nodeincluster = false;
				ereport(LOG, (errmsg("failover node(%s) is set to not inited, not incluster. nodezone(%s),oldMaster nodezone(%s), curZone(%s)", 
					NameStr(node->mgrNode->form.nodename), NameStr(node->mgrNode->form.nodezone), NameStr(oldMaster->mgrNode->form.nodezone), curZone)));
			}
			/* Update other failure slave node follow the new master, 
			* Then, The node "follow the new master node" task is handed over 
			* to the node doctor. */
			if (pg_strcasecmp(NameStr(node->oldCurestatus),
							CURE_STATUS_OLD_MASTER) == 0 ||
				pg_strcasecmp(NameStr(node->oldCurestatus),
							CURE_STATUS_ISOLATED) == 0)
			{
				updateMgrNodeAfterSwitch(node->mgrNode,
										NameStr(node->oldCurestatus),
										spiContext);
			}
			else
			{
				updateMgrNodeAfterSwitch(node->mgrNode,
										CURE_STATUS_FOLLOW_FAIL,
										spiContext);
			}
		}
	}
}

static void refreshOldMasterAfterSwitch(SwitcherNodeWrapper *oldMaster,
										SwitcherNodeWrapper *newMaster,
										MemoryContext spiContext,
										bool kickOutOldMaster)
{
	if (kickOutOldMaster)
	{
		/* Mark the data group to which the old master belongs */
		oldMaster->mgrNode->form.nodemasternameoid = newMaster->mgrNode->form.oid;
		oldMaster->mgrNode->form.nodetype =
			getMgrSlaveNodetype(oldMaster->mgrNode->form.nodetype);
		oldMaster->mgrNode->form.nodeinited = false;
		oldMaster->mgrNode->form.nodeincluster = false;
		oldMaster->mgrNode->form.allowcure = false;
		/* Kick the old master out of the cluster */
		updateMgrNodeAfterSwitch(oldMaster->mgrNode,
								 CURE_STATUS_NORMAL,
								 spiContext);
		ereport(LOG, (errmsg("%s has been kicked out of the cluster",
							 NameStr(oldMaster->mgrNode->form.nodename))));
		ereport(NOTICE, (errmsg("%s has been kicked out of the cluster",
								NameStr(oldMaster->mgrNode->form.nodename))));
	}
	else
	{
		/* Mark the data group to which the old master belongs */
		oldMaster->mgrNode->form.nodemasternameoid = newMaster->mgrNode->form.oid;
		oldMaster->mgrNode->form.nodetype =
			getMgrSlaveNodetype(oldMaster->mgrNode->form.nodetype);
		/* Update Old master follow the new master, 
		 * Then, the task of pg_rewind this old master is handled to the node doctor. */
		updateMgrNodeAfterSwitch(oldMaster->mgrNode,
								 CURE_STATUS_OLD_MASTER,
								 spiContext);

		ereportNoticeLog(errmsg("%s is waiting for rewinding. If the doctor is enabled, "
						"the doctor will automatically rewind it",
						NameStr(oldMaster->mgrNode->form.nodename)));
	}
}

static void refreshOtherNodeAfterSwitchGtmCoord(SwitcherNodeWrapper *node,
												MemoryContext spiContext)
{
	if (pg_strcasecmp(NameStr(node->oldCurestatus),
					  CURE_STATUS_NORMAL) == 0)
	{
		updateCureStatusForSwitch(node->mgrNode,
								  CURE_STATUS_SWITCHED,
								  spiContext);
	}
	else
	{
		updateCureStatusForSwitch(node->mgrNode,
								  NameStr(node->oldCurestatus),
								  spiContext);
	}
}

static void refreshOldMasterAfterSwitchover(SwitcherNodeWrapper *oldMaster,
											SwitcherNodeWrapper *newMaster,
											MemoryContext spiContext)
{
	oldMaster->mgrNode->form.nodetype =	getMgrSlaveNodetype(oldMaster->mgrNode->form.nodetype);
	/* Mark the data group to which the old master belongs */
	oldMaster->mgrNode->form.nodemasternameoid = newMaster->mgrNode->form.oid;
	/* Admit the reign of new master */
	updateMgrNodeAfterSwitch(oldMaster->mgrNode,
							 CURE_STATUS_SWITCHED,
							 spiContext);
}
static void refreshNewMasterAfterSwitchover(SwitcherNodeWrapper *newMaster,	MemoryContext spiContext)
{
	newMaster->mgrNode->form.nodetype = getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
	newMaster->mgrNode->form.nodemasternameoid = 0;
	/* Why is there a curestatus of CURE_STATUS_SWITCHED? 
	 * Because we can use this field as a tag to prevent the node doctor 
	 * from operating on this node simultaneously. */
	updateMgrNodeAfterSwitch(newMaster->mgrNode, CURE_STATUS_SWITCHED, spiContext);
}
static void refreshMgrUpdateparmAfterSwitch(MgrNodeWrapper *oldMaster,
											MgrNodeWrapper *newMaster,
											MemoryContext spiContext,
											bool kickOutOldMaster)
{
	if (kickOutOldMaster)
	{
		deleteMgrUpdateparmByNodenameType(NameStr(oldMaster->form.nodename),
										  getMgrMasterNodetype(oldMaster->form.nodetype),
										  spiContext);
	}
	else
	{
		/* old master update to slave */
		updateMgrUpdateparmNodetype(NameStr(oldMaster->form.nodename),
									getMgrSlaveNodetype(oldMaster->form.nodetype),
									spiContext);
	}

	/* new master update to master */
	updateMgrUpdateparmNodetype(NameStr(newMaster->form.nodename),
								getMgrMasterNodetype(newMaster->form.nodetype),
								spiContext);
}

static void refreshPgxcNodesOfCoordinators(SwitcherNodeWrapper *holdLockNode,
										   dlist_head *coordinators,
										   SwitcherNodeWrapper *oldMaster,
										   SwitcherNodeWrapper *newMaster)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	/*
	 * When cluster is locked, the connection which 
	 * execute the lock command is the only active connection 
	 */
	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		node->pgxcNodeChanged =
			updatePgxcNodeForSwitch(holdLockNode,
									node,
									oldMaster,
									newMaster,
									true);
		pgxcPoolReloadOnNode(holdLockNode,
							 node,
							 true);
	}
}
static void checkCreateDataNodeSlaveOnPgxcNodeOfMaster(PGconn *activeConn,
													   char *masterNodeName,
													   bool localExecute,
													   MgrNodeWrapper *dataNodeSlave,
													   bool complain)
{
	char *slaveNodeName;

	slaveNodeName = NameStr(dataNodeSlave->form.nodename);
	if (nodenameExistsInPgxcNode(activeConn,
								 masterNodeName,
								 localExecute,
								 slaveNodeName,
								 PGXC_NODE_DATANODESLAVE,
								 complain))
	{
		dropNodeFromPgxcNode(activeConn,
							 masterNodeName,
							 localExecute,
							 slaveNodeName,
							 complain);
	}
	createNodeOnPgxcNode(activeConn,
						 masterNodeName,
						 localExecute,
						 dataNodeSlave,
						 masterNodeName,
						 complain);
}

/* 
 * When the read-write separation is turned on, the reduce process on the 
 * standby node will read the node information in the pgxc_node table. 
 * Therefore, the information of the standby node added to the master node 
 * will be automatically synchronized to the slave node.
 */
static void refreshPgxcNodesOfNewDataNodeMaster(SwitcherNodeWrapper *holdLockNode,
												SwitcherNodeWrapper *oldMaster,
												SwitcherNodeWrapper *newMaster,
												bool complain)
{
	PGconn *activeConn;
	bool localExecute;
	char *newMasterNodeName;
	MgrNodeWrapper copyOfMgrNode;

	Assert(holdLockNode);
	activeConn = holdLockNode->pgConn;
	localExecute = (holdLockNode == newMaster);
	newMasterNodeName = NameStr(newMaster->mgrNode->form.nodename);

	updatePgxcNodeForSwitch(holdLockNode,
							newMaster,
							oldMaster,
							newMaster,
							complain);
								
	memcpy(&copyOfMgrNode, oldMaster->mgrNode, sizeof(MgrNodeWrapper));
	copyOfMgrNode.form.nodetype = getMgrSlaveNodetype(copyOfMgrNode.form.nodetype);
	checkCreateDataNodeSlaveOnPgxcNodeOfMaster(activeConn,
											   newMasterNodeName,
											   localExecute,
											   &copyOfMgrNode,
											   complain);
}
static void refreshReplicationSlots(SwitcherNodeWrapper *oldMaster,
									SwitcherNodeWrapper *newMaster)
{
	dn_master_replication_slot(NameStr(oldMaster->mgrNode->form.nodename), NameStr(newMaster->mgrNode->form.nodename),'d');
	dn_master_replication_slot(NameStr(newMaster->mgrNode->form.nodename), NameStr(oldMaster->mgrNode->form.nodename),'c');
}
/**
 * update curestatus can avoid adb doctor monitor this node
 */
void updateCureStatusForSwitch(MgrNodeWrapper *mgrNode,
							   char *newCurestatus,
							   MemoryContext spiContext)
{
	int spiRes;
	MemoryContext oldCtx;
	StringInfoData sql;

	initStringInfo(&sql);
	appendStringInfo(&sql,
					 "UPDATE pg_catalog.mgr_node \n"
					 "SET curestatus = '%s' \n"
					 "WHERE oid = %u \n"
					 "AND curestatus = '%s' \n"
					 "AND nodetype = '%c' \n",
					 newCurestatus,
					 mgrNode->form.oid,
					 NameStr(mgrNode->form.curestatus),
					 mgrNode->form.nodetype);
	oldCtx = MemoryContextSwitchTo(spiContext);
	spiRes = SPI_execute(sql.data, false, 0);
	MemoryContextSwitchTo(oldCtx);

	pfree(sql.data);
	if (spiRes != SPI_OK_UPDATE)
	{
		ereport(ERROR,
				(errmsg("SPI_execute failed: error code %d",
						spiRes)));
	}
	if (SPI_processed != 1)
	{
		ereport(ERROR,
				(errmsg("SPI_execute failed: expected rows:%d, actually:%lu",
						1,
						SPI_processed)));
	}
	namestrcpy(&mgrNode->form.curestatus, newCurestatus);
}
void updateSyncStatusForSwitch(MgrNodeWrapper *mgrNode,
							   char *newSyncState,
							   MemoryContext spiContext)
{
	int spiRes;
	MemoryContext oldCtx;
	StringInfoData sql;

	initStringInfo(&sql);
	appendStringInfo(&sql,
					 "UPDATE pg_catalog.mgr_node \n"
					 "SET nodesync = '%s' \n"
					 "WHERE oid = %u \n"
					 "AND nodetype = '%c' \n",
					 newSyncState,
					 mgrNode->form.oid,
					 mgrNode->form.nodetype);
	oldCtx = MemoryContextSwitchTo(spiContext);
	spiRes = SPI_execute(sql.data, false, 0);
	MemoryContextSwitchTo(oldCtx);

	pfree(sql.data);
	if (spiRes != SPI_OK_UPDATE)
	{
		ereport(ERROR,
				(errmsg("SPI_execute failed: error code %d",
						spiRes)));
	}
	if (SPI_processed != 1)
	{
		ereport(ERROR,
				(errmsg("SPI_execute failed: expected rows:%d, actually:%lu",
						1,
						SPI_processed)));
	}
	namestrcpy(&mgrNode->form.nodesync, newSyncState);
}

void updateMgrNodeAfterSwitch(MgrNodeWrapper *mgrNode,
									 char *newCurestatus,
									 MemoryContext spiContext)
{
	int spiRes;
	MemoryContext oldCtx;
	StringInfoData sql;

	initStringInfo(&sql);
	appendStringInfo(&sql,
					 "UPDATE pg_catalog.mgr_node \n"
					 "SET nodetype = '%c', \n"
					 "nodemasternameoid = %u, \n"
					 "nodeinited =%d::boolean, \n"
					 "nodeincluster =%d::boolean, \n"
					 "allowcure =%d::boolean, \n"
					 "curestatus = '%s' \n"
					 "WHERE oid = %u \n"
					 "AND curestatus = '%s' \n",
					 mgrNode->form.nodetype,
					 mgrNode->form.nodemasternameoid,
					 mgrNode->form.nodeinited,
					 mgrNode->form.nodeincluster,
					 mgrNode->form.allowcure,
					 newCurestatus,
					 mgrNode->form.oid,
					 NameStr(mgrNode->form.curestatus));
	oldCtx = MemoryContextSwitchTo(spiContext);
	spiRes = SPI_execute(sql.data, false, 0);
	MemoryContextSwitchTo(oldCtx);

	pfree(sql.data);
	if (spiRes != SPI_OK_UPDATE)
	{
		ereport(ERROR,
				(errmsg("SPI_execute failed: error code %d",
						spiRes)));
	}
	if (SPI_processed != 1)
	{
		ereport(ERROR,
				(errmsg("SPI_execute failed: expected rows:%d, actually:%lu",
						1,
						SPI_processed)));
	}
	namestrcpy(&mgrNode->form.curestatus, newCurestatus);
}

static void deleteMgrUpdateparmByNodenameType(char *updateparmnodename,
											  char updateparmnodetype,
											  MemoryContext spiContext)
{
	int spiRes;
	MemoryContext oldCtx;
	StringInfoData sql;

	initStringInfo(&sql);
	appendStringInfo(&sql,
					 "DELETE FROM mgr_updateparm \n"
					 "WHERE updateparmnodename = '%s' \n"
					 "AND updateparmnodetype = '%c' \n",
					 updateparmnodename,
					 updateparmnodetype);
	oldCtx = MemoryContextSwitchTo(spiContext);
	spiRes = SPI_execute(sql.data, false, 0);
	MemoryContextSwitchTo(oldCtx);

	pfree(sql.data);
	if (spiRes != SPI_OK_DELETE)
	{
		ereport(ERROR,
				(errmsg("SPI_execute failed: error code %d",
						spiRes)));
	}
}

static void updateMgrUpdateparmNodetype(char *updateparmnodename,
										char updateparmnodetype,
										MemoryContext spiContext)
{
	int spiRes;
	MemoryContext oldCtx;
	StringInfoData sql;

	initStringInfo(&sql);
	appendStringInfo(&sql,
					 "UPDATE mgr_updateparm \n"
					 "SET updateparmnodetype = '%c' \n"
					 "WHERE updateparmnodename = '%s' \n",
					 updateparmnodetype,
					 updateparmnodename);
	oldCtx = MemoryContextSwitchTo(spiContext);
	spiRes = SPI_execute(sql.data, false, 0);
	MemoryContextSwitchTo(oldCtx);

	pfree(sql.data);
	if (spiRes != SPI_OK_UPDATE)
	{
		ereport(ERROR,
				(errmsg("SPI_execute failed: error code %d",
						spiRes)));
	}
}

static void refreshPgxcNodeBeforeSwitchDataNode(dlist_head *coordinators)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		deletePgxcNodeDataNodeSlaves(node, true);
	}
}

static bool deletePgxcNodeDataNodeSlaves(SwitcherNodeWrapper *coordinator,
										 bool complain)
{
	char *sql;
	bool execOk;

	sql = psprintf("set FORCE_PARALLEL_MODE = off; "
				   "delete from pgxc_node where node_type = '%c';",
				   PGXC_NODE_DATANODESLAVE);
	execOk = PQexecCommandSql(coordinator->pgConn, sql, false);
	pfree(sql);
	if (!execOk)
	{
		ereport(complain ? ERROR : LOG,
				(errmsg("%s delete datanode slaves from pgxc_node failed",
						NameStr(coordinator->mgrNode->form.nodename))));
	}
	return execOk;
}

static bool updatePgxcNodeForSwitch(SwitcherNodeWrapper *holdLockNode,
									SwitcherNodeWrapper *executeOnNode, 
									SwitcherNodeWrapper *oldNode,
									SwitcherNodeWrapper *newNode, 
									bool complain)
{
	bool execOk = false;
	PGconn *activeConn;
	bool localExecute;
	char *executeOnNodeName;

	if (holdLockNode == NULL)
	{
		activeConn = executeOnNode->pgConn;
		localExecute = true;
	}
	else
	{
		activeConn = holdLockNode->pgConn;
		if ((holdLockNode == executeOnNode) || 
			(pg_strcasecmp(NameStr(holdLockNode->mgrNode->form.nodename), NameStr(executeOnNode->mgrNode->form.nodename)) == 0))
		{
			localExecute = true;
		}
		else{
			localExecute = false;
		}
	}

	executeOnNodeName = NameStr(executeOnNode->mgrNode->form.nodename);
	if (nodenameExistsInPgxcNode(activeConn,
								 executeOnNodeName,
								 localExecute,
								 NameStr(newNode->mgrNode->form.nodename),
								 PGXC_NODE_DATANODESLAVE,
								 complain))
	{
		dropNodeFromPgxcNode(activeConn,
							 executeOnNodeName,
							 localExecute,
							 NameStr(newNode->mgrNode->form.nodename),
							 complain);
	}
	if (nodenameExistsInPgxcNode(activeConn,
								 executeOnNodeName,
								 localExecute,
								 NameStr(newNode->mgrNode->form.nodename),
								 (char)0,
								 complain))
	{
		execOk = alterNodeOnPgxcNode(activeConn,
									 executeOnNodeName,
									 localExecute,
									 NameStr(newNode->mgrNode->form.nodename),
									 newNode->mgrNode,
									 complain);
	}
	else
	{
		if (nodenameExistsInPgxcNode(activeConn,
									 executeOnNodeName,
									 localExecute,
									 NameStr(oldNode->mgrNode->form.nodename),
									 (char)0,
									 complain))
		{
			execOk = alterNodeOnPgxcNode(activeConn,
									 executeOnNodeName,
									 localExecute,
									 NameStr(oldNode->mgrNode->form.nodename),
									 newNode->mgrNode,
									 complain);
		}
		else
		{
			ereport(complain ? ERROR : WARNING,
					(errmsg("old node %s do not exsits in pgxc_node of %s",
							NameStr(oldNode->mgrNode->form.nodename),
							NameStr(executeOnNode->mgrNode->form.nodename))));
		}
	}
	return execOk;
}

static bool pgxcPoolReloadOnNode(SwitcherNodeWrapper *holdLockNode,
								 SwitcherNodeWrapper *executeOnNode,
								 bool complain)
{
	PGconn *activeConn;
	bool localExecute;

	if (holdLockNode)
	{
		activeConn = holdLockNode->pgConn;
		if ((holdLockNode == executeOnNode) || 
			(pg_strcasecmp(NameStr(holdLockNode->mgrNode->form.nodename), NameStr(executeOnNode->mgrNode->form.nodename)) == 0))
		{
			localExecute = true;
		}
		else{
			localExecute = false;
		}
	}
	else
	{
		activeConn = executeOnNode->pgConn;
		localExecute = true;
	}
	return exec_pgxc_pool_reload(activeConn,
								 localExecute,
								 NameStr(executeOnNode->mgrNode->form.nodename),
								 complain);
}


SwitcherNodeWrapper *getHoldLockCoordinator(dlist_head *coordinators)
{
	dlist_iter iter;
	SwitcherNodeWrapper *coordinator;

	dlist_foreach(iter, coordinators)
	{
		coordinator = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (coordinator->holdClusterLock)
		{
			return coordinator;
		}
	}
	return NULL;
}

static SwitcherNodeWrapper *getGtmCoordMaster(dlist_head *coordinators)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (node->mgrNode->form.nodetype == CNDN_TYPE_GTM_COOR_MASTER)
		{
			return node;
		}
	}
	return NULL;
}
static void batchSetGtmInfoOnNodes(MgrNodeWrapper *gtmMaster,
								   dlist_head *nodes,
								   SwitcherNodeWrapper *ignoreNode,
								   bool complain)
{
	dlist_mutable_iter iter;
	SwitcherNodeWrapper *node;
	bool setSucc;

	dlist_foreach_modify(iter, nodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (node != ignoreNode)
		{
			setSucc = setGtmInfoInPGSqlConf(node->mgrNode,
											gtmMaster,
											complain);
			if (setSucc)
				node->gtmInfoChanged = true;
			
			ereport(LOG,
					(errmsg("set GTM information on %s %s",
							NameStr(node->mgrNode->form.nodename),
							setSucc ? "successfully" : "failed")));
		}
	}
}

static void batchCheckGtmInfoOnNodes(MgrNodeWrapper *gtmMaster,
									 dlist_head *nodes,
									 SwitcherNodeWrapper *ignoreNode,
									 int checkSeconds,
									 bool complain)
{
	dlist_mutable_iter iter;
	SwitcherNodeWrapper *node;
	SwitcherNodeWrapper *copyOfNode;
	dlist_head copyOfNodes = DLIST_STATIC_INIT(copyOfNodes);
	int seconds;

	dlist_foreach_modify(iter, nodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (node != ignoreNode)
		{
			copyOfNode = palloc(sizeof(SwitcherNodeWrapper));
			memcpy(copyOfNode, node, sizeof(SwitcherNodeWrapper));
			dlist_push_tail(&copyOfNodes, &copyOfNode->link);
		}
	}

	for (seconds = 0; seconds <= checkSeconds; seconds++)
	{
		dlist_foreach_modify(iter, &copyOfNodes)
		{
			node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			if (checkGtmInfoInPGSqlConf(node->pgConn,
										NameStr(node->mgrNode->form.nodename),
										true,
										gtmMaster))
			{
				ereport(LOG,
						(errmsg("check GTM information on %s is correct",
								NameStr(node->mgrNode->form.nodename))));
				dlist_delete(iter.cur);
				pfree(node);
			}
			else
			{
				ereport(LOG,
						(errmsg("check GTM information on %s is error, sleep %d s",
								NameStr(node->mgrNode->form.nodename), seconds+1)));
			}
		}
		if (dlist_is_empty(&copyOfNodes))
		{
			break;
		}
		else
		{
			if (seconds < checkSeconds)
				pg_usleep(1000000L);
		}
	}

	if (!dlist_is_empty(&copyOfNodes))
	{
		dlist_foreach_modify(iter, &copyOfNodes)
		{
			node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			ereport(complain ? ERROR : LOG,
					(errmsg("check GTM information on %s failed",
							NameStr(node->mgrNode->form.nodename))));
			dlist_delete(iter.cur);
			pfree(node);
		}
	}
}
/* resetart the node, makesure the snapreceiver of node connect to the new gtm */
static void restartNodes(dlist_head *nodes, bool complain)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;

	dlist_foreach(iter, nodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		Assert(node);
		if (node->mgrNode->form.nodetype != CNDN_TYPE_GTM_COOR_MASTER &&
			mgr_check_agent_running(node->mgrNode->form.nodehost))
		{
			shutdownNodeWithinSeconds(node->mgrNode,
									SHUTDOWN_NODE_FAST_SECONDS,
									SHUTDOWN_NODE_IMMEDIATE_SECONDS,
									complain);
			callAgentStartNode(node->mgrNode, false, complain);
			if (isMasterNode(node->mgrNode->form.nodetype, true))
				waitForNodeRunningOk(node->mgrNode, true, NULL, NULL);
			else
				waitForNodeRunningOk(node->mgrNode, false, NULL, NULL);	
		}
	}
}
static void batchSetCheckGtmInfoOnNodes(MgrNodeWrapper *gtmMaster,
										dlist_head *nodes,
										SwitcherNodeWrapper *ignoreNode,
										bool complain)
{
	batchSetGtmInfoOnNodes(gtmMaster, nodes, ignoreNode, complain);
	batchCheckGtmInfoOnNodes(gtmMaster, nodes, ignoreNode,
							 CHECK_GTM_INFO_SECONDS, complain);	
}
static bool isCurestatusForRunningOk(char *curestatus)
{
	return pg_strcasecmp(curestatus, CURE_STATUS_NORMAL) == 0 ||
		   pg_strcasecmp(curestatus, CURE_STATUS_SWITCHED) == 0;
}

static void checkTrackActivitiesForSwitchover(dlist_head *coordinators,
											  SwitcherNodeWrapper *oldMaster)
{
	dlist_iter iter;
	SwitcherNodeWrapper *node;
	char *paramName = "track_activities";
	char *paramValue = "on";

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (!equalsNodeParameter(NameStr(node->mgrNode->form.nodename), node->pgConn, paramName, paramValue))
		{
			ereport(ERROR,
					(errmsg("check %s parameter %s failed, do \"set (%s=%s)\" please",
							NameStr(node->mgrNode->form.nodename),
							paramName,
							paramName,
							paramValue)));
		}
	}
	if (!equalsNodeParameter(NameStr(oldMaster->mgrNode->form.nodename), oldMaster->pgConn, paramName, paramValue))
	{
		ereport(ERROR,
				(errmsg("check %s parameter %s failed, do \"set (%s=%s)\" please",
						NameStr(oldMaster->mgrNode->form.nodename),
						paramName,
						paramName,
						paramValue)));
	}
}

static void checkActiveConnectionsForSwitchover(dlist_head *coordinators,
												SwitcherNodeWrapper *oldMaster,
												int maxTrys)
{
	int iloop;
	bool execOk = false;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	SwitcherNodeWrapper *node;
	dlist_iter iter;

	ereportNoticeLog((errmsg("wait max %d seconds to wait there are no active "
					"connections on coordinators and master nodes",
					maxTrys)));
	for (iloop = 0; iloop < maxTrys; iloop++)
	{
		tryLockCluster(coordinators);
		holdLockCoordinator = getHoldLockCoordinator(coordinators);
		dlist_foreach(iter, coordinators)
		{
			node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			execOk = checkActiveConnections(holdLockCoordinator->pgConn,
											holdLockCoordinator == node,
											NameStr(node->mgrNode->form.nodename));
			if (!execOk)
				goto sleep_1s;
		}
		execOk = checkActiveConnections(holdLockCoordinator->pgConn,
										false,
										NameStr(oldMaster->mgrNode->form.nodename));
		if (execOk)
			break;
		else
			goto sleep_1s;

	sleep_1s:
		if (iloop < maxTrys - 1)
		{
			tryUnlockCluster(coordinators, true);
			pg_usleep(1000000L);
		}
	}
	if (!execOk)
		ereportErrorLog((errmsg("[Error] there are some active connections on "
						"coordinators or datanode masters")));
}

static bool checkActiveConnections(PGconn *activePGcoon,
								   bool localExecute,
								   char *nodename)
{
	char *sql;
	PGresult *pgResult = NULL;
	char *nCoons;
	bool checkRes = false;

	ereport(LOG,
			(errmsg("check %s active connections",
					nodename)));

	if (localExecute)
		sql = psprintf("SELECT count(*)-1 "
					   "FROM pg_stat_activity WHERE state = 'active' "
					   "and backend_type != 'walsender';");
	else
		sql = psprintf("EXECUTE DIRECT ON (\"%s\") "
					   "'SELECT count(*)-1  FROM pg_stat_activity "
					   "WHERE state = ''active'' "
					   "and backend_type != ''walsender'' '",
					   nodename);
	pgResult = PQexec(activePGcoon, sql);
	pfree(sql);
	if (PQresultStatus(pgResult) == PGRES_TUPLES_OK)
	{
		nCoons = PQgetvalue(pgResult, 0, 0);
		if (nCoons == NULL)
		{
			checkRes = false;
		}
		else if (strcmp(nCoons, "0") == 0)
		{
			checkRes = true;
		}
		else
		{
			ereport(NOTICE, (errmsg("%s has %s active connections",
									nodename,
									nCoons)));
			ereport(LOG, (errmsg("%s has %s active connections",
								 nodename,
								 nCoons)));
			checkRes = false;
		}
	}
	else
	{
		ereport(NOTICE, (errmsg("check %s active connections failed, %s",
								nodename,
								PQerrorMessage(activePGcoon))));
		ereport(LOG, (errmsg("check %s active connections failed, %s",
							 nodename,
							 PQerrorMessage(activePGcoon))));
		checkRes = false;
	}
	if (pgResult)
		PQclear(pgResult);
	return checkRes;
}

static void checkActiveLocksForSwitchover(dlist_head *coordinators,
										  SwitcherNodeWrapper *holdLockCoordIn,
										  SwitcherNodeWrapper *oldMaster,
										  int maxTrys)
{
	int iloop;
	bool execOk = false;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	SwitcherNodeWrapper *node;
	dlist_iter iter;

	ereport(LOG,
			(errmsg("wait max %d seconds to check there are no active locks "
					"in pg_locks except the locks on pg_locks table",
					maxTrys)));
	ereport(NOTICE,
			(errmsg("wait max %d seconds to check there are no active locks "
					"in pg_locks except the locks on pg_locks table",
					maxTrys)));

	if (maxTrys == 0){
		maxTrys = 10;
	}			
    if (holdLockCoordIn == NULL){
		holdLockCoordinator = getHoldLockCoordinator(coordinators);
	}
	else{
		holdLockCoordinator = holdLockCoordIn;
	}
	
	for (iloop = 0; iloop < maxTrys; iloop++)
	{
		dlist_foreach(iter, coordinators)
		{
			node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			execOk = checkActiveLocks(holdLockCoordinator->pgConn,
									  holdLockCoordinator == node,
									  node->mgrNode);
			if (!execOk)
				goto sleep_1s;
		}
		execOk = checkActiveLocks(holdLockCoordinator->pgConn,
								  holdLockCoordinator == oldMaster,
								  oldMaster->mgrNode);
		if (execOk)
			break;
		else
			goto sleep_1s;

	sleep_1s:
		if (iloop < maxTrys - 1)
			pg_usleep(1000000L);
	}
	if (!execOk)
		ereport(ERROR,
				(errmsg("check there are no active locks in pg_locks except "
						"the locks on pg_locks table failed")));
}

static bool checkActiveLocks(PGconn *activePGcoon,
							 bool localExecute,
							 MgrNodeWrapper *mgrNode)
{
	char *sql;
	PGresult *pgResult = NULL;
	char *nLocks;
	bool checkRes = false;

	ereport(LOG, (errmsg("check %s active locks",
						 NameStr(mgrNode->form.nodename))));
	if (localExecute)
		sql = psprintf("select count(*) from pg_locks "
					   "where pid != pg_backend_pid();");
	else
		sql = psprintf("EXECUTE DIRECT ON (\"%s\") "
					   "'select count(*)  from pg_locks "
					   "where pid != pg_backend_pid();'",
					   NameStr(mgrNode->form.nodename));
	pgResult = PQexec(activePGcoon, sql);
	pfree(sql);
	if (PQresultStatus(pgResult) == PGRES_TUPLES_OK)
	{
		nLocks = PQgetvalue(pgResult, 0, 0);
		if (nLocks == NULL)
		{
			checkRes = false;
		}
		else if (strcmp(nLocks, "0") == 0)
		{
			checkRes = true;
		}
		else
		{
			ereport(LOG, (errmsg("%s has %s active locks",
								 NameStr(mgrNode->form.nodename),
								 nLocks)));
			checkRes = false;
		}
	}
	else
	{
		ereport(LOG, (errmsg("check %s active locks failed, %s",
							 NameStr(mgrNode->form.nodename),
							 PQerrorMessage(activePGcoon))));
		checkRes = false;
	}
	if (pgResult)
		PQclear(pgResult);
	return checkRes;
}

static void checkXlogDiffForSwitchover(SwitcherNodeWrapper *oldMaster,
									   SwitcherNodeWrapper *newMaster,
									   int maxTrys)
{
	int iloop;
	bool execOk = false;
	char *sql;

	ereportNoticeLog((errmsg("wait max %d seconds to check %s and %s have the same xlog position",
					maxTrys,
					NameStr(oldMaster->mgrNode->form.nodename),
					NameStr(newMaster->mgrNode->form.nodename))));				

	PQexecCommandSql(oldMaster->pgConn, "checkpoint;", true);

	sql = psprintf("select pg_wal_lsn_diff "
				   "(pg_current_wal_insert_lsn(), replay_lsn) = 0 "
				   "from pg_stat_replication where application_name = '%s';",
				   NameStr(newMaster->mgrNode->form.nodename));
	for (iloop = 0; iloop < maxTrys; iloop++)
	{
		execOk = PQexecBoolQuery(oldMaster->pgConn, sql, true, false);
		if (execOk)
			break;
		else
			goto sleep_1s;

	sleep_1s:
		if (iloop < maxTrys - 1)
			pg_usleep(1000000L);
	}
	if (!execOk)
		ereport(ERROR,
				(errmsg("check %s and %s have the same xlog position failed",
						NameStr(oldMaster->mgrNode->form.nodename),
						NameStr(newMaster->mgrNode->form.nodename))));
}
static void checkXlogDiffForSwitchoverCoord(dlist_head *coordinators,
											SwitcherNodeWrapper *holdLockCoordIn,
											SwitcherNodeWrapper *oldMaster, 
											SwitcherNodeWrapper *newMaster,
											int maxTrys)
{
	int iloop;
	bool execOk = false;
	char *sql;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;

	ereportNoticeLog(errmsg("wait max %d seconds to check %s and %s have the same xlog position",
					maxTrys,
					NameStr(oldMaster->mgrNode->form.nodename),
					NameStr(newMaster->mgrNode->form.nodename)));

	if (holdLockCoordIn != NULL){
		holdLockCoordinator = holdLockCoordIn;
	}
	else{
		holdLockCoordinator = getHoldLockCoordinator(coordinators);
	}

	SetXcMaintenanceModeOn(holdLockCoordinator->pgConn);

	CheckpointCoord(holdLockCoordinator->pgConn, oldMaster);

	sql = psprintf("EXECUTE DIRECT ON (\"%s\") "
    			   "'select pg_wal_lsn_diff "
				   "(pg_current_wal_insert_lsn(), replay_lsn) = 0 "
				   "from pg_stat_replication where application_name = ''%s''';",
				   NameStr(oldMaster->mgrNode->form.nodename),
				   NameStr(newMaster->mgrNode->form.nodename));
	for (iloop = 0; iloop < maxTrys; iloop++)
	{
		execOk = PQexecBoolQuery(holdLockCoordinator->pgConn, sql, true, false);
		if (execOk)
			break;
		else
			goto sleep_1s;

	sleep_1s:
		if (iloop < maxTrys - 1)
			pg_usleep(1000000L);
	}
	if (!execOk)
		ereport(ERROR,
				(errmsg("check %s and %s have the same xlog position failed",
						NameStr(oldMaster->mgrNode->form.nodename),
						NameStr(newMaster->mgrNode->form.nodename))));
}
static void revertGtmInfoSetting(SwitcherNodeWrapper *oldGtmMaster,
								 SwitcherNodeWrapper *newGtmMaster,
								 dlist_head *coordinators,
								 dlist_head *coordinatorSlaves,
								 dlist_head *dataNodes)
{

	if (newGtmMaster)
	{
		if (newGtmMaster->gtmInfoChanged)
		{
			setGtmInfoInPGSqlConf(newGtmMaster->mgrNode,
								  newGtmMaster->mgrNode,
								  false);			  
		}
		if (oldGtmMaster && oldGtmMaster->gtmInfoChanged)
		{
			setGtmInfoInPGSqlConf(oldGtmMaster->mgrNode,
								  newGtmMaster->mgrNode,
								  false);			  
		}

		SetGtmInfoSettingToList(coordinators, newGtmMaster);
		SetGtmInfoSettingToList(coordinatorSlaves, newGtmMaster);
		SetGtmInfoSettingToList(dataNodes, newGtmMaster);
	}
}

static void SetGtmInfoSettingToList(dlist_head *nodes, SwitcherNodeWrapper *gtmMaster)
{
	dlist_iter 			iter;
	SwitcherNodeWrapper *node;

	if (nodes != NULL && gtmMaster != NULL)
	{
		dlist_foreach(iter, nodes)
		{
			node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			Assert(node);
			if (node->gtmInfoChanged)
			{
				setGtmInfoInPGSqlConf(node->mgrNode, gtmMaster->mgrNode, false);
			}
		}	
	}
}

static MgrNodeWrapper *checkGetMasterNodeBySlaveNodename(char *slaveNodename,
														 char slaveNodetype,
														 MemoryContext spiContext,
														 bool complain)
{
	MgrNodeWrapper *slaveMgrNode = NULL;
	MgrNodeWrapper *masterMgrNode = NULL;

	slaveMgrNode = selectMgrNodeByNodenameType(slaveNodename,
											   slaveNodetype,
											   spiContext);
	if (!slaveMgrNode ||
		slaveMgrNode->form.nodemasternameoid <= 0)
	{
		ereport(complain ? ERROR : WARNING,
				(errmsg("can not determin %s in mgr_node",
						slaveNodename)));
		if (slaveMgrNode)
		{
			pfree(slaveMgrNode);
			slaveMgrNode = NULL;
		}
	}
	else
	{
		masterMgrNode = selectMgrNodeByOid(slaveMgrNode->form.nodemasternameoid,
										   spiContext);
		if (!masterMgrNode ||
			masterMgrNode->form.nodetype != getMgrMasterNodetype(slaveNodetype))
		{
			ereport(complain ? ERROR : WARNING,
					(errmsg("can not determin the master of %s in mgr_node",
							slaveNodename)));
			if (slaveMgrNode)
			{
				pfree(slaveMgrNode);
				slaveMgrNode = NULL;
			}
			if (masterMgrNode)
			{
				pfree(masterMgrNode);
				masterMgrNode = NULL;
			}
		}
	}
	return masterMgrNode;
}

/*
 * One "waitswitch" datanode master may block the operation of "alter node",
 * so these "waitswitch" nodes may be bypassed in the last active/standby 
 * switching of other nodes. When these "waitswitch" datanode master back to 
 * normal or switch to a slave node, diff PGXC_NODE is very necessary.
 */
void diffPgxcNodesOfDataNode(PGconn *pgconn,
							 bool localExecute,
							 SwitcherNodeWrapper *dataNodeMaster,
							 dlist_head *siblingMasters,
							 MemoryContext spiContext,
							 bool complain)
{
	int i;
	int nDiffNodes;
	dlist_iter iter;
	SwitcherNodeWrapper *switcherNode;
	char *historicalDataNodeMasterName;
	MgrNodeWrapper *currentDataNodeMaster = NULL;
	PGresult *res = NULL;
	StringInfoData allMasterNames;
	StringInfoData sql;
	char slaveNodetype;
	char masterNodetype;

	initStringInfo(&allMasterNames);
	initStringInfo(&sql);
	slaveNodetype = getMgrSlaveNodetype(dataNodeMaster->mgrNode->form.nodetype);
	masterNodetype = getMgrMasterNodetype(dataNodeMaster->mgrNode->form.nodetype);

	dlist_foreach(iter, siblingMasters)
	{
		switcherNode = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (localExecute)
			appendStringInfo(&allMasterNames, "'%s',", NameStr(switcherNode->mgrNode->form.nodename));
		else
			appendStringInfo(&allMasterNames, "''%s'',", NameStr(switcherNode->mgrNode->form.nodename));
	}
	if (localExecute)
		appendStringInfo(&allMasterNames, "'%s'", NameStr(dataNodeMaster->mgrNode->form.nodename));
	else
		appendStringInfo(&allMasterNames, "''%s''", NameStr(dataNodeMaster->mgrNode->form.nodename));

	if (localExecute)
		appendStringInfo(&sql,
						 "select node_name from pg_catalog.pgxc_node where node_name not in (%s) and node_type ='%c ;",
						 allMasterNames.data,
						 getMappedPgxcNodetype(masterNodetype));
	else
		appendStringInfo(&sql,
						 "EXECUTE DIRECT ON (\"%s\") "
						 "'select node_name from pg_catalog.pgxc_node where node_name not in (%s) and node_type =''%c'' ;' ",
						 NameStr(dataNodeMaster->mgrNode->form.nodename),
						 allMasterNames.data,
						 getMappedPgxcNodetype(masterNodetype));
	res = PQexec(pgconn, sql.data);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		PQclear(res);
		res = NULL;
		ereport(complain ? ERROR : WARNING,
				(errmsg("PQexec \"%s\" failed",
						sql.data)));
		goto end;
	}
	nDiffNodes = PQntuples(res);
	for (i = 0; i < nDiffNodes; i++)
	{
		historicalDataNodeMasterName = PQgetvalue(res, i, 0);
		currentDataNodeMaster = checkGetMasterNodeBySlaveNodename(historicalDataNodeMasterName,
																  slaveNodetype,
																  spiContext,
																  complain);
		if (!currentDataNodeMaster)
			goto end;
		alterNodeOnPgxcNode(pgconn,
							NameStr(dataNodeMaster->mgrNode->form.nodename),
							localExecute,
							historicalDataNodeMasterName,
							currentDataNodeMaster,
							complain);
	}

end:
	PQclear(res);
	pfree(allMasterNames.data);
	pfree(sql.data);
	if (currentDataNodeMaster)
		pfreeMgrNodeWrapper(currentDataNodeMaster);
	return;
}

void beginSwitcherNodeTransaction(SwitcherNodeWrapper *switcherNode,
								  bool complain)
{
	if (switcherNode && !switcherNode->inTransactionBlock)
		switcherNode->inTransactionBlock =
			PQexecCommandSql(switcherNode->pgConn, SQL_BEGIN_TRANSACTION, complain);
}

void commitSwitcherNodeTransaction(SwitcherNodeWrapper *switcherNode,
								   bool complain)
{
	if (switcherNode && switcherNode->inTransactionBlock)
	{
		if (PQexecCommandSql(switcherNode->pgConn, SQL_COMMIT_TRANSACTION, complain))
			switcherNode->inTransactionBlock = false;
	}
}

void rollbackSwitcherNodeTransaction(SwitcherNodeWrapper *switcherNode,
									 bool complain)
{
	if (switcherNode && switcherNode->inTransactionBlock)
	{
		if (PQexecCommandSql(switcherNode->pgConn, SQL_ROLLBACK_TRANSACTION, complain))
			switcherNode->inTransactionBlock = false;
	}
}

static SwitcherNodeWrapper *getNewMasterNodeByNodename(dlist_head *runningSlaves,
													   dlist_head *failedSlaves,
													   char *newMasterName)
{
	SwitcherNodeWrapper *node;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_mutable_iter miter;

	dlist_foreach_modify(miter, runningSlaves)
	{
		node = dlist_container(SwitcherNodeWrapper, link, miter.cur);
		node->walLsn = getNodeWalLsn(node->pgConn, node->runningMode);
		if (strcmp(newMasterName, NameStr(node->mgrNode->form.nodename)) == 0)
		{
			newMaster = node;
			dlist_delete(miter.cur);
			node->walLsn = getNodeWalLsn(node->pgConn, node->runningMode);
			if (node->walLsn <= InvalidXLogRecPtr)
			{
				ereport(ERROR,
						(errmsg("%s get wal lsn failed",
								NameStr(node->mgrNode->form.nodename))));
			}
		}
		else
		{
			if (node->walLsn <= InvalidXLogRecPtr)
			{
				dlist_delete(miter.cur);
				dlist_push_tail(failedSlaves, &node->link);
				ereport(WARNING,
						(errmsg("%s get wal lsn failed",
								NameStr(node->mgrNode->form.nodename))));
			}
		}
	}
	return newMaster;
}
static void ShutdownRunningNotZone(dlist_head *runningSlaves, char *zone)
{
	SwitcherNodeWrapper 	*node;
	dlist_iter 				iter;

	dlist_foreach(iter, runningSlaves)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (pg_strcasecmp(NameStr(node->mgrNode->form.nodezone), zone) != 0)
		{
			shutdownNodeWithinSeconds(node->mgrNode,
									SHUTDOWN_NODE_FAST_SECONDS,
									SHUTDOWN_NODE_IMMEDIATE_SECONDS,
									true);
		}
	}
}

static void RefreshMgrNodesBeforeSwitchGtmCoord(MemoryContext spiContext, 
											SwitcherNodeWrapper *newMaster,
											SwitcherNodeWrapper *oldMaster,
											dlist_head *runningSlaves,
											dlist_head *failedSlaves,
											dlist_head *runningSlavesSecond,
											dlist_head *failedSlavesSecond,
											dlist_head *coordinators,
											dlist_head *coordinatorSlaves,
											dlist_head *dataNodes)
{
	dlist_iter 			iter;
	SwitcherNodeWrapper *node;

	refreshSlaveNodesBeforeSwitch(newMaster,
								runningSlaves,
								failedSlaves,
								runningSlavesSecond,
								failedSlavesSecond,
								spiContext);
	
	dlist_foreach(iter, coordinators) 
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (node != oldMaster)
			refreshMgrNodeBeforeSwitch(node, spiContext);
	}

	refreshMgrNodeListBeforeSwitch(spiContext, coordinatorSlaves);
	refreshMgrNodeListBeforeSwitch(spiContext, dataNodes);
}											

static void RefreshMgrNodesAfterSwitchGtmCoord(MemoryContext spiContext, 
											dlist_head *coordinators, 
											dlist_head *coordinatorSlaves,
											dlist_head *dataNodes,
											SwitcherNodeWrapper *newMaster)
{
	dlist_iter 			iter;
	SwitcherNodeWrapper *node;

	dlist_foreach(iter, coordinators)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		if (node != newMaster)
			refreshOtherNodeAfterSwitchGtmCoord(node, spiContext);
	}
	dlist_foreach(iter, coordinatorSlaves)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		refreshOtherNodeAfterSwitchGtmCoord(node, spiContext);
	}
	dlist_foreach(iter, dataNodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		refreshOtherNodeAfterSwitchGtmCoord(node, spiContext);
	}
}
static void RefreshPgxcNodeName(SwitcherNodeWrapper *node, char *nodeName)
{
	AppendNodeInfo  appendNode = {0};
	appendNode.nodepath = node->mgrNode->nodepath;
	appendNode.nodehost = node->mgrNode->host->form.oid;
	appendNode.nodetype = node->mgrNode->form.nodetype;
	appendNode.nodename = NameStr(node->mgrNode->form.nodename);
	hexp_update_conf_pgxc_node_name(&appendNode, nodeName);
	hexp_restart_node(&appendNode);
}
static void selectActiveMgrNodeChild(MemoryContext spiContext, 
									MgrNodeWrapper *mgrNode, 
									char slaveNodetype,
									dlist_head *slaveNodes)
{
	dlist_head 			mgrNodes = DLIST_STATIC_INIT(mgrNodes);
	dlist_iter 			iter;
	MgrNodeWrapper 		*slaveNode = NULL;
	SwitcherNodeWrapper *switcherNode;

	dlist_init(&mgrNodes);
	selectActiveMgrSlaveNodes(mgrNode->form.oid,
							slaveNodetype,
							spiContext,
							&mgrNodes);
	dlist_foreach(iter, &mgrNodes)
	{
		slaveNode = dlist_container(MgrNodeWrapper, link, iter.cur);
		Assert(slaveNode);
		mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
		switcherNode = palloc0(sizeof(SwitcherNodeWrapper));
		switcherNode->mgrNode = mgrNode;
		dlist_push_tail(slaveNodes, &switcherNode->link);
		selectActiveMgrNodeChild(spiContext, slaveNode, slaveNodetype, slaveNodes);
	}
}
static void PrintMgrNode(MemoryContext spiContext, dlist_head *mgrNodes)
{
	MgrNodeWrapper 		*mgrNode;
	dlist_iter 			iter;
	dlist_head 			resultList = DLIST_STATIC_INIT(resultList);

	dlist_foreach(iter, mgrNodes)
	{
		mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
		Assert(mgrNode);
		ereport(LOG, (errmsg("oid(%d), nodemasternameoid(%d), name(%s), host(%s), type(%c), sync(%s), port(%d), inited(%d), incluster(%d), zone(%s), allowcure(%d), curestatus(%s), path(%s).", 
				mgrNode->form.oid, mgrNode->form.nodemasternameoid, NameStr(mgrNode->form.nodename),
				mgrNode->host->hostaddr, mgrNode->form.nodetype, NameStr(mgrNode->form.nodesync), 
				mgrNode->form.nodeport, mgrNode->form.nodeinited, mgrNode->form.nodeincluster, NameStr(mgrNode->form.nodezone),
				mgrNode->form.allowcure, NameStr(mgrNode->form.curestatus), mgrNode->nodepath)));

		dlist_init(&resultList);
		selectMgrSlaveNodesByOidType(mgrNode->form.oid,
									getMgrSlaveNodetype(mgrNode->form.nodetype),
									spiContext,
									&resultList);
		if (!dlist_is_empty(&resultList)){
			PrintMgrNode(spiContext, &resultList);
		}		
	}	
}
void PrintMgrNodeList(MemoryContext spiContext)
{
	dlist_head 			masterGtm = DLIST_STATIC_INIT(masterGtm);
	dlist_head 			masterCoords = DLIST_STATIC_INIT(masterCoords);
	dlist_head 			masterDataNodes = DLIST_STATIC_INIT(masterDataNodes);

	selectMgrNodeByNodetype(spiContext, CNDN_TYPE_GTM_COOR_MASTER, &masterGtm);
	PrintMgrNode(spiContext, &masterGtm);

	selectMgrNodeByNodetype(spiContext, CNDN_TYPE_COORDINATOR_MASTER, &masterCoords);
	PrintMgrNode(spiContext, &masterCoords);

	selectMgrNodeByNodetype(spiContext, CNDN_TYPE_DATANODE_MASTER, &masterDataNodes);
	PrintMgrNode(spiContext, &masterDataNodes);
}
void MgrChildNodeFollowParentNode(MemoryContext spiContext, 
									Form_mgr_node childMgrNode, 
									Oid childNodeOid, 
									Form_mgr_node parentMgrNode, 
									Oid parentOid)
{
	SwitcherNodeWrapper *curSlave;
	SwitcherNodeWrapper *newParent;
	MgrNodeWrapper 		*ParentNode;
 	MgrNodeWrapper 		*slaveNode;

	Assert(childMgrNode);
	Assert(parentMgrNode);

	curSlave  = checkGetSwitchoverOldMaster(childNodeOid, childMgrNode->nodetype, spiContext);
	newParent = checkGetSwitchoverOldMaster(parentOid, parentMgrNode->nodetype, spiContext);
	Assert(curSlave);
	Assert(newParent);
	ParentNode = newParent->mgrNode;
	slaveNode  = curSlave->mgrNode;
	Assert(ParentNode);
	Assert(slaveNode);

	setPGHbaTrustSlaveReplication(ParentNode, slaveNode, true);

	setSlaveNodeRecoveryConf(ParentNode, slaveNode);

	shutdownNodeWithinSeconds(slaveNode,
							  SHUTDOWN_NODE_FAST_SECONDS,
							  SHUTDOWN_NODE_IMMEDIATE_SECONDS,
							  true);

	callAgentStartNode(slaveNode, false, false);

	waitForNodeRunningOk(slaveNode, false, NULL, NULL);

	appendToSyncStandbyNames(ParentNode,
							 slaveNode,
							 newParent->pgConn,
							 spiContext);

}
void MgrZoneSwitchoverGtm(MemoryContext spiContext, 
							char 	*currentZone,
							bool 	forceSwitch,
							int 	maxTrys, 
							ZoneOverGtm *zoGtm)
{
	SwitcherNodeWrapper *oldGtmMaster = NULL;	
	SwitcherNodeWrapper *newGtmMaster = NULL;
	dlist_head 			coordMasters = DLIST_STATIC_INIT(coordMasters);
	dlist_head 			gtmSlaves = DLIST_STATIC_INIT(gtmSlaves);
	NameData            newGtmName;

	Assert(spiContext);
	Assert(currentZone);
	Assert(zoGtm);

	checkGetMasterCoordinators(spiContext, &coordMasters, true, true);
    oldGtmMaster = getGtmCoordMaster(&coordMasters);
	Assert(oldGtmMaster);
	
	checkGetRunningSlaveNodesInZone(oldGtmMaster,
									spiContext,
									CNDN_TYPE_GTM_COOR_SLAVE,
									currentZone,
									&gtmSlaves);
	if (dlist_is_empty(&gtmSlaves))
	{
		ereport(ERROR,(errmsg("there is no gtmcoord slave in zone(%s).", currentZone)));
	}
	
	chooseNewMasterNodeForZone(oldGtmMaster,
								&newGtmMaster,
								&gtmSlaves,
								forceSwitch,
								currentZone);
	namestrcpy(&newGtmName, NameStr(newGtmMaster->mgrNode->form.nodename));							
	pfreeSwitcherNodeWrapperList(&coordMasters, NULL);
	pfreeSwitcherNodeWrapperList(&gtmSlaves, NULL);							

	switchoverGtmCoordForZone(spiContext, 
								NameStr(newGtmName), 
								forceSwitch, 
								currentZone, 
								maxTrys,
								zoGtm);
}
void MgrZoneSwitchoverCoord(MemoryContext spiContext, 
							char 	*currentZone, 
							bool	forceSwitch,
							int     maxTrys,
							ZoneOverGtm *zoGtm, 
							dlist_head *zoCoordList)
{
	dlist_iter 				iter;
	ZoneOverCoord 		 	*zoCoord = NULL;
	ZoneOverCoordWrapper 	*zoCoordWrapper = NULL;
	MgrNodeWrapper *mgrNode = NULL;
	NameData   newDataName;
	dlist_head masterMgrNodes = DLIST_STATIC_INIT(masterMgrNodes);
	dlist_head resultList = DLIST_STATIC_INIT(resultList);

	Assert(spiContext);
	Assert(currentZone);
	Assert(zoGtm);
	Assert(zoCoordList);

	selectActiveMgrNodeByNodetype(spiContext, CNDN_TYPE_COORDINATOR_MASTER, &masterMgrNodes);
	dlist_foreach(iter, &masterMgrNodes)
	{
		PG_TRY();
		{
			dlist_init(&resultList);
			mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
			Assert(mgrNode);
			GetSwitchSlaveByMaster(spiContext, mgrNode, currentZone, CNDN_TYPE_COORDINATOR_SLAVE, &newDataName);

			zoCoordWrapper = (ZoneOverCoordWrapper*)palloc0(sizeof(ZoneOverCoordWrapper));
			zoCoord = (ZoneOverCoord*)palloc0(sizeof(ZoneOverCoord));
			InitZoneOverCoord(zoCoord);
			zoCoordWrapper->zoCoord = zoCoord;

			switchoverCoordForZone(spiContext,
									NameStr(newDataName), 
									currentZone,
									forceSwitch, 
									maxTrys,
									zoGtm,
									zoCoord);
			dlist_push_tail(zoCoordList, &zoCoordWrapper->link);
		}
		PG_CATCH();
		{
			pfreeMgrNodeWrapperList(&masterMgrNodes, NULL);
			dlist_push_tail(zoCoordList, &zoCoordWrapper->link);
			PG_RE_THROW();
		}
		PG_END_TRY();
	}	
	pfreeMgrNodeWrapperList(&masterMgrNodes, NULL);
}
void MgrZoneSwitchoverDataNode(MemoryContext spiContext, 
								char *currentZone,
								bool forceSwitch,
								int maxTrys, 
								ZoneOverGtm *zoGtm,
								dlist_head *zoDNList)
{
	dlist_head 				masterMgrNodes = DLIST_STATIC_INIT(masterMgrNodes);
	NameData   				newDataName;
	dlist_iter 				iter;
	ZoneOverDNWrapper 		*zoDNWrapper = NULL;
	ZoneOverDN        		*zoDN = NULL;
	MgrNodeWrapper 			*mgrNode = NULL;

	Assert(spiContext);
	Assert(currentZone);
	Assert(zoGtm);
	Assert(zoDNList);

	selectActiveMgrNodeByNodetype(spiContext, CNDN_TYPE_DATANODE_MASTER, &masterMgrNodes);
	dlist_foreach(iter, &masterMgrNodes)
	{
		PG_TRY();
		{
			mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
			Assert(mgrNode);
			GetSwitchSlaveByMaster(spiContext, mgrNode, currentZone, CNDN_TYPE_DATANODE_SLAVE, &newDataName);

			zoDNWrapper = (ZoneOverDNWrapper*)palloc0(sizeof(ZoneOverDNWrapper));
			zoDN = (ZoneOverDN*)palloc0(sizeof(ZoneOverDN));
			InitZoneoverDN(zoDN);
			zoDNWrapper->zoDN = zoDN;

			switchoverDataNodeForZone(spiContext, 
										NameStr(newDataName), 
										forceSwitch,
										maxTrys, 
										currentZone, 
										zoGtm, 
										zoDN);
			dlist_push_tail(zoDNList, &zoDNWrapper->link);
		}
		PG_CATCH();
		{
			pfreeMgrNodeWrapperList(&masterMgrNodes, NULL);
			dlist_push_tail(zoDNList, &zoDNWrapper->link);
			PG_RE_THROW();
		}
		PG_END_TRY();	
	}
	pfreeMgrNodeWrapperList(&masterMgrNodes, NULL);
}
static void switchoverGtmCoordForZone(MemoryContext spiContext, 
										char *newMasterName, 
										bool forceSwitch, 
										char *curZone,
										int maxTrys,
										ZoneOverGtm *zoGtm)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	dlist_head  *coordinators		= &zoGtm->coordinators;
	dlist_head  *coordinatorSlaves  = &zoGtm->coordinatorSlaves;
	dlist_head  *runningSlaves 		= &zoGtm->runningSlaves;
	dlist_head  *runningSlavesSecond= &zoGtm->runningSlavesSecond;
	dlist_head  *failedSlaves 		= &zoGtm->failedSlaves;
	dlist_head  *failedSlavesSecond = &zoGtm->failedSlavesSecond;
	dlist_head  *dataNodes 			= &zoGtm->dataNodes;
	dlist_head  *runningSlaveOfNewMaster = &zoGtm->runningSlaveOfNewMaster;
	bool        complain = false;

	ereport(LOG, (errmsg("------------- switchoverGtmCoord newMasterName(%s) before -------------", newMasterName)));				
	PrintMgrNodeList(spiContext);

	newMaster = checkGetSwitchoverNewMaster(newMasterName,
											CNDN_TYPE_GTM_COOR_SLAVE,
											forceSwitch,
											spiContext);

	if (pg_strcasecmp(NameStr(newMaster->mgrNode->form.nodezone), curZone) != 0)
	{
		ereport(ERROR, (errmsg("the new gtmcoord(%s) is not in current zone(%s), can't switchover it.",	
				NameStr(newMaster->mgrNode->form.nodename), curZone)));
	}								
	oldMaster = checkGetSwitchoverOldMaster(newMaster->mgrNode->form.nodemasternameoid,
											CNDN_TYPE_GTM_COOR_MASTER,
											spiContext);

	oldMaster->startupAfterException = false;
	PrintReplicationInfo(oldMaster);

	checkGetSlaveNodesRunningStatus(oldMaster,
									spiContext,
									newMaster->mgrNode->form.oid,
									"",
									failedSlaves,
									runningSlaves);
	checkGetSlaveNodesRunningSecondStatus(oldMaster,
										spiContext,
										newMaster->mgrNode->form.oid,
										runningSlaves,
										failedSlaves,
										runningSlavesSecond,
										failedSlavesSecond);
	getRunningSlaveOfNewMaster(spiContext,
								newMaster->mgrNode,
								runningSlaveOfNewMaster);	
	validateFailedSlavesForSwitch(oldMaster->mgrNode,
									newMaster->mgrNode,
									failedSlaves,
									forceSwitch);
	checkGetMasterCoordinators(spiContext,
								coordinators,
								false, false);
	checkGetSlaveCoordinators(spiContext,
								coordinatorSlaves,
								false);						   
	checkGetAllDataNodes(dataNodes, spiContext);
	checkTrackActivitiesForSwitchover(coordinators, oldMaster);
	checkTrackActivitiesForSwitchover(coordinatorSlaves, oldMaster);								  
	/* oldMaster also is a coordinator */
	dlist_push_head(coordinators, &oldMaster->link);
     
	/* check interrupt before lock the cluster */
	CHECK_FOR_INTERRUPTS();

	zoGtm->newMaster = newMaster;																
	zoGtm->oldMaster = oldMaster;  

	if (forceSwitch)
	{
		tryLockCluster(coordinators);
	}
	else
	{
		checkActiveConnectionsForSwitchover(coordinators,
											oldMaster,
											maxTrys);												
	}
	checkActiveLocksForSwitchover(coordinators, NULL, oldMaster, maxTrys);
	checkXlogDiffForSwitchover(oldMaster, newMaster, maxTrys);
	CHECK_FOR_INTERRUPTS();
	/* Prevent doctor process from manipulating this node simultaneously. */
	refreshOldMasterBeforeSwitch(oldMaster, spiContext);

	RefreshMgrNodesBeforeSwitchGtmCoord(spiContext, 
										newMaster,
										oldMaster,
										runningSlaves,
										failedSlaves,
										runningSlavesSecond,
										failedSlavesSecond,
										coordinators,
										coordinatorSlaves,
										dataNodes);
	refreshSyncToAsync(spiContext, runningSlaveOfNewMaster);
	oldMaster->startupAfterException = (oldMaster->runningMode == NODE_RUNNING_MODE_MASTER &&
										newMaster->runningMode == NODE_RUNNING_MODE_SLAVE);
	shutdownNodeWithinSeconds(oldMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								true);
	if (oldMaster->holdClusterLock)
	{
		/* I am already dead. If I hold a cluster lock, I will automatically give up. */
		oldMaster->holdClusterLock = false;
		ereportNoticeLog(errmsg("%s has been shut down and the cluster is unlocked",
						NameStr(oldMaster->mgrNode->form.nodename)));
	}
	ClosePgConn(oldMaster->pgConn);
	DelNodeFromSwitcherNodeWrappers(coordinators, oldMaster);

	tryUnlockCluster(coordinators, true);

	setCheckGtmInfoInPGSqlConf(newMaster->mgrNode,
								newMaster->mgrNode,
								newMaster->pgConn,
								true,
								CHECK_GTM_INFO_SECONDS,
								true);
	newMaster->gtmInfoChanged = true;

	RefreshGtmAdbCheckSyncNextid(newMaster->mgrNode, ADB_CHECK_SYNC_NEXTID_OFF);
	promoteNewMasterStartReign(oldMaster, newMaster);

	ereportNoticeLog(errmsg("set gtmhost, gtmport to every node, please wait for a moment."));
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,	dataNodes, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, coordinators, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, coordinatorSlaves, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,	runningSlaves, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, runningSlavesSecond, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,	failedSlaves, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, failedSlavesSecond, NULL, complain);
  
	newMaster->mgrNode->form.nodetype =	getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
	dlist_push_head(coordinators, &newMaster->link);
	
	PrintCoordReplicationInfo(coordinators);

	tryLockCluster(coordinators);
	holdLockCoordinator = getHoldLockCoordinator(coordinators);
	zoGtm->holdLockCoordinator = holdLockCoordinator;

	refreshPgxcNodesOfCoordinators(holdLockCoordinator,
									coordinators,
									oldMaster,
									newMaster);
	
    appendSlaveNodeFollowMasterEx(spiContext,
									newMaster,
									oldMaster, 
									true);
	refreshOldMasterAfterSwitchover(oldMaster,
									newMaster,
									spiContext);
	refreshSlaveNodesAfterSwitch(newMaster,
								oldMaster, 	
								runningSlaves,
								failedSlaves,
								runningSlavesSecond,
								failedSlavesSecond,
								spiContext,
								OVERTYPE_SWITCHOVER,
								curZone);
	
	RefreshMgrNodesAfterSwitchGtmCoord(spiContext,
										coordinators,
										coordinatorSlaves, 
										dataNodes,
										newMaster);
	refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
									newMaster->mgrNode,
									spiContext,
									false);

	ereportNoticeLog((errmsg("Switchover the GTM master from %s to %s has been successfully completed",
					NameStr(oldMaster->mgrNode->form.nodename), NameStr(newMaster->mgrNode->form.nodename))));

	ereport(LOG, (errmsg("------------- switchoverGtmCoord newMasterName(%s) after -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);
}
static void switchoverCoordForZone(MemoryContext spiContext, 
									char 	*newMasterName,									
									char 	*curZone, 
									bool 	forceSwitch,
									int     maxTrys, 
									ZoneOverGtm *zoGtm,
									ZoneOverCoord *zoCoord)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	SwitcherNodeWrapper *gtmMaster = NULL;
	dlist_head  *coordinators  = &zoGtm->coordinators;
	dlist_head  *runningSlaves = &zoCoord->runningSlaves;
	dlist_head  *failedSlaves  = &zoCoord->failedSlaves;
	SwitcherNodeWrapper *holdLockCoordinator = zoGtm->holdLockCoordinator;
    Assert(holdLockCoordinator);

	ereport(LOG, (errmsg("------------- switchoverCoord newMasterName(%s) before -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);

	newMaster = checkGetSwitchoverNewMaster(newMasterName,
											CNDN_TYPE_COORDINATOR_SLAVE,
											forceSwitch,
											spiContext);
	if (pg_strcasecmp(NameStr(newMaster->mgrNode->form.nodezone), curZone) != 0)
	{
		ereport(ERROR, (errmsg("the new coord(%s) is not in current zone(%s), can't switchover it.",	
				NameStr(newMaster->mgrNode->form.nodename), curZone)));
	}												
	oldMaster = checkGetSwitchoverOldMasterForZone(holdLockCoordinator,
													newMaster->mgrNode->form.nodemasternameoid,
													CNDN_TYPE_COORDINATOR_MASTER,
													spiContext);
	oldMaster->startupAfterException = false;	

	checkGetSlaveNodesRunningStatus(oldMaster,
									spiContext,
									newMaster->mgrNode->form.oid,
									"",
									failedSlaves,
									runningSlaves);	
	validateFailedSlavesForSwitch(oldMaster->mgrNode,
									newMaster->mgrNode,
									failedSlaves,
									forceSwitch);
	checkXlogDiffForSwitchoverCoord(coordinators, 
									holdLockCoordinator, 
									oldMaster, 
									newMaster,
									maxTrys);
	CHECK_FOR_INTERRUPTS();

	zoCoord->oldMaster = oldMaster;
	zoCoord->newMaster = newMaster;

	refreshMgrNodeBeforeSwitch(oldMaster, spiContext);
	refreshMgrNodeBeforeSwitch(newMaster, spiContext);
	refreshMgrNodeListBeforeSwitch(spiContext, runningSlaves);
	refreshMgrNodeListBeforeSwitch(spiContext, failedSlaves);

	oldMaster->startupAfterException = (oldMaster->runningMode == NODE_RUNNING_MODE_MASTER &&
										newMaster->runningMode == NODE_RUNNING_MODE_SLAVE);
	shutdownNodeWithinSeconds(oldMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								true);	
	ClosePgConn(oldMaster->pgConn);

	gtmMaster = getGtmCoordMaster(coordinators);
	Assert(gtmMaster);
	setCheckGtmInfoInPGSqlConf(gtmMaster->mgrNode,
								newMaster->mgrNode,
								newMaster->pgConn,
								true,
								CHECK_GTM_INFO_SECONDS,
								true);

	promoteNewMasterStartReign(oldMaster, newMaster);

	appendSlaveNodeFollowMasterEx(spiContext, 
									newMaster, 
									oldMaster, 
									true);

    DelNodeFromSwitcherNodeWrappers(coordinators, oldMaster);
	newMaster->mgrNode->form.nodetype =	getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
	dlist_push_tail(coordinators, &newMaster->link);

	refreshPgxcNodesOfCoordinators(holdLockCoordinator,           	
									coordinators,
									oldMaster,
									newMaster);
	refreshOldMasterAfterSwitchover(oldMaster, 
									newMaster,
									spiContext);
	refreshSlaveNodesAfterSwitch(newMaster,
								oldMaster,
								runningSlaves,
								failedSlaves,
								NULL,
								NULL,
								spiContext,
								OVERTYPE_SWITCHOVER,
								curZone);
	refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode, 
									newMaster->mgrNode, 
									spiContext, 
									false);	
	ereportNoticeLog((errmsg("Switchover the coordinator master from %s to %s "
					"has been successfully completed",
					NameStr(oldMaster->mgrNode->form.nodename),
					NameStr(newMaster->mgrNode->form.nodename))));

	ereport(LOG, (errmsg("------------- switchoverCoord newMasterName(%s) after -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);
}
static void switchoverDataNodeForZone(MemoryContext spiContext,
										char *newMasterName, 
										bool forceSwitch,
										int maxTrys,
										char *curZone,
										ZoneOverGtm *zoGtm, 
										ZoneOverDN *zoDN)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	SwitcherNodeWrapper *gtmMaster = NULL;
	dlist_head *coordinators 		= &zoGtm->coordinators;
	dlist_head *runningSlaves 		= &zoDN->runningSlaves;
	dlist_head *runningSlavesSecond = &zoDN->runningSlavesSecond;
	dlist_head *failedSlaves 		= &zoDN->failedSlaves;
	dlist_head *failedSlavesSecond  = &zoDN->failedSlavesSecond;
	dlist_head runningSlaveOfNewMaster = DLIST_STATIC_INIT(runningSlaveOfNewMaster);
	SwitcherNodeWrapper *holdLockCoordinator = zoGtm->holdLockCoordinator;
    bool complain = true;

	Assert(holdLockCoordinator);

	ereport(LOG, (errmsg("------------- switchoverDataNode newMasterName(%s) before -------------", newMasterName)));			
	PrintMgrNodeList(spiContext);

	newMaster = checkGetSwitchoverNewMaster(newMasterName,
											CNDN_TYPE_DATANODE_SLAVE,
											forceSwitch,
											spiContext);
	if (pg_strcasecmp(NameStr(newMaster->mgrNode->form.nodezone), curZone) != 0)
	{
		ereport(ERROR, (errmsg("the new datanode(%s) is not in current zone(%s), can't switchover it.",	
				NameStr(newMaster->mgrNode->form.nodename), curZone)));
	}		
	
	PG_TRY();
	{
		oldMaster = checkGetSwitchoverOldMaster(newMaster->mgrNode->form.nodemasternameoid,
												CNDN_TYPE_DATANODE_MASTER,
												spiContext);
		oldMaster->startupAfterException = false;
		PrintReplicationInfo(oldMaster);

		checkGetSlaveNodesRunningStatus(oldMaster,
										spiContext,
										newMaster->mgrNode->form.oid,
										"",
										failedSlaves,
										runningSlaves);		
		checkGetSlaveNodesRunningSecondStatus(oldMaster,
											spiContext,
											newMaster->mgrNode->form.oid,
											runningSlaves,
											failedSlaves,
											runningSlavesSecond,
											failedSlavesSecond);
		getRunningSlaveOfNewMaster(spiContext,
									newMaster->mgrNode,
									&runningSlaveOfNewMaster);		
		validateFailedSlavesForSwitch(oldMaster->mgrNode,
										newMaster->mgrNode,
										failedSlaves,
										forceSwitch);
		checkTrackActivitiesForSwitchover(coordinators,
											oldMaster);
		refreshPgxcNodeBeforeSwitchDataNode(coordinators);

		/* check interrupt before lock the cluster */
		CHECK_FOR_INTERRUPTS();

		checkXlogDiffForSwitchover(oldMaster, newMaster, maxTrys);
		CHECK_FOR_INTERRUPTS();

		zoDN->newMaster = newMaster;
		zoDN->oldMaster = oldMaster;

		/* Prevent doctor process from manipulating this node simultaneously. */
		refreshOldMasterBeforeSwitch(oldMaster, spiContext);
		
		/* Prevent doctor processes from manipulating these nodes simultaneously. */
		refreshSlaveNodesBeforeSwitch(newMaster,
										runningSlaves,
										failedSlaves,
										runningSlavesSecond,
										failedSlavesSecond,
										spiContext);
		refreshSyncToAsync(spiContext, &runningSlaveOfNewMaster);

		oldMaster->startupAfterException = (oldMaster->runningMode == NODE_RUNNING_MODE_MASTER &&
											newMaster->runningMode == NODE_RUNNING_MODE_SLAVE);
		shutdownNodeWithinSeconds(oldMaster->mgrNode,
									SHUTDOWN_NODE_FAST_SECONDS,
									SHUTDOWN_NODE_IMMEDIATE_SECONDS,
									complain);
		/* ensure gtm info is correct */
		gtmMaster = getGtmCoordMaster(coordinators);
		Assert(gtmMaster);
		setCheckGtmInfoInPGSqlConf(gtmMaster->mgrNode,
									newMaster->mgrNode,
									newMaster->pgConn,
									true,
									CHECK_GTM_INFO_SECONDS,
									complain);

		promoteNewMasterStartReign(oldMaster, newMaster);

		appendSlaveNodeFollowMasterEx(spiContext,
										newMaster,
										oldMaster, 
										true);
		refreshPgxcNodesOfCoordinators(holdLockCoordinator,
										coordinators,
										oldMaster,
										newMaster);
		/* change pgxc_node on datanode master */
		refreshPgxcNodesOfNewDataNodeMaster(holdLockCoordinator,
											oldMaster,
											newMaster,
											true);
		refreshOldMasterAfterSwitchover(oldMaster,
										newMaster,
										spiContext);
		refreshSlaveNodesAfterSwitch(newMaster,
										oldMaster,	
										runningSlaves,
										failedSlaves,
										runningSlavesSecond,
										failedSlavesSecond,
										spiContext,
										OVERTYPE_SWITCHOVER,
										curZone);
		refreshAsyncToSync(spiContext, &runningSlaveOfNewMaster);
		refreshReplicationSlots(oldMaster,
								newMaster);
		refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
										newMaster->mgrNode,
										spiContext,
										false);
		pfreeSwitcherNodeWrapperList(&runningSlaveOfNewMaster, NULL);
	}
	PG_CATCH();
	{
		pfreeSwitcherNodeWrapperList(&runningSlaveOfNewMaster, NULL);
		PG_RE_THROW();
	}
	PG_END_TRY();

	ereportNoticeLog(errmsg("Switch the datanode master from %s to %s "
					"has been successfully completed",
					NameStr(oldMaster->mgrNode->form.nodename),
					NameStr(newMaster->mgrNode->form.nodename)));

	ereport(LOG, (errmsg("------------- switchoverDataNode newMasterName(%s) unknow(%d) slave(%d) master(%d) after -------------", 
		newMasterName, NODE_RUNNING_MODE_UNKNOW, NODE_RUNNING_MODE_SLAVE, NODE_RUNNING_MODE_MASTER)));			
	PrintMgrNodeList(spiContext);
}
static void GetSwitchSlaveByMaster(MemoryContext spiContext, 
									MgrNodeWrapper *mgrNode, 
									char *zone, 
									char type,
									NameData *newDataName)
{
	dlist_head resultList = DLIST_STATIC_INIT(resultList);
	dlist_node *ptr;
	MgrNodeWrapper *mgrNodeSlave = NULL;

	selectActiveMgrSlaveNodesInZone(mgrNode->form.oid,
									type,
									zone,
									spiContext,
									&resultList);
	if (dlist_is_empty(&resultList)){
		ereport(ERROR, (errmsg("because no coordinator slave in zone(%s), so can't switchover or failover.", zone)));
	}								
	ptr = dlist_pop_head_node(&resultList);
	mgrNodeSlave = dlist_container(MgrNodeWrapper, link, ptr);
	Assert(mgrNodeSlave);
	namestrcpy(newDataName, NameStr(mgrNodeSlave->form.nodename));
	pfreeMgrHostWrapperList(&resultList, NULL);
}
void ZoneSwitchoverFree(ZoneOverGtm *zoGtm, 
						dlist_head *zoCoordList, 
						dlist_head *zoDN)
{
	SwitchoverGtmFreeMalloc(zoGtm);
	SwitchoverCoordFreeMalloc(zoCoordList);
	SwitchoverDataNodeFreeMalloc(zoDN);
}
static void SwitchoverGtmFreeMalloc(ZoneOverGtm *zoGtm)
{
	pfreeSwitcherNodeWrapperList(&zoGtm->failedSlaves, NULL);
	pfreeSwitcherNodeWrapperList(&zoGtm->failedSlavesSecond, NULL);	
	pfreeSwitcherNodeWrapperList(&zoGtm->runningSlavesSecond, NULL);
	pfreeSwitcherNodeWrapperListEx(&zoGtm->runningSlaves, 
									zoGtm->newMaster, 
									zoGtm->oldMaster);
	pfreeSwitcherNodeWrapperList(&zoGtm->coordinatorSlaves, NULL);	
	pfreeSwitcherNodeWrapperList(&zoGtm->dataNodes, NULL);
	pfreeSwitcherNodeWrapperList(&zoGtm->runningSlaveOfNewMaster, NULL);
	pfreeSwitcherNodeWrapper(zoGtm->oldMaster);						
	pfreeSwitcherNodeWrapper(zoGtm->newMaster);
}
static void SwitchoverCoordFreeMalloc(dlist_head *zoCoordList)
{
	dlist_iter 					iter;
	ZoneOverCoordWrapper *node;

	if (zoCoordList != NULL)
	{
		dlist_foreach(iter, zoCoordList)
		{
			node = dlist_container(ZoneOverCoordWrapper, link, iter.cur);
			Assert(node);
			pfreeSwitcherNodeWrapperList(&node->zoCoord->runningSlaves, NULL);
			pfreeSwitcherNodeWrapperList(&node->zoCoord->failedSlaves, NULL);
			pfreeSwitcherNodeWrapper(node->zoCoord->oldMaster);		
			pfreeSwitcherNodeWrapper(node->zoCoord->newMaster);
		}
	}
}
static void SwitchoverDataNodeFreeMalloc(dlist_head *zoDNList)
{
	dlist_iter 			iter;
	ZoneOverDNWrapper 	*node;

	if (zoDNList != NULL)
	{
		dlist_foreach(iter, zoDNList)
		{
			node = dlist_container(ZoneOverDNWrapper, link, iter.cur);
			Assert(node);
			pfreeSwitcherNodeWrapperList(&node->zoDN->runningSlaves, NULL);
			pfreeSwitcherNodeWrapperList(&node->zoDN->failedSlaves, NULL);
			pfreeSwitcherNodeWrapperList(&node->zoDN->runningSlavesSecond, NULL);
			pfreeSwitcherNodeWrapperList(&node->zoDN->failedSlavesSecond, NULL);
			pfreeSwitcherNodeWrapper(node->zoDN->oldMaster);		
			pfreeSwitcherNodeWrapper(node->zoDN->newMaster);
		}
	}
}
void RevertZoneSwitchover(MemoryContext spiContext, 
							ZoneOverGtm *zoGtm,
							dlist_head *zoCoordList,
							dlist_head *zoDNList)
{
	RevertZoneOverDataNodes(spiContext,
							zoGtm,
							zoDNList,
							OVERTYPE_SWITCHOVER);
	RevertZoneOverCoords(spiContext, 
						zoGtm, 
						zoCoordList,
						OVERTYPE_SWITCHOVER);	
	RevertZoneSwitchOverGtm(spiContext, 
							zoGtm);
}
static void RevertZoneOverDataNodes(MemoryContext spiContext, 
									ZoneOverGtm *zoGtm, 
									dlist_head *zoDNList,
									char *overType)
{
	dlist_iter 			iter;
	ZoneOverDNWrapper 	*node;

	if (zoDNList != NULL)
	{
		dlist_foreach(iter, zoDNList)
		{
			node = dlist_container(ZoneOverDNWrapper, link, iter.cur);
			Assert(node);
			if (pg_strcasecmp(overType, OVERTYPE_SWITCHOVER) == 0){
				RevertZoneSwitchoverDataNode(spiContext, 
											zoGtm,
											node->zoDN);	
			}
			else{
				RevertZoneFailOverDataNode(spiContext, 
											zoGtm,
											node->zoDN);
			}
		}
	}
}
static void RevertZoneSwitchoverDataNode(MemoryContext spiContext, 
										ZoneOverGtm *zoGtm, 
										ZoneOverDN	*zoDN)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	SwitcherNodeWrapper *holdLockCoordinator = NULL;
	dlist_head 			*coordinators = NULL;
	dlist_head 			*dataNodes = NULL;
	bool 				complain = false;

	Assert(zoGtm);
	Assert(zoDN);	
	Assert(holdLockCoordinator = zoGtm->holdLockCoordinator);
	Assert(coordinators = &zoGtm->coordinators);
	Assert(dataNodes = &zoGtm->dataNodes);
	CheckNull(oldMaster	= zoDN->oldMaster);
	CheckNull(newMaster	= zoDN->newMaster);

	shutdownNodeWithinSeconds(newMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								complain);
	ClosePgConn(newMaster->pgConn);

	callAgentStartNode(oldMaster->mgrNode, 
						false, 
						complain);
	promoteNewMasterStartReign(newMaster, oldMaster);
								
	appendSlaveNodeFollowMasterEx(spiContext,
								oldMaster,
								newMaster,								 
								complain);
	RevertRefreshPgxcNodeList(holdLockCoordinator, 
								coordinators,
								newMaster,
								oldMaster,					 
								complain);
	refreshPgxcNodesOfNewDataNodeMaster(holdLockCoordinator,
										newMaster,
										oldMaster,
										complain);
	refreshReplicationSlots(newMaster, oldMaster);									
}
static void RevertZoneOverGtm(MemoryContext spiContext, 
								ZoneOverGtm *zoGtm, 
								dlist_head *zoCoordList,
								char *overType)
{
	if (pg_strcasecmp(overType, OVERTYPE_SWITCHOVER) == 0){
		RevertZoneSwitchOverGtm(spiContext, zoGtm);
	}
	else{
		RevertZoneFailOverGtm(spiContext, 
								zoGtm, 
								zoCoordList);
	}
}								 
static void RevertZoneOverCoords(MemoryContext spiContext, 
								 ZoneOverGtm *zoGtm, 
								 dlist_head *zoCoordList,
								 char *overType)
{
	dlist_iter 				iter;
	ZoneOverCoordWrapper 	*node;

	if (zoCoordList != NULL)
	{
		dlist_foreach(iter, zoCoordList)
		{
			node = dlist_container(ZoneOverCoordWrapper, link, iter.cur);
			Assert(node);
			if (pg_strcasecmp(overType, OVERTYPE_SWITCHOVER) == 0){
				RevertZoneSwitchoverCoord(spiContext,
										zoGtm,
										node->zoCoord->oldMaster, 
										node->zoCoord->newMaster);
			}
			else{
				RevertZoneFailOverCoord(spiContext,
										zoGtm,
										node->zoCoord->oldMaster,
										node->zoCoord->newMaster);		
			}
		}
	}
}
static void RevertZoneSwitchoverCoord(MemoryContext spiContext,
									ZoneOverGtm *zoGtm,
									SwitcherNodeWrapper *oldMaster,
									SwitcherNodeWrapper *newMaster)
{
	bool complain = false;
	dlist_head *coordinators = &zoGtm->coordinators;
	CheckNull(oldMaster);
	CheckNull(newMaster);

	shutdownNodeWithinSeconds(newMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								complain);
	ClosePgConn(newMaster->pgConn);

	callAgentStartNode(oldMaster->mgrNode, false, complain);

	promoteNewMasterStartReign(newMaster, oldMaster);

	appendSlaveNodeFollowMasterEx(spiContext,
								oldMaster,
								newMaster,					 
								complain);		
	RefreshOldNodeWrapperConfig(oldMaster, newMaster);

	DelNodeFromSwitcherNodeWrappers(coordinators, newMaster);
	dlist_push_tail(coordinators, &oldMaster->link);
	RevertRefreshPgxcNodeList(zoGtm->holdLockCoordinator, 
								coordinators,
								newMaster,
								oldMaster,					
								complain);
}
static void RevertZoneSwitchOverGtm(MemoryContext spiContext, 
								ZoneOverGtm *zoGtm)
{
	SwitcherNodeWrapper *oldGtmMaster = zoGtm->oldMaster;
	SwitcherNodeWrapper *newGtmMaster = zoGtm->newMaster;
	dlist_head *coordinators = &zoGtm->coordinators;
	bool 		complain = false;

    CheckNull(oldGtmMaster);
	CheckNull(newGtmMaster);

    batchSetCheckGtmInfoOnAllNodes(zoGtm, oldGtmMaster);

	RefreshOldNodeWrapperConfig(oldGtmMaster, newGtmMaster);
	RevertRefreshPgxcNodeList(zoGtm->holdLockCoordinator, 
								coordinators,
								newGtmMaster,
								oldGtmMaster,								 
								complain); 
								
	shutdownNodeWithinSeconds(newGtmMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								complain);
	if (newGtmMaster->holdClusterLock)
	{
		newGtmMaster->holdClusterLock = false;
		ereportNoticeLog(errmsg("%s has been shut down and the cluster is unlocked",
						NameStr(newGtmMaster->mgrNode->form.nodename)));
	}
	ClosePgConn(newGtmMaster->pgConn);
	tryUnlockCluster(coordinators, true);

	setGtmInfoInPGSqlConf(zoGtm->oldMaster->mgrNode, oldGtmMaster->mgrNode, false);
	callAgentStartNode(oldGtmMaster->mgrNode, 
						false, 
						complain);
	promoteNewMasterStartReign(newGtmMaster, oldGtmMaster);

	appendSlaveNodeFollowMasterEx(spiContext,
								oldGtmMaster,
								newGtmMaster,								 
								complain);

	refreshAsyncToSync(spiContext, &zoGtm->runningSlaveOfNewMaster);

	DelNodeFromSwitcherNodeWrappers(coordinators, newGtmMaster);
	dlist_push_head(coordinators, &oldGtmMaster->link);

}
static void batchSetCheckGtmInfoOnAllNodes(ZoneOverGtm *zoGtm,
											SwitcherNodeWrapper *gtmMaster)
{
	setGtmInfoInPGSqlConf(zoGtm->oldMaster->mgrNode, gtmMaster->mgrNode, false);
	setGtmInfoInPGSqlConf(zoGtm->newMaster->mgrNode, gtmMaster->mgrNode, false);
	SetGtmInfoToList(&zoGtm->coordinators, gtmMaster);
	SetGtmInfoToList(&zoGtm->coordinatorSlaves, gtmMaster);
	SetGtmInfoToList(&zoGtm->dataNodes, gtmMaster);
	SetGtmInfoToList(&zoGtm->failedSlaves, gtmMaster);
	SetGtmInfoToList(&zoGtm->failedSlavesSecond, gtmMaster);
	SetGtmInfoToList(&zoGtm->runningSlavesSecond, gtmMaster);
	SetGtmInfoToList(&zoGtm->runningSlaves, gtmMaster);
}
static bool RevertRefreshPgxcNodeList(SwitcherNodeWrapper *holdLockCoordinator,
										dlist_head *nodeList,
										SwitcherNodeWrapper *oldMaster,  
										SwitcherNodeWrapper *newMaster, 
										bool complain)
{
	dlist_iter 			iter;
	SwitcherNodeWrapper *node;
	bool 				execOk = true;

	dlist_foreach(iter, nodeList)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		Assert(node);
		if (node->pgxcNodeChanged)
		{
			if (updatePgxcNodeForSwitch(holdLockCoordinator,
										node,       
										oldMaster, 
										newMaster,	 						
										false))
			{
				ereport(LOG, (errmsg("revert pgxc_node on node(%s) from %s to %s success", 
								NameStr(node->mgrNode->form.nodename),
								NameStr(oldMaster->mgrNode->form.nodename), 
								NameStr(newMaster->mgrNode->form.nodename))));
			}
			else
			{
				execOk = false;
				ereport(ERROR, (errmsg("revert pgxc_node on node(%s) from %s to %s failed", 
								NameStr(node->mgrNode->form.nodename),
								NameStr(oldMaster->mgrNode->form.nodename), 
								NameStr(newMaster->mgrNode->form.nodename))));				
			}
			pgxcPoolReloadOnNode(holdLockCoordinator, node, false);
		}
	}
	return execOk;
}
static void InitZoneOverCoord(ZoneOverCoord *zoCoord)
{
	zoCoord->oldMaster = NULL;
	zoCoord->newMaster = NULL;
	dlist_init(&zoCoord->runningSlaves);
	dlist_init(&zoCoord->failedSlaves);
}
static void InitZoneoverDN(ZoneOverDN *zoDN)
{
	zoDN->oldMaster = NULL;
	zoDN->newMaster = NULL;
	dlist_init(&zoDN->runningSlaves);
	dlist_init(&zoDN->runningSlavesSecond);
	dlist_init(&zoDN->failedSlaves);
	dlist_init(&zoDN->failedSlavesSecond);
}
static NodeRunningMode getNodeRunningModeEx(SwitcherNodeWrapper *holdLockNode, 
											SwitcherNodeWrapper *executeOnNode)
{
	PGconn 	*activeConn;
	bool 	localExecute;
	char 	*sql;
	bool 	res;
	char 	*value;
	PGresult *pgResult;

	if (holdLockNode)
	{
		activeConn = holdLockNode->pgConn;
		if (pg_strcasecmp(NameStr(holdLockNode->mgrNode->form.nodename), NameStr(executeOnNode->mgrNode->form.nodename)) == 0){
			localExecute = true;
		}
		else{
			localExecute = false;
		}
	}
	else
	{
		activeConn = executeOnNode->pgConn;
		localExecute = true;
	}

	if (localExecute)
		sql = psprintf("select * from pg_catalog.pg_is_in_recovery();");
	else
		sql = psprintf("EXECUTE DIRECT ON (\"%s\") "
					   "'select * from pg_catalog.pg_is_in_recovery();';",
					   NameStr(executeOnNode->mgrNode->form.nodename));

	pgResult = PQexec(activeConn, sql);
	if (PQresultStatus(pgResult) == PGRES_TUPLES_OK)
	{
		value = PQgetvalue(pgResult, 0, 0);
		if (pg_strcasecmp(value, "t") == 0)
		{
			res = NODE_RUNNING_MODE_SLAVE;
		}
		else if (pg_strcasecmp(value, "f") == 0)
		{
			res = NODE_RUNNING_MODE_MASTER;
		}
		else
		{
			res = NODE_RUNNING_MODE_UNKNOW;
		}
	}
	else
	{
		ereport(LOG, (errmsg("execute %s failed:%s", sql, PQerrorMessage(activeConn))));
		res = NODE_RUNNING_MODE_UNKNOW;
	}
	if (pgResult)
		PQclear(pgResult);
	return res;
}

static void SetXcMaintenanceModeOn(PGconn *pgConn)
{
	char *sql = "set xc_maintenance_mode = on";
	PQexecCommandSql(pgConn, sql, true);
}
static void CheckpointCoord(PGconn *pgConn, SwitcherNodeWrapper *node)
{
	char *sql = psprintf("EXECUTE DIRECT ON (\"%s\") "
					   "'checkpoint;'",
					   NameStr(node->mgrNode->form.nodename));
	PQexecCommandSql(pgConn, sql, true);
}
static void DelNodeFromSwitcherNodeWrappers(dlist_head *nodes, 
											SwitcherNodeWrapper *delNode)
{
	dlist_mutable_iter miter;
	SwitcherNodeWrapper *node;

	dlist_foreach_modify(miter, nodes) 
	{
		node = dlist_container(SwitcherNodeWrapper, link, miter.cur);
		if (isSameNodeName(node->mgrNode, delNode->mgrNode))
		{
			dlist_delete(miter.cur);
			break;
		}
	}
}
void MgrZoneFailoverGtm(MemoryContext spiContext, 
						char 	*currentZone,
						bool 	forceSwitch,
						int 	maxTrys, 
						ZoneOverGtm *zoGtm)
{
	MgrNodeWrapper 	*oldGtmMaster = NULL;
	NameData       	oldMasterName = {{0}};
	bool 			kickOutOldMaster = true;

	Assert(spiContext);
	Assert(currentZone);
	Assert(zoGtm);

	oldGtmMaster = MgrGetOldGtmMasterNotZone(spiContext, currentZone, OVERTYPE_FAILOVER);
	Assert(oldGtmMaster);
	namestrcpy(&oldMasterName, NameStr(oldGtmMaster->form.nodename));
	pfreeMgrNodeWrapper(oldGtmMaster);
	FailOverGtmCoordMasterForZone(spiContext,
									NameStr(oldMasterName), 
									currentZone,
									forceSwitch,
									maxTrys,
									kickOutOldMaster,
									zoGtm);	
	return;
}
void MgrZoneFailoverCoord(MemoryContext spiContext, 
								char 	*currentZone,
								bool	forceSwitch,
								int 	maxTrys,
								ZoneOverGtm *zoGtm, 
								dlist_head *zoCoordList)
{
	dlist_head 			masterList = DLIST_STATIC_INIT(masterList);
	MgrNodeWrapper 		*mgrNode;
	dlist_iter 			iter;
	NameData 			newMasterName = {{0}};
	bool 				kickOutOldMaster = true;
	ZoneOverCoord 		   *zoCoord = NULL;
	ZoneOverCoordWrapper   *zoCoordWrapper = NULL;
	
	MgrGetOldDnMasterNotZone(spiContext, 
							currentZone, 
							CNDN_TYPE_COORDINATOR_MASTER, 
							&masterList, 
							OVERTYPE_FAILOVER);
	dlist_foreach(iter, &masterList)
	{
		PG_TRY();
		{
			mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
			Assert(mgrNode);
            memset(&newMasterName, 0x00, sizeof(NameData));
			zoCoordWrapper = (ZoneOverCoordWrapper*)palloc0(sizeof(ZoneOverCoordWrapper));
			zoCoord = (ZoneOverCoord*)palloc0(sizeof(ZoneOverCoord));
			InitZoneOverCoord(zoCoord);
			zoCoordWrapper->zoCoord = zoCoord;
			
			FailOverCoordMasterForZone(spiContext,
										NameStr(mgrNode->form.nodename), 
										currentZone, 
										forceSwitch, 
										kickOutOldMaster, 
										maxTrys,
										zoGtm, 
										zoCoord);
			dlist_push_tail(zoCoordList, &zoCoordWrapper->link);					
		}
		PG_CATCH();
		{
			pfreeMgrNodeWrapperList(&masterList, NULL);
			dlist_push_tail(zoCoordList, &zoCoordWrapper->link);
			PG_RE_THROW();
		}PG_END_TRY();
	}
	
	
	pfreeMgrNodeWrapperList(&masterList, NULL);
	return;
}
void MgrZoneFailoverDN(MemoryContext spiContext, 
								char 	*currentZone, 
								bool	forceSwitch,
								int 	maxTrys, 
								ZoneOverGtm *zoGtm,
								dlist_head *zoDNList)
{
	dlist_head 			masterList = DLIST_STATIC_INIT(masterList);
	MgrNodeWrapper 		*mgrNode;
	dlist_iter 			iter;
	NameData 			newMasterName = {{0}};
	ZoneOverDNWrapper 	*zoDNWrapper = NULL;
	ZoneOverDN        	*zoDN = NULL;
	bool 				kickOutOldMaster = true;
	
	MgrGetOldDnMasterNotZone(spiContext, 
							currentZone, 
							CNDN_TYPE_DATANODE_MASTER, 
							&masterList, 
							OVERTYPE_FAILOVER);
	dlist_foreach(iter, &masterList)
	{
		PG_TRY();
		{
			mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
			Assert(mgrNode);
			memset(&newMasterName, 0x00, sizeof(NameData));

			zoDNWrapper = (ZoneOverDNWrapper*)palloc0(sizeof(ZoneOverDNWrapper));
			zoDN = (ZoneOverDN*)palloc0(sizeof(ZoneOverDN));
			InitZoneoverDN(zoDN);
			zoDNWrapper->zoDN = zoDN;

			FailOverDataNodeMasterForZone(spiContext,
										NameStr(mgrNode->form.nodename),
										currentZone,
										forceSwitch,
										kickOutOldMaster,
										maxTrys,
										zoGtm,
										zoDN);

			dlist_push_tail(zoDNList, &zoDNWrapper->link);
		}PG_CATCH();
		{
			pfreeMgrNodeWrapperList(&masterList, NULL);
			dlist_push_tail(zoDNList, &zoDNWrapper->link);
			PG_RE_THROW();			
		}PG_END_TRY();
	}
	pfreeMgrNodeWrapperList(&masterList, NULL);
	return;
}
static void FailOverGtmCoordMasterForZone(MemoryContext spiContext,
									char 	*oldMasterName,
									char  	*curZone,
									bool 	forceSwitch,
									int 	maxTrys,
									bool 	kickOutOldMaster,
									ZoneOverGtm *zoGtm)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head  *coordinators		= &zoGtm->coordinators;
	dlist_head  *coordinatorSlaves  = &zoGtm->coordinatorSlaves;
	dlist_head  *runningSlaves 		= &zoGtm->runningSlaves;
	dlist_head  *runningSlavesSecond= &zoGtm->runningSlavesSecond;
	dlist_head  *failedSlaves 		= &zoGtm->failedSlaves;
	dlist_head  *failedSlavesSecond = &zoGtm->failedSlavesSecond;
	dlist_head  *dataNodes 			= &zoGtm->dataNodes;
	bool 		complain = true;
	NameData    newMasterName = {{0}};

	ereport(LOG, (errmsg("------------- FailOverGtmCoordMaster oldMasterName(%s) before -------------", oldMasterName)));			
	PrintMgrNodeList(spiContext);

	oldMaster = checkGetOldMasterForZoneCoord(spiContext,
											CNDN_TYPE_GTM_COOR_MASTER,											
											oldMasterName);
	oldMaster->startupAfterException = false;
	
	checkGetSlaveNodesRunningStatus(oldMaster,
									spiContext,
									(Oid)0,
									curZone,
									failedSlaves,
									runningSlaves);							
	checkGetCoordinatorsForZone(spiContext,
								oldMaster,
								curZone,
								CNDN_TYPE_COORDINATOR_MASTER,	
								coordinators);
	checkGetCoordinatorsForZone(spiContext,
								oldMaster,
								curZone,
								CNDN_TYPE_COORDINATOR_SLAVE,	
								coordinatorSlaves);								  
	checkGetAllDataNodesForZone(dataNodes, 
								curZone,
								spiContext);
	chooseNewMasterNode(oldMaster,
						&newMaster,
						runningSlaves,
						failedSlaves,
						spiContext,
						forceSwitch,
						NameStr(newMasterName),
						curZone);					
	checkGetSlaveNodesRunningSecondStatus(oldMaster,
										spiContext,
										(Oid)newMaster->mgrNode->form.oid,
										runningSlaves,
										failedSlaves,
										runningSlavesSecond,
										failedSlavesSecond);
	CHECK_FOR_INTERRUPTS();

	zoGtm->oldMaster = oldMaster;					
	zoGtm->newMaster = newMaster;

	/* Prevent other doctor processes from manipulating these nodes simultaneously */
	refreshSlaveNodesBeforeSwitch(newMaster,
								runningSlaves,
								failedSlaves,
								runningSlavesSecond,
								failedSlavesSecond,
								spiContext);
	refreshMgrNodeListBeforeSwitch(spiContext, coordinators);
	refreshMgrNodeListBeforeSwitch(spiContext, coordinatorSlaves);
	refreshMgrNodeListBeforeSwitch(spiContext, dataNodes);							  
	setCheckGtmInfoInPGSqlConf(newMaster->mgrNode,
								newMaster->mgrNode,
								newMaster->pgConn,
								true,
								CHECK_GTM_INFO_SECONDS,
								true);
	newMaster->gtmInfoChanged = true;

	RefreshGtmAdbCheckSyncNextid(newMaster->mgrNode, ADB_CHECK_SYNC_NEXTID_OFF);
	promoteNewMasterStartReign(oldMaster, newMaster);

	UpdateSyncInfo(newMaster->mgrNode,
					newMaster->pgConn,
					"",
					spiContext);

	ereportNoticeLog(errmsg("set gtmhost, gtmport to every node, please wait for a moment."));
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,	dataNodes, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, coordinators, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, coordinatorSlaves, NULL, complain);
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode,	runningSlaves, NULL, complain);
	/* runningSlavesSecond may contain node in zone 'local', if the agent of local is down, the SetCheckGtm and stop may be fail  */
	batchSetCheckGtmInfoOnNodes(newMaster->mgrNode, runningSlavesSecond, NULL, false);
	RestartCurzoneNodes(dataNodes, 
						coordinators, 
						coordinatorSlaves, 
						runningSlaves, 
						complain);
	restartNodes(runningSlavesSecond, false);
	
	/* newMaster also is a coordinator */
	newMaster->mgrNode->form.nodetype =	getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
	dlist_push_head(coordinators, &newMaster->link);

	/* The better slave node is in front of the list */
	sortNodesByWalLsnDesc(runningSlaves);	
	runningSlavesFollowNewMaster(newMaster,
								oldMaster,
								runningSlaves,
								NULL,
								spiContext,
								OVERTYPE_FAILOVER,
								curZone);

	zoGtm->holdLockCoordinator = getGtmCoordMaster(coordinators);


	refreshOldMasterAfterSwitch(oldMaster,
								newMaster,
								spiContext,
								kickOutOldMaster);
	refreshSlaveNodesAfterSwitch(newMaster,
								oldMaster,			
								runningSlaves,
								failedSlaves,
								runningSlavesSecond,
								failedSlavesSecond,
								spiContext,
								OVERTYPE_FAILOVER,
								curZone);
	refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
									newMaster->mgrNode,
									spiContext,
									kickOutOldMaster);
	refreshMgrNodeListAfterFailoverGtm(spiContext, coordinators);
	refreshMgrNodeListAfterFailoverGtm(spiContext, coordinatorSlaves);
	refreshMgrNodeListAfterFailoverGtm(spiContext, dataNodes);

	ereportNoticeLog(errmsg("Switch the GTM master from %s to %s "
					"has been successfully completed",
					NameStr(oldMaster->mgrNode->form.nodename),
					NameStr(newMaster->mgrNode->form.nodename)));

	ereport(LOG, (errmsg("------------- FailOverGtmCoordMaster oldMasterName(%s) after -------------", oldMasterName)));			
	PrintMgrNodeList(spiContext);

}
static void FailOverCoordMasterForZone(MemoryContext spiContext,
										char 	*oldMasterName,
										char 	*curZone,
										bool 	forceSwitch,
										bool 	kickOutOldMaster,
										int 	maxTrys,		  
										ZoneOverGtm *zoGtm,
										ZoneOverCoord *zoCoord)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head  *coordinators  = &zoGtm->coordinators;
	dlist_head  *runningSlaves = &zoCoord->runningSlaves;
	dlist_head  *failedSlaves  = &zoCoord->failedSlaves;
	SwitcherNodeWrapper *holdLockCoordinator = zoGtm->holdLockCoordinator;
	NameData       		newMasterName = {{0}};
	bool complain = true;

	Assert(holdLockCoordinator);

	ereport(LOG, (errmsg("------------- FailOverCoordMaster oldMasterName(%s) before -------------", oldMasterName)));				
	PrintMgrNodeList(spiContext);

	oldMaster = checkGetOldMasterForZoneCoord(spiContext,
											CNDN_TYPE_COORDINATOR_MASTER,											
											oldMasterName);
	oldMaster->startupAfterException = false;

	checkGetSlaveNodesRunningStatus(oldMaster,
									spiContext,
									(Oid)0,
									curZone,
									failedSlaves,
									runningSlaves);	
	chooseNewMasterNode(oldMaster,
						&newMaster,
						runningSlaves,
						failedSlaves,
						spiContext,
						forceSwitch,
						NameStr(newMasterName),
						curZone);
	DelNodeFromSwitcherNodeWrappers(coordinators, oldMaster);

	CHECK_FOR_INTERRUPTS();

	zoCoord->oldMaster = oldMaster;					
	zoCoord->newMaster = newMaster;

	/* Prevent other doctor processes from manipulating these nodes simultaneously */
	refreshMgrNodeBeforeSwitch(newMaster, spiContext);
	refreshMgrNodeListBeforeSwitch(spiContext, runningSlaves);
	refreshMgrNodeListBeforeSwitch(spiContext, failedSlaves);							  

	setCheckGtmInfoInPGSqlConf(holdLockCoordinator->mgrNode,
								newMaster->mgrNode,
								newMaster->pgConn,
								true,
								CHECK_GTM_INFO_SECONDS,
								complain);

	promoteNewMasterStartReign(oldMaster, newMaster);

	newMaster->mgrNode->form.nodetype =	getMgrMasterNodetype(newMaster->mgrNode->form.nodetype);
	dlist_push_tail(coordinators, &newMaster->link);

	sortNodesByWalLsnDesc(runningSlaves);
	runningSlavesFollowNewMaster(newMaster,
								oldMaster,
								runningSlaves,
								NULL,
								spiContext,
								OVERTYPE_FAILOVER,
								curZone);
	refreshOldMasterAfterSwitch(oldMaster,
								newMaster,
								spiContext,
								kickOutOldMaster);
	refreshSlaveNodesAfterSwitch(newMaster,
								oldMaster,
								runningSlaves,
								failedSlaves,
								NULL,
								NULL,
								spiContext,
								OVERTYPE_FAILOVER,
								curZone);
	
	refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
									newMaster->mgrNode,
									spiContext,
									kickOutOldMaster);

	ereportNoticeLog((errmsg("Switch the coordinator master from %s to %s "
					"has been successfully completed",
					NameStr(oldMaster->mgrNode->form.nodename),
					NameStr(newMaster->mgrNode->form.nodename))));						

	ereport(LOG, (errmsg("------------- FailOverCoordMaster oldMasterName(%s) after -------------", oldMasterName)));			
	PrintMgrNodeList(spiContext);
}
static void FailOverDataNodeMasterForZone(MemoryContext spiContext,
										char 	*oldMasterName,
										char 	*curZone,
										bool 	forceSwitch,
										bool 	kickOutOldMaster,
										int 	maxTrys,
										ZoneOverGtm *zoGtm,
										ZoneOverDN *zoDN)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	SwitcherNodeWrapper *holdLockCoordinator = zoGtm->holdLockCoordinator;
	dlist_head *runningSlaves 		= &zoDN->runningSlaves;
	dlist_head *runningSlavesSecond = &zoDN->runningSlavesSecond;
	dlist_head *failedSlaves 		= &zoDN->failedSlaves;
	dlist_head *failedSlavesSecond  = &zoDN->failedSlavesSecond;
	bool 		complain = true;
	NameData	newMasterName = {{0}};
	Assert(holdLockCoordinator);

	ereport(LOG, (errmsg("------------- FailOverDataNodeMaster oldMasterName(%s) before -------------", oldMasterName)));				
	PrintMgrNodeList(spiContext);

	oldMaster = checkGetOldMasterForZoneCoord(spiContext,
											CNDN_TYPE_DATANODE_MASTER,											
											oldMasterName);
	oldMaster->startupAfterException = false;
	checkGetSlaveNodesRunningStatus(oldMaster,
									spiContext,
									(Oid)0,
									curZone,
									failedSlaves,
									runningSlaves);
	chooseNewMasterNode(oldMaster,
						&newMaster,
						runningSlaves,
						failedSlaves,
						spiContext,
						forceSwitch,
						NameStr(newMasterName),
						curZone);						
	checkGetSlaveNodesRunningSecondStatus(oldMaster,
										spiContext,
										(Oid)newMaster->mgrNode->form.oid,
										runningSlaves,
										failedSlaves,
										runningSlavesSecond,
										failedSlavesSecond);
	CHECK_FOR_INTERRUPTS();

	zoDN->oldMaster = oldMaster;					
	zoDN->newMaster = newMaster;

	/* Prevent other doctor processes from manipulating these nodes simultaneously */
	refreshSlaveNodesBeforeSwitch(newMaster,
								runningSlaves,
								failedSlaves,
								runningSlavesSecond,
								failedSlavesSecond,
								spiContext);

	/* ensure gtm info is correct */
	setCheckGtmInfoInPGSqlConf(holdLockCoordinator->mgrNode,
								newMaster->mgrNode,
								newMaster->pgConn,
								true,
								CHECK_GTM_INFO_SECONDS,
								complain);

	promoteNewMasterStartReign(oldMaster, newMaster);

	UpdateSyncInfo(newMaster->mgrNode,
					newMaster->pgConn,
					"",
					spiContext);
					
	/* The better slave node is in front of the list */
	sortNodesByWalLsnDesc(runningSlaves);
	runningSlavesFollowNewMaster(newMaster,
								oldMaster,
								runningSlaves,
								holdLockCoordinator->mgrNode,
								spiContext,
								OVERTYPE_FAILOVER,
								curZone);
	refreshOldMasterAfterSwitch(oldMaster,
								newMaster,
								spiContext,
								kickOutOldMaster);
	refreshSlaveNodesAfterSwitch(newMaster,
								oldMaster, 
								runningSlaves,
								failedSlaves,
								runningSlavesSecond,
								failedSlavesSecond,
								spiContext,
								OVERTYPE_FAILOVER,
								curZone);
	refreshMgrUpdateparmAfterSwitch(oldMaster->mgrNode,
									newMaster->mgrNode,
									spiContext,
									kickOutOldMaster);

	ereportNoticeLog(errmsg("Switch the datanode master from %s to %s "
							"has been successfully completed",
							NameStr(oldMaster->mgrNode->form.nodename),
							NameStr(newMaster->mgrNode->form.nodename)));

	ereport(LOG, (errmsg("------------- FailOverDataNodeMaster oldMasterName(%s) after -------------", oldMasterName)));			
	PrintMgrNodeList(spiContext);
}
void MgrRefreshAllPgxcNode(MemoryContext spiContext, 
							ZoneOverGtm *zoGtm, 
							dlist_head *zoCoordList,
							dlist_head *zoDNList)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head 			*coordinators = NULL;
	dlist_iter 			iter;
	ZoneOverCoordWrapper *ZoneOverCoord = NULL;
	ZoneOverDNWrapper    *ZoneOverDN = NULL;

	coordinators = &zoGtm->coordinators;
	CheckNull(coordinators);

	oldMaster = zoGtm->oldMaster;
	newMaster = zoGtm->newMaster;
	if (oldMaster != NULL && newMaster != NULL)
	{
		refreshPgxcNodesOfCoordinators(NULL,
									coordinators,
									oldMaster,
									newMaster);
	}

	dlist_foreach(iter, zoCoordList)
	{
		ZoneOverCoord = dlist_container(ZoneOverCoordWrapper, link, iter.cur);
		Assert(ZoneOverCoord);
		oldMaster = ZoneOverCoord->zoCoord->oldMaster;
		newMaster = ZoneOverCoord->zoCoord->newMaster;
		if (oldMaster != NULL && newMaster != NULL)
		{
			refreshPgxcNodesOfCoordinators(NULL,
											coordinators,
											oldMaster,
											newMaster);
		}
	}

	refreshPgxcNodeBeforeSwitchDataNode(coordinators);
	dlist_foreach(iter, zoDNList)
	{
		ZoneOverDN = dlist_container(ZoneOverDNWrapper, link, iter.cur);
		Assert(ZoneOverDN);
		oldMaster = ZoneOverDN->zoDN->oldMaster;
		newMaster = ZoneOverDN->zoDN->newMaster;
		if (oldMaster != NULL && newMaster != NULL)
		{
			refreshPgxcNodesOfCoordinators(NULL,
										coordinators,
										oldMaster,
										newMaster);
		}
	}	

	dlist_foreach(iter, zoDNList)
	{
		ZoneOverDN = dlist_container(ZoneOverDNWrapper, link, iter.cur);
		Assert(ZoneOverDN);
		oldMaster = ZoneOverDN->zoDN->oldMaster;
		newMaster = ZoneOverDN->zoDN->newMaster;
		if (oldMaster != NULL && newMaster != NULL)
		{
			refreshPgxcNodesOfNewDataNodeMaster(zoGtm->holdLockCoordinator,
												oldMaster,
												newMaster,
												true);	
		}
	}
}
void RevertZoneFailover(MemoryContext spiContext, 
						ZoneOverGtm *zoGtm, 
						dlist_head *zoCoordList,
						dlist_head *zoDNList)
{
	RevertZoneOverDataNodes(spiContext,
							zoGtm,
							zoDNList,
							OVERTYPE_FAILOVER);
	RevertZoneOverCoords(spiContext, 
						zoGtm, 
						zoCoordList,
						OVERTYPE_FAILOVER);	
	RevertZoneOverGtm(spiContext, 
					   zoGtm,
					   zoCoordList, 
					   OVERTYPE_FAILOVER);					   
}
static void RevertZoneFailOverDataNode(MemoryContext spiContext, 
										ZoneOverGtm *zoGtm, 
										ZoneOverDN	*zoDN)
{
	SwitcherNodeWrapper *oldMaster = NULL;
	SwitcherNodeWrapper *newMaster = NULL;
	dlist_head 			*coordinators = NULL;
	dlist_head 			*dataNodes = NULL;
	bool 				complain = false;

	Assert(zoGtm);
	Assert(zoDN);	
	Assert(coordinators = &zoGtm->coordinators);
	Assert(dataNodes = &zoGtm->dataNodes);
	CheckNull(oldMaster	= zoDN->oldMaster);
	CheckNull(newMaster	= zoDN->newMaster);

	RevertRefreshPgxcNodeList(NULL,
							coordinators,
							newMaster,
							oldMaster,								 
							complain);
	shutdownNodeWithinSeconds(newMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								complain);
	ClosePgConn(newMaster->pgConn);

	oldMaster->runningMode = NODE_RUNNING_MODE_UNKNOW;							
	appendSlaveNodeFollowMasterEx(spiContext,
									oldMaster,
									newMaster,								 
									complain);	
}
static void RevertZoneFailOverCoord(MemoryContext spiContext,
									ZoneOverGtm *zoGtm,
									SwitcherNodeWrapper *oldMaster,
									SwitcherNodeWrapper *newMaster)
{
	bool complain = false;
	dlist_head *coordinators = &zoGtm->coordinators;
	CheckNull(oldMaster);
	CheckNull(newMaster);

	if (newMaster->runningMode == NODE_RUNNING_MODE_MASTER){
		RevertRefreshPgxcNodeList(NULL, 
								coordinators,
								newMaster,  
								oldMaster,  
								complain);
	}
}
static void RevertZoneFailOverGtm(MemoryContext spiContext,
								  ZoneOverGtm *zoGtm,
								  dlist_head *zoCoordList)
{
	SwitcherNodeWrapper *oldGtmMaster = zoGtm->oldMaster;
	SwitcherNodeWrapper *newGtmMaster = zoGtm->newMaster;
	dlist_head *coordinators = &zoGtm->coordinators;
	bool 		complain = false;

    CheckNull(oldGtmMaster);
	CheckNull(newGtmMaster);

    RevertRefreshPgxcNodeList(NULL, 
							coordinators,
							newGtmMaster,
							oldGtmMaster,								 
							complain);

	RevertFailOverShutdownCoords(spiContext, 
								 zoCoordList);

	batchSetCheckGtmInfoOnAllNodes(zoGtm, oldGtmMaster);

	RefreshOldNodeWrapperConfig(oldGtmMaster, newGtmMaster);
								
	shutdownNodeWithinSeconds(newGtmMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								complain);
	if (newGtmMaster->holdClusterLock)
	{
		newGtmMaster->holdClusterLock = false;
		ereportNoticeLog(errmsg("%s has been shut down and the cluster is unlocked",
						NameStr(newGtmMaster->mgrNode->form.nodename)));
	}
	ClosePgConn(newGtmMaster->pgConn);

	oldGtmMaster->runningMode = NODE_RUNNING_MODE_UNKNOW;
	appendSlaveNodeFollowMasterEx(spiContext,
									oldGtmMaster,
									newGtmMaster,								 
									complain);								
}
static void RefreshOldNodeWrapperConfig(SwitcherNodeWrapper *oldMaster,
										SwitcherNodeWrapper *newMaster)
{
	oldMaster->pgxcNodeChanged = newMaster->pgxcNodeChanged;
}
static void SetGtmInfoToList(dlist_head *nodes, SwitcherNodeWrapper *gtmMaster)
{
	dlist_iter 			iter;
	SwitcherNodeWrapper *node;

	if (nodes != NULL && gtmMaster != NULL)
	{
		dlist_foreach(iter, nodes)
		{
			node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
			Assert(node);
			ereport(LOG, (errmsg("SetGtmInfoToList nodename(%s), gtmInfoChanged(%d)", 
					NameStr(node->mgrNode->form.nodename), node->gtmInfoChanged)));
			setGtmInfoInPGSqlConf(node->mgrNode, gtmMaster->mgrNode, false);
		}	
	}
}
static void RevertFailOverShutdownCoords(MemoryContext spiContext, 
										 dlist_head *zoCoordList)
{
	dlist_iter 				iter;
	ZoneOverCoordWrapper 	*node;
	SwitcherNodeWrapper 	*oldMaster = NULL;
	SwitcherNodeWrapper 	*newMaster = NULL;
	bool 					complain = false;

	dlist_foreach(iter, zoCoordList)
	{
		node = dlist_container(ZoneOverCoordWrapper, link, iter.cur);
		Assert(node);
		oldMaster = node->zoCoord->oldMaster;
		newMaster = node->zoCoord->newMaster;
		CheckNull(oldMaster);
		CheckNull(newMaster);

		shutdownNodeWithinSeconds(newMaster->mgrNode,
								SHUTDOWN_NODE_FAST_SECONDS,
								SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								complain);
		ClosePgConn(newMaster->pgConn);

		oldMaster->runningMode = NODE_RUNNING_MODE_UNKNOW;
		appendSlaveNodeFollowMasterEx(spiContext,
										oldMaster,
										newMaster,					 
										complain);		
	}
}
int GetSlaveNodeNumInZone(MemoryContext spiContext, 
						MgrNodeWrapper *mgrNode, 
						char slaveType, 
						char *zone)
{
	dlist_head slaveNodes = DLIST_STATIC_INIT(slaveNodes);
	SwitcherNodeWrapper *node;
	dlist_iter iter;
	int slaveNum = 0;

	selectActiveMgrNodeChild(spiContext,
							mgrNode,
							slaveType,
							&slaveNodes);
	dlist_foreach(iter, &slaveNodes)
	{
		node = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		Assert(node);
		if (pg_strcasecmp(NameStr(node->mgrNode->form.nodezone), zone) == 0){
			slaveNum++;
		}
	}

	pfreeSwitcherNodeWrapperList(&slaveNodes, NULL);
	return slaveNum;
}
static void ExecuteSqlOnPostgresGrammar(Form_mgr_node mgrNode, int newPort, char *sql, int sqlType)
{
	MemoryContext oldContext = NULL;
	MemoryContext switchContext = NULL;
	MemoryContext spiContext = NULL;
	int spiRes;
	PGconn *pgConn = NULL;

	PG_TRY();
	{
		oldContext = CurrentMemoryContext;
		switchContext = AllocSetContextCreate(oldContext,
											"ExecuteSqlOnPostgresGrammar",
											ALLOCSET_DEFAULT_SIZES);
		spiRes = SPI_connect();
		if (spiRes != SPI_OK_CONNECT)
		{
			ereport(ERROR,
					(errmsg("SPI_connect failed, connect return:%d",
							spiRes)));
		}
		spiContext = CurrentMemoryContext;
		MemoryContextSwitchTo(switchContext);

		pgConn = GetNodeConn(mgrNode,
							newPort,	
							10,
							spiContext);
		if (pgConn == NULL)
		{
			ereport(LOG,
					(errmsg("[Error] get conn from %s fail. nodeport(%d), newPort(%d).", 
					NameStr(mgrNode->nodename), mgrNode->nodeport, newPort)));
			ereport(ERROR,
					(errmsg("get conn from %s fail.", NameStr(mgrNode->nodename))));		
		}
		if(sqlType == SQL_TYPE_COMMAND)
			PQexecCommandSql(pgConn, sql, true);
		else
			PQexecCountSql(pgConn, sql, true);

		ClosePgConn(pgConn);
		(void)MemoryContextSwitchTo(oldContext);
		MemoryContextDelete(switchContext);
		SPI_finish();
	}PG_CATCH();
	{
		ClosePgConn(pgConn);
		(void)MemoryContextSwitchTo(oldContext);
		MemoryContextDelete(switchContext);
		SPI_finish();	
		PG_RE_THROW();
	}PG_END_TRY();
}

static PGconn *GetNodeConn(Form_mgr_node nodeIn,
							int newPort,
							int connectTimeout,
							MemoryContext spiContext)
{
	MgrNodeWrapper *mgrNode;
	PGconn 	*conn = NULL;
	char 	*nodeName = NameStr(nodeIn->nodename);
	char  	nodeType = nodeIn->nodetype;

	mgrNode = selectMgrNodeByNodenameType(nodeName,
										  nodeType,
										  spiContext);
	if (!mgrNode)
	{
		ereport(ERROR,
				(errmsg("%s does not exist or is not a %s node",
						nodeName, mgr_get_nodetype_desc(nodeType))));
	}
	if (mgrNode->form.nodetype != nodeType)
	{
		ereport(ERROR,
				(errmsg("%s is not a %s node",
						nodeName, mgr_get_nodetype_desc(nodeType))));
	}
	if (newPort != 0)
		mgrNode->form.nodeport = newPort;
	conn = getNodeDefaultDBConnection(mgrNode, connectTimeout);

	pfreeMgrNodeWrapper(mgrNode);

	return conn;
}
bool ExecuteSqlOnPostgres(Form_mgr_node mgrNode, int newPort, char *sql)
{
	if (strlen(sql) == 0)
		return true;
	
	ExecuteSqlOnPostgresGrammar(mgrNode, newPort, sql, SQL_TYPE_COMMAND);
	ExecuteSqlOnPostgresGrammar(mgrNode, newPort, SELECT_PGXC_POOL_RELOAD, SQL_TYPE_QUERY);

	return true;	
}
bool CheckNodeExistInPgxcNode(Form_mgr_node mgrNode, char *existNodeName, char nodeType)
{
	MemoryContext oldContext = NULL;
	MemoryContext switchContext = NULL;
	MemoryContext spiContext = NULL;
	int spiRes;
	bool exist = false;
	PGconn 	*pgConn = NULL;
	StringInfoData	sql;

	initStringInfo(&sql);

	PG_TRY();
	{
		oldContext = CurrentMemoryContext;
		switchContext = AllocSetContextCreate(oldContext,
											"CheckNodeExistInPgxcNode",
											ALLOCSET_DEFAULT_SIZES);
		spiRes = SPI_connect();
		if (spiRes != SPI_OK_CONNECT)
		{
			ereport(ERROR,
					(errmsg("SPI_connect failed, connect return:%d",
							spiRes)));
		}
		spiContext = CurrentMemoryContext;
		MemoryContextSwitchTo(switchContext);

		pgConn = GetNodeConn(mgrNode,
							0,
							10,
							spiContext);
		if (pgConn == NULL)
			ereport(ERROR,
					(errmsg("get conn from %s fail.", NameStr(mgrNode->nodename))));

		exist = nodenameExistsInPgxcNode(pgConn,
										NULL,
										true,
										existNodeName,
										getMappedPgxcNodetype(nodeType),
										true);

		ClosePgConn(pgConn);
		(void)MemoryContextSwitchTo(oldContext);
		MemoryContextDelete(switchContext);
		SPI_finish();
	}PG_CATCH();
	{
		ClosePgConn(pgConn);
		(void)MemoryContextSwitchTo(oldContext);
		MemoryContextDelete(switchContext);
		SPI_finish();	
		PG_RE_THROW();
	}PG_END_TRY();

	return exist;
}
void MgrDelPgxcNodeSlaveFromCoord(Form_mgr_node coordMgrNode)
{
	MemoryContext oldContext = NULL;
	MemoryContext switchContext = NULL;
	MemoryContext spiContext = NULL;
	StringInfoData	sql;
	dlist_head 		slaveDataNodes = DLIST_STATIC_INIT(slaveDataNodes);
	MgrNodeWrapper 	*mgrNode;
	dlist_iter 		iter;
	int 			spiRes;
	PGconn 			*pgConn = NULL;

	initStringInfo(&sql);

	PG_TRY();
	{
		oldContext = CurrentMemoryContext;
		switchContext = AllocSetContextCreate(oldContext,
											"MgrDelPgxcNodeSlaveFromCoord",
											ALLOCSET_DEFAULT_SIZES);
		spiRes = SPI_connect();
		if (spiRes != SPI_OK_CONNECT)
		{
			ereport(ERROR,
					(errmsg("SPI_connect failed, connect return:%d",
							spiRes)));
		}
		spiContext = CurrentMemoryContext;
		MemoryContextSwitchTo(switchContext);

		selectMgrNodeByNodetype(spiContext, CNDN_TYPE_DATANODE_SLAVE, &slaveDataNodes);
		pgConn = GetNodeConn(coordMgrNode,
								0,
								10,
								spiContext);
		if (pgConn == NULL)
			ereport(ERROR,
					(errmsg("get conn from %s fail.", NameStr(coordMgrNode->nodename))));

		dlist_foreach(iter, &slaveDataNodes)
		{
			mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);			
			DeletePgxcNodeDataNodeByName(pgConn, NameStr(mgrNode->form.nodename), false);
		}

		ClosePgConn(pgConn);
		(void)MemoryContextSwitchTo(oldContext);
		MemoryContextDelete(switchContext);
		SPI_finish();
	}PG_CATCH();
	{
		ClosePgConn(pgConn);
		(void)MemoryContextSwitchTo(oldContext);
		MemoryContextDelete(switchContext);
		SPI_finish();	
		PG_RE_THROW();
	}PG_END_TRY();
}
static bool DeletePgxcNodeDataNodeByName(PGconn *pgConn, char *nodeName, bool complain)
{
	char *sql;
	bool execOk;

	sql = psprintf("set FORCE_PARALLEL_MODE = off; "
				   "delete from pgxc_node where node_name = '%s';",
				   nodeName);
	execOk = PQexecCommandSql(pgConn, sql, false);
	pfree(sql);
	if (!execOk)
	{
		ereport(complain ? ERROR : LOG,
				(errmsg("%s delete datanode slaves from pgxc_node failed", nodeName)));
	}
	return execOk;
}
void RefreshGtmAdbCheckSyncNextid(MgrNodeWrapper *mgrNode, char *value)
{
	GetAgentCmdRst 		getAgentCmdRst;
	StringInfoData  	infosendmsg;

	CheckNull(mgrNode);

	initStringInfo(&(getAgentCmdRst.description));
	initStringInfo(&infosendmsg);

	mgr_append_pgconf_paras_str_quotastr("adb_check_sync_nextid", value, &infosendmsg);
	mgr_send_conf_parameters(AGT_CMD_CNDN_REFRESH_PGSQLCONF_RELOAD, mgrNode->nodepath, &infosendmsg
							,mgrNode->form.nodehost, &getAgentCmdRst);
	if (!getAgentCmdRst.ret)
		ereport(ERROR, (errmsg("set adb_check_sync_nextid = '%s' in postgresql.conf of %s fail"
			, infosendmsg.data, NameStr(mgrNode->form.nodename))));
}

static void PrintReplicationInfo(SwitcherNodeWrapper *masterNode)
{
	int row=0;
	int i=0;
	PGresult *res = NULL;
	StringInfoData  	infosendmsg;
	char *sql = "select pid,\
				usesysid,\
				usename,\
				application_name,\
				client_addr,\
				client_hostname,\
				client_port,\
				backend_start,\
				backend_xmin,\
				state,\
				sent_lsn,\
				write_lsn,\
				flush_lsn,\
				replay_lsn,\
				write_lag,\
				flush_lag,\
				replay_lag,\
				sync_priority,\
				sync_state \
				from pg_stat_replication;";
	
	CheckNull(masterNode);
	CheckNull(masterNode->pgConn);

	initStringInfo(&infosendmsg);
	res = PQexec(masterNode->pgConn, sql);
	if (PQresultStatus(res) == PGRES_TUPLES_OK)
	{
		row = PQntuples(res);
		for (i=0; i<row; i++)
		{
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "pid", PQgetvalue(res, i, 0)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "usesysid", PQgetvalue(res, i, 1));
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "usename", PQgetvalue(res, i, 2)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "application_name", PQgetvalue(res, i, 3)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "client_addr", PQgetvalue(res, i, 4)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "client_hostname", PQgetvalue(res, i, 5)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "client_port", PQgetvalue(res, i, 6)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "backend_start", PQgetvalue(res, i, 7)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "backend_xmin", PQgetvalue(res, i, 8)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "state", PQgetvalue(res, i, 9)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "sent_lsn", PQgetvalue(res, i, 10)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "write_lsn", PQgetvalue(res, i, 11)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "flush_lsn", PQgetvalue(res, i, 12)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "replay_lsn", PQgetvalue(res, i, 13)); 
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "write_lag", PQgetvalue(res, i, 14));
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "flush_lag", PQgetvalue(res, i, 15));
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "replay_lag", PQgetvalue(res, i, 16));
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s,", "sync_priority", PQgetvalue(res, i, 17));
			infosendmsg.len += snprintf(infosendmsg.data+infosendmsg.len, infosendmsg.maxlen-infosendmsg.len-1, "%s=%s.", "sync_state", PQgetvalue(res, i, 18)); 

			ereport(LOG, (errmsg("the %d/%d of master node \"%s\" in pg_stat_replication: %s", i+1, row, NameStr(masterNode->mgrNode->form.nodename), infosendmsg.data)));
			resetStringInfo(&infosendmsg);
		}
	}
}
static void PrintReplicationInfoOfMasterNode(MemoryContext spiContext,
											char *currentZone,
											char nodeType)
{
	dlist_head 			nodeList = DLIST_STATIC_INIT(nodeList);
	dlist_iter 			iter;
	MgrNodeWrapper 		*mgrNode = NULL;
	char 				*hostaddr = NULL;
	char 				*user = NULL;
	SwitcherNodeWrapper *masterNode = NULL;

	selectNodeNotZoneForFailover(spiContext, currentZone, nodeType, &nodeList);
	dlist_foreach(iter, &nodeList)
	{
		mgrNode = dlist_container(MgrNodeWrapper, link, iter.cur);
		Assert(mgrNode);

		hostaddr = get_hostaddress_from_hostoid(mgrNode->form.nodehost);
		user = get_hostuser_from_hostoid(mgrNode->form.nodehost);
		if (!is_node_running(hostaddr, mgrNode->form.nodeport, user, mgrNode->form.nodetype))
		{
			MgrFree(hostaddr);
			MgrFree(user);
			ereport(LOG, (errmsg("%s \"%s\" is not running, so you can't get the pg_stat_replication information.", 
					mgr_get_nodetype_desc(mgrNode->form.nodetype), NameStr(mgrNode->form.nodename))));			
		}
		else
		{
			masterNode = palloc0(sizeof(SwitcherNodeWrapper));
			masterNode->mgrNode = mgrNode;
			if (tryConnectNode(masterNode, 10))
			{
				PrintReplicationInfo(masterNode);
			}
			pfreeSwitcherNodeWrapperPGconn(masterNode);
			MgrFree(masterNode);
			masterNode = NULL;
		}		
	}
}
void PrintReplicationInfoOfMasterZone(MemoryContext spiContext,
										char *currentZone)
{
	if (check_gtm_is_running(CNDN_TYPE_GTM_COOR_MASTER))
	{
		PrintReplicationInfoOfMasterNode(spiContext, currentZone, CNDN_TYPE_GTM_COOR_MASTER);
		PrintReplicationInfoOfMasterNode(spiContext, currentZone, CNDN_TYPE_COORDINATOR_MASTER);
		PrintReplicationInfoOfMasterNode(spiContext, currentZone, CNDN_TYPE_DATANODE_MASTER);
	}
}
static void PrintCoordReplicationInfo(dlist_head  *coordinators)
{
	dlist_iter iter;
	SwitcherNodeWrapper *coordinator;

	dlist_foreach(iter, coordinators)
	{
		coordinator = dlist_container(SwitcherNodeWrapper, link, iter.cur);
		Assert(coordinator);
		if (coordinator->mgrNode->form.nodetype == CNDN_TYPE_COORDINATOR_MASTER)
		{
			PrintReplicationInfo(coordinator);
		}
	}
}
static void 
RewindCheckLsn(MemoryContext spiContext,
				SwitcherNodeWrapper *masterNode, 
				SwitcherNodeWrapper *slaveNode)
{
	slaveNode->pgConn = getNodeDefaultDBConnection(slaveNode->mgrNode, 10);
	if (slaveNode->pgConn == NULL)
		ereport(ERROR,
					(errmsg("get node %s connection failed",
							NameStr(slaveNode->mgrNode->form.nodename))));

	checkSetMgrNodeGtmInfo(slaveNode->mgrNode, slaveNode->pgConn, spiContext);

	masterNode->pgConn = checkMasterRunningStatus(masterNode->mgrNode);
}
static void 
RewindCheckMasterNodeParamter(MemoryContext spiContext,
								SwitcherNodeWrapper *masterNode,
								bool complain)
{
	MgrNodeWrapper *mgrNode = masterNode->mgrNode;

	if (checkSetRewindNodeParamter(mgrNode, masterNode->pgConn))
	{
		PQfinish(masterNode->pgConn);
		masterNode->pgConn = NULL;
		/* this node my be monitored by other doctor process, don't interfere with it */
		tryUpdateMgrNodeCurestatus(mgrNode,
								   CURE_STATUS_SWITCHING,
								   spiContext);
		shutdownNodeWithinSeconds(mgrNode,
								  SHUTDOWN_NODE_FAST_SECONDS,
								  SHUTDOWN_NODE_IMMEDIATE_SECONDS,
								  complain);
		startupNodeWithinSeconds(mgrNode,
								 STARTUP_NODE_SECONDS,
								 true,
								 complain);
		masterNode->pgConn = checkMasterRunningStatus(mgrNode);
		tryUpdateMgrNodeCurestatus(mgrNode, CURE_STATUS_SWITCHED, spiContext);
	}
}
static void 
RewindCheckSlaveNodeParamter(SwitcherNodeWrapper *slaveNode, bool complain)
{
	/* set slave node postgresql.conf wal_log_hints, full_page_writes if necessary */
	if (checkSetRewindNodeParamter(slaveNode->mgrNode, slaveNode->pgConn))
	{
		PQfinish(slaveNode->pgConn);
		slaveNode->pgConn = NULL;
		shutdownNodeWithinSeconds(slaveNode->mgrNode,
								  SHUTDOWN_NODE_SECONDS_ON_REWIND,
								  0, complain);
		startupNodeWithinSeconds(slaveNode->mgrNode,
								 STARTUP_NODE_SECONDS,
								 true,
								 complain);
	}
}
static void
RewindCheckParamter(MemoryContext spiContext,
					SwitcherNodeWrapper *masterNode, 
					SwitcherNodeWrapper *slaveNode)
{
	RewindCheckLsn(spiContext,
					masterNode, 
					slaveNode);
	RewindCheckMasterNodeParamter(spiContext, 
									masterNode, 
									false);
	RewindCheckSlaveNodeParamter(slaveNode, 
								false);
}
static void 
RewindSlaveToMaster(MemoryContext spiContext,
					SwitcherNodeWrapper *masterNode, 
					SwitcherNodeWrapper *slaveNode)
{
	RewindMgrNodeObject *rewindObject = NULL;

	RewindCheckParamter(spiContext, 
						masterNode, 
						slaveNode);

	rewindObject = palloc0(sizeof(RewindMgrNodeObject));
	rewindObject->slaveNode = slaveNode->mgrNode;
	rewindObject->masterNode = masterNode->mgrNode;
	rewindObject->masterPGconn = masterNode->pgConn;

	rewindMgrNodeOperation(rewindObject, spiContext);
}
static void
RestartCurzoneNodes(dlist_head *dataNodes,
					dlist_head *coordinators, 
					dlist_head *coordinatorSlaves, 
					dlist_head *runningSlaves, 
					bool complain)
{
	restartNodes(dataNodes, complain);
	restartNodes(coordinators, complain);
	restartNodes(coordinatorSlaves, complain);
	restartNodes(runningSlaves, complain);
}
