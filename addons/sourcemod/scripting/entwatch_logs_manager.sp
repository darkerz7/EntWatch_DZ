#pragma semicolon 1
#pragma newdecls required
#include <sourcemod>

#include <EntWatch>
//Using discordWebhookAPI: https://github.com/srcdslab/sm-plugin-DiscordWebhookAPI
#tryinclude <discordWebhookAPI>

#undef REQUIRE_PLUGIN
#tryinclude <ExtendedDiscord> //Using Extended-Discord: https://github.com/srcdslab/sm-plugin-Extended-Discord
#define REQUIRE_PLUGIN

#define DB_ENTWATCH_SECTION "EntWatch"
#define DB_ENTWATCH_CHARSET "utf8mb4"
#define DB_ENTWATCH_COLLATION "utf8mb4_unicode_ci"

ConVar	g_hCvar_System_Server,
		g_hCvar_Item_Server,
		g_hCvar_Admin_Server,
		#if defined _discordWebhookAPI_included_
		g_hCvar_System_Discord,
		g_hCvar_Item_Discord,
		g_hCvar_Admin_Discord,
		g_hCvar_System_Webhook,
		g_hCvar_Item_Webhook,
		g_hCvar_Admin_Webhook,
		g_hCvar_Webhook_Retry,
		g_hCvar_Username,
		g_hCvar_Avatar,
		g_hCvar_Item_ChannelType,
		g_hCvar_Item_ThreadID,
		g_hCvar_Admin_ChannelType,
		g_hCvar_Admin_ThreadID,
		g_hCvar_System_ChannelType,
		g_hCvar_System_ThreadName,
		g_hCvar_System_ThreadID,
		#endif
		g_hCvar_System_Database,
		g_hCvar_Item_Database,
		g_hCvar_Admin_Database;

bool	g_bSystem_Server = true,
		g_bItem_Server = true,
		g_bAdmin_Server = true,
		#if defined _discordWebhookAPI_included_
		g_bSystem_Discord = false,
		g_bItem_Discord = false,
		g_bAdmin_Discord = false,
		g_Plugin_ExtDiscord = false,
		#endif
		g_bSystem_Database = false,
		g_bItem_Database = false,
		g_bAdmin_Database = false;

#if defined _discordWebhookAPI_included_
int		g_iRetry;

char	g_sSystem_URL[WEBHOOK_URL_MAX_SIZE],
		g_sItem_URL[WEBHOOK_URL_MAX_SIZE],
		g_sAdmin_URL[WEBHOOK_URL_MAX_SIZE],
		sMessageDiscord[WEBHOOK_MSG_MAX_SIZE];
#endif

Database g_hDB;
int g_iDBStatus = 0; // 0 - No Connect, 1 - Need to Connect , 2 - Reconnect, 3 - Unknown Driver, 4 - Create Table, 5 - Ready to Query
Handle g_hTimer;
char g_sCurrentMap[128];
char g_sServer[64];
char g_sSteamIDs[MAXPLAYERS+1][32];

public Plugin myinfo =
{
	name = "EntWatch Logs Manager",
	author = "DarkerZ[RUS], .Rushaway",
	description = "Allows you to manage logs from the plugin EntWatch.",
	version = "1.DZ.4",
	url = "dark-skill.ru"
};

public void OnPluginStart()
{
	g_hCvar_System_Server			= CreateConVar("entwatch_system_server", "1", "Whether LM will write system logs to the server");
	g_hCvar_Item_Server				= CreateConVar("entwatch_item_server", "1", "Whether LM will write items logs to the server");
	g_hCvar_Admin_Server			= CreateConVar("entwatch_admin_server", "1", "Whether LM will write admins activites logs to the server");

	#if defined _discordWebhookAPI_included_
	g_hCvar_Item_Discord			= CreateConVar("entwatch_item_discord", "0", "Whether LM will write items logs to the discord");
	g_hCvar_Admin_Discord			= CreateConVar("entwatch_admin_discord", "0", "Whether LM will write admins activites logs to the discord");
	g_hCvar_System_Discord			= CreateConVar("entwatch_system_discord", "0", "Whether LM will write system logs to the discord");
	g_hCvar_System_Webhook			= CreateConVar("entwatch_system_webhook", "", "The system messages webhook URL of your Discord channel.", FCVAR_PROTECTED);
	g_hCvar_Item_Webhook			= CreateConVar("entwatch_item_webhook", "", "The items messages webhook URL of your Discord channel.", FCVAR_PROTECTED);
	g_hCvar_Admin_Webhook			= CreateConVar("entwatch_admin_webhook", "", "The admin activites messages webhook URL of your Discord channel.", FCVAR_PROTECTED);
	g_hCvar_Webhook_Retry	 		= CreateConVar("entwatch_webhook_retry", "3", "Number of retries if webhook fails.", FCVAR_PROTECTED);
	g_hCvar_Username 				= CreateConVar("entwatch_username", "EntWatch Logs Manager", "Discord username.");
	g_hCvar_Avatar 					= CreateConVar("entwatch_avatar", "https://avatars.githubusercontent.com/u/25752428?v=4", "URL to Avatar image.");

	/* Thread config */
	g_hCvar_Item_ChannelType 		= CreateConVar("entwatch_item_channel_type", "0", "Items logs: Type of your channel: (1 = Thread Reply, 0 = Classic Text channel");
	g_hCvar_Item_ThreadID 			= CreateConVar("entwatch_item_threadid", "0", "Items logs: If thread_id is provided, the message will send in that thread.", FCVAR_PROTECTED);
	g_hCvar_Admin_ChannelType 		= CreateConVar("entwatch_admin_channel_type", "0", "Admin activites: Type of your channel: (1 = Thread Reply, 0 = Classic Text channel");
	g_hCvar_Admin_ThreadID 			= CreateConVar("entwatch_admin_threadid", "0", "Admin activites: If thread_id is provided, the message will send in that thread.", FCVAR_PROTECTED);
	g_hCvar_System_ChannelType 		= CreateConVar("entwatch_system_channel_type", "0", "System Logs: Type of your channel: (1 = Thread, 0 = Classic Text channel");
	g_hCvar_System_ThreadName 		= CreateConVar("entwatch_system_threadname", "EntWatch System Logs", "System Logs: The Thread Name of your Discord forums. (If not empty, will create a new thread)", FCVAR_PROTECTED);
	g_hCvar_System_ThreadID 		= CreateConVar("entwatch_system_threadid", "0", "System Logs: If thread_id is provided, the message will send in that thread.", FCVAR_PROTECTED);
	#endif

	g_hCvar_System_Database			= CreateConVar("entwatch_system_database", "0", "Whether LM will write system logs to the database");
	g_hCvar_Item_Database			= CreateConVar("entwatch_item_database", "0", "Whether LM will write items logs to the database");
	g_hCvar_Admin_Database			= CreateConVar("entwatch_admin_database", "0", "Whether LM will write admins activites logs to the database");

	g_bSystem_Server = GetConVarBool(g_hCvar_System_Server);
	g_bItem_Server = GetConVarBool(g_hCvar_Item_Server);
	g_bAdmin_Server = GetConVarBool(g_hCvar_Admin_Server);

	#if defined _discordWebhookAPI_included_
	g_bSystem_Discord = GetConVarBool(g_hCvar_System_Discord);
	g_bItem_Discord = GetConVarBool(g_hCvar_Item_Discord);
	g_bAdmin_Discord = GetConVarBool(g_hCvar_Admin_Discord);
	#endif

	g_bSystem_Database = GetConVarBool(g_hCvar_System_Database);
	g_bItem_Database = GetConVarBool(g_hCvar_Item_Database);
	g_bAdmin_Database = GetConVarBool(g_hCvar_Admin_Database);

	if(g_bSystem_Database || g_bItem_Database || g_bAdmin_Database) Connect_to_DB();

	#if defined _discordWebhookAPI_included_
	GetConVarString(g_hCvar_System_Webhook, g_sSystem_URL, sizeof(g_sSystem_URL));
	GetConVarString(g_hCvar_Item_Webhook, g_sItem_URL, sizeof(g_sItem_URL));
	GetConVarString(g_hCvar_Admin_Webhook, g_sAdmin_URL, sizeof(g_sAdmin_URL));
	g_iRetry = GetConVarInt(g_hCvar_Webhook_Retry);
	#endif

	HookConVarChange(g_hCvar_System_Server, Cvar_System_Changed);
	HookConVarChange(g_hCvar_Item_Server, Cvar_Item_Changed);
	HookConVarChange(g_hCvar_Admin_Server, Cvar_Admin_Changed);

	#if defined _discordWebhookAPI_included_
	HookConVarChange(g_hCvar_System_Discord, Cvar_System_Changed);
	HookConVarChange(g_hCvar_Item_Discord, Cvar_Item_Changed);
	HookConVarChange(g_hCvar_Admin_Discord, Cvar_Admin_Changed);
	HookConVarChange(g_hCvar_System_Webhook, Cvar_System_Changed);
	HookConVarChange(g_hCvar_Item_Webhook, Cvar_Item_Changed);
	HookConVarChange(g_hCvar_Admin_Webhook, Cvar_Admin_Changed);
	HookConVarChange(g_hCvar_Webhook_Retry, Cvar_Changed);
	#endif

	HookConVarChange(g_hCvar_System_Database, Cvar_System_Changed);
	HookConVarChange(g_hCvar_Item_Database, Cvar_Item_Changed);
	HookConVarChange(g_hCvar_Admin_Database, Cvar_Admin_Changed);

	AutoExecConfig(true, "EntWatch_LM_DZ");
}

#if defined _discordWebhookAPI_included_
public void OnAllPluginsLoaded()
{
	g_Plugin_ExtDiscord = LibraryExists("ExtendedDiscord");
}

public void OnLibraryAdded(const char[] sName)
{
	if (strcmp(sName, "ExtendedDiscord", false) == 0)
		g_Plugin_ExtDiscord = true;
}

public void OnLibraryRemoved(const char[] sName)
{
	if (strcmp(sName, "ExtendedDiscord", false) == 0)
		g_Plugin_ExtDiscord = false;
}
#endif

public void OnClientPostAdminCheck(int iClient)
{
	char sSteamID[32];
	GetClientAuthId(iClient, AuthId_Steam3, sSteamID, sizeof(sSteamID), false);
	FormatEx(g_sSteamIDs[iClient], sizeof(g_sSteamIDs[]), "%s", sSteamID);
}

public void OnClientDisconnect(int iClient)
{
	FormatEx(g_sSteamIDs[iClient], sizeof(g_sSteamIDs[]), "");
}

public void OnMapInit(const char[] mapName)
{
	FormatEx(g_sCurrentMap, sizeof(g_sCurrentMap), mapName);
}

// Database connection function
public void Connect_to_DB()
{
	if(g_iDBStatus < 2)
	{
		g_iDBStatus = 1;
		if(g_hDB != null) delete g_hDB;
		Database.Connect(DB_ConnectCallBack, DB_ENTWATCH_SECTION);
		if(g_hTimer == null) g_hTimer = CreateTimer(60.0, DB_Timer_Checker, _, TIMER_REPEAT);
	}
}

// Database disconnection function
public void Disconnect_to_DB()
{
	delete g_hDB;
	delete g_hTimer;
	g_iDBStatus = 0;
}

// Database connection check function
Action DB_Timer_Checker(Handle timer)
{
	if(g_iDBStatus == 2)
	{
		g_iDBStatus = 1;
		Connect_to_DB();
	}
	return Plugin_Continue;
}

void DB_ConnectCallBack(Database hDatabase, const char[] sError, any data)
{
	if (g_iDBStatus == 0) return; //Disconnect
	if (hDatabase == null)	// Fail Connect
	{
		LogError("[EW-LM DB] Database failure: %s, ReConnect after 60 sec", sError);
		g_iDBStatus = 2; //ReConnect
		return;
	}
	g_hDB = hDatabase;
	LogMessage("[EW-LM DB] Successful connection to DB");
	DB_CreateTables(); // Create Tables
	g_hDB.SetCharset(DB_ENTWATCH_CHARSET); // Set Charset UTF8
}

void DB_CreateTables()
{
	char sConnectDriverDB[16];
	g_hDB.Driver.GetIdentifier(sConnectDriverDB, sizeof(sConnectDriverDB));
	if(strcmp(sConnectDriverDB, "mysql") == 0)
	{
		g_iDBStatus = 4;
		//Create MySQL Tables
		char sSQL_Query[1024];
		Transaction T_CreateTables = SQL_CreateTransaction();
		FormatEx(sSQL_Query, sizeof(sSQL_Query), "CREATE TABLE IF NOT EXISTS `EntWatch_LM`(	`id` int(10) unsigned NOT NULL auto_increment, \
																								`type` int, \
																								`admin` varchar(32), \
																								`admin_steamid` varchar(64), \
																								`client` varchar(32), \
																								`client_steamid` varchar(64), \
																								`receiver` varchar(32), \
																								`receiver_steamid` varchar(64), \
																								`server` varchar(64) NOT NULL, \
																								`map` varchar(32) NOT NULL, \
																								`message` varchar(128), \
																								`other` varchar(32), \
																								`reason` varchar(64), \
																								`timestamp` int NOT NULL, \
																								PRIMARY KEY (id)) CHARACTER SET %s COLLATE %s;", DB_ENTWATCH_CHARSET, DB_ENTWATCH_COLLATION);
		T_CreateTables.AddQuery(sSQL_Query);
		SQL_ExecuteTransaction(g_hDB, T_CreateTables, DB_SQLCreateTables_Success, DB_SQLCreateTables_Error, _, DBPrio_High);
	} else if(strcmp(sConnectDriverDB, "sqlite") == 0)
	{
		g_iDBStatus = 4;
		//Create SQLite Tables
		char sSQL_Query[1024];
		Transaction T_CreateTables = SQL_CreateTransaction();
		FormatEx(sSQL_Query, sizeof(sSQL_Query), "CREATE TABLE IF NOT EXISTS `EntWatch_LM`(	`id` INTEGER PRIMARY KEY AUTOINCREMENT, \
																								`type` INTEGER, \
																								`admin` varchar(32), \
																								`admin_steamid` varchar(64), \
																								`client` varchar(32), \
																								`client_steamid` varchar(64), \
																								`receiver` varchar(32), \
																								`receiver_steamid` varchar(64), \
																								`server` varchar(64) NOT NULL, \
																								`map` varchar(32) NOT NULL, \
																								`message` varchar(128), \
																								`other` varchar(32), \
																								`reason` varchar(64), \
																								`timestamp` INTEGER NOT NULL) CHARACTER SET %s COLLATE %s;", DB_ENTWATCH_CHARSET, DB_ENTWATCH_COLLATION);
		T_CreateTables.AddQuery(sSQL_Query);
		SQL_ExecuteTransaction(g_hDB, T_CreateTables, DB_SQLCreateTables_Success, DB_SQLCreateTables_Error, _, DBPrio_High);
	} else
	{
		g_iDBStatus = 3;
		LogError("[EW-LM DB] Unknown Driver: %s, cannot create tables.", sConnectDriverDB);
	}
}

void DB_SQLCreateTables_Success(Database hDatabase, any Data, int iNumQueries, Handle[] hResults, any[] QueryData)
{
	if (g_iDBStatus == 0) return; //Disconnect
	g_iDBStatus = 5;
	LogMessage("[EW-LM DB] DB Ready");
}

void DB_SQLCreateTables_Error(Database hDatabase, any Data, int iNumQueries, const  char[] sError, int iFailIndex, any[] QueryData)
{
	if (g_iDBStatus == 0) return; //Disconnect
	g_iDBStatus = 2;
	LogError("[EW-LM DB] SQL CreateTables Error: %s", sError);
}

public void Cvar_System_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_System_Server)
		g_bSystem_Server = GetConVarBool(convar);
	else if(convar==g_hCvar_System_Database)
	{
		g_bSystem_Database = GetConVarBool(convar);
		if(g_bSystem_Database && g_iDBStatus == 0) Connect_to_DB();
		else if(!g_bItem_Database && !g_bAdmin_Database) Disconnect_to_DB();
	}
	#if defined _discordWebhookAPI_included_
	else if(convar==g_hCvar_System_Discord)
		g_bSystem_Discord = GetConVarBool(convar);
	else if(convar==g_hCvar_System_Webhook)
		GetConVarString(convar, g_sSystem_URL, sizeof(g_sSystem_URL));
	#endif
}

public void Cvar_Item_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_Item_Server)
		g_bItem_Server = GetConVarBool(convar);
	else if(convar==g_hCvar_Item_Database)
	{
		g_bItem_Database = GetConVarBool(convar);
		if(g_bItem_Database && g_iDBStatus == 0) Connect_to_DB();
		else if(!g_bSystem_Database && !g_bAdmin_Database) Disconnect_to_DB();
	}
	#if defined _discordWebhookAPI_included_
	else if(convar==g_hCvar_Item_Discord)
		g_bItem_Discord = GetConVarBool(convar);
	else if(convar==g_hCvar_Item_Webhook)
		GetConVarString(convar, g_sItem_URL, sizeof(g_sItem_URL));
	#endif
}

public void Cvar_Admin_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_Admin_Server)
		g_bAdmin_Server = GetConVarBool(convar);
	else if(convar==g_hCvar_Admin_Database)
	{
		g_bAdmin_Database = GetConVarBool(convar);
		if(g_bAdmin_Database && g_iDBStatus == 0) Connect_to_DB();
		else if(!g_bSystem_Database && !g_bItem_Database) Disconnect_to_DB();
	}
	#if defined _discordWebhookAPI_included_
	else if(convar==g_hCvar_Admin_Discord)
		g_bAdmin_Discord = GetConVarBool(convar);
	else if(convar==g_hCvar_Admin_Webhook)
		GetConVarString(convar, g_sAdmin_URL, sizeof(g_sAdmin_URL));
	#endif
}

#if defined _discordWebhookAPI_included_
public void Cvar_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_Webhook_Retry)
		g_iRetry = GetConVarInt(convar);
}
#endif

// System event handler
stock void System_Handler(const char[] sMessage, bool bType)
{
	char sMessageToLog[1024], sTime[64];
	// We form a common line for the log and discord
	#if defined _discordWebhookAPI_included_
	if(g_bSystem_Server || g_bSystem_Discord)
	#else
	if(g_bSystem_Server)
	#endif
	{
		FormatTime(sTime, sizeof(sTime), NULL_STRING, GetTime());
		if(bType) FormatEx(sMessageToLog, sizeof(sMessageToLog), "%s - %s - %s", g_sCurrentMap, sTime, sMessage);
		else FormatEx(sMessageToLog, sizeof(sMessageToLog), "%s - %s - {Error} %s", g_sCurrentMap, sTime, sMessage);
		if(StrContains(sMessageToLog, "\"") != -1) ReplaceString(sMessageToLog, sizeof(sMessageToLog), "\"", "");
	}
	// Send a message to the log
	if(g_bSystem_Server)
		if(bType) LogMessage("[EW-LM] %s", sMessageToLog);
		else LogError("[EW-LM] %s", sMessageToLog);
	#if defined _discordWebhookAPI_included_
	// Send a message to the discord
	if(g_bSystem_Discord && g_sSystem_URL[0])
	{
		if(bType) FormatEx(sMessageDiscord, sizeof(sMessageDiscord), "*%s - %s* ```%s```", g_sCurrentMap, sTime, sMessage);
		else FormatEx(sMessageDiscord, sizeof(sMessageDiscord), "**Error** *%s - %s* ```%s```", g_sCurrentMap, sTime, sMessage);
		if(StrContains(sMessageDiscord, "\"") != -1) ReplaceString(sMessageDiscord, sizeof(sMessageDiscord), "\"", "");
		SendWebHook(g_sSystem_URL, sMessageDiscord, 1);
	}
	#endif
	// Send a message to the database, if connection succeeded
	if(g_bSystem_Database && g_iDBStatus == 5 && g_sServer[0])
	{
		char sTQuery[1024];
		FormatEx(sTQuery, sizeof(sTQuery), "INSERT INTO `EntWatch_LM` (`type`, `server`, `map`, `timestamp`, `message`) VALUES (0, '%s', '%s', %d, '%s')", g_sServer, g_sCurrentMap, GetTime(), sMessage);
		SQL_TQuery(g_hDB, SQLTCallBack, sTQuery, 0);
	}
}

// Item action handler
stock void Item_Handler(const char[] sMessage, int iClient, const char[] sItemName)
{
	char sMessageToLog[1024], sTime[64];
	#if defined _discordWebhookAPI_included_
	if(g_bItem_Server || g_bItem_Discord)
	#else
	if(g_bItem_Server)
	#endif
	{
		FormatTime(sTime, sizeof(sTime), NULL_STRING, GetTime());
		FormatEx(sMessageToLog, sizeof(sMessageToLog), "%s - %s - %N [%s] %s %s", g_sCurrentMap, sTime, iClient, g_sSteamIDs[iClient], sMessage, sItemName);
		if(StrContains(sMessageToLog, "\"") != -1) ReplaceString(sMessageToLog, sizeof(sMessageToLog), "\"", "");
	}
	if(g_bItem_Server) LogAction(iClient, -1, "[EW-LM] %s", sMessageToLog);
	#if defined _discordWebhookAPI_included_
	if(g_bItem_Discord && g_sItem_URL[0])
	{
		FormatEx(sMessageDiscord, sizeof(sMessageDiscord), "*%s - %s* ```%N [%s] %s %s```", g_sCurrentMap, sTime, iClient, g_sSteamIDs[iClient], sMessage, sItemName);
		if(StrContains(sMessageDiscord, "\"") != -1) ReplaceString(sMessageDiscord, sizeof(sMessageDiscord), "\"", "");
		SendWebHook(g_sItem_URL, sMessageDiscord, 2);
	}
	#endif
	if(g_bItem_Database && g_iDBStatus == 5 && g_sServer[0])
	{
		char sTQuery[1024], sClient_Name[32], szClient_Name[32];
		GetClientName(iClient, sClient_Name, sizeof(sClient_Name));//Get Client Name
		g_hDB.Escape(sClient_Name, szClient_Name, sizeof(szClient_Name));//Client Name to DB
		FormatEx(sTQuery, sizeof(sTQuery), "INSERT INTO `EntWatch_LM` (`type`, `server`, `map`, `timestamp`, `client`, `client_steamid`, `message`, `other`) VALUES (1, '%s', '%s', %d, '%s', '%s', '%s', '%s')", g_sServer, g_sCurrentMap, GetTime(), szClient_Name, g_sSteamIDs[iClient], sMessage, sItemName);
		SQL_TQuery(g_hDB, SQLTCallBack, sTQuery, 0);
	}
}

// Record of actions of admins for issuing e-bans/e-unban
stock void Admin_Eban_Handler(const char[] sMessage, int iAdmin, const char[] sClientName, const char[] sClientSteamID, const char[] sActionTime, const char[] sReason)
{
	char sMessageToLog[1024], sTime[64];
	#if defined _discordWebhookAPI_included_
	if(g_bAdmin_Server || g_bAdmin_Discord)
	#else
	if(g_bAdmin_Server)
	#endif
	{
		FormatTime(sTime, sizeof(sTime), NULL_STRING, GetTime());
		if(sActionTime[0]) FormatEx(sMessageToLog, sizeof(sMessageToLog), "%s - %s - %N [%s] %s '%s [%s]' %s. Reason: %s", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, sClientName, sClientSteamID, sActionTime, sReason);
		else FormatEx(sMessageToLog, sizeof(sMessageToLog), "%s - %s - %N [%s] %s '%s [%s]'. Reason: %s", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, sClientName, sClientSteamID, sReason);
		if(StrContains(sMessageToLog, "\"") != -1) ReplaceString(sMessageToLog, sizeof(sMessageToLog), "\"", "");
	}
	if(g_bAdmin_Server) LogAction(iAdmin, -1, "[EW-LM] %s", sMessageToLog);
	#if defined _discordWebhookAPI_included_
	if(g_bAdmin_Discord && g_sAdmin_URL[0])
	{
		if(sActionTime[0]) FormatEx(sMessageDiscord, sizeof(sMessageDiscord), "*%s - %s* ```%N [%s] %s '%s [%s]' %s. Reason: %s```", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, sClientName, sClientSteamID, sActionTime, sReason);
		else FormatEx(sMessageDiscord, sizeof(sMessageDiscord), "*%s - %s* ```%N [%s] %s '%s [%s]'. Reason: %s```", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, sClientName, sClientSteamID, sReason);
		if(StrContains(sMessageDiscord, "\"") != -1) ReplaceString(sMessageDiscord, sizeof(sMessageDiscord), "\"", "");
		SendWebHook(g_sAdmin_URL, sMessageDiscord, 3);
	}
	#endif
	if(g_bAdmin_Database && g_iDBStatus == 5 && g_sServer[0])
	{
		char sTQuery[1024], sAdmin_Name[32], szAdmin_Name[32];
		GetClientName(iAdmin, sAdmin_Name, sizeof(sAdmin_Name));//Get Admin Name
		g_hDB.Escape(sAdmin_Name, szAdmin_Name, sizeof(szAdmin_Name));//Admin Name to DB
		FormatEx(sTQuery, sizeof(sTQuery), "INSERT INTO `EntWatch_LM` (`type`, `server`, `map`, `timestamp`, `admin`, `admin_steamid`, `client`, `client_steamid`, `message`, `other`, `reason` ) VALUES (2, '%s', '%s', %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s')", g_sServer, g_sCurrentMap, GetTime(), szAdmin_Name, g_sSteamIDs[iAdmin], sClientName, sClientSteamID, sMessage, sActionTime, sReason);
		SQL_TQuery(g_hDB, SQLTCallBack, sTQuery, 0);
	}
}

// Record of actions of admins on the transfer and spawn of items
stock void Admin_Other_Handler(const char[] sMessage, int iAdmin, int iTarget, int iReceiver, const char[] sItemName, const char[] sMessage2)
{
	char sMessageToLog[1024], sTime[64];
	#if defined _discordWebhookAPI_included_
	if(g_bAdmin_Server || g_bAdmin_Discord)
	#else
	if(g_bAdmin_Server)
	#endif
	{
		FormatTime(sTime, sizeof(sTime), NULL_STRING, GetTime());
		if(iReceiver == -1) FormatEx(sMessageToLog, sizeof(sMessageToLog), "%s - %s - %N [%s] %s %s %s %N [%s]", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, sItemName, sMessage2, iTarget, g_sSteamIDs[iTarget]);
		else if(iTarget == -1) FormatEx(sMessageToLog, sizeof(sMessageToLog), "%s - %s - %N [%s] %s %s %s %N [%s]", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, sItemName, sMessage2, iReceiver, g_sSteamIDs[iReceiver]);
		else FormatEx(sMessageToLog, sizeof(sMessageToLog), "%s - %s - %N [%s] %s %N [%s] %s %N [%s]", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, iTarget, g_sSteamIDs[iTarget], sMessage2, iReceiver, g_sSteamIDs[iReceiver]);
		if(StrContains(sMessageToLog, "\"") != -1) ReplaceString(sMessageToLog, sizeof(sMessageToLog), "\"", "");
	}
	if(g_bAdmin_Server)
		if(iTarget == -1) LogAction(iAdmin, iReceiver, "[EW-LM] %s", sMessageToLog);
		else LogAction(iAdmin, iTarget, "[EW-LM] %s", sMessageToLog);
	#if defined _discordWebhookAPI_included_
	if(g_bAdmin_Discord && g_sAdmin_URL[0])
	{
		if(iReceiver == -1) FormatEx(sMessageDiscord, sizeof(sMessageDiscord), "*%s - %s* ```%N [%s] %s %s %s %N [%s]```", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, sItemName, sMessage2, iTarget, g_sSteamIDs[iTarget]);
		else if(iTarget == -1) FormatEx(sMessageDiscord, sizeof(sMessageDiscord), "*%s - %s* ```%N [%s] %s %s %s %N [%s]```", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, sItemName, sMessage2, iReceiver, g_sSteamIDs[iReceiver]);
		else FormatEx(sMessageDiscord, sizeof(sMessageDiscord), "*%s - %s* ```%N [%s] %s %N [%s] %s %N [%s]```", g_sCurrentMap, sTime, iAdmin, g_sSteamIDs[iAdmin], sMessage, iTarget, g_sSteamIDs[iTarget], sMessage2, iReceiver, g_sSteamIDs[iReceiver]);
		if(StrContains(sMessageDiscord, "\"") != -1) ReplaceString(sMessageDiscord, sizeof(sMessageDiscord), "\"", "");
		SendWebHook(g_sAdmin_URL, sMessageDiscord, 3);
	}
	#endif
	if(g_bAdmin_Database && g_iDBStatus == 5 && g_sServer[0])
	{
		char sTQuery[1024], sAdmin_Name[32], szAdmin_Name[32], sTarget_Name[32], szTarget_Name[32], sReceiver_Name[32], szReceiver_Name[32], sTarget_SteamID[32], sReceiver_SteamID[32];
		GetClientName(iAdmin, sAdmin_Name, sizeof(sAdmin_Name));//Get Admin Name
		g_hDB.Escape(sAdmin_Name, szAdmin_Name, sizeof(szAdmin_Name));//Admin Name to DB
		if(iTarget != -1)
		{
			GetClientName(iTarget, sTarget_Name, sizeof(sTarget_Name));//Get Target Name
			g_hDB.Escape(sTarget_Name, szTarget_Name, sizeof(szTarget_Name));//Target Name to DB
			FormatEx(sTarget_SteamID, sizeof(sTarget_SteamID), "%s", g_sSteamIDs[iTarget]);
		}
		if(iReceiver != -1)
		{
			GetClientName(iReceiver, sReceiver_Name, sizeof(sReceiver_Name));//Get Receiver Name
			g_hDB.Escape(sReceiver_Name, szReceiver_Name, sizeof(szReceiver_Name));//Receiver Name to DB
			FormatEx(sReceiver_SteamID, sizeof(sReceiver_SteamID), "%s", g_sSteamIDs[iReceiver]);
		}
		FormatEx(sTQuery, sizeof(sTQuery), "INSERT INTO `EntWatch_LM` (`type`, `server`, `map`, `timestamp`, `admin`, `admin_steamid`, `client`, `client_steamid`, `receiver`, `receiver_steamid`, `message`, `other`) VALUES (3, '%s', '%s', %d, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')", g_sServer, g_sCurrentMap, GetTime(), szAdmin_Name, g_sSteamIDs[iAdmin], szTarget_Name, sTarget_SteamID, szReceiver_Name, sReceiver_SteamID, sMessage, sItemName);
		SQL_TQuery(g_hDB, SQLTCallBack, sTQuery, 0);
	}
}

void SQLTCallBack(Handle hDatabase, Handle hResults, const char[] sError, any data)
{
	if(sError[0]) LogError("[EW-LM DB] SQL CallBack Error: %s", sError);
}

#if defined _discordWebhookAPI_included_
stock void SendWebHook(const char[] sWebhookURL, const char[] sMessage, int type = 1)
{
	Webhook webhook = new Webhook(sMessage);

	char sThreadID[32], sThreadName[WEBHOOK_THREAD_NAME_MAX_SIZE];
	bool IsThread = false;

	switch(type)
	{
		case 1: // System
		{
			IsThread = g_hCvar_System_ChannelType.BoolValue;
			g_hCvar_System_ThreadID.GetString(sThreadID, sizeof sThreadID);
			g_hCvar_System_ThreadName.GetString(sThreadName, sizeof sThreadName);
		}
		case 2: // Items
		{
			IsThread = g_hCvar_Item_ChannelType.BoolValue;
			g_hCvar_Item_ThreadID.GetString(sThreadID, sizeof sThreadID);
		}
		case 3: // Admin Activites
		{
			IsThread = g_hCvar_Admin_ChannelType.BoolValue;
			g_hCvar_Admin_ThreadID.GetString(sThreadID, sizeof sThreadID);
		}
	}

	if (IsThread)
	{
		if (type <= 1 && !sThreadName[0] && !sThreadID[0])
		{
			LogError("[EW-LM Discord] Thread Name or ThreadID not found or specified.");
			delete webhook;
			return;
		}
		else
		{
			if (strlen(sThreadName) > 0)
			{
				webhook.SetThreadName(sThreadName);
				sThreadID[0] = '\0';
			}
		}
	}

	char sName[128], sAvatar[256];
	g_hCvar_Username.GetString(sName, sizeof(sName));
	g_hCvar_Avatar.GetString(sAvatar, sizeof(sAvatar));
	if (strlen(sName) > 0) webhook.SetUsername(sName);
	if (strlen(sAvatar) > 0) webhook.SetAvatarURL(sAvatar);

	DataPack pack = new DataPack();

	if (IsThread && strlen(sThreadName) <= 0 && strlen(sThreadID) > 0)
		pack.WriteCell(1);
	else
		pack.WriteCell(0);

	pack.WriteCell(type);
	pack.WriteString(sMessage);
	pack.WriteString(sWebhookURL);

	webhook.Execute(sWebhookURL, OnWebHookExecuted, pack, sThreadID);
	delete webhook;
}

public void OnWebHookExecuted(HTTPResponse response, DataPack pack)
{
	static int s_iRetries = 0;
	pack.Reset();
	bool IsThreadReply = pack.ReadCell();
	int iType = pack.ReadCell();
	char sMessage[1024], sWebhookURL[WEBHOOK_URL_MAX_SIZE];
	pack.ReadString(sMessage, sizeof(sMessage));
	pack.ReadString(sWebhookURL, sizeof(sWebhookURL));
	delete pack;

	if ((!IsThreadReply && response.Status != HTTPStatus_OK) || (IsThreadReply && response.Status != HTTPStatus_NoContent))
	{
		if (s_iRetries < g_iRetry)
		{
			PrintToServer("[EW-LM Discord] Failed to send the webhook. Resending it .. (%d/%d)", s_iRetries, g_iRetry);
			SendWebHook(sMessage, sWebhookURL, iType);
			s_iRetries++;
			return;
		} else {
		#if defined _extendeddiscord_included
			if (g_Plugin_ExtDiscord)
				ExtendedDiscord_LogError("[EW-LM Discord] Failed to send the webhook after %d retries, aborting.", s_iRetries);
			else
				LogError("[EW-LM Discord] Failed to send the webhook after %d retries, aborting.", s_iRetries);
		#else
			LogError("[EW-LM Discord] Failed to send the webhook after %d retries, aborting.", s_iRetries);
		#endif
		}
	}
	s_iRetries = 0;
}
#endif

// Forwards from EntWatch
public void EntWatch_OnSchemeServerName(const char[] sServerName)
{
	FormatEx(g_sServer, sizeof(g_sServer), "%s", sServerName);
}

public void EntWatch_OnUseItem(const char[] sItemName, int iClient, int iAbility)
{
	if(iAbility == 0) Item_Handler("used item", iClient, sItemName);
	else if(iAbility == 1) Item_Handler("used First Ability of item", iClient, sItemName);
	else Item_Handler("used Second Ability of item", iClient, sItemName);
}

public void EntWatch_OnPickUpItem(const char[] sItemName, int iClient)
{
	Item_Handler("has picked up item", iClient, sItemName);
}

public void EntWatch_OnDropItem(const char[] sItemName, int iClient)
{
	Item_Handler("has dropped item", iClient, sItemName);
}

public void EntWatch_OnPlayerDisconnectWithItem(const char[] sItemName, int iClient)
{
	Item_Handler("disconnected with item", iClient, sItemName);
}

public void EntWatch_OnPlayerDeathWithItem(const char[] sItemName, int iClient)
{
	Item_Handler("has died with item", iClient, sItemName);
}

public void EntWatch_OnCfgLoading(const char[] sFileName)
{
	char sMessage[PLATFORM_MAX_PATH+8];
	FormatEx(sMessage, sizeof(sMessage), "Loading %s", sFileName);
	System_Handler(sMessage, true);
}

public void EntWatch_OnCfgNotFound(const char[] sFileName)
{
	char sMessage[PLATFORM_MAX_PATH+15];
	FormatEx(sMessage, sizeof(sMessage), "Could not load %s", sFileName);
	System_Handler(sMessage, false);
}

public void EntWatch_OnSchemeNotFound(const char[] sFileName)
{
	char sMessage[PLATFORM_MAX_PATH+30];
	FormatEx(sMessage, sizeof(sMessage), "Don't open file to keyvalues: %s", sFileName);
	System_Handler(sMessage, false);
}

public void EntWatch_OnDatabaseFailure(const char[] sError)
{
	char sMessage[256];
	FormatEx(sMessage, sizeof(sMessage), "[EBan DB] Database failure: %s, ReConnect after 60 sec", sError);
	System_Handler(sMessage, false);
}

public void EntWatch_OnDatabaseSuccess()
{
	System_Handler("[EBan DB] Successful connection to DB", true);
}

public void EntWatch_OnDatabaseUnknownDriver(const char[] sDriverDB)
{
	char sMessage[128];
	FormatEx(sMessage, sizeof(sMessage), "[EBan DB] Unknown Driver: %s, cannot create tables.", sDriverDB);
	System_Handler(sMessage, false);
}

public void EntWatch_OnDatabaseReady()
{
	System_Handler("[EBan DB] DB Ready", true);
}

public void EntWatch_OnDatabaseCreateTablesError(const char[] sError)
{
	char sMessage[256];
	FormatEx(sMessage, sizeof(sMessage), "[EBan DB] SQL CreateTables Error: %s", sError);
	System_Handler(sMessage, false);
}

public void EntWatch_OnDatabaseBanError(const char[] sError)
{
	char sMessage[256];
	FormatEx(sMessage, sizeof(sMessage), "[EBan DB] SQL CallBack Ban Error: %s", sError);
	System_Handler(sMessage, false);
}

public void EntWatch_OnDatabaseBanSuccess()
{
	System_Handler("[EBan DB] Successful Ban", true);
}

public void EntWatch_OnDatabaseUnBanError(const char[] sError)
{
	char sMessage[256];
	FormatEx(sMessage, sizeof(sMessage), "[EBan DB] SQL CallBack UnBan Error: %s", sError);
	System_Handler(sMessage, false);
}

public void EntWatch_OnDatabaseUnBanSuccess()
{
	System_Handler("[EBan DB] Successful UnBan", true);
}

public void EntWatch_OnDatabaseBanUpdateError(const char[] sError)
{
	char sMessage[256];
	FormatEx(sMessage, sizeof(sMessage), "[EBan DB] SQL CallBack Update Error: %s", sError);
	System_Handler(sMessage, false);
}

public void EntWatch_OnDatabaseOfflineUnBanUpdateError(const char[] sError)
{
	char sMessage[256];
	FormatEx(sMessage, sizeof(sMessage), "[EBan DB] SQL CallBack Offline Update Error: %s", sError);
	System_Handler(sMessage, false);
}

public void EntWatch_OnOfflineBanError(const char[] sError)
{
	char sMessage[256];
	FormatEx(sMessage, sizeof(sMessage), "[EBan Offline DB] SQL CallBack Offline Ban Error: %s", sError);
	System_Handler(sMessage, false);
}

public void EntWatch_OnOfflineBanSuccess()
{
	System_Handler("[EBan Offline DB] Successful Offline Ban", true);
}

public void EntWatch_OnAdminSpawnItem(int iAdmin, const char[] sItemName, int iTarget)
{
	Admin_Other_Handler("spawn item", iAdmin, iTarget, -1, sItemName, "on player");
}

public void EntWatch_OnAdminTransferedAllItems(int iAdmin, int iTarget, int iReceiver)
{
	Admin_Other_Handler("transfered all items from", iAdmin, iTarget, iReceiver, "", "to");
}

public void EntWatch_OnAdminTransferedItem(int iAdmin, const char[] sItemName, int iReceiver)
{
	Admin_Other_Handler("transfered", iAdmin, -1, iReceiver, sItemName, "to");
}

public void EntWatch_OnClientBanned(int iAdmin, int iDuration, int iClient, const char[] sReason)
{
	char sClientName[32], sActionTime[24];
	GetClientName(iClient, sClientName, sizeof(sClientName));
	if(iDuration == -1) FormatEx(sActionTime, sizeof(sActionTime), "temporary");
	else if (iDuration == 0) FormatEx(sActionTime, sizeof(sActionTime), "permanently");
	else FormatEx(sActionTime, sizeof(sActionTime), "for %d minutes", iDuration);
	Admin_Eban_Handler("restricted", iAdmin, sClientName, g_sSteamIDs[iClient], sActionTime, sReason);
}

public void EntWatch_OnClientUnbanned(int iAdmin, int iClient, const char[] sReason)
{
	char sClientName[32];
	GetClientName(iClient, sClientName, sizeof(sClientName));
	Admin_Eban_Handler("unrestricted", iAdmin, sClientName, g_sSteamIDs[iClient], "", sReason);
}

public void EntWatch_OnClientOfflineBanned(int iAdmin, int iDuration, const char[] sClientName, const char[] sClientSteamID, const char[] sReason)
{
	char sActionTime[24];
	if(iDuration == 0) FormatEx(sActionTime, sizeof(sActionTime), "permanently");
	else FormatEx(sActionTime, sizeof(sActionTime), "for %d minutes", iDuration);
	Admin_Eban_Handler("offline restricted", iAdmin, sClientName, sClientSteamID, sActionTime, sReason);
}
