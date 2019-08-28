
/*
 * NOTES
 *  ******************************
 *  *** DO NOT EDIT THIS FILE! ***
 *  ******************************
 *
 *  It has been GENERATED by src/backend/nodes/gen_nodes.pl
 */

#ifndef BEGIN_ENUM
#	define BEGIN_ENUM(e)
#endif
#ifndef END_ENUM
#	define END_ENUM(e)
#endif
#ifndef ENUM_VALUE
#	define ENUM_VALUE(v)
#endif

#ifndef NO_ENUM_CmdType
BEGIN_ENUM(CmdType)
	ENUM_VALUE(CMD_UNKNOWN)
	ENUM_VALUE(CMD_SELECT)
	ENUM_VALUE(CMD_UPDATE)
	ENUM_VALUE(CMD_INSERT)
	ENUM_VALUE(CMD_DELETE)
	ENUM_VALUE(CMD_UTILITY)
	ENUM_VALUE(CMD_NOTHING)
END_ENUM(CmdType)
#endif /* NO_ENUM_CmdType */

#ifndef NO_ENUM_NodeTag
BEGIN_ENUM(NodeTag)
	ENUM_VALUE(T_IndexInfo)
	ENUM_VALUE(T_ExprContext)
	ENUM_VALUE(T_ProjectionInfo)
	ENUM_VALUE(T_JunkFilter)
	ENUM_VALUE(T_OnConflictSetState)
	ENUM_VALUE(T_ResultRelInfo)
	ENUM_VALUE(T_EState)
	ENUM_VALUE(T_TupleTableSlot)
	ENUM_VALUE(T_Plan)
	ENUM_VALUE(T_Result)
	ENUM_VALUE(T_ProjectSet)
	ENUM_VALUE(T_ModifyTable)
	ENUM_VALUE(T_Append)
	ENUM_VALUE(T_MergeAppend)
	ENUM_VALUE(T_RecursiveUnion)
	ENUM_VALUE(T_BitmapAnd)
	ENUM_VALUE(T_BitmapOr)
	ENUM_VALUE(T_Scan)
	ENUM_VALUE(T_SeqScan)
	ENUM_VALUE(T_SampleScan)
	ENUM_VALUE(T_IndexScan)
	ENUM_VALUE(T_IndexOnlyScan)
	ENUM_VALUE(T_BitmapIndexScan)
	ENUM_VALUE(T_BitmapHeapScan)
	ENUM_VALUE(T_TidScan)
	ENUM_VALUE(T_SubqueryScan)
	ENUM_VALUE(T_FunctionScan)
	ENUM_VALUE(T_ValuesScan)
	ENUM_VALUE(T_TableFuncScan)
	ENUM_VALUE(T_CteScan)
	ENUM_VALUE(T_NamedTuplestoreScan)
	ENUM_VALUE(T_WorkTableScan)
	ENUM_VALUE(T_ForeignScan)
	ENUM_VALUE(T_CustomScan)
	ENUM_VALUE(T_Join)
	ENUM_VALUE(T_NestLoop)
	ENUM_VALUE(T_MergeJoin)
	ENUM_VALUE(T_HashJoin)
	ENUM_VALUE(T_Material)
	ENUM_VALUE(T_Sort)
	ENUM_VALUE(T_Group)
	ENUM_VALUE(T_Agg)
	ENUM_VALUE(T_WindowAgg)
	ENUM_VALUE(T_Unique)
	ENUM_VALUE(T_Gather)
	ENUM_VALUE(T_GatherMerge)
	ENUM_VALUE(T_Hash)
	ENUM_VALUE(T_SetOp)
	ENUM_VALUE(T_LockRows)
	ENUM_VALUE(T_Limit)
#ifdef ADB_EXT
	ENUM_VALUE(T_BatchSort)
#endif
#ifdef ADB_GRAM_ORA
	ENUM_VALUE(T_ConnectByPlan)
#endif
	ENUM_VALUE(T_NestLoopParam)
	ENUM_VALUE(T_PlanRowMark)
	ENUM_VALUE(T_PartitionPruneInfo)
	ENUM_VALUE(T_PartitionedRelPruneInfo)
	ENUM_VALUE(T_PartitionPruneStepOp)
	ENUM_VALUE(T_PartitionPruneStepCombine)
	ENUM_VALUE(T_PlanInvalItem)
#ifdef ADB
	ENUM_VALUE(T_ExecNodes)
	ENUM_VALUE(T_SimpleSort)
	ENUM_VALUE(T_RemoteQuery)
	ENUM_VALUE(T_AlterNodeStmt)
	ENUM_VALUE(T_CreateNodeStmt)
	ENUM_VALUE(T_DropNodeStmt)
	ENUM_VALUE(T_CreateGroupStmt)
	ENUM_VALUE(T_DropGroupStmt)
	ENUM_VALUE(T_CreateAuxStmt)
	ENUM_VALUE(T_PaddingAuxDataStmt)
	ENUM_VALUE(T_ClusterGather)
	ENUM_VALUE(T_ClusterMergeGather)
	ENUM_VALUE(T_ClusterReduce)
	ENUM_VALUE(T_ReduceScan)
	ENUM_VALUE(T_EmptyResult)
	ENUM_VALUE(T_ParamTuplestoreScan)
#endif
	ENUM_VALUE(T_PlanState)
	ENUM_VALUE(T_ResultState)
	ENUM_VALUE(T_ProjectSetState)
	ENUM_VALUE(T_ModifyTableState)
	ENUM_VALUE(T_AppendState)
	ENUM_VALUE(T_MergeAppendState)
	ENUM_VALUE(T_RecursiveUnionState)
	ENUM_VALUE(T_BitmapAndState)
	ENUM_VALUE(T_BitmapOrState)
	ENUM_VALUE(T_ScanState)
	ENUM_VALUE(T_SeqScanState)
	ENUM_VALUE(T_SampleScanState)
	ENUM_VALUE(T_IndexScanState)
	ENUM_VALUE(T_IndexOnlyScanState)
	ENUM_VALUE(T_BitmapIndexScanState)
	ENUM_VALUE(T_BitmapHeapScanState)
	ENUM_VALUE(T_TidScanState)
	ENUM_VALUE(T_SubqueryScanState)
	ENUM_VALUE(T_FunctionScanState)
	ENUM_VALUE(T_TableFuncScanState)
	ENUM_VALUE(T_ValuesScanState)
	ENUM_VALUE(T_CteScanState)
	ENUM_VALUE(T_NamedTuplestoreScanState)
	ENUM_VALUE(T_WorkTableScanState)
	ENUM_VALUE(T_ForeignScanState)
	ENUM_VALUE(T_CustomScanState)
	ENUM_VALUE(T_JoinState)
	ENUM_VALUE(T_NestLoopState)
	ENUM_VALUE(T_MergeJoinState)
	ENUM_VALUE(T_HashJoinState)
	ENUM_VALUE(T_MaterialState)
	ENUM_VALUE(T_SortState)
	ENUM_VALUE(T_GroupState)
	ENUM_VALUE(T_AggState)
	ENUM_VALUE(T_WindowAggState)
	ENUM_VALUE(T_UniqueState)
	ENUM_VALUE(T_GatherState)
	ENUM_VALUE(T_GatherMergeState)
	ENUM_VALUE(T_HashState)
	ENUM_VALUE(T_SetOpState)
	ENUM_VALUE(T_LockRowsState)
	ENUM_VALUE(T_LimitState)
#ifdef ADB_EXT
	ENUM_VALUE(T_BatchSortState)
#endif
#ifdef ADB
	ENUM_VALUE(T_RemoteCopyState)
	ENUM_VALUE(T_RemoteQueryState)
	ENUM_VALUE(T_ClusterGatherState)
	ENUM_VALUE(T_ClusterMergeGatherState)
	ENUM_VALUE(T_ClusterReduceState)
	ENUM_VALUE(T_ReduceScanState)
	ENUM_VALUE(T_EmptyResultState)
	ENUM_VALUE(T_ParamTuplestoreScanState)
#endif
#ifdef ADB_GRAM_ORA
	ENUM_VALUE(T_ConnectByState)
#endif
	ENUM_VALUE(T_Alias)
	ENUM_VALUE(T_RangeVar)
	ENUM_VALUE(T_TableFunc)
	ENUM_VALUE(T_Expr)
	ENUM_VALUE(T_Var)
	ENUM_VALUE(T_Const)
	ENUM_VALUE(T_Param)
	ENUM_VALUE(T_Aggref)
	ENUM_VALUE(T_GroupingFunc)
	ENUM_VALUE(T_WindowFunc)
	ENUM_VALUE(T_ArrayRef)
	ENUM_VALUE(T_FuncExpr)
	ENUM_VALUE(T_NamedArgExpr)
	ENUM_VALUE(T_OpExpr)
	ENUM_VALUE(T_DistinctExpr)
	ENUM_VALUE(T_NullIfExpr)
	ENUM_VALUE(T_ScalarArrayOpExpr)
	ENUM_VALUE(T_BoolExpr)
	ENUM_VALUE(T_SubLink)
	ENUM_VALUE(T_SubPlan)
	ENUM_VALUE(T_AlternativeSubPlan)
	ENUM_VALUE(T_FieldSelect)
	ENUM_VALUE(T_FieldStore)
	ENUM_VALUE(T_RelabelType)
	ENUM_VALUE(T_CoerceViaIO)
	ENUM_VALUE(T_ArrayCoerceExpr)
	ENUM_VALUE(T_ConvertRowtypeExpr)
	ENUM_VALUE(T_CollateExpr)
	ENUM_VALUE(T_CaseExpr)
	ENUM_VALUE(T_CaseWhen)
	ENUM_VALUE(T_CaseTestExpr)
	ENUM_VALUE(T_ArrayExpr)
	ENUM_VALUE(T_RowExpr)
	ENUM_VALUE(T_RowCompareExpr)
	ENUM_VALUE(T_CoalesceExpr)
	ENUM_VALUE(T_MinMaxExpr)
	ENUM_VALUE(T_SQLValueFunction)
	ENUM_VALUE(T_XmlExpr)
	ENUM_VALUE(T_NullTest)
	ENUM_VALUE(T_BooleanTest)
	ENUM_VALUE(T_CoerceToDomain)
	ENUM_VALUE(T_CoerceToDomainValue)
	ENUM_VALUE(T_SetToDefault)
	ENUM_VALUE(T_CurrentOfExpr)
	ENUM_VALUE(T_NextValueExpr)
#ifdef ADB_GRAM_ORA
	ENUM_VALUE(T_RownumExpr)
	ENUM_VALUE(T_LevelExpr)
	ENUM_VALUE(T_SysConnectByPathExpr)
	ENUM_VALUE(T_ConnectByRootExpr)
	ENUM_VALUE(T_OracleConnectBy)
#endif
	ENUM_VALUE(T_InferenceElem)
	ENUM_VALUE(T_TargetEntry)
	ENUM_VALUE(T_RangeTblRef)
	ENUM_VALUE(T_JoinExpr)
	ENUM_VALUE(T_FromExpr)
	ENUM_VALUE(T_OnConflictExpr)
	ENUM_VALUE(T_IntoClause)
#ifdef ADB
	ENUM_VALUE(T_PGXCSubCluster)
#endif
	ENUM_VALUE(T_ExprState)
	ENUM_VALUE(T_AggrefExprState)
	ENUM_VALUE(T_WindowFuncExprState)
	ENUM_VALUE(T_SetExprState)
	ENUM_VALUE(T_SubPlanState)
	ENUM_VALUE(T_AlternativeSubPlanState)
	ENUM_VALUE(T_DomainConstraintState)
	ENUM_VALUE(T_PlannerInfo)
	ENUM_VALUE(T_PlannerGlobal)
	ENUM_VALUE(T_RelOptInfo)
	ENUM_VALUE(T_IndexOptInfo)
	ENUM_VALUE(T_ForeignKeyOptInfo)
	ENUM_VALUE(T_ParamPathInfo)
	ENUM_VALUE(T_Path)
	ENUM_VALUE(T_IndexPath)
	ENUM_VALUE(T_BitmapHeapPath)
	ENUM_VALUE(T_BitmapAndPath)
	ENUM_VALUE(T_BitmapOrPath)
	ENUM_VALUE(T_TidPath)
	ENUM_VALUE(T_SubqueryScanPath)
	ENUM_VALUE(T_ForeignPath)
	ENUM_VALUE(T_CustomPath)
	ENUM_VALUE(T_NestPath)
	ENUM_VALUE(T_MergePath)
	ENUM_VALUE(T_HashPath)
	ENUM_VALUE(T_AppendPath)
	ENUM_VALUE(T_MergeAppendPath)
	ENUM_VALUE(T_ResultPath)
	ENUM_VALUE(T_MaterialPath)
	ENUM_VALUE(T_UniquePath)
	ENUM_VALUE(T_GatherPath)
	ENUM_VALUE(T_GatherMergePath)
	ENUM_VALUE(T_ProjectionPath)
	ENUM_VALUE(T_ProjectSetPath)
	ENUM_VALUE(T_SortPath)
	ENUM_VALUE(T_GroupPath)
	ENUM_VALUE(T_UpperUniquePath)
	ENUM_VALUE(T_AggPath)
	ENUM_VALUE(T_GroupingSetsPath)
	ENUM_VALUE(T_MinMaxAggPath)
	ENUM_VALUE(T_WindowAggPath)
	ENUM_VALUE(T_SetOpPath)
	ENUM_VALUE(T_RecursiveUnionPath)
	ENUM_VALUE(T_LockRowsPath)
	ENUM_VALUE(T_ModifyTablePath)
	ENUM_VALUE(T_LimitPath)
#ifdef ADB_EXT
	ENUM_VALUE(T_BatchSortPath)
#endif
#ifdef ADB
	ENUM_VALUE(T_RemoteQueryPath)
	ENUM_VALUE(T_ClusterGatherPath)
	ENUM_VALUE(T_ClusterMergeGatherPath)
	ENUM_VALUE(T_ClusterReducePath)
	ENUM_VALUE(T_ReduceScanPath)
	ENUM_VALUE(T_FilterPath)
#endif
#ifdef ADB_GRAM_ORA
	ENUM_VALUE(T_ConnectByPath)
#endif
	ENUM_VALUE(T_EquivalenceClass)
	ENUM_VALUE(T_EquivalenceMember)
	ENUM_VALUE(T_PathKey)
	ENUM_VALUE(T_PathTarget)
	ENUM_VALUE(T_RestrictInfo)
	ENUM_VALUE(T_PlaceHolderVar)
	ENUM_VALUE(T_SpecialJoinInfo)
	ENUM_VALUE(T_AppendRelInfo)
	ENUM_VALUE(T_PlaceHolderInfo)
	ENUM_VALUE(T_MinMaxAggInfo)
	ENUM_VALUE(T_PlannerParamItem)
	ENUM_VALUE(T_RollupData)
	ENUM_VALUE(T_GroupingSetData)
	ENUM_VALUE(T_StatisticExtInfo)
	ENUM_VALUE(T_MemoryContext)
	ENUM_VALUE(T_AllocSetContext)
	ENUM_VALUE(T_SlabContext)
	ENUM_VALUE(T_GenerationContext)
	ENUM_VALUE(T_Value)
	ENUM_VALUE(T_Integer)
	ENUM_VALUE(T_Float)
	ENUM_VALUE(T_String)
	ENUM_VALUE(T_BitString)
	ENUM_VALUE(T_Null)
	ENUM_VALUE(T_List)
	ENUM_VALUE(T_IntList)
	ENUM_VALUE(T_OidList)
	ENUM_VALUE(T_ExtensibleNode)
	ENUM_VALUE(T_RawStmt)
	ENUM_VALUE(T_Query)
	ENUM_VALUE(T_PlannedStmt)
	ENUM_VALUE(T_InsertStmt)
	ENUM_VALUE(T_DeleteStmt)
	ENUM_VALUE(T_UpdateStmt)
	ENUM_VALUE(T_SelectStmt)
	ENUM_VALUE(T_AlterTableStmt)
	ENUM_VALUE(T_AlterTableCmd)
	ENUM_VALUE(T_AlterDomainStmt)
	ENUM_VALUE(T_SetOperationStmt)
	ENUM_VALUE(T_GrantStmt)
	ENUM_VALUE(T_GrantRoleStmt)
	ENUM_VALUE(T_AlterDefaultPrivilegesStmt)
	ENUM_VALUE(T_ClosePortalStmt)
	ENUM_VALUE(T_ClusterStmt)
	ENUM_VALUE(T_CopyStmt)
	ENUM_VALUE(T_CreateStmt)
	ENUM_VALUE(T_DefineStmt)
	ENUM_VALUE(T_DropStmt)
	ENUM_VALUE(T_TruncateStmt)
	ENUM_VALUE(T_CommentStmt)
	ENUM_VALUE(T_FetchStmt)
	ENUM_VALUE(T_IndexStmt)
	ENUM_VALUE(T_CreateFunctionStmt)
	ENUM_VALUE(T_AlterFunctionStmt)
	ENUM_VALUE(T_DoStmt)
	ENUM_VALUE(T_RenameStmt)
	ENUM_VALUE(T_RuleStmt)
	ENUM_VALUE(T_NotifyStmt)
	ENUM_VALUE(T_ListenStmt)
	ENUM_VALUE(T_UnlistenStmt)
	ENUM_VALUE(T_TransactionStmt)
	ENUM_VALUE(T_ViewStmt)
	ENUM_VALUE(T_LoadStmt)
	ENUM_VALUE(T_CreateDomainStmt)
	ENUM_VALUE(T_CreatedbStmt)
	ENUM_VALUE(T_DropdbStmt)
	ENUM_VALUE(T_VacuumStmt)
	ENUM_VALUE(T_ExplainStmt)
	ENUM_VALUE(T_CreateTableAsStmt)
	ENUM_VALUE(T_CreateSeqStmt)
	ENUM_VALUE(T_AlterSeqStmt)
	ENUM_VALUE(T_VariableSetStmt)
	ENUM_VALUE(T_VariableShowStmt)
	ENUM_VALUE(T_DiscardStmt)
	ENUM_VALUE(T_CreateTrigStmt)
	ENUM_VALUE(T_CreatePLangStmt)
	ENUM_VALUE(T_CreateRoleStmt)
	ENUM_VALUE(T_AlterRoleStmt)
	ENUM_VALUE(T_DropRoleStmt)
	ENUM_VALUE(T_LockStmt)
	ENUM_VALUE(T_ConstraintsSetStmt)
	ENUM_VALUE(T_ReindexStmt)
	ENUM_VALUE(T_CheckPointStmt)
	ENUM_VALUE(T_CreateSchemaStmt)
	ENUM_VALUE(T_AlterDatabaseStmt)
	ENUM_VALUE(T_AlterDatabaseSetStmt)
	ENUM_VALUE(T_AlterRoleSetStmt)
	ENUM_VALUE(T_CreateConversionStmt)
	ENUM_VALUE(T_CreateCastStmt)
	ENUM_VALUE(T_CreateOpClassStmt)
	ENUM_VALUE(T_CreateOpFamilyStmt)
	ENUM_VALUE(T_AlterOpFamilyStmt)
	ENUM_VALUE(T_PrepareStmt)
	ENUM_VALUE(T_ExecuteStmt)
	ENUM_VALUE(T_DeallocateStmt)
	ENUM_VALUE(T_DeclareCursorStmt)
	ENUM_VALUE(T_CreateTableSpaceStmt)
	ENUM_VALUE(T_DropTableSpaceStmt)
	ENUM_VALUE(T_AlterObjectDependsStmt)
	ENUM_VALUE(T_AlterObjectSchemaStmt)
	ENUM_VALUE(T_AlterOwnerStmt)
	ENUM_VALUE(T_AlterOperatorStmt)
	ENUM_VALUE(T_DropOwnedStmt)
	ENUM_VALUE(T_ReassignOwnedStmt)
	ENUM_VALUE(T_CompositeTypeStmt)
	ENUM_VALUE(T_CreateEnumStmt)
	ENUM_VALUE(T_CreateRangeStmt)
	ENUM_VALUE(T_AlterEnumStmt)
	ENUM_VALUE(T_AlterTSDictionaryStmt)
	ENUM_VALUE(T_AlterTSConfigurationStmt)
	ENUM_VALUE(T_CreateFdwStmt)
	ENUM_VALUE(T_AlterFdwStmt)
	ENUM_VALUE(T_CreateForeignServerStmt)
	ENUM_VALUE(T_AlterForeignServerStmt)
	ENUM_VALUE(T_CreateUserMappingStmt)
	ENUM_VALUE(T_AlterUserMappingStmt)
	ENUM_VALUE(T_DropUserMappingStmt)
#ifdef ADB
	ENUM_VALUE(T_ExecDirectStmt)
	ENUM_VALUE(T_CleanConnStmt)
#endif
	ENUM_VALUE(T_AlterTableSpaceOptionsStmt)
	ENUM_VALUE(T_AlterTableMoveAllStmt)
	ENUM_VALUE(T_SecLabelStmt)
	ENUM_VALUE(T_CreateForeignTableStmt)
	ENUM_VALUE(T_ImportForeignSchemaStmt)
	ENUM_VALUE(T_CreateExtensionStmt)
	ENUM_VALUE(T_AlterExtensionStmt)
	ENUM_VALUE(T_AlterExtensionContentsStmt)
	ENUM_VALUE(T_CreateEventTrigStmt)
	ENUM_VALUE(T_AlterEventTrigStmt)
	ENUM_VALUE(T_RefreshMatViewStmt)
	ENUM_VALUE(T_ReplicaIdentityStmt)
	ENUM_VALUE(T_AlterSystemStmt)
	ENUM_VALUE(T_CreatePolicyStmt)
	ENUM_VALUE(T_AlterPolicyStmt)
	ENUM_VALUE(T_CreateTransformStmt)
	ENUM_VALUE(T_CreateAmStmt)
	ENUM_VALUE(T_CreatePublicationStmt)
	ENUM_VALUE(T_AlterPublicationStmt)
	ENUM_VALUE(T_CreateSubscriptionStmt)
	ENUM_VALUE(T_AlterSubscriptionStmt)
	ENUM_VALUE(T_DropSubscriptionStmt)
	ENUM_VALUE(T_CreateStatsStmt)
	ENUM_VALUE(T_AlterCollationStmt)
	ENUM_VALUE(T_CallStmt)
#ifdef ADB
	ENUM_VALUE(T_BarrierStmt)
#endif
	ENUM_VALUE(T_A_Expr)
	ENUM_VALUE(T_ColumnRef)
#ifdef ADB_GRAM_ORA
	ENUM_VALUE(T_ColumnRefJoin)
	ENUM_VALUE(T_PriorExpr)
	ENUM_VALUE(T_OraImplicitConvertStmt)
#endif
#ifdef ADB_EXT
	ENUM_VALUE(T_KeepClause)
#endif
	ENUM_VALUE(T_ParamRef)
	ENUM_VALUE(T_A_Const)
	ENUM_VALUE(T_FuncCall)
	ENUM_VALUE(T_A_Star)
	ENUM_VALUE(T_A_Indices)
	ENUM_VALUE(T_A_Indirection)
	ENUM_VALUE(T_A_ArrayExpr)
	ENUM_VALUE(T_ResTarget)
	ENUM_VALUE(T_MultiAssignRef)
	ENUM_VALUE(T_TypeCast)
	ENUM_VALUE(T_CollateClause)
	ENUM_VALUE(T_SortBy)
	ENUM_VALUE(T_WindowDef)
	ENUM_VALUE(T_RangeSubselect)
	ENUM_VALUE(T_RangeFunction)
	ENUM_VALUE(T_RangeTableSample)
	ENUM_VALUE(T_RangeTableFunc)
	ENUM_VALUE(T_RangeTableFuncCol)
	ENUM_VALUE(T_TypeName)
	ENUM_VALUE(T_ColumnDef)
	ENUM_VALUE(T_IndexElem)
	ENUM_VALUE(T_Constraint)
	ENUM_VALUE(T_DefElem)
	ENUM_VALUE(T_RangeTblEntry)
	ENUM_VALUE(T_RangeTblFunction)
	ENUM_VALUE(T_TableSampleClause)
	ENUM_VALUE(T_WithCheckOption)
	ENUM_VALUE(T_SortGroupClause)
	ENUM_VALUE(T_GroupingSet)
	ENUM_VALUE(T_WindowClause)
	ENUM_VALUE(T_ObjectWithArgs)
	ENUM_VALUE(T_AccessPriv)
	ENUM_VALUE(T_CreateOpClassItem)
	ENUM_VALUE(T_TableLikeClause)
	ENUM_VALUE(T_FunctionParameter)
	ENUM_VALUE(T_LockingClause)
	ENUM_VALUE(T_RowMarkClause)
	ENUM_VALUE(T_XmlSerialize)
	ENUM_VALUE(T_WithClause)
	ENUM_VALUE(T_InferClause)
	ENUM_VALUE(T_OnConflictClause)
	ENUM_VALUE(T_CommonTableExpr)
	ENUM_VALUE(T_RoleSpec)
	ENUM_VALUE(T_TriggerTransition)
	ENUM_VALUE(T_PartitionElem)
	ENUM_VALUE(T_PartitionSpec)
	ENUM_VALUE(T_PartitionBoundSpec)
	ENUM_VALUE(T_PartitionRangeDatum)
	ENUM_VALUE(T_PartitionCmd)
	ENUM_VALUE(T_VacuumRelation)
	ENUM_VALUE(T_IdentifySystemCmd)
	ENUM_VALUE(T_BaseBackupCmd)
	ENUM_VALUE(T_CreateReplicationSlotCmd)
	ENUM_VALUE(T_DropReplicationSlotCmd)
	ENUM_VALUE(T_StartReplicationCmd)
	ENUM_VALUE(T_TimeLineHistoryCmd)
	ENUM_VALUE(T_SQLCmd)
	ENUM_VALUE(T_TriggerData)
	ENUM_VALUE(T_EventTriggerData)
	ENUM_VALUE(T_ReturnSetInfo)
	ENUM_VALUE(T_WindowObjectData)
	ENUM_VALUE(T_TIDBitmap)
	ENUM_VALUE(T_InlineCodeBlock)
	ENUM_VALUE(T_FdwRoutine)
	ENUM_VALUE(T_IndexAmRoutine)
	ENUM_VALUE(T_TsmRoutine)
	ENUM_VALUE(T_ForeignKeyCacheInfo)
	ENUM_VALUE(T_CallContext)
#ifdef ADBMGRD
	ENUM_VALUE(T_MGRAddHost)
	ENUM_VALUE(T_MGRDropHost)
	ENUM_VALUE(T_MGRAlterHost)
	ENUM_VALUE(T_MGRAddNode)
	ENUM_VALUE(T_MGRAlterNode)
	ENUM_VALUE(T_MGRDropNode)
	ENUM_VALUE(T_MGRUpdateparm)
	ENUM_VALUE(T_MGRUpdateparmReset)
	ENUM_VALUE(T_MGRStartAgent)
	ENUM_VALUE(T_MGRFlushHost)
	ENUM_VALUE(T_MGRDoctorSet)
	ENUM_VALUE(T_MonitorJobitemAdd)
	ENUM_VALUE(T_MonitorJobitemAlter)
	ENUM_VALUE(T_MonitorJobitemDrop)
	ENUM_VALUE(T_MonitorJobAdd)
	ENUM_VALUE(T_MonitorJobAlter)
	ENUM_VALUE(T_MonitorJobDrop)
	ENUM_VALUE(T_MgrExtensionAdd)
	ENUM_VALUE(T_MgrExtensionDrop)
	ENUM_VALUE(T_MgrRemoveNode)
	ENUM_VALUE(T_MGRSetClusterInit)
	ENUM_VALUE(T_MonitorDeleteData)
	ENUM_VALUE(T_ClusterSlotInitStmt)
	ENUM_VALUE(T_MGRFlushParam)
	ENUM_VALUE(T_MGRFlushReadonlySlave)
#endif
#ifdef ADB
	ENUM_VALUE(T_CreateSlotStmt)
	ENUM_VALUE(T_AlterSlotStmt)
	ENUM_VALUE(T_DropSlotStmt)
	ENUM_VALUE(T_FlushSlotStmt)
	ENUM_VALUE(T_CleanSlotStmt)
#endif
END_ENUM(NodeTag)
#endif /* NO_ENUM_NodeTag */

#ifndef NO_ENUM_JoinType
BEGIN_ENUM(JoinType)
	ENUM_VALUE(JOIN_INNER)
	ENUM_VALUE(JOIN_LEFT)
	ENUM_VALUE(JOIN_FULL)
	ENUM_VALUE(JOIN_RIGHT)
	ENUM_VALUE(JOIN_SEMI)
	ENUM_VALUE(JOIN_ANTI)
	ENUM_VALUE(JOIN_UNIQUE_OUTER)
	ENUM_VALUE(JOIN_UNIQUE_INNER)
END_ENUM(JoinType)
#endif /* NO_ENUM_JoinType */

#ifndef NO_ENUM_AggStrategy
BEGIN_ENUM(AggStrategy)
	ENUM_VALUE(AGG_PLAIN)
	ENUM_VALUE(AGG_SORTED)
	ENUM_VALUE(AGG_HASHED)
	ENUM_VALUE(AGG_MIXED)
#ifdef ADB_EXT
	ENUM_VALUE(AGG_BATCH_HASH)
#endif
END_ENUM(AggStrategy)
#endif /* NO_ENUM_AggStrategy */

#ifndef NO_ENUM_AggSplit
BEGIN_ENUM(AggSplit)
	ENUM_VALUE(AGGSPLIT_SIMPLE)
	ENUM_VALUE(AGGSPLIT_INITIAL_SERIAL)
	ENUM_VALUE(AGGSPLIT_FINAL_DESERIAL)
END_ENUM(AggSplit)
#endif /* NO_ENUM_AggSplit */

#ifndef NO_ENUM_SetOpCmd
BEGIN_ENUM(SetOpCmd)
	ENUM_VALUE(SETOPCMD_INTERSECT)
	ENUM_VALUE(SETOPCMD_INTERSECT_ALL)
	ENUM_VALUE(SETOPCMD_EXCEPT)
	ENUM_VALUE(SETOPCMD_EXCEPT_ALL)
END_ENUM(SetOpCmd)
#endif /* NO_ENUM_SetOpCmd */

#ifndef NO_ENUM_SetOpStrategy
BEGIN_ENUM(SetOpStrategy)
	ENUM_VALUE(SETOP_SORTED)
	ENUM_VALUE(SETOP_HASHED)
END_ENUM(SetOpStrategy)
#endif /* NO_ENUM_SetOpStrategy */

#ifndef NO_ENUM_OnConflictAction
BEGIN_ENUM(OnConflictAction)
	ENUM_VALUE(ONCONFLICT_NONE)
	ENUM_VALUE(ONCONFLICT_NOTHING)
	ENUM_VALUE(ONCONFLICT_UPDATE)
END_ENUM(OnConflictAction)
#endif /* NO_ENUM_OnConflictAction */

#ifndef NO_ENUM_OnCommitAction
BEGIN_ENUM(OnCommitAction)
	ENUM_VALUE(ONCOMMIT_NOOP)
	ENUM_VALUE(ONCOMMIT_PRESERVE_ROWS)
	ENUM_VALUE(ONCOMMIT_DELETE_ROWS)
	ENUM_VALUE(ONCOMMIT_DROP)
END_ENUM(OnCommitAction)
#endif /* NO_ENUM_OnCommitAction */

#ifndef NO_ENUM_ParamKind
BEGIN_ENUM(ParamKind)
	ENUM_VALUE(PARAM_EXTERN)
	ENUM_VALUE(PARAM_EXEC)
	ENUM_VALUE(PARAM_SUBLINK)
	ENUM_VALUE(PARAM_MULTIEXPR)
END_ENUM(ParamKind)
#endif /* NO_ENUM_ParamKind */

#ifndef NO_ENUM_CoercionContext
BEGIN_ENUM(CoercionContext)
	ENUM_VALUE(COERCION_IMPLICIT)
	ENUM_VALUE(COERCION_ASSIGNMENT)
	ENUM_VALUE(COERCION_EXPLICIT)
END_ENUM(CoercionContext)
#endif /* NO_ENUM_CoercionContext */

#ifndef NO_ENUM_CoercionForm
BEGIN_ENUM(CoercionForm)
	ENUM_VALUE(COERCE_EXPLICIT_CALL)
	ENUM_VALUE(COERCE_EXPLICIT_CAST)
	ENUM_VALUE(COERCE_IMPLICIT_CAST)
END_ENUM(CoercionForm)
#endif /* NO_ENUM_CoercionForm */

#ifndef NO_ENUM_BoolExprType
BEGIN_ENUM(BoolExprType)
	ENUM_VALUE(AND_EXPR)
	ENUM_VALUE(OR_EXPR)
	ENUM_VALUE(NOT_EXPR)
END_ENUM(BoolExprType)
#endif /* NO_ENUM_BoolExprType */

#ifndef NO_ENUM_SubLinkType
BEGIN_ENUM(SubLinkType)
	ENUM_VALUE(EXISTS_SUBLINK)
	ENUM_VALUE(ALL_SUBLINK)
	ENUM_VALUE(ANY_SUBLINK)
	ENUM_VALUE(ROWCOMPARE_SUBLINK)
	ENUM_VALUE(EXPR_SUBLINK)
	ENUM_VALUE(MULTIEXPR_SUBLINK)
	ENUM_VALUE(ARRAY_SUBLINK)
	ENUM_VALUE(CTE_SUBLINK)
END_ENUM(SubLinkType)
#endif /* NO_ENUM_SubLinkType */

#ifndef NO_ENUM_RowCompareType
BEGIN_ENUM(RowCompareType)
	ENUM_VALUE(ROWCOMPARE_LT)
	ENUM_VALUE(ROWCOMPARE_LE)
	ENUM_VALUE(ROWCOMPARE_EQ)
	ENUM_VALUE(ROWCOMPARE_GE)
	ENUM_VALUE(ROWCOMPARE_GT)
	ENUM_VALUE(ROWCOMPARE_NE)
END_ENUM(RowCompareType)
#endif /* NO_ENUM_RowCompareType */

#ifndef NO_ENUM_MinMaxOp
BEGIN_ENUM(MinMaxOp)
	ENUM_VALUE(IS_GREATEST)
	ENUM_VALUE(IS_LEAST)
END_ENUM(MinMaxOp)
#endif /* NO_ENUM_MinMaxOp */

#ifndef NO_ENUM_SQLValueFunctionOp
BEGIN_ENUM(SQLValueFunctionOp)
	ENUM_VALUE(SVFOP_CURRENT_DATE)
	ENUM_VALUE(SVFOP_CURRENT_TIME)
	ENUM_VALUE(SVFOP_CURRENT_TIME_N)
	ENUM_VALUE(SVFOP_CURRENT_TIMESTAMP)
	ENUM_VALUE(SVFOP_CURRENT_TIMESTAMP_N)
	ENUM_VALUE(SVFOP_LOCALTIME)
	ENUM_VALUE(SVFOP_LOCALTIME_N)
	ENUM_VALUE(SVFOP_LOCALTIMESTAMP)
	ENUM_VALUE(SVFOP_LOCALTIMESTAMP_N)
	ENUM_VALUE(SVFOP_CURRENT_ROLE)
	ENUM_VALUE(SVFOP_CURRENT_USER)
	ENUM_VALUE(SVFOP_USER)
	ENUM_VALUE(SVFOP_SESSION_USER)
	ENUM_VALUE(SVFOP_CURRENT_CATALOG)
	ENUM_VALUE(SVFOP_CURRENT_SCHEMA)
END_ENUM(SQLValueFunctionOp)
#endif /* NO_ENUM_SQLValueFunctionOp */

#ifndef NO_ENUM_XmlExprOp
BEGIN_ENUM(XmlExprOp)
	ENUM_VALUE(IS_XMLCONCAT)
	ENUM_VALUE(IS_XMLELEMENT)
	ENUM_VALUE(IS_XMLFOREST)
	ENUM_VALUE(IS_XMLPARSE)
	ENUM_VALUE(IS_XMLPI)
	ENUM_VALUE(IS_XMLROOT)
	ENUM_VALUE(IS_XMLSERIALIZE)
	ENUM_VALUE(IS_DOCUMENT)
END_ENUM(XmlExprOp)
#endif /* NO_ENUM_XmlExprOp */

#ifndef NO_ENUM_XmlOptionType
BEGIN_ENUM(XmlOptionType)
	ENUM_VALUE(XMLOPTION_DOCUMENT)
	ENUM_VALUE(XMLOPTION_CONTENT)
END_ENUM(XmlOptionType)
#endif /* NO_ENUM_XmlOptionType */

#ifndef NO_ENUM_NullTestType
BEGIN_ENUM(NullTestType)
	ENUM_VALUE(IS_NULL)
	ENUM_VALUE(IS_NOT_NULL)
END_ENUM(NullTestType)
#endif /* NO_ENUM_NullTestType */

#ifndef NO_ENUM_BoolTestType
BEGIN_ENUM(BoolTestType)
	ENUM_VALUE(IS_TRUE)
	ENUM_VALUE(IS_NOT_TRUE)
	ENUM_VALUE(IS_FALSE)
	ENUM_VALUE(IS_NOT_FALSE)
	ENUM_VALUE(IS_UNKNOWN)
	ENUM_VALUE(IS_NOT_UNKNOWN)
END_ENUM(BoolTestType)
#endif /* NO_ENUM_BoolTestType */

#if defined(ADB)
#ifndef NO_ENUM_PGXCSubClusterType
BEGIN_ENUM(PGXCSubClusterType)
	ENUM_VALUE(SUBCLUSTER_NONE)
	ENUM_VALUE(SUBCLUSTER_NODE)
	ENUM_VALUE(SUBCLUSTER_GROUP)
END_ENUM(PGXCSubClusterType)
#endif /* NO_ENUM_PGXCSubClusterType */
#endif

#ifndef NO_ENUM_OverridingKind
BEGIN_ENUM(OverridingKind)
	ENUM_VALUE(OVERRIDING_NOT_SET)
	ENUM_VALUE(OVERRIDING_USER_VALUE)
	ENUM_VALUE(OVERRIDING_SYSTEM_VALUE)
END_ENUM(OverridingKind)
#endif /* NO_ENUM_OverridingKind */

#ifndef NO_ENUM_QuerySource
BEGIN_ENUM(QuerySource)
	ENUM_VALUE(QSRC_ORIGINAL)
	ENUM_VALUE(QSRC_PARSER)
	ENUM_VALUE(QSRC_INSTEAD_RULE)
	ENUM_VALUE(QSRC_QUAL_INSTEAD_RULE)
	ENUM_VALUE(QSRC_NON_INSTEAD_RULE)
END_ENUM(QuerySource)
#endif /* NO_ENUM_QuerySource */

#ifndef NO_ENUM_SortByDir
BEGIN_ENUM(SortByDir)
	ENUM_VALUE(SORTBY_DEFAULT)
	ENUM_VALUE(SORTBY_ASC)
	ENUM_VALUE(SORTBY_DESC)
	ENUM_VALUE(SORTBY_USING)
END_ENUM(SortByDir)
#endif /* NO_ENUM_SortByDir */

#ifndef NO_ENUM_SortByNulls
BEGIN_ENUM(SortByNulls)
	ENUM_VALUE(SORTBY_NULLS_DEFAULT)
	ENUM_VALUE(SORTBY_NULLS_FIRST)
	ENUM_VALUE(SORTBY_NULLS_LAST)
END_ENUM(SortByNulls)
#endif /* NO_ENUM_SortByNulls */

#if defined(ADB_MULTI_GRAM)
#ifndef NO_ENUM_ParseGrammar
BEGIN_ENUM(ParseGrammar)
	ENUM_VALUE(PARSE_GRAM_POSTGRES)
	ENUM_VALUE(PARSE_GRAM_ORACLE)
	ENUM_VALUE(PARSE_GRAM_DB2)
END_ENUM(ParseGrammar)
#endif /* NO_ENUM_ParseGrammar */
#endif

#if defined(ADBMGRD)
#ifndef NO_ENUM_CommandMode
BEGIN_ENUM(CommandMode)
	ENUM_VALUE(CMD_MODE_SQL)
	ENUM_VALUE(CMD_MODE_MGR)
END_ENUM(CommandMode)
#endif /* NO_ENUM_CommandMode */
#endif

#ifndef NO_ENUM_A_Expr_Kind
BEGIN_ENUM(A_Expr_Kind)
	ENUM_VALUE(AEXPR_OP)
	ENUM_VALUE(AEXPR_OP_ANY)
	ENUM_VALUE(AEXPR_OP_ALL)
	ENUM_VALUE(AEXPR_DISTINCT)
	ENUM_VALUE(AEXPR_NOT_DISTINCT)
	ENUM_VALUE(AEXPR_NULLIF)
	ENUM_VALUE(AEXPR_OF)
	ENUM_VALUE(AEXPR_IN)
	ENUM_VALUE(AEXPR_LIKE)
	ENUM_VALUE(AEXPR_ILIKE)
	ENUM_VALUE(AEXPR_SIMILAR)
	ENUM_VALUE(AEXPR_BETWEEN)
	ENUM_VALUE(AEXPR_NOT_BETWEEN)
	ENUM_VALUE(AEXPR_BETWEEN_SYM)
	ENUM_VALUE(AEXPR_NOT_BETWEEN_SYM)
	ENUM_VALUE(AEXPR_PAREN)
END_ENUM(A_Expr_Kind)
#endif /* NO_ENUM_A_Expr_Kind */

#ifndef NO_ENUM_RoleSpecType
BEGIN_ENUM(RoleSpecType)
	ENUM_VALUE(ROLESPEC_CSTRING)
	ENUM_VALUE(ROLESPEC_CURRENT_USER)
	ENUM_VALUE(ROLESPEC_SESSION_USER)
	ENUM_VALUE(ROLESPEC_PUBLIC)
END_ENUM(RoleSpecType)
#endif /* NO_ENUM_RoleSpecType */

#ifndef NO_ENUM_TableLikeOption
BEGIN_ENUM(TableLikeOption)
	ENUM_VALUE(CREATE_TABLE_LIKE_COMMENTS)
	ENUM_VALUE(CREATE_TABLE_LIKE_CONSTRAINTS)
	ENUM_VALUE(CREATE_TABLE_LIKE_DEFAULTS)
	ENUM_VALUE(CREATE_TABLE_LIKE_IDENTITY)
	ENUM_VALUE(CREATE_TABLE_LIKE_INDEXES)
	ENUM_VALUE(CREATE_TABLE_LIKE_STATISTICS)
	ENUM_VALUE(CREATE_TABLE_LIKE_STORAGE)
	ENUM_VALUE(CREATE_TABLE_LIKE_ALL)
END_ENUM(TableLikeOption)
#endif /* NO_ENUM_TableLikeOption */

#ifndef NO_ENUM_DefElemAction
BEGIN_ENUM(DefElemAction)
	ENUM_VALUE(DEFELEM_UNSPEC)
	ENUM_VALUE(DEFELEM_SET)
	ENUM_VALUE(DEFELEM_ADD)
	ENUM_VALUE(DEFELEM_DROP)
END_ENUM(DefElemAction)
#endif /* NO_ENUM_DefElemAction */

#ifndef NO_ENUM_PartitionRangeDatumKind
BEGIN_ENUM(PartitionRangeDatumKind)
	ENUM_VALUE(PARTITION_RANGE_DATUM_MINVALUE)
	ENUM_VALUE(PARTITION_RANGE_DATUM_VALUE)
	ENUM_VALUE(PARTITION_RANGE_DATUM_MAXVALUE)
END_ENUM(PartitionRangeDatumKind)
#endif /* NO_ENUM_PartitionRangeDatumKind */

#ifndef NO_ENUM_RTEKind
BEGIN_ENUM(RTEKind)
	ENUM_VALUE(RTE_RELATION)
	ENUM_VALUE(RTE_SUBQUERY)
	ENUM_VALUE(RTE_JOIN)
	ENUM_VALUE(RTE_FUNCTION)
	ENUM_VALUE(RTE_TABLEFUNC)
	ENUM_VALUE(RTE_VALUES)
	ENUM_VALUE(RTE_CTE)
	ENUM_VALUE(RTE_NAMEDTUPLESTORE)
#ifdef ADB
	ENUM_VALUE(RTE_PARAMTS)
	ENUM_VALUE(RTE_REMOTE_DUMMY)
#endif
END_ENUM(RTEKind)
#endif /* NO_ENUM_RTEKind */

#ifndef NO_ENUM_WCOKind
BEGIN_ENUM(WCOKind)
	ENUM_VALUE(WCO_VIEW_CHECK)
	ENUM_VALUE(WCO_RLS_INSERT_CHECK)
	ENUM_VALUE(WCO_RLS_UPDATE_CHECK)
	ENUM_VALUE(WCO_RLS_CONFLICT_CHECK)
END_ENUM(WCOKind)
#endif /* NO_ENUM_WCOKind */

#ifndef NO_ENUM_GroupingSetKind
BEGIN_ENUM(GroupingSetKind)
	ENUM_VALUE(GROUPING_SET_EMPTY)
	ENUM_VALUE(GROUPING_SET_SIMPLE)
	ENUM_VALUE(GROUPING_SET_ROLLUP)
	ENUM_VALUE(GROUPING_SET_CUBE)
	ENUM_VALUE(GROUPING_SET_SETS)
END_ENUM(GroupingSetKind)
#endif /* NO_ENUM_GroupingSetKind */

#ifndef NO_ENUM_SetOperation
BEGIN_ENUM(SetOperation)
	ENUM_VALUE(SETOP_NONE)
	ENUM_VALUE(SETOP_UNION)
	ENUM_VALUE(SETOP_INTERSECT)
	ENUM_VALUE(SETOP_EXCEPT)
END_ENUM(SetOperation)
#endif /* NO_ENUM_SetOperation */

#ifndef NO_ENUM_ObjectType
BEGIN_ENUM(ObjectType)
	ENUM_VALUE(OBJECT_ACCESS_METHOD)
	ENUM_VALUE(OBJECT_AGGREGATE)
	ENUM_VALUE(OBJECT_AMOP)
	ENUM_VALUE(OBJECT_AMPROC)
	ENUM_VALUE(OBJECT_ATTRIBUTE)
	ENUM_VALUE(OBJECT_CAST)
	ENUM_VALUE(OBJECT_COLUMN)
	ENUM_VALUE(OBJECT_COLLATION)
	ENUM_VALUE(OBJECT_CONVERSION)
	ENUM_VALUE(OBJECT_DATABASE)
	ENUM_VALUE(OBJECT_DEFAULT)
	ENUM_VALUE(OBJECT_DEFACL)
	ENUM_VALUE(OBJECT_DOMAIN)
	ENUM_VALUE(OBJECT_DOMCONSTRAINT)
	ENUM_VALUE(OBJECT_EVENT_TRIGGER)
	ENUM_VALUE(OBJECT_EXTENSION)
	ENUM_VALUE(OBJECT_FDW)
	ENUM_VALUE(OBJECT_FOREIGN_SERVER)
	ENUM_VALUE(OBJECT_FOREIGN_TABLE)
	ENUM_VALUE(OBJECT_FUNCTION)
	ENUM_VALUE(OBJECT_INDEX)
	ENUM_VALUE(OBJECT_LANGUAGE)
	ENUM_VALUE(OBJECT_LARGEOBJECT)
	ENUM_VALUE(OBJECT_MATVIEW)
	ENUM_VALUE(OBJECT_OPCLASS)
	ENUM_VALUE(OBJECT_OPERATOR)
	ENUM_VALUE(OBJECT_OPFAMILY)
	ENUM_VALUE(OBJECT_POLICY)
	ENUM_VALUE(OBJECT_PROCEDURE)
	ENUM_VALUE(OBJECT_PUBLICATION)
	ENUM_VALUE(OBJECT_PUBLICATION_REL)
	ENUM_VALUE(OBJECT_ROLE)
	ENUM_VALUE(OBJECT_ROUTINE)
	ENUM_VALUE(OBJECT_RULE)
	ENUM_VALUE(OBJECT_SCHEMA)
	ENUM_VALUE(OBJECT_SEQUENCE)
	ENUM_VALUE(OBJECT_SUBSCRIPTION)
	ENUM_VALUE(OBJECT_STATISTIC_EXT)
	ENUM_VALUE(OBJECT_TABCONSTRAINT)
	ENUM_VALUE(OBJECT_TABLE)
#ifdef ADB
	ENUM_VALUE(OBJECT_AUX_TABLE)
#endif
	ENUM_VALUE(OBJECT_TABLESPACE)
	ENUM_VALUE(OBJECT_TRANSFORM)
	ENUM_VALUE(OBJECT_TRIGGER)
	ENUM_VALUE(OBJECT_TSCONFIGURATION)
	ENUM_VALUE(OBJECT_TSDICTIONARY)
	ENUM_VALUE(OBJECT_TSPARSER)
	ENUM_VALUE(OBJECT_TSTEMPLATE)
	ENUM_VALUE(OBJECT_TYPE)
	ENUM_VALUE(OBJECT_USER_MAPPING)
	ENUM_VALUE(OBJECT_VIEW)
END_ENUM(ObjectType)
#endif /* NO_ENUM_ObjectType */

#ifndef NO_ENUM_DropBehavior
BEGIN_ENUM(DropBehavior)
	ENUM_VALUE(DROP_RESTRICT)
	ENUM_VALUE(DROP_CASCADE)
END_ENUM(DropBehavior)
#endif /* NO_ENUM_DropBehavior */

#ifndef NO_ENUM_AlterTableType
BEGIN_ENUM(AlterTableType)
	ENUM_VALUE(AT_AddColumn)
	ENUM_VALUE(AT_AddColumnRecurse)
	ENUM_VALUE(AT_AddColumnToView)
	ENUM_VALUE(AT_ColumnDefault)
	ENUM_VALUE(AT_DropNotNull)
	ENUM_VALUE(AT_SetNotNull)
	ENUM_VALUE(AT_SetStatistics)
	ENUM_VALUE(AT_SetOptions)
	ENUM_VALUE(AT_ResetOptions)
	ENUM_VALUE(AT_SetStorage)
	ENUM_VALUE(AT_DropColumn)
	ENUM_VALUE(AT_DropColumnRecurse)
	ENUM_VALUE(AT_AddIndex)
	ENUM_VALUE(AT_ReAddIndex)
	ENUM_VALUE(AT_AddConstraint)
	ENUM_VALUE(AT_AddConstraintRecurse)
	ENUM_VALUE(AT_ReAddConstraint)
	ENUM_VALUE(AT_ReAddDomainConstraint)
	ENUM_VALUE(AT_AlterConstraint)
	ENUM_VALUE(AT_ValidateConstraint)
	ENUM_VALUE(AT_ValidateConstraintRecurse)
	ENUM_VALUE(AT_ProcessedConstraint)
	ENUM_VALUE(AT_AddIndexConstraint)
	ENUM_VALUE(AT_DropConstraint)
	ENUM_VALUE(AT_DropConstraintRecurse)
	ENUM_VALUE(AT_ReAddComment)
	ENUM_VALUE(AT_AlterColumnType)
	ENUM_VALUE(AT_AlterColumnGenericOptions)
	ENUM_VALUE(AT_ChangeOwner)
	ENUM_VALUE(AT_ClusterOn)
	ENUM_VALUE(AT_DropCluster)
	ENUM_VALUE(AT_SetLogged)
	ENUM_VALUE(AT_SetUnLogged)
	ENUM_VALUE(AT_AddOids)
	ENUM_VALUE(AT_AddOidsRecurse)
	ENUM_VALUE(AT_DropOids)
	ENUM_VALUE(AT_SetTableSpace)
	ENUM_VALUE(AT_SetRelOptions)
	ENUM_VALUE(AT_ResetRelOptions)
	ENUM_VALUE(AT_ReplaceRelOptions)
	ENUM_VALUE(AT_EnableTrig)
	ENUM_VALUE(AT_EnableAlwaysTrig)
	ENUM_VALUE(AT_EnableReplicaTrig)
	ENUM_VALUE(AT_DisableTrig)
	ENUM_VALUE(AT_EnableTrigAll)
	ENUM_VALUE(AT_DisableTrigAll)
	ENUM_VALUE(AT_EnableTrigUser)
	ENUM_VALUE(AT_DisableTrigUser)
	ENUM_VALUE(AT_EnableRule)
	ENUM_VALUE(AT_EnableAlwaysRule)
	ENUM_VALUE(AT_EnableReplicaRule)
	ENUM_VALUE(AT_DisableRule)
	ENUM_VALUE(AT_AddInherit)
	ENUM_VALUE(AT_DropInherit)
	ENUM_VALUE(AT_AddOf)
	ENUM_VALUE(AT_DropOf)
	ENUM_VALUE(AT_ReplicaIdentity)
	ENUM_VALUE(AT_EnableRowSecurity)
	ENUM_VALUE(AT_DisableRowSecurity)
	ENUM_VALUE(AT_ForceRowSecurity)
	ENUM_VALUE(AT_NoForceRowSecurity)
	ENUM_VALUE(AT_GenericOptions)
	ENUM_VALUE(AT_AttachPartition)
	ENUM_VALUE(AT_DetachPartition)
	ENUM_VALUE(AT_AddIdentity)
	ENUM_VALUE(AT_SetIdentity)
	ENUM_VALUE(AT_DropIdentity)
#ifdef ADB
	ENUM_VALUE(AT_DistributeBy)
	ENUM_VALUE(AT_SubCluster)
	ENUM_VALUE(AT_AddNodeList)
	ENUM_VALUE(AT_DeleteNodeList)
#endif
END_ENUM(AlterTableType)
#endif /* NO_ENUM_AlterTableType */

#ifndef NO_ENUM_GrantTargetType
BEGIN_ENUM(GrantTargetType)
	ENUM_VALUE(ACL_TARGET_OBJECT)
	ENUM_VALUE(ACL_TARGET_ALL_IN_SCHEMA)
	ENUM_VALUE(ACL_TARGET_DEFAULTS)
END_ENUM(GrantTargetType)
#endif /* NO_ENUM_GrantTargetType */

#ifndef NO_ENUM_VariableSetKind
BEGIN_ENUM(VariableSetKind)
	ENUM_VALUE(VAR_SET_VALUE)
	ENUM_VALUE(VAR_SET_DEFAULT)
	ENUM_VALUE(VAR_SET_CURRENT)
	ENUM_VALUE(VAR_SET_MULTI)
	ENUM_VALUE(VAR_RESET)
	ENUM_VALUE(VAR_RESET_ALL)
END_ENUM(VariableSetKind)
#endif /* NO_ENUM_VariableSetKind */

#ifndef NO_ENUM_ConstrType
BEGIN_ENUM(ConstrType)
	ENUM_VALUE(CONSTR_NULL)
	ENUM_VALUE(CONSTR_NOTNULL)
	ENUM_VALUE(CONSTR_DEFAULT)
	ENUM_VALUE(CONSTR_IDENTITY)
	ENUM_VALUE(CONSTR_CHECK)
	ENUM_VALUE(CONSTR_PRIMARY)
	ENUM_VALUE(CONSTR_UNIQUE)
	ENUM_VALUE(CONSTR_EXCLUSION)
	ENUM_VALUE(CONSTR_FOREIGN)
	ENUM_VALUE(CONSTR_ATTR_DEFERRABLE)
	ENUM_VALUE(CONSTR_ATTR_NOT_DEFERRABLE)
	ENUM_VALUE(CONSTR_ATTR_DEFERRED)
	ENUM_VALUE(CONSTR_ATTR_IMMEDIATE)
END_ENUM(ConstrType)
#endif /* NO_ENUM_ConstrType */

#ifndef NO_ENUM_ImportForeignSchemaType
BEGIN_ENUM(ImportForeignSchemaType)
	ENUM_VALUE(FDW_IMPORT_SCHEMA_ALL)
	ENUM_VALUE(FDW_IMPORT_SCHEMA_LIMIT_TO)
	ENUM_VALUE(FDW_IMPORT_SCHEMA_EXCEPT)
END_ENUM(ImportForeignSchemaType)
#endif /* NO_ENUM_ImportForeignSchemaType */

#ifndef NO_ENUM_RoleStmtType
BEGIN_ENUM(RoleStmtType)
	ENUM_VALUE(ROLESTMT_ROLE)
	ENUM_VALUE(ROLESTMT_USER)
	ENUM_VALUE(ROLESTMT_GROUP)
END_ENUM(RoleStmtType)
#endif /* NO_ENUM_RoleStmtType */

#ifndef NO_ENUM_FetchDirection
BEGIN_ENUM(FetchDirection)
	ENUM_VALUE(FETCH_FORWARD)
	ENUM_VALUE(FETCH_BACKWARD)
	ENUM_VALUE(FETCH_ABSOLUTE)
	ENUM_VALUE(FETCH_RELATIVE)
END_ENUM(FetchDirection)
#endif /* NO_ENUM_FetchDirection */

#ifndef NO_ENUM_FunctionParameterMode
BEGIN_ENUM(FunctionParameterMode)
	ENUM_VALUE(FUNC_PARAM_IN)
	ENUM_VALUE(FUNC_PARAM_OUT)
	ENUM_VALUE(FUNC_PARAM_INOUT)
	ENUM_VALUE(FUNC_PARAM_VARIADIC)
	ENUM_VALUE(FUNC_PARAM_TABLE)
END_ENUM(FunctionParameterMode)
#endif /* NO_ENUM_FunctionParameterMode */

#ifndef NO_ENUM_TransactionStmtKind
BEGIN_ENUM(TransactionStmtKind)
	ENUM_VALUE(TRANS_STMT_BEGIN)
	ENUM_VALUE(TRANS_STMT_START)
	ENUM_VALUE(TRANS_STMT_COMMIT)
	ENUM_VALUE(TRANS_STMT_ROLLBACK)
	ENUM_VALUE(TRANS_STMT_SAVEPOINT)
	ENUM_VALUE(TRANS_STMT_RELEASE)
	ENUM_VALUE(TRANS_STMT_ROLLBACK_TO)
	ENUM_VALUE(TRANS_STMT_PREPARE)
	ENUM_VALUE(TRANS_STMT_COMMIT_PREPARED)
	ENUM_VALUE(TRANS_STMT_ROLLBACK_PREPARED)
END_ENUM(TransactionStmtKind)
#endif /* NO_ENUM_TransactionStmtKind */

#ifndef NO_ENUM_ViewCheckOption
BEGIN_ENUM(ViewCheckOption)
	ENUM_VALUE(NO_CHECK_OPTION)
	ENUM_VALUE(LOCAL_CHECK_OPTION)
	ENUM_VALUE(CASCADED_CHECK_OPTION)
END_ENUM(ViewCheckOption)
#endif /* NO_ENUM_ViewCheckOption */

#ifndef NO_ENUM_VacuumOption
BEGIN_ENUM(VacuumOption)
	ENUM_VALUE(VACOPT_VACUUM)
	ENUM_VALUE(VACOPT_ANALYZE)
	ENUM_VALUE(VACOPT_VERBOSE)
	ENUM_VALUE(VACOPT_FREEZE)
	ENUM_VALUE(VACOPT_FULL)
	ENUM_VALUE(VACOPT_NOWAIT)
	ENUM_VALUE(VACOPT_SKIPTOAST)
	ENUM_VALUE(VACOPT_DISABLE_PAGE_SKIPPING)
#ifdef ADB
	ENUM_VALUE(VACOPT_IN_CLUSTER)
	ENUM_VALUE(VACOPT_ANALYZE_FORCE_INH)
#endif
END_ENUM(VacuumOption)
#endif /* NO_ENUM_VacuumOption */

#ifndef NO_ENUM_DiscardMode
BEGIN_ENUM(DiscardMode)
	ENUM_VALUE(DISCARD_ALL)
	ENUM_VALUE(DISCARD_PLANS)
	ENUM_VALUE(DISCARD_SEQUENCES)
	ENUM_VALUE(DISCARD_TEMP)
END_ENUM(DiscardMode)
#endif /* NO_ENUM_DiscardMode */

#ifndef NO_ENUM_ReindexObjectType
BEGIN_ENUM(ReindexObjectType)
	ENUM_VALUE(REINDEX_OBJECT_INDEX)
	ENUM_VALUE(REINDEX_OBJECT_TABLE)
	ENUM_VALUE(REINDEX_OBJECT_SCHEMA)
	ENUM_VALUE(REINDEX_OBJECT_SYSTEM)
	ENUM_VALUE(REINDEX_OBJECT_DATABASE)
END_ENUM(ReindexObjectType)
#endif /* NO_ENUM_ReindexObjectType */

#ifndef NO_ENUM_AlterTSConfigType
BEGIN_ENUM(AlterTSConfigType)
	ENUM_VALUE(ALTER_TSCONFIG_ADD_MAPPING)
	ENUM_VALUE(ALTER_TSCONFIG_ALTER_MAPPING_FOR_TOKEN)
	ENUM_VALUE(ALTER_TSCONFIG_REPLACE_DICT)
	ENUM_VALUE(ALTER_TSCONFIG_REPLACE_DICT_FOR_TOKEN)
	ENUM_VALUE(ALTER_TSCONFIG_DROP_MAPPING)
END_ENUM(AlterTSConfigType)
#endif /* NO_ENUM_AlterTSConfigType */

#ifndef NO_ENUM_AlterSubscriptionType
BEGIN_ENUM(AlterSubscriptionType)
	ENUM_VALUE(ALTER_SUBSCRIPTION_OPTIONS)
	ENUM_VALUE(ALTER_SUBSCRIPTION_CONNECTION)
	ENUM_VALUE(ALTER_SUBSCRIPTION_PUBLICATION)
	ENUM_VALUE(ALTER_SUBSCRIPTION_REFRESH)
	ENUM_VALUE(ALTER_SUBSCRIPTION_ENABLED)
END_ENUM(AlterSubscriptionType)
#endif /* NO_ENUM_AlterSubscriptionType */

#if defined(ADB_GRAM_ORA)
#ifndef NO_ENUM_IConvertAction
BEGIN_ENUM(IConvertAction)
	ENUM_VALUE(ICONVERT_CREATE)
	ENUM_VALUE(ICONVERT_UPDATE)
	ENUM_VALUE(ICONVERT_DELETE)
END_ENUM(IConvertAction)
#endif /* NO_ENUM_IConvertAction */
#endif

#if defined(ADB)
#ifndef NO_ENUM_ClusterGatherType
BEGIN_ENUM(ClusterGatherType)
	ENUM_VALUE(CLUSTER_GATHER_COORD)
	ENUM_VALUE(CLUSTER_GATHER_DATANODE)
	ENUM_VALUE(CLUSTER_GATHER_ALL)
END_ENUM(ClusterGatherType)
#endif /* NO_ENUM_ClusterGatherType */
#endif

#ifndef NO_ENUM_RowMarkType
BEGIN_ENUM(RowMarkType)
	ENUM_VALUE(ROW_MARK_EXCLUSIVE)
	ENUM_VALUE(ROW_MARK_NOKEYEXCLUSIVE)
	ENUM_VALUE(ROW_MARK_SHARE)
	ENUM_VALUE(ROW_MARK_KEYSHARE)
	ENUM_VALUE(ROW_MARK_REFERENCE)
	ENUM_VALUE(ROW_MARK_COPY)
END_ENUM(RowMarkType)
#endif /* NO_ENUM_RowMarkType */

#ifndef NO_ENUM_PartitionPruneCombineOp
BEGIN_ENUM(PartitionPruneCombineOp)
	ENUM_VALUE(PARTPRUNE_COMBINE_UNION)
	ENUM_VALUE(PARTPRUNE_COMBINE_INTERSECT)
END_ENUM(PartitionPruneCombineOp)
#endif /* NO_ENUM_PartitionPruneCombineOp */

#ifndef NO_ENUM_CostSelector
BEGIN_ENUM(CostSelector)
	ENUM_VALUE(STARTUP_COST)
	ENUM_VALUE(TOTAL_COST)
END_ENUM(CostSelector)
#endif /* NO_ENUM_CostSelector */

#ifndef NO_ENUM_UpperRelationKind
BEGIN_ENUM(UpperRelationKind)
	ENUM_VALUE(UPPERREL_SETOP)
	ENUM_VALUE(UPPERREL_PARTIAL_GROUP_AGG)
	ENUM_VALUE(UPPERREL_GROUP_AGG)
	ENUM_VALUE(UPPERREL_WINDOW)
	ENUM_VALUE(UPPERREL_DISTINCT)
	ENUM_VALUE(UPPERREL_ORDERED)
#ifdef ADB_GRAM_ORA
	ENUM_VALUE(UPPERREL_CONNECT_BY)
#endif
	ENUM_VALUE(UPPERREL_FINAL)
END_ENUM(UpperRelationKind)
#endif /* NO_ENUM_UpperRelationKind */

#ifndef NO_ENUM_InheritanceKind
BEGIN_ENUM(InheritanceKind)
	ENUM_VALUE(INHKIND_NONE)
	ENUM_VALUE(INHKIND_INHERITED)
	ENUM_VALUE(INHKIND_PARTITIONED)
END_ENUM(InheritanceKind)
#endif /* NO_ENUM_InheritanceKind */

#ifndef NO_ENUM_RelOptKind
BEGIN_ENUM(RelOptKind)
	ENUM_VALUE(RELOPT_BASEREL)
	ENUM_VALUE(RELOPT_JOINREL)
	ENUM_VALUE(RELOPT_OTHER_MEMBER_REL)
	ENUM_VALUE(RELOPT_OTHER_JOINREL)
	ENUM_VALUE(RELOPT_UPPER_REL)
	ENUM_VALUE(RELOPT_OTHER_UPPER_REL)
	ENUM_VALUE(RELOPT_DEADREL)
END_ENUM(RelOptKind)
#endif /* NO_ENUM_RelOptKind */

#ifndef NO_ENUM_UniquePathMethod
BEGIN_ENUM(UniquePathMethod)
	ENUM_VALUE(UNIQUE_PATH_NOOP)
	ENUM_VALUE(UNIQUE_PATH_HASH)
	ENUM_VALUE(UNIQUE_PATH_SORT)
END_ENUM(UniquePathMethod)
#endif /* NO_ENUM_UniquePathMethod */

#ifndef NO_ENUM_PartitionwiseAggregateType
BEGIN_ENUM(PartitionwiseAggregateType)
	ENUM_VALUE(PARTITIONWISE_AGGREGATE_NONE)
	ENUM_VALUE(PARTITIONWISE_AGGREGATE_FULL)
	ENUM_VALUE(PARTITIONWISE_AGGREGATE_PARTIAL)
END_ENUM(PartitionwiseAggregateType)
#endif /* NO_ENUM_PartitionwiseAggregateType */

#ifndef NO_ENUM_ExprDoneCond
BEGIN_ENUM(ExprDoneCond)
	ENUM_VALUE(ExprSingleResult)
	ENUM_VALUE(ExprMultipleResult)
	ENUM_VALUE(ExprEndResult)
END_ENUM(ExprDoneCond)
#endif /* NO_ENUM_ExprDoneCond */

#ifndef NO_ENUM_SetFunctionReturnMode
BEGIN_ENUM(SetFunctionReturnMode)
	ENUM_VALUE(SFRM_ValuePerCall)
	ENUM_VALUE(SFRM_Materialize)
	ENUM_VALUE(SFRM_Materialize_Random)
	ENUM_VALUE(SFRM_Materialize_Preferred)
END_ENUM(SetFunctionReturnMode)
#endif /* NO_ENUM_SetFunctionReturnMode */

#ifndef NO_ENUM_DomainConstraintType
BEGIN_ENUM(DomainConstraintType)
	ENUM_VALUE(DOM_CONSTRAINT_NOTNULL)
	ENUM_VALUE(DOM_CONSTRAINT_CHECK)
END_ENUM(DomainConstraintType)
#endif /* NO_ENUM_DomainConstraintType */

#ifndef NO_ENUM_SharedBitmapState
BEGIN_ENUM(SharedBitmapState)
	ENUM_VALUE(BM_INITIAL)
	ENUM_VALUE(BM_INPROGRESS)
	ENUM_VALUE(BM_FINISHED)
END_ENUM(SharedBitmapState)
#endif /* NO_ENUM_SharedBitmapState */

#ifndef NO_ENUM_LimitStateCond
BEGIN_ENUM(LimitStateCond)
	ENUM_VALUE(LIMIT_INITIAL)
	ENUM_VALUE(LIMIT_RESCAN)
	ENUM_VALUE(LIMIT_EMPTY)
	ENUM_VALUE(LIMIT_INWINDOW)
	ENUM_VALUE(LIMIT_SUBPLANEOF)
	ENUM_VALUE(LIMIT_WINDOWEND)
	ENUM_VALUE(LIMIT_WINDOWSTART)
END_ENUM(LimitStateCond)
#endif /* NO_ENUM_LimitStateCond */

#ifndef NO_ENUM_ReplicationKind
BEGIN_ENUM(ReplicationKind)
	ENUM_VALUE(REPLICATION_KIND_PHYSICAL)
	ENUM_VALUE(REPLICATION_KIND_LOGICAL)
END_ENUM(ReplicationKind)
#endif /* NO_ENUM_ReplicationKind */

#ifndef NO_ENUM_LockClauseStrength
BEGIN_ENUM(LockClauseStrength)
	ENUM_VALUE(LCS_NONE)
	ENUM_VALUE(LCS_FORKEYSHARE)
	ENUM_VALUE(LCS_FORSHARE)
	ENUM_VALUE(LCS_FORNOKEYUPDATE)
	ENUM_VALUE(LCS_FORUPDATE)
END_ENUM(LockClauseStrength)
#endif /* NO_ENUM_LockClauseStrength */

#ifndef NO_ENUM_LockWaitPolicy
BEGIN_ENUM(LockWaitPolicy)
	ENUM_VALUE(LockWaitBlock)
	ENUM_VALUE(LockWaitSkip)
	ENUM_VALUE(LockWaitError)
END_ENUM(LockWaitPolicy)
#endif /* NO_ENUM_LockWaitPolicy */

#ifndef NO_ENUM_ScanDirection
BEGIN_ENUM(ScanDirection)
	ENUM_VALUE(BackwardScanDirection)
	ENUM_VALUE(NoMovementScanDirection)
	ENUM_VALUE(ForwardScanDirection)
END_ENUM(ScanDirection)
#endif /* NO_ENUM_ScanDirection */

#if defined(ADB)
#ifndef NO_ENUM_RelationAccessType
BEGIN_ENUM(RelationAccessType)
	ENUM_VALUE(RELATION_ACCESS_READ)
	ENUM_VALUE(RELATION_ACCESS_READ_FOR_UPDATE)
	ENUM_VALUE(RELATION_ACCESS_UPDATE)
	ENUM_VALUE(RELATION_ACCESS_INSERT)
END_ENUM(RelationAccessType)
#endif /* NO_ENUM_RelationAccessType */
#endif

#if defined(ADB)
#ifndef NO_ENUM_CombineType
BEGIN_ENUM(CombineType)
	ENUM_VALUE(COMBINE_TYPE_NONE)
	ENUM_VALUE(COMBINE_TYPE_SUM)
	ENUM_VALUE(COMBINE_TYPE_SAME)
END_ENUM(CombineType)
#endif /* NO_ENUM_CombineType */
#endif

#if defined(ADB)
#ifndef NO_ENUM_RemoteQueryExecType
BEGIN_ENUM(RemoteQueryExecType)
	ENUM_VALUE(EXEC_ON_DATANODES)
	ENUM_VALUE(EXEC_ON_COORDS)
	ENUM_VALUE(EXEC_ON_ALL_NODES)
	ENUM_VALUE(EXEC_ON_NONE)
END_ENUM(RemoteQueryExecType)
#endif /* NO_ENUM_RemoteQueryExecType */
#endif

#if defined(ADB)
#ifndef NO_ENUM_ExecDirectType
BEGIN_ENUM(ExecDirectType)
	ENUM_VALUE(EXEC_DIRECT_NONE)
	ENUM_VALUE(EXEC_DIRECT_LOCAL)
	ENUM_VALUE(EXEC_DIRECT_LOCAL_UTILITY)
	ENUM_VALUE(EXEC_DIRECT_UTILITY)
	ENUM_VALUE(EXEC_DIRECT_SELECT)
	ENUM_VALUE(EXEC_DIRECT_INSERT)
	ENUM_VALUE(EXEC_DIRECT_UPDATE)
	ENUM_VALUE(EXEC_DIRECT_DELETE)
END_ENUM(ExecDirectType)
#endif /* NO_ENUM_ExecDirectType */
#endif

#ifndef NO_ENUM_ForceParallelMode
BEGIN_ENUM(ForceParallelMode)
	ENUM_VALUE(FORCE_PARALLEL_OFF)
	ENUM_VALUE(FORCE_PARALLEL_ON)
	ENUM_VALUE(FORCE_PARALLEL_REGRESS)
END_ENUM(ForceParallelMode)
#endif /* NO_ENUM_ForceParallelMode */
