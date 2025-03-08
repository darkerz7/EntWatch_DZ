#define EW_MODULE_EBAN

#define EW_EBAN_ENTWATCH_SECTION "EntWatch"
#define EW_EBAN_DB_ENTWATCH_CHARSET "utf8mb4"
#define EW_EBAN_DB_ENTWATCH_COLLATION "utf8mb4_unicode_ci"

// Module settings
ConVar	g_hCvar_Default_BanTime,
		g_hCvar_Default_BanReason,
		g_hCvar_Admin_BanLong,
		g_hCvar_Default_UnBanReason,
		g_hCvar_Keep_Expired_Bans,
		g_hCvar_Use_Reason_Menu,
		g_hCvar_MaxBanTime,
		g_hCvar_DropOnEBan,
		g_hCvar_EbanInvalidSteamID;

// Module Variables
class_ClientSettings_EBan g_EbanClients[MAXPLAYERS+1];

bool 	g_bKeepExpiredBans,
		g_bUseReasonMenu,
		g_bDropItemOnEBan,
		g_bEbanInvalidSteamID;

Database g_hEbanDB;
int g_iDBStatus = 0, // 0 - Def., 1 - Reconnect, 2 - Unknown Driver, 3 - Create Table, 4 - Ready to Query
	g_iDefaultBanTime,
	g_iAdminBanLong,
	g_iMaxBanTime;

char g_sDefaultBanReason[64],
	g_sDefaultUnBanReason[64];

stock void EWM_Eban_OnPluginStart()
{
	g_hCvar_Default_BanTime			= CreateConVar("entwatch_bantime", "0", "Default ban time (0-43200)", _, true, 0.0, true, 43200.0);
	g_hCvar_Default_BanReason		= CreateConVar("entwatch_banreason", "Item misuse", "Default ban reason. (64 characters MAX)", _);
	g_hCvar_Admin_BanLong			= CreateConVar("entwatch_ban_long", "720", "Maximum time in minutes that an admin is able to eban.", _, true, 1.0);
	g_hCvar_Default_UnBanReason		= CreateConVar("entwatch_unbanreason", "Giving another chance", "Default Unban reason. (64 characters MAX)", _);
	g_hCvar_Keep_Expired_Bans		= CreateConVar("entwatch_keep_expired_ban", "1", "Enable/Disable keep expired bans.", _, true, 0.0, true, 1.0);
	g_hCvar_Use_Reason_Menu			= CreateConVar("entwatch_use_reason_menu", "0", "Enable/Disable menu if the admin has not indicated a reason", _, true, 0.0, true, 1.0);
	g_hCvar_MaxBanTime				= CreateConVar("entwatch_max_bantime", "0", "Maximum ban time allowed via console command (0 = disabled)", _, true, 0.0);
	g_hCvar_DropOnEBan				= CreateConVar("entwatch_ban_dropitem", "1", "Automatically drop the item if client is ebanned", _, true, 0.0, true, 1.0);
	g_hCvar_EbanInvalidSteamID		= CreateConVar("entwatch_eban_invalid_steamid", "1", "Eban temporary invalid steamid", _, true, 0.0, true, 1.0);

	//Hook CVARs
	HookConVarChange(g_hCvar_Default_BanTime, Cvar_EBAN_Changed);
	HookConVarChange(g_hCvar_Admin_BanLong, Cvar_EBAN_Changed);
	HookConVarChange(g_hCvar_Keep_Expired_Bans, Cvar_EBAN_Changed);
	HookConVarChange(g_hCvar_Use_Reason_Menu, Cvar_EBAN_Changed);
	HookConVarChange(g_hCvar_MaxBanTime, Cvar_EBAN_Changed);
	HookConVarChange(g_hCvar_DropOnEBan, Cvar_EBAN_Changed);
	HookConVarChange(g_hCvar_Default_BanReason, Cvar_EBAN_Changed);
	HookConVarChange(g_hCvar_Default_UnBanReason, Cvar_EBAN_Changed);
	HookConVarChange(g_hCvar_EbanInvalidSteamID, Cvar_EBAN_Changed);

	//Initialize values
	g_iDefaultBanTime = GetConVarInt(g_hCvar_Default_BanTime);
	g_iAdminBanLong = GetConVarInt(g_hCvar_Admin_BanLong);
	g_bKeepExpiredBans = GetConVarBool(g_hCvar_Keep_Expired_Bans);
	g_bUseReasonMenu = GetConVarBool(g_hCvar_Use_Reason_Menu);
	g_iMaxBanTime = GetConVarInt(g_hCvar_MaxBanTime);
	g_bDropItemOnEBan = GetConVarBool(g_hCvar_DropOnEBan);
	g_bEbanInvalidSteamID = GetConVarBool(g_hCvar_EbanInvalidSteamID);
	GetConVarString(g_hCvar_Default_BanReason, g_sDefaultBanReason, sizeof(g_sDefaultBanReason));
	GetConVarString(g_hCvar_Default_UnBanReason, g_sDefaultUnBanReason, sizeof(g_sDefaultUnBanReason));

	RegConsoleCmd("sm_status", EWM_Eban_Command_Status);

	RegAdminCmd("sm_eban", EWM_Eban_Command_Restrict, ADMFLAG_BAN);
	RegAdminCmd("sm_eunban", EWM_Eban_Command_Unrestrict, ADMFLAG_BAN);
	RegAdminCmd("sm_ebanlist", EWM_Eban_Command_EBanlist, ADMFLAG_BAN);

	Database.Connect(EWM_Eban_ConnectCallBack, EW_EBAN_ENTWATCH_SECTION);
	//Timer Checker
	CreateTimer(30.0, EWM_Eban_Timer_Checker, _, TIMER_REPEAT);
}

stock bool IsDBConnected()
{
	return g_hEbanDB != null && g_iDBStatus == 4;
}

stock void EWM_Eban_OnClientPostAdminCheck(int iClient)
{
	if(g_iDBStatus != 4)
		return;

	char sTQuery[1024];
	FormatEx(sTQuery, sizeof(sTQuery),
		"SELECT SUM(cnt) AS total_count " ...
		"FROM ( " ...
		"SELECT COUNT(*) AS cnt " ...
		"FROM `EntWatch_Current_Eban` " ...
		"WHERE `client_steamid` = '%s' " ...
		"UNION " ...
		"SELECT COUNT(*) AS cnt " ...
		"FROM `EntWatch_Old_Eban` " ...
		"WHERE `client_steamid` = '%s' " ...
		") " ...
		"AS counts", g_sSteamIDs[iClient], g_sSteamIDs[iClient]);
	SQL_TQuery(g_hEbanDB, EWM_Eban_SQLGetEbansNumber_Current, sTQuery, g_iUserIDs[iClient]);
}

void Cvar_EBAN_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_Default_BanTime)
		g_iDefaultBanTime = GetConVarInt(convar);
	else if(convar==g_hCvar_Admin_BanLong)
		g_iAdminBanLong = GetConVarInt(convar);
	else if(convar==g_hCvar_Keep_Expired_Bans)
		g_bKeepExpiredBans = GetConVarBool(convar);
	else if(convar==g_hCvar_Use_Reason_Menu)
		g_bUseReasonMenu = GetConVarBool(convar);
	else if(convar==g_hCvar_MaxBanTime)
		g_iMaxBanTime = GetConVarInt(convar);
	else if(convar==g_hCvar_DropOnEBan)
		g_bDropItemOnEBan = GetConVarBool(convar);
	else if(convar==g_hCvar_Default_BanReason)
		GetConVarString(convar, g_sDefaultBanReason, sizeof(g_sDefaultBanReason));
	else if(convar==g_hCvar_Default_UnBanReason)
		GetConVarString(convar, g_sDefaultUnBanReason, sizeof(g_sDefaultUnBanReason));
	else if(convar==g_hCvar_EbanInvalidSteamID)
		g_bEbanInvalidSteamID = GetConVarBool(convar);
}

void EWM_Eban_ConnectCallBack(Database hDatabase, const char[] sError, any data)
{
	if (hDatabase == null)	// Fail Connect
	{
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnDatabaseFailure);
		Call_PushString(sError);
		Call_Finish();
		#endif
		g_iDBStatus = 1; //ReConnect
		return;
	}
	g_hEbanDB = hDatabase;
	#if defined EW_MODULE_FORWARDS
	Call_StartForward(g_hOnDatabaseSuccess);
	Call_Finish();
	#endif
	EWM_Eban_CreateTables(); // Create Tables
	g_hEbanDB.SetCharset(EW_EBAN_DB_ENTWATCH_CHARSET); // Set Charset UTF8
}

void EWM_Eban_CreateTables()
{
	char sConnectDriverDB[16];
	g_hEbanDB.Driver.GetIdentifier(sConnectDriverDB, sizeof(sConnectDriverDB));
	if(strcmp(sConnectDriverDB, "mysql") == 0)
	{
		g_iDBStatus = 3;
		//Create MySQL Tables
		char sSQL_Query[1024];
		Transaction T_CreateTables = SQL_CreateTransaction();
		FormatEx(sSQL_Query, sizeof(sSQL_Query), "CREATE TABLE IF NOT EXISTS `EntWatch_Current_Eban`(	`id` int(10) unsigned NOT NULL auto_increment, \
																										`client_name` varchar(32) NOT NULL, \
																										`client_steamid` varchar(64) NOT NULL, \
																										`admin_name` varchar(32) NOT NULL, \
																										`admin_steamid` varchar(64) NOT NULL, \
																										`server` varchar(64), \
																										`duration` int unsigned NOT NULL, \
																										`timestamp_issued` int NOT NULL, \
																										`reason` varchar(64), \
																										`reason_unban` varchar(64), \
																										`admin_name_unban` varchar(32), \
																										`admin_steamid_unban` varchar(64), \
																										`timestamp_unban` int, \
																										PRIMARY KEY (id)) CHARACTER SET %s COLLATE %s;", EW_EBAN_DB_ENTWATCH_CHARSET, EW_EBAN_DB_ENTWATCH_COLLATION);
		T_CreateTables.AddQuery(sSQL_Query);
		FormatEx(sSQL_Query, sizeof(sSQL_Query), "CREATE TABLE IF NOT EXISTS `EntWatch_Old_Eban`(	`id` int(10) unsigned NOT NULL auto_increment, \
																									`client_name` varchar(32) NOT NULL, \
																									`client_steamid` varchar(64) NOT NULL, \
																									`admin_name` varchar(32) NOT NULL, \
																									`admin_steamid` varchar(64) NOT NULL, \
																									`server` varchar(64), \
																									`duration` int unsigned NOT NULL, \
																									`timestamp_issued` int NOT NULL, \
																									`reason` varchar(64), \
																									`reason_unban` varchar(64), \
																									`admin_name_unban` varchar(32), \
																									`admin_steamid_unban` varchar(64), \
																									`timestamp_unban` int, \
																									PRIMARY KEY (id)) CHARACTER SET %s COLLATE %s;", EW_EBAN_DB_ENTWATCH_CHARSET, EW_EBAN_DB_ENTWATCH_COLLATION);
		T_CreateTables.AddQuery(sSQL_Query);
		SQL_ExecuteTransaction(g_hEbanDB, T_CreateTables, EWM_Eban_SQLCreateTables_Success, EWM_Eban_SQLCreateTables_Error, _, DBPrio_High);
	} else if(strcmp(sConnectDriverDB, "sqlite") == 0)
	{
		g_iDBStatus = 3;
		//Create SQLite Tables
		char sSQL_Query[1024];
		Transaction T_CreateTables = SQL_CreateTransaction();
		FormatEx(sSQL_Query, sizeof(sSQL_Query), "CREATE TABLE IF NOT EXISTS `EntWatch_Current_Eban`(	`id` INTEGER PRIMARY KEY AUTOINCREMENT, \
																										`client_name` varchar(32) NOT NULL, \
																										`client_steamid` varchar(64) NOT NULL, \
																										`admin_name` varchar(32) NOT NULL, \
																										`admin_steamid` varchar(64) NOT NULL, \
																										`server` varchar(64), \
																										`duration` INTEGER NOT NULL, \
																										`timestamp_issued` INTEGER NOT NULL, \
																										`reason` varchar(64), \
																										`reason_unban` varchar(64), \
																										`admin_name_unban` varchar(32), \
																										`admin_steamid_unban` varchar(64), \
																										`timestamp_unban` INTEGER) CHARACTER SET %s COLLATE %s;", EW_EBAN_DB_ENTWATCH_CHARSET, EW_EBAN_DB_ENTWATCH_COLLATION);
		T_CreateTables.AddQuery(sSQL_Query);																							
		FormatEx(sSQL_Query, sizeof(sSQL_Query), "CREATE TABLE IF NOT EXISTS `EntWatch_Old_Eban`(	`id` INTEGER PRIMARY KEY AUTOINCREMENT, \
																									`client_name` varchar(32) NOT NULL, \
																									`client_steamid` varchar(64) NOT NULL, \
																									`admin_name` varchar(32) NOT NULL, \
																									`admin_steamid` varchar(64) NOT NULL, \
																									`server` varchar(64), \
																									`duration` INTEGER NOT NULL, \
																									`timestamp_issued` INTEGER NOT NULL, \
																									`reason` varchar(64), \
																									`reason_unban` varchar(64), \
																									`admin_name_unban` varchar(32), \
																									`admin_steamid_unban` varchar(64), \
																									`timestamp_unban` INTEGER) CHARACTER SET %s COLLATE %s;", EW_EBAN_DB_ENTWATCH_CHARSET, EW_EBAN_DB_ENTWATCH_COLLATION);
		T_CreateTables.AddQuery(sSQL_Query);
		SQL_ExecuteTransaction(g_hEbanDB, T_CreateTables, EWM_Eban_SQLCreateTables_Success, EWM_Eban_SQLCreateTables_Error, _, DBPrio_High);
	} else
	{
		g_iDBStatus = 2;
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnDatabaseUnknownDriver);
		Call_PushString(sConnectDriverDB);
		Call_Finish();
		#endif
	}
}

void EWM_Eban_SQLCreateTables_Success(Database hDatabase, any Data, int iNumQueries, Handle[] hResults, any[] QueryData)
{
	g_iDBStatus = 4;
	#if defined EW_MODULE_FORWARDS
	Call_StartForward(g_hOnDatabaseReady);
	Call_Finish();
	#endif
	EWM_Eban_CleanData_All();
}

void EWM_Eban_SQLCreateTables_Error(Database hDatabase, any Data, int iNumQueries, const  char[] sError, int iFailIndex, any[] QueryData)
{
	g_iDBStatus = 1;
	#if defined EW_MODULE_FORWARDS
	Call_StartForward(g_hOnDatabaseCreateTablesError);
	Call_PushString(sError);
	Call_Finish();
	#endif
}

void EWM_Eban_SQLGetEbansNumber_Current(Handle hDatabase, Handle hResults, const char[] sError, int userid)
{
	if(hDatabase == null || hResults == null || sError[0])
	{
		LogError("[EBan DB] SQL Get Ebans Number Error: %s", sError);
		return;
	}

	int iClient = GetClientOfUserId(userid);
	if(iClient < 1) return;

	if (SQL_FetchRow(hResults)) g_iClientEbansNumber[iClient] = SQL_FetchInt(hResults, 0);
}

// Main function of player e-ban
stock void EWM_Eban_BanClient(int iClient, int iAdmin, int iDuration, char[] sReason, bool bInvalidSteamID = false)
{
	if (g_EbanClients[iClient].Banned)
	{
		CReplyToCommand(iAdmin, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Already EBanned");
		CReplyToCommand(iAdmin, "%s%t %s%t %s%N %s%t %t %s%s%s(%s%s%s)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Player", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Warning, "Restricted By", "Admin", g_SchemeConfig.Color_Name, g_EbanClients[iClient].Admin_Name, g_SchemeConfig.Color_Warning, g_SchemeConfig.Color_SteamID, g_EbanClients[iClient].Admin_SteamID, g_SchemeConfig.Color_Warning);
		return;
	}

	int iBanDuration = iDuration;
	if (iBanDuration < -1) iBanDuration = -1;

	if (g_iMaxBanTime != 0 && iDuration > g_iMaxBanTime)
	{
		LogError("Invalid ban duration time %d", iBanDuration);
		return;
	}

	// Reason is too long to get inserted into the DB - Send reason menu instead
	if (strlen(sReason) > 64)
	{
		if (iAdmin != 0)
		{
			EWM_Eban_Menu_EBanReason(iAdmin, iClient, iDuration);
			return;
		} else FormatEx(sReason, 64, g_sDefaultBanReason);
	}

	if (iBanDuration >= g_iAdminBanLong && !CheckCommandAccess(iAdmin, "sm_eban_long", ADMFLAG_ROOT))
	{
		CReplyToCommand(iAdmin, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Not have permission lEBan", g_iAdminBanLong);
		iBanDuration = g_iAdminBanLong;
	}

	if (iBanDuration == 0 && !CheckCommandAccess(iAdmin, "sm_eban_perm", ADMFLAG_ROOT))
	{
		CReplyToCommand(iAdmin, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Not have permission pEBan");
		return;
	}

	char sAdmin_Name[32], sAdmin_SteamID[64];
	if (iAdmin != 0)
	{
		FormatEx(sAdmin_Name, sizeof(sAdmin_Name), "%N", iAdmin);
		FormatEx(sAdmin_SteamID, sizeof(sAdmin_SteamID), "%s", g_sSteamIDs[iAdmin]);
	} else {
		FormatEx(sAdmin_Name, sizeof(sAdmin_Name), "Console");
		FormatEx(sAdmin_SteamID, sizeof(sAdmin_SteamID), "SERVER");
	}

	// Since this can be called via API, we force the client as verified
	g_EbanClients[iClient].Verified = true;
	g_EbanClients[iClient].Banned = true;
	FormatEx(g_EbanClients[iClient].Admin_Name, 32, "%s", sAdmin_Name);
	FormatEx(g_EbanClients[iClient].Admin_SteamID, 64, "%s", sAdmin_SteamID);
	g_EbanClients[iClient].Duration = iBanDuration;
	g_EbanClients[iClient].TimeStamp_Issued = GetTime();
	FormatEx(g_EbanClients[iClient].Reason, 64, "%s", sReason);
	TrimString(g_EbanClients[iClient].Reason);
	StripQuotes(g_EbanClients[iClient].Reason);

	if (bInvalidSteamID)
	{
		CPrintToChat(iClient, "%s%t %s%s %s%t %s%N %s%t.", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, sAdmin_Name, g_SchemeConfig.Color_Disabled, "Restricted", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Disabled, "Temporary");
		CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Invalid SteamID");
		return;
	}

	switch (iBanDuration)
	{
		case 0:
			CPrintToChatAll("%s%t %s%s %s%t %s%N %s%t.", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, sAdmin_Name, g_SchemeConfig.Color_Disabled, "Restricted", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Disabled, "Permanently");
		case -1:
			CPrintToChatAll("%s%t %s%s %s%t %s%N %s%t.", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, sAdmin_Name, g_SchemeConfig.Color_Disabled, "Restricted", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Disabled, "Temporary");
		default: {
			CPrintToChatAll("%s%t %s%s %s%t %s%N %s%t.", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, sAdmin_Name, g_SchemeConfig.Color_Disabled, "Restricted", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Enabled, "For Minutes", iBanDuration);
			g_EbanClients[iClient].TimeStamp_Issued += iBanDuration * 60;
		}
	}

	CPrintToChatAll("%s%t %s%t: %s", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Reason", g_EbanClients[iClient].Reason);

	// Save to DB
	if (IsDBConnected() && iBanDuration >= 0)
	{
		char szAdmin_Name[32 * 2 + 1], sClient_Name[32 * 2 + 1], szClient_Name[32 * 2 + 1], szReason[64 * 2 + 1];
		g_hEbanDB.Escape(sAdmin_Name, szAdmin_Name, sizeof(szAdmin_Name)); // Admin Name to DB
		GetClientName(iClient, sClient_Name, sizeof(sClient_Name)); // Get Client Name
		g_hEbanDB.Escape(sClient_Name, szClient_Name, sizeof(szClient_Name)); // Client Name to DB
		g_hEbanDB.Escape(sReason, szReason, sizeof(szReason)); // Reason to DB

		char sTQuery[1024];
		FormatEx(sTQuery, sizeof(sTQuery), "INSERT INTO `EntWatch_Current_Eban` (`client_name`, `client_steamid`, `admin_name`, `admin_steamid`, `server`, `duration`, `timestamp_issued`, `reason`) VALUES ('%s', '%s', '%s', '%s', '%s', %d, %d, '%s')", szClient_Name, g_sSteamIDs[iClient], szAdmin_Name, sAdmin_SteamID, g_SchemeConfig.Server_Name, iBanDuration, g_EbanClients[iClient].TimeStamp_Issued, szReason);
		SQL_TQuery(g_hEbanDB, EWM_Eban_SQLTCallBackBan, sTQuery, 0);
	}

	// Handle item dropping on ban
	if (g_bDropItemOnEBan && EntWatch_HasSpecialItem(iClient))
	{
		for (int x = CS_SLOT_PRIMARY; x <= CS_SLOT_C4; x++)
		{
			int iWeapon = INVALID_ENT_REFERENCE;
			if ((iWeapon = GetPlayerWeaponSlot(iClient, x)) != INVALID_ENT_REFERENCE)
				for (int i = 0; i < g_ItemList.Length; i++)
				{
					class_ItemList ItemTest;
					g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
					if (ItemTest.WeaponID != INVALID_ENT_REFERENCE && ItemTest.WeaponID == iWeapon)
					{
						char sClassname[32];
						if (GetEdictClassname(iWeapon, sClassname, sizeof(sClassname)) && strcmp(sClassname, "weapon_knife", false) != 0)
						{
							CS_DropWeapon(iClient, iWeapon, false, false);
							GivePlayerItem(iClient, sClassname);
						}
					}
				}
		}
	}

	// Call forwards
	#if defined EW_MODULE_FORWARDS
	Call_StartForward(g_hOnBanForward);
	Call_PushCell(iAdmin);
	Call_PushCell(iBanDuration);
	Call_PushCell(iClient);
	Call_PushString(g_EbanClients[iClient].Reason);
	Call_Finish();
	#endif
}


void EWM_Eban_SQLTCallBackBan(Handle hDatabase, Handle hResults, const char[] sError, any data)
{
	#if defined EW_MODULE_FORWARDS
	if(sError[0])
	{
		Call_StartForward(g_hOnDatabaseBanError);
		Call_PushString(sError);
		Call_Finish();
	}
	else
	{
		Call_StartForward(g_hOnDatabaseBanSuccess);
		Call_Finish();
	}
	#endif
}

// Main function of player e-unban
public void EWM_Eban_UnBanClient(int iClient, int iAdmin, char[] sReason)
{
	if (!g_EbanClients[iClient].Banned)
	{
		CReplyToCommand(iClient, "%s%t %s%t %s%N %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Player", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Warning, "Is not restricted");
		return;
	}

	if (strlen(sReason) > 64)
	{
		if (iAdmin != 0)
		{
			EWM_Eban_Menu_EUnBanReason(iAdmin, iClient);
			return;
		} else FormatEx(sReason, 64, g_sDefaultUnBanReason);
	}

	char sAdmin_Name[32];
	char sAdmin_SteamID[64];
	if (iAdmin != 0)
	{
		//WESKER EDIT -- Restrict perma unbans with command override
		int iBanDuration = g_EbanClients[iClient].Duration;
		if (iBanDuration == 0)
		{
			//Check if admin has permission
			if (!CheckCommandAccess(iAdmin, "sm_eban_perm", ADMFLAG_ROOT))
			{
				CPrintToChat(iAdmin, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Not have permission rEBan");
				return;
			}
		}else if(iBanDuration >= g_iAdminBanLong)
		{
			//Check if admin has permission
			if (!CheckCommandAccess(iAdmin, "sm_eban_long", ADMFLAG_ROOT))
			{
				CPrintToChat(iAdmin, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Not have permission lEBan", g_iAdminBanLong);
				return;
			}
		}
		
		//Admin
		FormatEx(sAdmin_Name, sizeof(sAdmin_Name), "%N", iAdmin);
		FormatEx(sAdmin_SteamID, sizeof(sAdmin_SteamID), "%s", g_sSteamIDs[iAdmin]);
	}else
	{
		//Server
		FormatEx(sAdmin_Name, sizeof(sAdmin_Name), "Console");
		FormatEx(sAdmin_SteamID, sizeof(sAdmin_SteamID), "SERVER");
	}
	
	if (IsDBConnected())
	{
		char sSQL_Query[1024], szReason[64 * 2 + 1], szAdmin_Name[32 * 2 + 1];
		g_hEbanDB.Escape(sReason, szReason, sizeof(szReason));//Reason Unban to DB
		g_hEbanDB.Escape(sAdmin_Name, szAdmin_Name, sizeof(szAdmin_Name));//Admin Name Unban to DB
		
		Transaction T_UnBan = SQL_CreateTransaction();
		
		if(g_bKeepExpiredBans)
		{
			FormatEx(sSQL_Query, sizeof(sSQL_Query), "UPDATE `EntWatch_Current_Eban` SET `reason_unban`='%s', `admin_name_unban`='%s', `admin_steamid_unban`='%s', `timestamp_unban`=%d \
														WHERE `client_steamid`='%s' and `server`='%s' and `admin_steamid_unban` IS NULL", szReason, szAdmin_Name, sAdmin_SteamID, GetTime(), g_sSteamIDs[iClient], g_SchemeConfig.Server_Name);
			T_UnBan.AddQuery(sSQL_Query);
			FormatEx(sSQL_Query, sizeof(sSQL_Query), "INSERT INTO `EntWatch_Old_Eban` (`client_name`, `client_steamid`, `admin_name`, `admin_steamid`, `server`, `duration`, `timestamp_issued`, `reason`, `reason_unban`, `admin_name_unban`, `admin_steamid_unban`, `timestamp_unban`) \
														SELECT `client_name`, `client_steamid`, `admin_name`, `admin_steamid`, `server`, `duration`, `timestamp_issued`, `reason`, `reason_unban`, `admin_name_unban`, `admin_steamid_unban`, `timestamp_unban` FROM `EntWatch_Current_Eban` \
														WHERE `client_steamid`='%s' and `server`='%s'", g_sSteamIDs[iClient], g_SchemeConfig.Server_Name);
			T_UnBan.AddQuery(sSQL_Query);
		}
		
		FormatEx(sSQL_Query, sizeof(sSQL_Query), "DELETE FROM `EntWatch_Current_Eban` \
													WHERE `client_steamid`='%s' and `server`='%s'", g_sSteamIDs[iClient], g_SchemeConfig.Server_Name);
		T_UnBan.AddQuery(sSQL_Query);
		SQL_ExecuteTransaction(g_hEbanDB, T_UnBan, EWM_Eban_SQLUnBan_Success, EWM_Eban_SQLUnBan_Error, _, DBPrio_Normal);
	}
	
	EWM_Eban_CleanData_Client(iClient, true);

	#if defined EW_MODULE_FORWARDS
	Call_StartForward(g_hOnUnbanForward);
	Call_PushCell(iAdmin);
	Call_PushCell(iClient);
	Call_PushString(sReason);
	Call_Finish();
	#endif

	EWM_Eban_Update_Client(iClient);
	CPrintToChatAll("%s%t %s%s %s%t %s%N.", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, sAdmin_Name, g_SchemeConfig.Color_Enabled, "Unrestricted", g_SchemeConfig.Color_Name, iClient);
	CPrintToChatAll("%s%t %s%t: %s", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Reason", sReason);
}

void EWM_Eban_SQLUnBan_Success(Database hDatabase, any Data, int iNumQueries, Handle[] hResults, any[] QueryData)
{
	#if defined EW_MODULE_FORWARDS
	Call_StartForward(g_hOnDatabaseUnBanSuccess);
	Call_Finish();
	#endif
}

void EWM_Eban_SQLUnBan_Error(Database hDatabase, any Data, int iNumQueries, const  char[] sError, int iFailIndex, any[] QueryData)
{
	#if defined EW_MODULE_FORWARDS
	Call_StartForward(g_hOnDatabaseUnBanError);
	Call_PushString(sError);
	Call_Finish();
	#endif
}

// Function to send a request about active player bans
void EWM_Eban_Update_Client(int iClient)
{
	if (IsFakeClient(iClient))
		return;

	if (g_bEbanInvalidSteamID && (strncmp(g_sSteamIDs[iClient][6], "ID_", 3) == 0 || strncmp(g_sSteamIDs_short[iClient][0], "ID_", 3) == 0))
	{
		g_EbanClients[iClient].Verified = true;
		EWM_Eban_BanClient(iClient, 0, -1, "SteamID not verified.", true);
		return;
	}

	if (IsDBConnected())
	{
		char sTQuery[1024];
		FormatEx(sTQuery, sizeof(sTQuery), "SELECT `admin_name`, `admin_steamid`, `duration`, `timestamp_issued`, `reason` FROM `EntWatch_Current_Eban` \
												WHERE `client_steamid`='%s' and `server`='%s'", g_sSteamIDs[iClient], g_SchemeConfig.Server_Name);
		SQL_TQuery(g_hEbanDB, EWM_Eban_SQLTCallBackUpdate, sTQuery, g_iUserIDs[iClient]);
	}
}

// Function to update the ban when receiving a response from the database
void EWM_Eban_SQLTCallBackUpdate(Handle hDatabase, Handle hResults, const char[] sError, any iUserID)
{
	if(sError[0])
	{
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnDatabaseBanUpdateError);
		Call_PushString(sError);
		Call_Finish();
		#endif
		return;
	}
	
	int iClient = GetClientOfUserId(iUserID);
	if(iClient && IsClientInGame(iClient))
	{
		class_ClientSettings_EBan ClientSet;
		bool bBanFound = false;

		while(SQL_FetchRow(hResults))
		{
			bBanFound = true;
			class_ClientSettings_EBan ClientBuffer;
			SQL_FetchString(hResults, 0, ClientBuffer.Admin_Name, sizeof(ClientBuffer.Admin_Name));
			SQL_FetchString(hResults, 1, ClientBuffer.Admin_SteamID, sizeof(ClientBuffer.Admin_SteamID));
			ClientBuffer.Duration = SQL_FetchInt(hResults, 2);
			ClientBuffer.TimeStamp_Issued = SQL_FetchInt(hResults, 3);
			SQL_FetchString(hResults, 4, ClientBuffer.Reason, sizeof(ClientBuffer.Reason));

			ClientSet = ClientBuffer;
		}

		g_EbanClients[iClient].Verified = true;
		g_EbanClients[iClient].Banned = bBanFound;
		FormatEx(g_EbanClients[iClient].Admin_Name, 32, "%s", bBanFound ? ClientSet.Admin_Name : "");
		FormatEx(g_EbanClients[iClient].Admin_SteamID, 64, "%s", bBanFound ? ClientSet.Admin_SteamID : "");
		g_EbanClients[iClient].Duration = bBanFound ? ClientSet.Duration : 0;
		g_EbanClients[iClient].TimeStamp_Issued = bBanFound ? ClientSet.TimeStamp_Issued : 0;
		FormatEx(g_EbanClients[iClient].Reason, 64, "%s", bBanFound ? ClientSet.Reason : "");
	}
}

// Getting a list of banned people whose e-ban has expired
void EWM_Eban_Offline_Unban(int iTime)
{
	if (IsDBConnected())
	{
		char sTQuery[1024];
		FormatEx(sTQuery, sizeof(sTQuery), "SELECT `id` FROM `EntWatch_Current_Eban` \
												WHERE `server`='%s' and `duration` > 0 and `timestamp_issued` < %d", g_SchemeConfig.Server_Name, iTime);
		SQL_TQuery(g_hEbanDB, EWM_Eban_SQLTCallBackOfflineUnban, sTQuery, iTime);
	}
}

// E-unbanned a player and moving the ban entry to the e-unbanned table
void EWM_Eban_SQLTCallBackOfflineUnban(Handle hDatabase, Handle hResults, const char[] sError, any iTime)
{
	if(sError[0])
	{
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnDatabaseOfflineUnBanError);
		Call_PushString(sError);
		Call_Finish();
		#endif
	}
	else
	{
		int iCurrentID = 0;
		char sSQL_Query[1024];
		while(SQL_FetchRow(hResults))
		{
			iCurrentID = SQL_FetchInt(hResults, 0);
			Transaction T_UnBan = SQL_CreateTransaction();
		
			if(g_bKeepExpiredBans)
			{
				FormatEx(sSQL_Query, sizeof(sSQL_Query), "UPDATE `EntWatch_Current_Eban` SET `reason_unban`='Expired', `admin_name_unban`='Console', `admin_steamid_unban`='SERVER', `timestamp_unban`=%d \
															WHERE `id`=%d", iTime, iCurrentID);
				T_UnBan.AddQuery(sSQL_Query);
				FormatEx(sSQL_Query, sizeof(sSQL_Query), "INSERT INTO `EntWatch_Old_Eban` (`client_name`, `client_steamid`, `admin_name`, `admin_steamid`, `server`, `duration`, `timestamp_issued`, `reason`, `reason_unban`, `admin_name_unban`, `admin_steamid_unban`, `timestamp_unban`) \
															SELECT `client_name`, `client_steamid`, `admin_name`, `admin_steamid`, `server`, `duration`, `timestamp_issued`, `reason`, `reason_unban`, `admin_name_unban`, `admin_steamid_unban`, `timestamp_unban` FROM `EntWatch_Current_Eban` \
															WHERE `id`=%d", iCurrentID);
				T_UnBan.AddQuery(sSQL_Query);
			}
			
			FormatEx(sSQL_Query, sizeof(sSQL_Query), "DELETE FROM `EntWatch_Current_Eban` \
														WHERE `id`=%d", iCurrentID);
			T_UnBan.AddQuery(sSQL_Query);
			SQL_ExecuteTransaction(g_hEbanDB, T_UnBan, _, _, _, DBPrio_Normal);
		}
	}
}

// E-unban player
Action EWM_Eban_Timer_Checker(Handle timer)
{
	switch (g_iDBStatus)
	{
		case 1: // ReConnect
			Database.Connect(EWM_Eban_ConnectCallBack, EW_EBAN_ENTWATCH_SECTION);

		case 4: // Success Connect
		{
			int iCurrentTimeStamp = GetTime();
			for(int i=1; i<MaxClients+1; i++)
			{
				if(!IsClientInGame(i))
					continue;
				if(!g_EbanClients[i].Verified)
				{
					EWM_Eban_Update_Client(i);
					continue;
				}
				if(!g_EbanClients[i].Banned)
					continue;
				if(g_EbanClients[i].Duration <= 0)
					continue;

				if(iCurrentTimeStamp > g_EbanClients[i].TimeStamp_Issued)
					EWM_Eban_UnBanClient(i, 0, "Expired");
			}

			EWM_Eban_Offline_Unban(iCurrentTimeStamp);
		}
	}

	return Plugin_Continue;
}

stock void EWM_Eban_OnMapStart()
{
	EWM_Eban_CleanData_All();
}

stock void EWM_Eban_OnClientDisconnect(int iClient)
{
	if (!IsClientConnected(iClient) || IsFakeClient(iClient))
		return;

	EWM_Eban_CleanData_Client(iClient, false);
	g_iClientEbansNumber[iClient] = 0;
}

void EWM_Eban_CleanData_Client(int iClient, bool bForceVerified = false)
{
	if (IsFakeClient(iClient))
		return;

	if (!bForceVerified)
		g_EbanClients[iClient].Verified = false;

	g_EbanClients[iClient].Banned = false;
	FormatEx(g_EbanClients[iClient].Admin_Name, 32, "");
	FormatEx(g_EbanClients[iClient].Admin_SteamID, 64, "");
	g_EbanClients[iClient].Duration = 0;
	g_EbanClients[iClient].TimeStamp_Issued = 0;
	FormatEx(g_EbanClients[iClient].Reason, 64, "");
}

void EWM_Eban_CleanData_All()
{
	for(int i=1; i<=MaxClients; i++)
	{
		if (!IsClientInGame(i) || IsFakeClient(i))
			continue;

		EWM_Eban_CleanData_Client(i, false);
		EWM_Eban_Update_Client(i);
	}
}

// Ban a client
public Action EWM_Eban_Command_Restrict(int iClient, int iArgs)
{
	if (GetCmdArgs() < 1)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_eban <target> [<duration>] [<reason>]", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	int len, next_len, iDuration = -1;
	char Arguments[256], sArg[64], sTime[20];
	GetCmdArgString(Arguments, sizeof(Arguments));

	len = BreakString(Arguments, sArg, sizeof(sArg));
	if(len == -1)
	{
		len = 0;
		Arguments[0] = '\0';
	}

	int iTarget = -1;
	if((iTarget = FindTarget(iClient, sArg, true)) == -1) return Plugin_Handled;
	
	if((next_len = BreakString(Arguments[len], sTime, sizeof(sTime))) != -1)
	{
		len += next_len;
	} else
	{
		len = 0;
		Arguments[0] = '\0';
	}

	if(!sTime[0] || !StringToIntEx(sTime, iDuration)) iDuration = g_iDefaultBanTime;
	if(GetCmdArgs() == 1) iDuration = g_iDefaultBanTime;

	char sReason[64];
	if (g_bUseReasonMenu && IsValidClient(iClient))
	{
		EWM_Eban_Menu_EBanReason(iClient, iTarget, iDuration);
		return Plugin_Handled;
	} else
	{	
		FormatEx(sReason, sizeof(sReason), Arguments[len]);
		if(!sReason[0]) sReason = g_sDefaultBanReason;
		TrimString(sReason);
		StripQuotes(sReason);
	}

	if (g_iMaxBanTime != 0 && iDuration > g_iMaxBanTime || iDuration < -1)
	{
		CReplyToCommand(iClient, "%s%t %s%t: Invalid duration supplied, value must be between 1 and %d (0 = Perma, -1 = Temporary)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage", g_iMaxBanTime);
		return Plugin_Handled;
	}
	EWM_Eban_BanClient(iTarget, iClient, iDuration, sReason);

	return Plugin_Handled;
}

// Unban a client
public Action EWM_Eban_Command_Unrestrict(int iClient, int iArgs)
{
	if (GetCmdArgs() < 1)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_eunban <target> [<reason>]", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	int len;
	char Arguments[256], sArg[64];
	GetCmdArgString(Arguments, sizeof(Arguments));
	
	len = BreakString(Arguments, sArg, sizeof(sArg));
	if(len == -1)
	{
		len = 0;
		Arguments[0] = '\0';
	}

	int iTarget = -1;
	if((iTarget = FindTarget(iClient, sArg, true)) == -1) return Plugin_Handled;

	if (!g_EbanClients[iTarget].Banned)
	{
		CReplyToCommand(iClient, "%s%t %s%t %s%N %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Player", g_SchemeConfig.Color_Name, iTarget, g_SchemeConfig.Color_Warning, "Is not restricted");
		return Plugin_Handled;
	}

	char sReason[64];
	if (g_bUseReasonMenu && IsValidClient(iClient))
	{
		EWM_Eban_Menu_EUnBanReason(iClient, iTarget);
		return Plugin_Handled;
	} else
	{
		FormatEx(sReason, sizeof(sReason), Arguments[len]);
		if (!sReason[0]) sReason = g_sDefaultUnBanReason;
		TrimString(sReason);
		StripQuotes(sReason);
		EWM_Eban_UnBanClient(iTarget, iClient, sReason);
	}

	return Plugin_Handled;
}

// Lists the clients that are currently on the server and banned
public Action EWM_Eban_Command_EBanlist(int iClient, int iArgs)
{
	bool bFirst = false;

	for(int i=1; i<MaxClients+1; i++)
	{
		if(!IsClientInGame(i)) continue;
		
		if(g_EbanClients[i].Banned)
		{
			bFirst = true;
			char sBuff[256];
			CReplyToCommand(iClient, "%s%t %s%t:", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Currently Eban");
			if(g_EbanClients[i].Duration==-1)
				FormatEx(sBuff, sizeof(sBuff), "%T: %T", "Duration", iClient, "Temporary", iClient);
			else if(g_EbanClients[i].Duration==0)
				FormatEx(sBuff, sizeof(sBuff), "%T: %T", "Duration", iClient, "Permanently", iClient);
			else 
			{
				char sIssuedBuf[64];
				FormatTime(sIssuedBuf, sizeof(sIssuedBuf), NULL_STRING, g_EbanClients[i].TimeStamp_Issued);
				FormatEx(sBuff, sizeof(sBuff), "%T: %i %T: %s", "Duration", iClient, g_EbanClients[i].Duration, "Issued", iClient, sIssuedBuf);
			}
			CReplyToCommand(iClient, "%s#%i \t%N \t%t: %s(%s) \t%s \t%t: %s", g_SchemeConfig.Color_Warning, g_iUserIDs[i], i, "Admin", g_EbanClients[i].Admin_Name,  g_EbanClients[i].Admin_SteamID, sBuff, "Reason", g_EbanClients[i].Reason);
		}
	}

	if (!bFirst)
		CReplyToCommand(iClient, "%s%t.", g_SchemeConfig.Color_Warning, "No EBans");

	return Plugin_Handled;
}

public Action EWM_Eban_Command_Status(int iClient, int iArgs)
{
	int iTarget = -1;

	if (iArgs != 0)
	{
		char sArg[MAX_NAME_LENGTH];
		GetCmdArg(1, sArg, sizeof(sArg));
		iTarget = FindTarget(iClient, sArg, false, true);
	}
	else
		iTarget = iClient;

	if (iTarget == -1)
	{
		CReplyToCommand(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Player is not valid anymore");
		return Plugin_Handled;
	}

	int iClientEbans = g_iClientEbansNumber[iTarget];

	if (iClientEbans > 0)
		CReplyToCommand(iClient, "%s%t %s%N %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, iTarget, g_SchemeConfig.Color_Warning, "Client Eban numbers", iClientEbans);

	if(g_EbanClients[iTarget].Banned)
	{
		char sDuration[256], sIssuedBuf[64];
		switch (g_EbanClients[iTarget].Duration)
		{
			case -1:
				FormatEx(sDuration, sizeof(sDuration), "%s%t %s%T: %T", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Duration", iClient, "Temporary", iClient);
			case 0:
				FormatEx(sDuration, sizeof(sDuration), "%s%t %s%T: %T", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Duration", iClient, "Permanently", iClient);
			default:
				FormatEx(sDuration, sizeof(sDuration), "%s%t %s%T: %i %T", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Duration", iClient, g_EbanClients[iTarget].Duration, "Minutes", iClient);
		}
		FormatTime(sIssuedBuf, sizeof(sIssuedBuf), NULL_STRING, g_EbanClients[iTarget].TimeStamp_Issued);

		CReplyToCommand(iClient, "%s%t %s%t %s%N %s%t.", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Player", g_SchemeConfig.Color_Name, iTarget, g_SchemeConfig.Color_Warning, "Has a Restrict");
		CReplyToCommand(iClient, "%s%t %s%t: %s%s%s(%s%s%s)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Admin", g_SchemeConfig.Color_Name, g_EbanClients[iTarget].Admin_Name, g_SchemeConfig.Color_Warning, g_SchemeConfig.Color_SteamID, g_EbanClients[iTarget].Admin_SteamID, g_SchemeConfig.Color_Warning);
		CReplyToCommand(iClient, sDuration);
		CReplyToCommand(iClient, "%s%t %s%T: %s", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Expires", iClient, sIssuedBuf);
		CReplyToCommand(iClient, "%s%t%s %t: %s", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Reason", g_EbanClients[iTarget].Reason);
	} else CReplyToCommand(iClient, "%s%t %s%t %s%N %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Player", g_SchemeConfig.Color_Name, iTarget, g_SchemeConfig.Color_Warning, "Is not restricted");

	return Plugin_Handled;
}

//Menus
void EWM_Eban_Menu_EBanReason(int iClient, int iTarget, int iDuration)
{
	Menu hMenu = CreateMenu(EWM_Eban_Menu_EBanReason_Handler);
	char sMenuTranslate[128], sMenuTemp[64], sIndexTemp[96];
	if(iDuration == -1) FormatEx(sMenuTranslate, sizeof(sMenuTranslate), "%T %T %N [%T]", "EW_Tag", iClient, "EBan Reason for", iClient, iTarget, "Temporary", iClient);
	else if(iDuration == 0) FormatEx(sMenuTranslate, sizeof(sMenuTranslate), "%T %T %N [%T]", "EW_Tag", iClient, "EBan Reason for", iClient, iTarget, "Permanently", iClient);
		else FormatEx(sMenuTranslate, sizeof(sMenuTranslate), "%T %T %N [%i %T]", "EW_Tag", iClient, "EBan Reason for", iClient, iTarget, iDuration, "Minutes", iClient);
	hMenu.SetTitle(sMenuTranslate);
	
	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i/%s", iDuration, g_iUserIDs[iTarget], "Item misuse");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Item misuse", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);
	
	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i/%s", iDuration, g_iUserIDs[iTarget], "Trolling on purpose");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Trolling on purpose", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);
	
	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i/%s", iDuration, g_iUserIDs[iTarget], "Throwing item away");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Throwing item away", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);
	
	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i/%s", iDuration, g_iUserIDs[iTarget], "Not using an item");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Not using an item", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);
	
	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i/%s", iDuration, g_iUserIDs[iTarget], "Trimming team");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Trimming team", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);
	
	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i/%s", iDuration, g_iUserIDs[iTarget], "Not listening to leader");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Not listening to leader", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);
	
	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i/%s", iDuration, g_iUserIDs[iTarget], "Spamming an item");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Spamming an item", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);
	
	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i/%s", iDuration, g_iUserIDs[iTarget], "Other");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Other", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);
	
	hMenu.Display(iClient, MENU_TIME_FOREVER);
}

public int EWM_Eban_Menu_EBanReason_Handler(Menu hMenu, MenuAction hAction, int iClient, int iParam2)
{
	switch(hAction)
	{
		case MenuAction_End: delete hMenu;
		case MenuAction_Select:
		{
			char sSelected[96], Explode_sParam[3][96], sReason[32];
			hMenu.GetItem(iParam2, sSelected, sizeof(sSelected));
			ExplodeString(sSelected, "/", Explode_sParam, 3, 96);
			int iDuration = StringToInt(Explode_sParam[0]);
			int iTarget = GetClientOfUserId(StringToInt(Explode_sParam[1]));
			FormatEx(sReason, sizeof(sReason), "%s", Explode_sParam[2]);
			if(IsValidClient(iTarget))
				EWM_Eban_BanClient(iTarget, iClient, iDuration, sReason);
			else
				CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Player is not valid anymore");
		}
	}
	return 0;
}
void EWM_Eban_Menu_EUnBanReason(int iClient, int iTarget)
{
	Menu hMenu = CreateMenu(EWM_Eban_Menu_EUnBanReason_Handler);
	char sMenuTranslate[128], sMenuTemp[64], sIndexTemp[96];
	FormatEx(sMenuTranslate, sizeof(sMenuTranslate), "%T %T %N", "EW_Tag", iClient, "EUnBan Reason for", iClient, iTarget, iClient);
	hMenu.SetTitle(sMenuTranslate);

	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%s", g_iUserIDs[iTarget], "Wrong target");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Wrong target", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);

	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%s", g_iUserIDs[iTarget], "Giving another chance");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Giving another chance", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);

	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%s", g_iUserIDs[iTarget], "Bad duration");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Bad duration", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);

	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%s", g_iUserIDs[iTarget], "Was not on purpose");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Was not on purpose", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);

	FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%s", g_iUserIDs[iTarget], "Other");
	FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T", "Other", iClient);
	hMenu.AddItem(sIndexTemp, sMenuTemp);

	hMenu.Display(iClient, MENU_TIME_FOREVER);
}

public int EWM_Eban_Menu_EUnBanReason_Handler(Menu hMenu, MenuAction hAction, int iClient, int iParam2)
{
	switch(hAction)
	{
		case MenuAction_End: delete hMenu;
		case MenuAction_Select:
		{
			char sSelected[96], Explode_sParam[2][96], sReason[32];
			hMenu.GetItem(iParam2, sSelected, sizeof(sSelected));
			ExplodeString(sSelected, "/", Explode_sParam, 2, 96);
			int iTarget = GetClientOfUserId(StringToInt(Explode_sParam[0]));
			FormatEx(sReason, sizeof(sReason), "%s", Explode_sParam[1]);
			if(IsValidClient(iTarget))
				EWM_Eban_UnBanClient(iTarget, iClient, sReason);
			else
				CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Player is not valid anymore");
		}
	}
	return 0;
}