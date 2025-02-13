#define EW_MODULE_FORWARDS

// Purpose: Announcement of all forwards

Handle	g_hOnBanForward,
		g_hOnUnbanForward,
		g_hOnUseItem,
		g_hOnPickUpItem,
		g_hOnDropItem,
		g_hOnPlayerDisconnectWithItem,
		g_hOnPlayerDeathWithItem,
		g_hOnCfgLoading,
		g_hOnCfgNotFound,
		g_hOnSchemeServerName,
		g_hOnSchemeNotFound,
		g_hOnDatabaseFailure,
		g_hOnDatabaseSuccess,
		g_hOnDatabaseUnknownDriver,
		g_hOnDatabaseReady,
		g_hOnDatabaseCreateTablesError,
		g_hOnDatabaseBanError,
		g_hOnDatabaseBanSuccess,
		g_hOnDatabaseUnBanError,
		g_hOnDatabaseUnBanSuccess,
		g_hOnDatabaseBanUpdateError,
		g_hOnDatabaseOfflineUnBanError,
		g_hOnOfflineBanForward,
		g_hOnOfflineBanError,
		g_hOnOfflineBanSuccess,
		g_hOnAdminSpawnItem,
		g_hOnAdminTransferedAllItems,
		g_hOnAdminTransferedItem;

stock void EWM_Forwards_OnPluginStart()
{
	g_hOnBanForward = CreateGlobalForward("EntWatch_OnClientBanned", ET_Ignore, Param_Cell, Param_Cell, Param_Cell, Param_String);
	g_hOnUnbanForward = CreateGlobalForward("EntWatch_OnClientUnbanned", ET_Ignore, Param_Cell, Param_Cell, Param_String);
	g_hOnUseItem = CreateGlobalForward("EntWatch_OnUseItem", ET_Ignore, Param_String, Param_Cell, Param_Cell);
	g_hOnPickUpItem = CreateGlobalForward("EntWatch_OnPickUpItem", ET_Ignore, Param_String, Param_Cell);
	g_hOnDropItem = CreateGlobalForward("EntWatch_OnDropItem", ET_Ignore, Param_String, Param_Cell);
	g_hOnPlayerDisconnectWithItem = CreateGlobalForward("EntWatch_OnPlayerDisconnectWithItem", ET_Ignore, Param_String, Param_Cell);
	g_hOnPlayerDeathWithItem = CreateGlobalForward("EntWatch_OnPlayerDeathWithItem", ET_Ignore, Param_String, Param_Cell);
	g_hOnCfgLoading = CreateGlobalForward("EntWatch_OnCfgLoading", ET_Ignore, Param_String);
	g_hOnCfgNotFound = CreateGlobalForward("EntWatch_OnCfgNotFound", ET_Ignore, Param_String);
	g_hOnSchemeServerName = CreateGlobalForward("EntWatch_OnSchemeServerName", ET_Ignore, Param_String);
	g_hOnSchemeNotFound = CreateGlobalForward("EntWatch_OnSchemeNotFound", ET_Ignore, Param_String);
	g_hOnDatabaseFailure = CreateGlobalForward("EntWatch_OnDatabaseFailure", ET_Ignore, Param_String);
	g_hOnDatabaseSuccess = CreateGlobalForward("EntWatch_OnDatabaseSuccess", ET_Ignore);
	g_hOnDatabaseUnknownDriver = CreateGlobalForward("EntWatch_OnDatabaseUnknownDriver", ET_Ignore, Param_String);
	g_hOnDatabaseReady = CreateGlobalForward("EntWatch_OnDatabaseReady", ET_Ignore);
	g_hOnDatabaseCreateTablesError = CreateGlobalForward("EntWatch_OnDatabaseCreateTablesError", ET_Ignore, Param_String);
	g_hOnDatabaseBanError = CreateGlobalForward("EntWatch_OnDatabaseBanError", ET_Ignore, Param_String);
	g_hOnDatabaseBanSuccess = CreateGlobalForward("EntWatch_OnDatabaseBanSuccess", ET_Ignore);
	g_hOnDatabaseUnBanError = CreateGlobalForward("EntWatch_OnDatabaseUnBanError", ET_Ignore, Param_String);
	g_hOnDatabaseUnBanSuccess = CreateGlobalForward("EntWatch_OnDatabaseUnBanSuccess", ET_Ignore);
	g_hOnDatabaseBanUpdateError = CreateGlobalForward("EntWatch_OnDatabaseBanUpdateError", ET_Ignore, Param_String);
	g_hOnDatabaseOfflineUnBanError = CreateGlobalForward("EntWatch_OnDatabaseOfflineUnBanError", ET_Ignore, Param_String);
	g_hOnOfflineBanForward = CreateGlobalForward("EntWatch_OnClientOfflineBanned", ET_Ignore, Param_Cell, Param_Cell, Param_String, Param_String, Param_String);
	g_hOnOfflineBanError = CreateGlobalForward("EntWatch_OnOfflineBanError", ET_Ignore, Param_String);
	g_hOnOfflineBanSuccess = CreateGlobalForward("EntWatch_OnOfflineBanSuccess", ET_Ignore);
	g_hOnAdminSpawnItem = CreateGlobalForward("EntWatch_OnAdminSpawnItem", ET_Ignore, Param_Cell, Param_String, Param_Cell);
	g_hOnAdminTransferedAllItems = CreateGlobalForward("EntWatch_OnAdminTransferedAllItems", ET_Ignore, Param_Cell, Param_Cell, Param_Cell);
	g_hOnAdminTransferedItem = CreateGlobalForward("EntWatch_OnAdminTransferedItem", ET_Ignore, Param_Cell, Param_String, Param_Cell);
}

stock void Forward_OnUseItem(char sItemName[32], int iActivator, int iAbility) {
	Call_StartForward(g_hOnUseItem);
	Call_PushString(sItemName);
	Call_PushCell(iActivator);
	Call_PushCell(iAbility);
	Call_Finish();
}