#pragma semicolon 1
#pragma newdecls required
#pragma dynamic 131072
#include <sourcemod>
#include <sdktools>
#include <sdkhooks>
#include <cstrike>
#include <clientprefs>
#include <smlib>
//Priority: 1.csgocolors_fix - 85 kB, 2. multicolors - 93 kB, 3. morecolors - 89 kB
#tryinclude <csgocolors_fix>
#if !defined _csgocolors_included
#tryinclude <multicolors>
#endif
#if !defined _csgocolors_included && !defined _multicolors_included
#include <morecolors>
#endif

#include "entwatch/function.inc"

ArrayList g_ItemConfig;
ArrayList g_ItemList;
class_Scheme g_SchemeConfig;

EngineVersion g_evGameEngine;

//-------------------------------------------------------
// Purpose: Plugin settings
//-------------------------------------------------------
ConVar	g_hCvar_TeamOnly,
		g_hCvar_Delay_Use,
		g_hCvar_Scheme,
		g_hCvar_BlockEPick,
		g_hCvar_GlobalBlock;

//-------------------------------------------------------
// Purpose: Plugin Local settings
//-------------------------------------------------------
bool	g_bTeamOnly = true,
		g_bBlockEPick = true,
		g_bGlobalBlock = false;
float	g_fDelayUse = 3.0;

//-------------------------------------------------------
// Purpose: Plugin Variables
//-------------------------------------------------------
bool g_bConfigLoaded = false;
bool g_bIsAdmin[MAXPLAYERS+1] = {false,...};
char g_sSteamIDs[MAXPLAYERS+1][32];
char g_sSteamIDs_short[MAXPLAYERS+1][32];
int  g_iUserIDs[MAXPLAYERS+1];

//Using DynamicChannels: https://github.com/Vauff/DynamicChannels
#tryinclude <DynamicChannels>

//Modules can be included as you wish. To do this, comment out or uncomment the corresponding module
#include "entwatch/module_forwards.inc" //For the include EntWatch.inc to work correctly, use with module_eban and module_database
#include "entwatch/module_chat.inc"
#include "entwatch/module_hud.inc"
#include "entwatch/module_eban.inc"
#include "entwatch/module_offline_eban.inc" // Need module_eban. Experimental
#include "entwatch/module_highlight.inc"
#include "entwatch/module_natives.inc" //For the include EntWatch.inc to work correctly, use with module_eban
#include "entwatch/module_transfer.inc"
#include "entwatch/module_spawn_item.inc"
#include "entwatch/module_menu.inc"
#include "entwatch/module_blink.inc" //glow for CS:S
//#include "entwatch/module_glow.inc" //change to HighLight
#include "entwatch/module_use_priority.inc"
//#include "entwatch/module_clantag.inc"

//#include "entwatch/module_physbox.inc" //Heavy module for the server. Not recommended. Need Collision Hook Ext https://forums.alliedmods.net/showthread.php?t=197815
//#include "entwatch/module_debug.inc"
//End Section Modules

ArrayList g_TriggerArray;

public Plugin myinfo = 
{
	name = "EntWatch",
	author = "DarkerZ[RUS], AgentWesker, notkoen, sTc2201, maxime1907",
	description = "Notify players about entity interactions.",
	version = "3.DZ.46",
	url = "dark-skill.ru"
};
 
public void OnPluginStart()
{
	if(g_ItemConfig == INVALID_HANDLE) g_ItemConfig = new ArrayList(512);
	if(g_ItemList == INVALID_HANDLE) g_ItemList = new ArrayList(512);
	
	if(g_TriggerArray == INVALID_HANDLE) g_TriggerArray = new ArrayList(512);
	
	g_evGameEngine = GetEngineVersion();
	
	#if defined EW_MODULE_PHYSBOX
	EWM_Physbox_OnPluginStart();
	#endif
	
	//CVARs
	g_hCvar_TeamOnly		= CreateConVar("entwatch_mode_teamonly", "1", "Enable/Disable team only mode.", _, true, 0.0, true, 1.0);
	g_hCvar_Delay_Use		= CreateConVar("entwatch_delay_use", "3.0", "Change delay before use", _, true, 0.0, true, 60.0);
	g_hCvar_Scheme			= CreateConVar("entwatch_scheme", "classic", "The name of the scheme config.", _);
	g_hCvar_BlockEPick		= CreateConVar("entwatch_blockepick", "1", "Block players from using E key to grab items.", _, true, 0.0, true, 1.0);
	g_hCvar_GlobalBlock		= CreateConVar("entwatch_globalblock", "0", "Blocks the pickup of any items by players.", _, true, 0.0, true, 1.0);
	
	//Commands
	RegAdminCmd("sm_ew_reloadconfig", EW_Command_ReloadConfig, ADMFLAG_CONFIG);
	RegAdminCmd("sm_setcooldown", EW_Command_Cooldown, ADMFLAG_BAN);
	RegAdminCmd("sm_setmaxuses", EW_Command_Setmaxuses, ADMFLAG_BAN);
	RegAdminCmd("sm_addmaxuses", EW_Command_Addmaxuses, ADMFLAG_BAN);
	RegAdminCmd("sm_ewsetmode", EW_Command_Setmode, ADMFLAG_BAN);
	
	RegAdminCmd("sm_setcooldown2", EW_Command_Cooldown2, ADMFLAG_BAN);
	RegAdminCmd("sm_setmaxuses2", EW_Command_Setmaxuses2, ADMFLAG_BAN);
	RegAdminCmd("sm_addmaxuses2", EW_Command_Addmaxuses2, ADMFLAG_BAN);
	RegAdminCmd("sm_ewsetmode2", EW_Command_Setmode2, ADMFLAG_BAN);
	
	RegAdminCmd("sm_ewsetname", EW_Command_Setname, ADMFLAG_BAN);
	RegAdminCmd("sm_ewsetshortname", EW_Command_Setshortname, ADMFLAG_BAN);
	
	RegAdminCmd("sm_ewblock", EW_Command_BlockItem, ADMFLAG_BAN);
	RegAdminCmd("sm_ewlockbutton", EW_Command_LockButton, ADMFLAG_BAN);
	RegAdminCmd("sm_ewlockbutton2", EW_Command_LockButton2, ADMFLAG_BAN);
	
	//Hook CVARs
	HookConVarChange(g_hCvar_TeamOnly, Cvar_Main_Changed);
	HookConVarChange(g_hCvar_Delay_Use, Cvar_Main_Changed);
	HookConVarChange(g_hCvar_BlockEPick, Cvar_Main_Changed);
	HookConVarChange(g_hCvar_GlobalBlock, Cvar_Main_Changed);
	
	//Fix for plugin reload (?)
	g_bTeamOnly = GetConVarBool(g_hCvar_TeamOnly);
	g_fDelayUse = GetConVarFloat(g_hCvar_Delay_Use);
	g_bBlockEPick = GetConVarBool(g_hCvar_BlockEPick);
	g_bGlobalBlock = GetConVarBool(g_hCvar_GlobalBlock);
	
	//Hook Events
	HookEvent("round_start", Event_RoundStart, EventHookMode_Pre);
	HookEvent("round_end", Event_RoundEnd, EventHookMode_Pre);
	HookEvent("player_death", Event_PlayerDeath, EventHookMode_Pre);
	HookEvent("player_team", Event_PlayerTeam, EventHookMode_Pre);
	
	//Hook Output Right-Click
	HookEntityOutput("game_ui", "PressedAttack2", Event_GameUI_RightClick);
	
	//Hook Output OutValue
	HookEntityOutput("math_counter", "OutValue", Event_OutValue);
	
	#if defined EW_MODULE_FORWARDS
	EWM_Forwards_OnPluginStart();
	#endif
	
	//Load Scheme
	LoadScheme();
	
	#if defined EW_MODULE_EBAN
	EWM_Eban_OnPluginStart();
	#endif
	#if defined EW_MODULE_OFFLINE_EBAN
	EWM_OfflineEban_OnPluginStart();
	#endif
	#if defined EW_MODULE_TRANSFER
	EWM_Transfer_OnPluginStart();
	#endif
	#if defined EW_MODULE_SPAWN
	EWM_Spawn_OnPluginStart();
	#endif
	#if defined EW_MODULE_MENU
	EWM_Menu_OnPluginStart();
	#endif
	#if defined EW_MODULE_CHAT
	EWM_Chat_OnPluginStart();
	#endif
	#if defined EW_MODULE_HUD
	EWM_Hud_OnPluginStart();
	#endif
	#if defined EW_MODULE_BLINK
	EWM_Blink_OnPluginStart();
	#endif
	#if defined EW_MODULE_GLOW
	EWM_Glow_OnPluginStart();
	#endif
	#if defined EW_MODULE_HIGHLIGHT
	EWM_HLight_OnPluginStart();
	#endif
	#if defined EW_MODULE_DEBUG
	EWM_Debug_OnPluginStart();
	#endif
	#if defined EW_MODULE_USE_PRIORITY
	EWM_Use_Priority_OnPluginStart();
	#endif
	
	#if defined EW_MODULE_CLANTAG
	EWM_Clantag_OnPluginStart();
	#endif
	
	LoadTranslations("EntWatch_DZ.phrases");
	LoadTranslations("common.phrases");
	
	AutoExecConfig(true, "EntWatch_DZ");
	
	#if defined EW_MODULE_NATIVES
	EWM_Natives_OnPluginStart();
	#endif
}

public void OnPluginEnd()
{
	#if defined EW_MODULE_CLANTAG
	EWM_Clantag_Mass_Reset();
	#endif
}

public void Cvar_Main_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_TeamOnly)
		g_bTeamOnly = GetConVarBool(convar);
	else if(convar==g_hCvar_Delay_Use)
		g_fDelayUse = GetConVarFloat(convar);
	else if (convar == g_hCvar_BlockEPick)
		g_bBlockEPick = GetConVarBool(convar);
	else if (convar == g_hCvar_GlobalBlock)
		g_bGlobalBlock = GetConVarBool(convar);
}

public void OnMapStart()
{
	CleanData();
	LoadConfig();
	LoadScheme();
	#if defined EW_MODULE_EBAN
	EWM_Eban_OnMapStart();
	#endif
	#if defined EW_MODULE_BLINK
	EWM_Blink_OnMapStart();
	#endif
	#if defined EW_MODULE_GLOW
	EWM_Glow_OnMapStart();
	#endif
	#if defined EW_MODULE_HIGHLIGHT
	EWM_HLight_OnMapStart();
	#endif
	#if defined EW_MODULE_HUD
	EWM_Hud_OnMapStart();
	#endif
}

public void OnMapEnd()
{
	#if defined EW_MODULE_BLINK
	EWM_Blink_OnMapEnd();
	#endif
	#if defined EW_MODULE_GLOW
	EWM_Glow_OnMapEnd();
	#endif
	#if defined EW_MODULE_HIGHLIGHT
	EWM_HLight_OnMapEnd();
	#endif
	
	#if defined EW_MODULE_CLANTAG
	EWM_Clantag_Mass_Reset();
	#endif
}

public void Event_RoundStart(Event hEvent, const char[] sName, bool bDontBroadcast)
{
	if(g_bConfigLoaded) CPrintToChatAll("%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Welcome");
	
	#if defined EW_MODULE_CLANTAG
	EWM_Clantag_Mass_Reset();
	#endif
	#if defined EW_MODULE_HIGHLIGHT
	EWM_HLight_RoundStart();
	#endif
}

public void Event_RoundEnd(Event hEvent, const char[] sName, bool bDontBroadcast)
{
	if(g_bConfigLoaded) 
	{
		//Unhook Buttons
		for(int i = 0; i < g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			for(int j = 0; j < ItemTest.ButtonsArray.Length; j++)
			{
				int CurrentButton = ItemTest.ButtonsArray.Get(j);
				SDKUnhook(CurrentButton, SDKHook_Use, OnButtonUse);
			}
			SDKUnhook(ItemTest.WeaponID, SDKHook_SpawnPost, OnItemSpawned);
			ItemTest.ClearHandles();
		}
		//Unhook Triggers
		for(int i = 0; i < g_TriggerArray.Length; i++)
		{
			int iEntity = g_TriggerArray.Get(i);
			SDKUnhook(iEntity, SDKHook_Touch, OnTrigger);
			SDKUnhook(iEntity, SDKHook_EndTouch, OnTrigger);
			SDKUnhook(iEntity, SDKHook_StartTouch, OnTrigger);
		}
		g_ItemList.Clear();
		g_TriggerArray.Clear();
		#if defined EW_MODULE_PHYSBOX
		EWM_Physbox_Event_RoundEnd();
		#endif
	}
}

public Action Event_PlayerTeam(Event hEvent, const char[] sName, bool bDontBroadcast)
{
	int team = GetEventInt(hEvent, "team");
	
	if (team != 1) return Plugin_Continue;
	
	EWM_Drop_Forward(hEvent); //Re-use code for death and team changes
	
	return Plugin_Continue;
}

public Action Event_PlayerDeath(Event hEvent, const char[] sName, bool bDontBroadcast)
{
	EWM_Drop_Forward(hEvent); //Re-use code for death and team changes
	return Plugin_Continue;
}

stock void EWM_Drop_Forward(Handle hEvent)
{
	int iClient = GetClientOfUserId(GetEventInt(hEvent, "userid"));

	if (g_bConfigLoaded)
	{
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.OwnerID == iClient)
			{
				ItemTest.OwnerID = INVALID_ENT_REFERENCE;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				
				#if defined EW_MODULE_BLINK
				EWM_Blink_BlinkWeapon(ItemTest);
				#endif
				
				#if defined EW_MODULE_GLOW
				EWM_Glow_GlowWeapon(ItemTest, i, false);
				#endif
				
				#if defined EW_MODULE_HIGHLIGHT
				if(g_evGameEngine == Engine_CSGO) 
				{
					EWM_HLight_PRemove(iClient);
					EWM_HLight_Set(ItemTest);
				}
				#endif
				
				#if defined EW_MODULE_FORWARDS
				Call_StartForward(g_hOnPlayerDeathWithItem);
				Call_PushString(ItemTest.Name);
				Call_PushCell(iClient);
				Call_Finish();
				#endif
				
				if(IsValidEdict(ItemTest.WeaponID) && GetSlotCSGO(ItemTest.WeaponID) != -1)
				{
					if(ItemTest.ForceDrop)
					{
						SDKHooks_DropWeapon(iClient, ItemTest.WeaponID);
						#if defined EW_MODULE_CHAT
						if(ItemTest.Chat) EWM_Chat_PlayerDeath_Drop(ItemTest, iClient);
						#endif
						
						#if defined EW_MODULE_CLANTAG
						EWM_Clantag_Reset(iClient);
						#endif
					}
					else
					{
						if(GetSlotCSGO(ItemTest.WeaponID) == 2)
						{
							#if defined EW_MODULE_CHAT
							if(ItemTest.Chat) EWM_Chat_PlayerDeath(ItemTest, iClient);
							#endif
							
							#if defined EW_MODULE_CLANTAG
							EWM_Clantag_Reset(iClient);
							#endif
							AcceptEntityInput(ItemTest.WeaponID, "Kill");
						}else
						{
							#if defined EW_MODULE_CHAT
							if(ItemTest.Chat) EWM_Chat_PlayerDeath_Drop(ItemTest, iClient);
							#endif
							
							#if defined EW_MODULE_CLANTAG
							EWM_Clantag_Reset(iClient);
							#endif
							
							SDKHooks_DropWeapon(iClient, ItemTest.WeaponID);
						}
					}
				}
			}
		}
	}
}

public void OnClientPutInServer(int iClient)
{
	SDKHook(iClient, SDKHook_WeaponDropPost, OnWeaponDrop);
	SDKHook(iClient, SDKHook_WeaponEquipPost, OnWeaponEquip);
	SDKHook(iClient, SDKHook_WeaponCanUse, OnWeaponCanUse);
	
	g_bIsAdmin[iClient] = false;
	char sSteamID[32];
	GetClientAuthId(iClient, AuthId_Steam2, sSteamID, sizeof(sSteamID));
	FormatEx(g_sSteamIDs[iClient], sizeof(g_sSteamIDs[]), "%s", sSteamID);
	FormatEx(g_sSteamIDs_short[iClient], sizeof(g_sSteamIDs_short[]), "%s", sSteamID);
	ReplaceString(g_sSteamIDs_short[iClient], sizeof(g_sSteamIDs_short[]), "STEAM_", "", true);
	g_iUserIDs[iClient] = GetClientUserId(iClient);
	
	#if defined EW_MODULE_EBAN
	EWM_Eban_OnClientPutInServer(iClient);
	#endif
	#if defined EW_MODULE_HUD
	if(!AreClientCookiesCached(iClient)) EWM_Hud_LoadDefaultClientSettings(iClient);
	#endif
	#if defined EW_MODULE_USE_PRIORITY
	if(!AreClientCookiesCached(iClient)) EWM_Use_Priority_LoadDefaultClientSettings(iClient);
	#endif
}

public void OnClientCookiesCached(int iClient)
{
	#if defined EW_MODULE_HUD
	EWM_Hud_OnClientCookiesCached(iClient);
	#endif
	#if defined EW_MODULE_USE_PRIORITY
	EWM_Use_Priority_OnClientCookiesCached(iClient);
	#endif
}

public void OnClientPostAdminCheck(int iClient)
{
	if(!IsValidClient(iClient) || !IsClientConnected(iClient) || IsFakeClient(iClient)) return;
	#if defined EW_MODULE_OFFLINE_EBAN
	EWM_OfflineEban_OnClientPostAdminCheck(iClient);
	#endif
	int iFlags = GetUserFlagBits(iClient);
	if(iFlags & ADMFLAG_KICK || iFlags & ADMFLAG_ROOT) g_bIsAdmin[iClient] = true;
}

public void OnClientDisconnect(int iClient)
{
	if(g_bConfigLoaded)
	{
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.OwnerID == iClient)
			{
				ItemTest.OwnerID = INVALID_ENT_REFERENCE;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				
				#if defined EW_MODULE_FORWARDS
				Call_StartForward(g_hOnPlayerDisconnectWithItem);
				Call_PushString(ItemTest.Name);
				Call_PushCell(iClient);
				Call_Finish();
				#endif
				if(IsValidEdict(ItemTest.WeaponID) && GetSlotCSGO(ItemTest.WeaponID) != -1)
				{
					if(ItemTest.ForceDrop)
					{
						SDKHooks_DropWeapon(iClient, ItemTest.WeaponID);
						#if defined EW_MODULE_CHAT
						if(ItemTest.Chat) EWM_Chat_Disconnect_Drop(ItemTest, iClient);
						#endif
					}
					else
					{
						if(GetSlotCSGO(ItemTest.WeaponID) == 2)
						{
							#if defined EW_MODULE_CHAT
							if(ItemTest.Chat) EWM_Chat_Disconnect(ItemTest, iClient);
							#endif
							
							AcceptEntityInput(ItemTest.WeaponID, "Kill");
						}else
						{
							#if defined EW_MODULE_CHAT
							if(ItemTest.Chat) EWM_Chat_Disconnect_Drop(ItemTest, iClient);
							#endif
							
							SDKHooks_DropWeapon(iClient, ItemTest.WeaponID);
						}
					}
				}
				#if defined EW_MODULE_BLINK
				EWM_Blink_BlinkWeapon(ItemTest);
				#endif
				
				#if defined EW_MODULE_GLOW
				EWM_Glow_GlowWeapon(ItemTest, i, false);
				#endif
				
				#if defined EW_MODULE_HIGHLIGHT
				if(g_evGameEngine == Engine_CSGO) EWM_HLight_Set(ItemTest);
				#endif
			}
		}
	}
	
	g_bIsAdmin[iClient] = false;
	
	#if defined EW_MODULE_EBAN
	EWM_Eban_OnClientDisconnect(iClient);
	#endif
	#if defined EW_MODULE_OFFLINE_EBAN
	EWM_OfflineEban_OnClientDisconnect(iClient);
	#endif
	#if defined EW_MODULE_HUD
	EWM_Hud_LoadDefaultClientSettings(iClient);
	#endif
	#if defined EW_MODULE_CLANTAG
	EWM_Clantag_OnClientDisconnect(iClient);
	#endif
	#if defined EW_MODULE_HIGHLIGHT
	EWM_HLight_OnClientPrivilegeReset(iClient);
	#endif
	
	FormatEx(g_sSteamIDs[iClient], sizeof(g_sSteamIDs[]), "");
	FormatEx(g_sSteamIDs_short[iClient], sizeof(g_sSteamIDs_short[]), "");
	g_iUserIDs[iClient] = -1;
	//SDKHooks automatically handles unhooking on disconnect
	/*SDKUnhook(iClient, SDKHook_WeaponDropPost, OnWeaponDrop);
	SDKUnhook(iClient, SDKHook_WeaponEquipPost, OnWeaponEquip);
	SDKUnhook(iClient, SDKHook_WeaponCanUse, OnWeaponCanUse);*/
}

public void EntWatch_OnDBConnected()
{
	#if defined EW_MODULE_EBAN
	EWM_Eban_CleanData_All();
	#endif
}

void CleanData()
{
	if(g_ItemList != null)
	{
		class_ItemList ItemTest;
		for(int i = 0; i < g_ItemList.Length; i++)
		{
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			ItemTest.ClearHandles();
		}
		g_ItemList.Clear();
		CloseHandle(g_ItemList);
	}
	g_ItemList = new ArrayList(512);
	
	if(g_TriggerArray != null)
	{
		g_TriggerArray.Clear();
		CloseHandle(g_TriggerArray);
	}
	g_TriggerArray = new ArrayList(512);
	
	if(g_ItemConfig != null)
	{
		g_ItemConfig.Clear();
		CloseHandle(g_ItemConfig);
	}
	g_ItemConfig = new ArrayList(512);
	
	#if defined EW_MODULE_PHYSBOX
	EWM_Physbox_CleanData();
	#endif
}

stock void LoadConfig()
{
	Handle hKeyValues = CreateKeyValues("entities");
	char sBuffer_map[128], sBuffer_path[PLATFORM_MAX_PATH], sBuffer_path_override[PLATFORM_MAX_PATH], sBuffer_temp[32];

	GetCurrentMap(sBuffer_map, sizeof(sBuffer_map));
	FormatEx(sBuffer_path, sizeof(sBuffer_path), "cfg/sourcemod/entwatch/maps/%s.cfg", sBuffer_map);
	FormatEx(sBuffer_path_override, sizeof(sBuffer_path_override), "cfg/sourcemod/entwatch/maps/%s_override.cfg", sBuffer_map);
	// If there is an override config then load it
	if(FileExists(sBuffer_path_override))
	{
		FileToKeyValues(hKeyValues, sBuffer_path_override);
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnCfgLoading);
		Call_PushString(sBuffer_path_override);
		Call_Finish();
		#endif
	}else
	{
		FileToKeyValues(hKeyValues, sBuffer_path);
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnCfgLoading);
		Call_PushString(sBuffer_path);
		Call_Finish();
		#endif
	}
	
	KvRewind(hKeyValues);
	if(KvGotoFirstSubKey(hKeyValues))
	{
		do
		{
			class_ItemConfig NewItem;
			KvGetString(hKeyValues, "name", sBuffer_temp, sizeof(sBuffer_temp), "");
			FormatEx(NewItem.Name, sizeof(NewItem.Name), "%s", sBuffer_temp);

			KvGetString(hKeyValues, "shortname", sBuffer_temp, sizeof(sBuffer_temp), "");
			FormatEx(NewItem.ShortName, sizeof(NewItem.ShortName), "%s", sBuffer_temp);

			KvGetString(hKeyValues, "color", sBuffer_temp, sizeof(sBuffer_temp), "");
			FormatEx(NewItem.Color, sizeof(NewItem.Color), "%s", sBuffer_temp);

			NewItem.GlowColor[0]=255;
			NewItem.GlowColor[1]=255;
			NewItem.GlowColor[2]=255;
			NewItem.GlowColor[3]=200;
			
			#if defined EW_MODULE_GLOW || defined EW_MODULE_HIGHLIGHT || defined EW_MODULE_BLINK
			if(strcmp(sBuffer_temp,"{green}",false)==0){NewItem.GlowColor[0]=0;NewItem.GlowColor[1]=255;NewItem.GlowColor[2]=0;}
			else if(strcmp(sBuffer_temp,"{default}",false)==0){NewItem.GlowColor[0]=255;NewItem.GlowColor[1]=255;NewItem.GlowColor[2]=255;}
			else if(strcmp(sBuffer_temp,"{darkred}",false)==0){NewItem.GlowColor[0]=175;NewItem.GlowColor[1]=0;NewItem.GlowColor[2]=0;}
			else if(strcmp(sBuffer_temp,"{purple}",false)==0){NewItem.GlowColor[0]=128;NewItem.GlowColor[1]=0;NewItem.GlowColor[2]=128;}
			else if(strcmp(sBuffer_temp,"{lightgreen}",false)==0){NewItem.GlowColor[0]=104;NewItem.GlowColor[1]=238;NewItem.GlowColor[2]=104;}
			else if(strcmp(sBuffer_temp,"{lime}",false)==0){NewItem.GlowColor[0]=119;NewItem.GlowColor[1]=234;NewItem.GlowColor[2]=7;}
			else if(strcmp(sBuffer_temp,"{red}",false)==0){NewItem.GlowColor[0]=255;NewItem.GlowColor[1]=30;NewItem.GlowColor[2]=30;}
			else if(strcmp(sBuffer_temp,"{grey}",false)==0){NewItem.GlowColor[0]=128;NewItem.GlowColor[1]=128;NewItem.GlowColor[2]=128;}
			else if(strcmp(sBuffer_temp,"{olive}",false)==0){NewItem.GlowColor[0]=112;NewItem.GlowColor[1]=130;NewItem.GlowColor[2]=56;}
			else if(strcmp(sBuffer_temp,"{a}",false)==0){NewItem.GlowColor[0]=192;NewItem.GlowColor[1]=192;NewItem.GlowColor[2]=192;}
			else if(strcmp(sBuffer_temp,"{lightblue}",false)==0){NewItem.GlowColor[0]=93;NewItem.GlowColor[1]=130;NewItem.GlowColor[2]=255;}
			else if(strcmp(sBuffer_temp,"{blue}",false)==0){NewItem.GlowColor[0]=0;NewItem.GlowColor[1]=0;NewItem.GlowColor[2]=255;}
			else if(strcmp(sBuffer_temp,"{d}",false)==0){NewItem.GlowColor[0]=102;NewItem.GlowColor[1]=153;NewItem.GlowColor[2]=204;}
			else if(strcmp(sBuffer_temp,"{pink}",false)==0){NewItem.GlowColor[0]=255;NewItem.GlowColor[1]=105;NewItem.GlowColor[2]=180;}
			else if(strcmp(sBuffer_temp,"{darkorange}",false)==0){NewItem.GlowColor[0]=240;NewItem.GlowColor[1]=94;NewItem.GlowColor[2]=35;}
			else if(strcmp(sBuffer_temp,"{orange}",false)==0){NewItem.GlowColor[0]=255;NewItem.GlowColor[1]=140;NewItem.GlowColor[2]=0;}
			else if(strcmp(sBuffer_temp,"{white}",false)==0){NewItem.GlowColor[0]=255;NewItem.GlowColor[1]=255;NewItem.GlowColor[2]=255;}
			else if(strcmp(sBuffer_temp,"{yellow}",false)==0){NewItem.GlowColor[0]=199;NewItem.GlowColor[1]=234;NewItem.GlowColor[2]=7;}
			else if(strcmp(sBuffer_temp,"{magenta}",false)==0){NewItem.GlowColor[0]=255;NewItem.GlowColor[1]=105;NewItem.GlowColor[2]=180;}
			else if(strcmp(sBuffer_temp,"{silver}",false)==0){NewItem.GlowColor[0]=192;NewItem.GlowColor[1]=192;NewItem.GlowColor[2]=192;}
			else if(strcmp(sBuffer_temp,"{bluegrey}",false)==0){NewItem.GlowColor[0]=102;NewItem.GlowColor[1]=153;NewItem.GlowColor[2]=204;}
			else if(strcmp(sBuffer_temp,"{lightred}",false)==0){NewItem.GlowColor[0]=255;NewItem.GlowColor[1]=90;NewItem.GlowColor[2]=90;}
			else if(strcmp(sBuffer_temp,"{cyan}",false)==0){NewItem.GlowColor[0]=0;NewItem.GlowColor[1]=150;NewItem.GlowColor[2]=220;}
			else if(strcmp(sBuffer_temp,"{gray}",false)==0){NewItem.GlowColor[0]=128;NewItem.GlowColor[1]=128;NewItem.GlowColor[2]=128;}
			#endif
			
			KvGetString(hKeyValues, "buttonclass", sBuffer_temp, sizeof(sBuffer_temp), "");
			FormatEx(NewItem.ButtonClass, sizeof(NewItem.ButtonClass), "%s", sBuffer_temp);
			
			KvGetString(hKeyValues, "filtername", sBuffer_temp, sizeof(sBuffer_temp), "");
			FormatEx(NewItem.FilterName, sizeof(NewItem.FilterName), "%s", sBuffer_temp);
			
			KvGetString(hKeyValues, "blockpickup", sBuffer_temp, sizeof(sBuffer_temp), "false");
			NewItem.BlockPickup = StrEqual(sBuffer_temp, "true", false);
			
			KvGetString(hKeyValues, "allowtransfer", sBuffer_temp, sizeof(sBuffer_temp), "false");
			NewItem.AllowTransfer = StrEqual(sBuffer_temp, "true", false);
			
			KvGetString(hKeyValues, "forcedrop", sBuffer_temp, sizeof(sBuffer_temp), "false");
			NewItem.ForceDrop = StrEqual(sBuffer_temp, "true", false);
			
			KvGetString(hKeyValues, "chat", sBuffer_temp, sizeof(sBuffer_temp), "false");
			NewItem.Chat = StrEqual(sBuffer_temp, "true", false);
			
			KvGetString(hKeyValues, "chat_uses", sBuffer_temp, sizeof(sBuffer_temp), "false");
			NewItem.Chat_Uses = StrEqual(sBuffer_temp, "true", false);
			
			KvGetString(hKeyValues, "hud", sBuffer_temp, sizeof(sBuffer_temp), "false");
			NewItem.Hud = StrEqual(sBuffer_temp, "true", false);
			
			KvGetString(hKeyValues, "hammerid", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.HammerID = StringToInt(sBuffer_temp);
			
			KvGetString(hKeyValues, "energyid", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.EnergyID = StringToInt(sBuffer_temp);
			
			KvGetString(hKeyValues, "mode", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.Mode = StringToInt(sBuffer_temp);
			
			KvGetString(hKeyValues, "maxuses", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.MaxUses = StringToInt(sBuffer_temp);
			
			KvGetString(hKeyValues, "cooldown", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.CoolDown = StringToInt(sBuffer_temp);
			
			if(!StrEqual(NewItem.ButtonClass, "game_ui"))
			{
				KvGetString(hKeyValues, "buttonid", sBuffer_temp, sizeof(sBuffer_temp), "0");
				NewItem.ButtonID = StringToInt(sBuffer_temp);
			}else
			{
				NewItem.ButtonID = -5;
			}
			
			KvGetString(hKeyValues, "trigger", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.Trigger = StringToInt(sBuffer_temp);
			
			KvGetString(hKeyValues, "pt_spawner", sBuffer_temp, sizeof(sBuffer_temp), "");
			FormatEx(NewItem.Spawner, sizeof(NewItem.Spawner), "%s", sBuffer_temp);
			
			KvGetString(hKeyValues, "physbox", sBuffer_temp, sizeof(sBuffer_temp), "false");
			NewItem.PhysBox = StrEqual(sBuffer_temp, "true", false);
			
			KvGetString(hKeyValues, "use_priority", sBuffer_temp, sizeof(sBuffer_temp), "true");
			NewItem.UsePriority = StrEqual(sBuffer_temp, "true", false);
			
			//Second Button
			KvGetString(hKeyValues, "buttonclass2", sBuffer_temp, sizeof(sBuffer_temp), "");
			FormatEx(NewItem.ButtonClass2, sizeof(NewItem.ButtonClass2), "%s", sBuffer_temp);

			KvGetString(hKeyValues, "energyid2", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.EnergyID2 = StringToInt(sBuffer_temp);

			KvGetString(hKeyValues, "mode2", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.Mode2 = StringToInt(sBuffer_temp);

			KvGetString(hKeyValues, "maxuses2", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.MaxUses2 = StringToInt(sBuffer_temp);

			KvGetString(hKeyValues, "cooldown2", sBuffer_temp, sizeof(sBuffer_temp), "0");
			NewItem.CoolDown2 = StringToInt(sBuffer_temp);

			if(!StrEqual(NewItem.ButtonClass2, "game_ui"))
			{
				KvGetString(hKeyValues, "buttonid2", sBuffer_temp, sizeof(sBuffer_temp), "0");
				NewItem.ButtonID2 = StringToInt(sBuffer_temp);
			}else
			{
				NewItem.ButtonID2 = -5;
			}
			
			g_ItemConfig.PushArray(NewItem, sizeof(NewItem));
		} while (KvGotoNextKey(hKeyValues));
		g_bConfigLoaded = true;
	} else {
		g_bConfigLoaded = false;
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnCfgNotFound);
		Call_PushString(sBuffer_path);
		Call_Finish();
		#endif
	}
}

stock void LoadScheme()
{
	//SetDefault
	g_SchemeConfig.Color_Tag		= "{green}";
	g_SchemeConfig.Color_Name		= "{default}";
	g_SchemeConfig.Color_SteamID	= "{grey}";
	g_SchemeConfig.Color_Use		= "{lightblue}";
	g_SchemeConfig.Color_Pickup		= "{lime}";
	g_SchemeConfig.Color_Drop		= "{pink}";
	g_SchemeConfig.Color_Disconnect	= "{orange}";
	g_SchemeConfig.Color_Death		= "{orange}";
	g_SchemeConfig.Color_Warning	= "{orange}";
	g_SchemeConfig.Color_Enabled	= "{green}";
	g_SchemeConfig.Color_Disabled	= "{red}";
	g_SchemeConfig.Color_HUD[0]		= 255;
	g_SchemeConfig.Color_HUD[1]		= 255;
	g_SchemeConfig.Color_HUD[2]		= 255;
	g_SchemeConfig.Color_HUD[3]		= 255;
	g_SchemeConfig.Pos_HUD_X		= 0.0;
	g_SchemeConfig.Pos_HUD_Y		= 0.4;
	
	KeyValues KvConfig = CreateKeyValues("EW_Scheme");
	char	ConfigFullPath[PLATFORM_MAX_PATH],
			ConfigFile[16];
	GetConVarString(g_hCvar_Scheme, ConfigFile, sizeof(ConfigFile));
	FormatEx(ConfigFullPath, sizeof(ConfigFullPath), "cfg/sourcemod/entwatch/scheme/%s.cfg", ConfigFile);
	if(!FileToKeyValues(KvConfig, ConfigFullPath))
	{
		CloseHandle(KvConfig);
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnSchemeNotFound);
		Call_PushString(ConfigFullPath);
		Call_Finish();
		#endif
		return;
	}
	
	char szBuffer[64];
	KvConfig.Rewind();
	
	KvConfig.GetString("color_tag", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Tag, sizeof(g_SchemeConfig.Color_Tag), "%s", szBuffer);
	
	KvConfig.GetString("color_name", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Name, sizeof(g_SchemeConfig.Color_Name), "%s", szBuffer);
	
	KvConfig.GetString("color_steamid", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_SteamID, sizeof(g_SchemeConfig.Color_SteamID), "%s", szBuffer);
	
	KvConfig.GetString("color_use", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Use, sizeof(g_SchemeConfig.Color_Use), "%s", szBuffer);
	
	KvConfig.GetString("color_pickup", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Pickup, sizeof(g_SchemeConfig.Color_Pickup), "%s", szBuffer);
	
	KvConfig.GetString("color_drop", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Drop, sizeof(g_SchemeConfig.Color_Drop), "%s", szBuffer);
	
	KvConfig.GetString("color_disconnect", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Disconnect, sizeof(g_SchemeConfig.Color_Disconnect), "%s", szBuffer);
	
	KvConfig.GetString("color_death", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Death, sizeof(g_SchemeConfig.Color_Death), "%s", szBuffer);
	
	KvConfig.GetString("color_warning", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Warning, sizeof(g_SchemeConfig.Color_Warning), "%s", szBuffer);
	
	KvConfig.GetString("color_enabled", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Enabled, sizeof(g_SchemeConfig.Color_Enabled), "%s", szBuffer);
	
	KvConfig.GetString("color_disabled", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Color_Disabled, sizeof(g_SchemeConfig.Color_Disabled), "%s", szBuffer);
	
	#if defined EW_MODULE_EBAN || defined EW_MODULE_FORWARDS
	KvConfig.GetString("server_name", szBuffer, sizeof(szBuffer));
	if(!StrEqual(szBuffer,"")) FormatEx(g_SchemeConfig.Server_Name, sizeof(g_SchemeConfig.Server_Name), "%s", szBuffer);
		else FormatEx(g_SchemeConfig.Server_Name, sizeof(g_SchemeConfig.Server_Name), "Server");
	#endif
	#if defined EW_MODULE_HUD
	KvConfig.GetColor4("color_hud", g_SchemeConfig.Color_HUD);
	g_SchemeConfig.Pos_HUD_X = KvConfig.GetFloat("pos_hud_x");
	g_SchemeConfig.Pos_HUD_Y = KvConfig.GetFloat("pos_hud_y");
	#endif
	
	CloseHandle(KvConfig);
	#if defined EW_MODULE_FORWARDS
	Call_StartForward(g_hOnSchemeServerName);
	Call_PushString(g_SchemeConfig.Server_Name);
	Call_Finish();
	#endif
}

public bool RegisterItem(class_ItemConfig ItemConfig, int iEntity, int iHammerID)
{
	if (ItemConfig.HammerID && ItemConfig.HammerID == iHammerID)
	{
		//register New Item
		class_ItemList NewItem;
		FormatEx(NewItem.Name,			sizeof(NewItem.Name),			"%s",	ItemConfig.Name);
		FormatEx(NewItem.ShortName,		sizeof(NewItem.ShortName),		"%s",	ItemConfig.ShortName);
		FormatEx(NewItem.Color,			sizeof(NewItem.Color),			"%s",	ItemConfig.Color);
		FormatEx(NewItem.ButtonClass,	sizeof(NewItem.ButtonClass),	"%s",	ItemConfig.ButtonClass);
		FormatEx(NewItem.FilterName,	sizeof(NewItem.FilterName),		"%s",	ItemConfig.FilterName);
		NewItem.BlockPickup = ItemConfig.BlockPickup;
		NewItem.AllowTransfer = ItemConfig.AllowTransfer;
		NewItem.ForceDrop = ItemConfig.ForceDrop;
		NewItem.Chat = ItemConfig.Chat;
		NewItem.Chat_Uses = ItemConfig.Chat_Uses;
		NewItem.Hud = ItemConfig.Hud;
		NewItem.HammerID = ItemConfig.HammerID;
		
		if(ItemConfig.EnergyID==0) NewItem.EnergyID = INVALID_ENT_REFERENCE;
			else NewItem.EnergyID = ItemConfig.EnergyID;
		NewItem.MathID = INVALID_ENT_REFERENCE;
		NewItem.MathValue = -1;
		NewItem.MathValueMax = -1;
		
		NewItem.Mode = ItemConfig.Mode;
		NewItem.MaxUses = ItemConfig.MaxUses;
		NewItem.CoolDown = ItemConfig.CoolDown;
		NewItem.GlowColor[0] = ItemConfig.GlowColor[0];
		NewItem.GlowColor[1] = ItemConfig.GlowColor[1];
		NewItem.GlowColor[2] = ItemConfig.GlowColor[2];
		NewItem.GlowColor[3] = ItemConfig.GlowColor[3];
		
		NewItem.WeaponID = iEntity;
		NewItem.ButtonsArray = new ArrayList();
		
		NewItem.OwnerID = INVALID_ENT_REFERENCE;
		NewItem.CoolDownTime = -1.0;
		if(ItemConfig.ButtonID==0) NewItem.ButtonID = INVALID_ENT_REFERENCE;
			else NewItem.ButtonID = ItemConfig.ButtonID;
		
		//Second Button
		FormatEx(NewItem.ButtonClass2,	sizeof(NewItem.ButtonClass2),	"%s",	ItemConfig.ButtonClass2);
		if(ItemConfig.EnergyID2==0) NewItem.EnergyID2 = INVALID_ENT_REFERENCE;
			else NewItem.EnergyID2 = ItemConfig.EnergyID2;
		NewItem.MathID2 = INVALID_ENT_REFERENCE;
		NewItem.MathValue2 = -1;
		NewItem.MathValueMax2 = -1;
		
		NewItem.Mode2 = ItemConfig.Mode2;
		NewItem.MaxUses2 = ItemConfig.MaxUses2;
		NewItem.CoolDown2 = ItemConfig.CoolDown2;
		
		if(ItemConfig.ButtonID2==0) NewItem.ButtonID2 = INVALID_ENT_REFERENCE;
			else NewItem.ButtonID2 = ItemConfig.ButtonID2;
		
		UpdateTime();
		NewItem.SetDelay(g_fDelayUse);
		NewItem.GlowEnt = INVALID_ENT_REFERENCE;
		
		NewItem.PhysBox = ItemConfig.PhysBox;
		NewItem.UsePriority = ItemConfig.UsePriority;
		NewItem.Team = -1;
		NewItem.LockButton = false;
		NewItem.LockButton2 = false;
		//PrintToServer("[EW]Item Spawned: %s |%i", NewItem.ShortName, iEntity);
		
		g_ItemList.PushArray(NewItem, sizeof(NewItem));
		
		#if defined EW_MODULE_BLINK
		EWM_Blink_BlinkWeapon(NewItem);
		#endif
		
		#if defined EW_MODULE_GLOW
		if(g_bGlow_Spawn)
			for(int i = 0; i<g_ItemList.Length; i++)
			{
				class_ItemList ItemTest;
				g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
				if(ItemTest.WeaponID == iEntity)
				{
					EWM_Glow_GlowWeapon(ItemTest, i, true);
					break;
				}
			}
		#endif
		
		return true;
	}
	return false;
}

public bool RegisterButton(class_ItemList ItemInstance, int iEntity)
{
	if(IsValidEntity(ItemInstance.WeaponID))
	{
		char Item_Weapon_Targetname[64], Item_Weapon_Parent[64];
		Entity_GetTargetName(ItemInstance.WeaponID, Item_Weapon_Targetname, sizeof(Item_Weapon_Targetname));
		Entity_GetParentName(iEntity, Item_Weapon_Parent, sizeof(Item_Weapon_Parent));
		if (!StrEqual(Item_Weapon_Targetname,"") && StrEqual(Item_Weapon_Targetname, Item_Weapon_Parent))
		{
			if(ItemInstance.ButtonID == INVALID_ENT_REFERENCE) ItemInstance.ButtonID = Entity_GetHammerId(iEntity); //Default the first button spawned will be the main button. Need to module use_priority
			else if(ItemInstance.ButtonID2 == INVALID_ENT_REFERENCE) ItemInstance.ButtonID2 = Entity_GetHammerId(iEntity); //May be Second Button?
			SDKHookEx(iEntity, SDKHook_Use, OnButtonUse);
			ItemInstance.ButtonsArray.Push(iEntity);
			return true;
		}
	}
	return false;
}

public bool RegisterMath(class_ItemList ItemInstance, int iEntity)
{
	if (IsValidEntity(ItemInstance.WeaponID))
	{
		if (ItemInstance.EnergyID == Entity_GetHammerId(iEntity))
		{
			char Item_Counter_Targetname[64];
			Entity_GetTargetName(iEntity, Item_Counter_Targetname, sizeof(Item_Counter_Targetname));
			int iTLocCounter = FindCharInString(Item_Counter_Targetname, '&', true);
			if(iTLocCounter == -1)
			{
				ItemInstance.MathID = iEntity;
				int max = RoundFloat(GetEntPropFloat(iEntity, Prop_Data, "m_flMax"));
				int value = GetCounterValue(iEntity);
				if (ItemInstance.Mode == 6) ItemInstance.MathValue = value;
				else if (ItemInstance.Mode == 7) ItemInstance.MathValue = (max - value);
				ItemInstance.MathValueMax = max;
				return true;
			}else
			{
				char Item_Weapon_Targetname[64];
				Entity_GetTargetName(ItemInstance.WeaponID, Item_Weapon_Targetname, sizeof(Item_Weapon_Targetname));
				int iTLocWeapon = FindCharInString(Item_Weapon_Targetname, '&', true);
				if(iTLocWeapon == -1) return false;
				if(strcmp(Item_Counter_Targetname[iTLocCounter+1], Item_Weapon_Targetname[iTLocWeapon+1], false) == 0)
				{
					ItemInstance.MathID = iEntity;
					int max = RoundFloat(GetEntPropFloat(iEntity, Prop_Data, "m_flMax"));
					int value = GetCounterValue(iEntity);
					if (ItemInstance.Mode == 6) ItemInstance.MathValue = value;
					else if (ItemInstance.Mode == 7) ItemInstance.MathValue = (max - value);
					ItemInstance.MathValueMax = max;
					return true;
				}
			}
		}else if (ItemInstance.EnergyID2 == Entity_GetHammerId(iEntity))
		{
			char Item_Counter_Targetname[64];
			Entity_GetTargetName(iEntity, Item_Counter_Targetname, sizeof(Item_Counter_Targetname));
			int iTLocCounter = FindCharInString(Item_Counter_Targetname, '&', true);
			if(iTLocCounter == -1)
			{
				ItemInstance.MathID2 = iEntity;
				int max = RoundFloat(GetEntPropFloat(iEntity, Prop_Data, "m_flMax"));
				int value = GetCounterValue(iEntity);
				if (ItemInstance.Mode2 == 6) ItemInstance.MathValue2 = value;
				else if (ItemInstance.Mode2 == 7) ItemInstance.MathValue2 = (max - value);
				ItemInstance.MathValueMax2 = max;
				return true;
			}else
			{
				char Item_Weapon_Targetname[64];
				Entity_GetTargetName(ItemInstance.WeaponID, Item_Weapon_Targetname, sizeof(Item_Weapon_Targetname));
				int iTLocWeapon = FindCharInString(Item_Weapon_Targetname, '&', true);
				if(iTLocWeapon == -1) return false;
				if(strcmp(Item_Counter_Targetname[iTLocCounter+1], Item_Weapon_Targetname[iTLocWeapon+1], false) == 0)
				{
					ItemInstance.MathID2 = iEntity;
					int max = RoundFloat(GetEntPropFloat(iEntity, Prop_Data, "m_flMax"));
					int value = GetCounterValue(iEntity);
					if (ItemInstance.Mode2 == 6) ItemInstance.MathValue2 = value;
					else if (ItemInstance.Mode2 == 7) ItemInstance.MathValue2 = (max - value);
					ItemInstance.MathValueMax2 = max;
					return true;
				}
			}
		}
	}
	return false;
}

public void OnEntityCreated(int iEntity, const char[] sClassname)
{
	if(IsValidEntity(iEntity))
	{
		if(StrContains(sClassname, "weapon_", false) != -1) SDKHook(iEntity, SDKHook_SpawnPost, OnItemSpawned);
		else if(StrEqual(sClassname,"func_button")||StrEqual(sClassname,"func_rot_button")||
			StrEqual(sClassname,"func_door")||StrEqual(sClassname,"func_door_rotating")) SDKHook(iEntity, SDKHook_SpawnPost, OnButtonSpawned);
		else if (StrEqual(sClassname,"math_counter")) SDKHook(iEntity, SDKHook_SpawnPost, OnMathSpawned);
		else if(StrContains(sClassname, "trigger_", false) != -1) SDKHook(iEntity, SDKHook_SpawnPost, OnTriggerSpawned);
		#if defined EW_MODULE_PHYSBOX
		else if(StrContains(sClassname, "func_physbox", false) != -1) SDKHook(iEntity, SDKHook_SpawnPost, OnPhysboxSpawned);
		#endif
	}
}

public void OnEntityDestroyed(int iEntity)
{
	if(IsValidEdict(iEntity))
	{
		char sClassname[32];
		GetEdictClassname(iEntity, sClassname, sizeof(sClassname));
		if(StrContains(sClassname, "weapon_", false) != -1)
		{
			for(int i = 0; i < g_ItemList.Length; i++)
			{
				class_ItemList ItemTest;
				g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
				if(ItemTest.WeaponID == iEntity)
				{
					#if defined EW_MODULE_CLANTAG
					EWM_Clantag_Reset(ItemTest.OwnerID);
					#endif
					ItemTest.MathID = INVALID_ENT_REFERENCE;
					ItemTest.MathID2 = INVALID_ENT_REFERENCE;
					ItemTest.WeaponID = INVALID_ENT_REFERENCE;
					ItemTest.OwnerID = INVALID_ENT_REFERENCE;
					ItemTest.GlowEnt = INVALID_ENT_REFERENCE;
					g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				}
			}
		}
		#if defined EW_MODULE_PHYSBOX
		else if(StrContains(sClassname, "func_physbox", false) != -1) EWM_Physbox_OnEntityDestroyed(iEntity);
		#endif
	}
}

public void OnItemSpawned(int iEntity)
{
	if(!IsValidEntity(iEntity) || !g_bConfigLoaded) return;
	
	int iHammerID = Entity_GetHammerId(iEntity);
	if(iHammerID>0)
	{
		for(int i = 0; i<g_ItemConfig.Length; i++)
		{
			class_ItemConfig ItemTest;
			g_ItemConfig.GetArray(i, ItemTest, sizeof(ItemTest));
			if(RegisterItem(ItemTest, iEntity, iHammerID)) return;
		}
	}
}

public void OnMathSpawned(int iEntity)
{
	//In case the math entity spawns just before the weapon entity (?)
	CreateTimer(1.5, Timer_OnMathSpawned, iEntity);
	
}

public Action Timer_OnMathSpawned(Handle timer, int iEntity)
{
	if(!IsValidEntity(iEntity) || !g_bConfigLoaded) return Plugin_Stop;
	
	for(int i = 0; i < g_ItemList.Length; i++)
	{
		class_ItemList ItemTest;
		g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
		if (RegisterMath(ItemTest, iEntity))
		{
			g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			return Plugin_Stop;
		}
	}
	return Plugin_Stop;
}

public void OnButtonSpawned(int iEntity) //Button with parent spawns after weapon entity. With timer button don't register if item spawns with module items_spawn
{
	if(!IsValidEntity(iEntity) || !g_bConfigLoaded) return;
	
	char sClassname[32];
	GetEdictClassname(iEntity, sClassname, sizeof(sClassname));
	if (StrEqual(sClassname,"func_door") || StrEqual(sClassname,"func_door_rotating"))
	{
		int spawnflags = GetEntProp(iEntity, Prop_Data, "m_spawnflags");
		if (!(spawnflags & 256))return; //The entity cannot be pressed so don't register it as a button
	}
	
	for(int i = 0; i < g_ItemList.Length; i++)
	{
		class_ItemList ItemTest;
		g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
		if (RegisterButton(ItemTest,iEntity))
		{
			g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			return;
		}
	}
}

public void OnTriggerSpawned(int iEntity)
{
	//In case the trigger entity spawns just before the weapon entity (?)
	CreateTimer(0.5, Timer_OnTriggerSpawned, iEntity);
}

public Action Timer_OnTriggerSpawned(Handle timer, int iEntity)
{
	if(!IsValidEntity(iEntity) || !g_bConfigLoaded) return Plugin_Stop;
	
	int iHammerID = Entity_GetHammerId(iEntity);
	if(iHammerID>0)
	{
		for(int i = 0; i<g_ItemConfig.Length; i++)
		{
			class_ItemConfig ItemTest;
			g_ItemConfig.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.Trigger == iHammerID)
			{
				g_TriggerArray.Push(iEntity);
				SDKHookEx(iEntity, SDKHook_Touch, OnTrigger);
				SDKHookEx(iEntity, SDKHook_EndTouch, OnTrigger);
				SDKHookEx(iEntity, SDKHook_StartTouch, OnTrigger);
			}
		}
	}
	return Plugin_Stop;
}

public Action OnTrigger(int iEntity, int iClient)
{
	#if defined EW_MODULE_EBAN
    if (IsValidClient(iClient) && IsClientConnected(iClient)) if (g_bGlobalBlock || g_EbanClients[iClient].Banned) return Plugin_Handled;
	#else
	if (IsValidClient(iClient) && IsClientConnected(iClient) && g_bGlobalBlock) return Plugin_Handled;
	#endif
	
    return Plugin_Continue;
}

//-------------------------------------------------------
//Purpose: Notify when they use a special weapon
//-------------------------------------------------------
public Action OnButtonUse(int iButton, int iActivator, int iCaller, UseType uType, float fvalue)
{
	if(g_bConfigLoaded && IsValidEdict(iButton))
	{
		//DEBUG SHIT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//char edebug[32];
		//Entity_GetTargetName(iButton, edebug, sizeof(edebug));
		//PrintToConsoleAll("[EntWatch] PRESS Button %s by %N - ID %i", edebug, iActivator, iActivator);
		
		int iOffset = FindDataMapInfo(iButton, "m_bLocked");
		if (iOffset != -1 && GetEntData(iButton, iOffset, 1)) return Plugin_Handled;

		for(int i = 0; i < g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(IsValidEdict(ItemTest.WeaponID))
			{
				for(int j = 0; j < ItemTest.ButtonsArray.Length; j++)
				{
					if(ItemTest.ButtonsArray.Get(j) == iButton)
					{
						//DEBUG SHIT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
						//PrintToConsoleAll("[EntWatch] DEBUG: Name - %s, WeaponID - %i", ItemTest.Name, ItemTest.WeaponID);
						//PrintToConsoleAll("[EntWatch] DEBUG: Uses - %i", ItemTest.Uses);
						//PrintToConsoleAll("[EntWatch] DEBUG: CooldownTime - %i", ItemTest.CoolDownTime);
						//PrintToConsoleAll("[EntWatch] DEBUG: Owner - %i", ItemTest.OwnerID);
						//if (!IsValidClient(ItemTest.OwnerID))
						//{
						//	LogError("No valid client for %s - %i", ItemTest.Name, ItemTest.OwnerID);
						//}
						//PrintToConsoleAll("[EntWatch] DEBUG: Chat - %s", ItemTest.Chat ? "True" : "False");
						//PrintToConsoleAll("[EntWatch] DEBUG: Mode - %i", ItemTest.Mode);
						//PrintToConsoleAll("[EntWatch] DEBUG: Delay - %i", ItemTest.Delay);
						//PrintToConsoleAll("[EntWatch] DEBUG: ButtonID - %i", ItemTest.ButtonID);
						//PrintToConsoleAll("[EntWatch] DEBUG: iButton - %i", iButton);
						//PrintToConsoleAll("[EntWatch] DEBUG: HammerID of iButton - %i", Entity_GetHammerId(iButton));
						
						
						if(ItemTest.OwnerID != iActivator && ItemTest.OwnerID != iCaller) return Plugin_Handled;
							else if(!(StrEqual(ItemTest.FilterName,""))) DispatchKeyValue(iActivator, "targetname", ItemTest.FilterName);
						
						UpdateTime();
						if(ItemTest.CheckDelay() > 0.0) return Plugin_Handled;
						
						int iAbility = 0; //0 - once button, 1 - first button, 2 - second button
						
						if(ItemTest.ButtonID != INVALID_ENT_REFERENCE && ItemTest.ButtonID == Entity_GetHammerId(iButton))
						{
							if(ItemTest.LockButton) return Plugin_Handled;
							if(ItemTest.CheckWaitTime() > 0 ) return Plugin_Handled;
							iAbility = 1;
						}
						else if (ItemTest.ButtonID2 != INVALID_ENT_REFERENCE && ItemTest.ButtonID2 == Entity_GetHammerId(iButton))
						{
							if(ItemTest.LockButton2) return Plugin_Handled;
							if(ItemTest.CheckWaitTime2() > 0 ) return Plugin_Handled;
							iAbility = 2;
						}
						else return Plugin_Changed;

						if(ItemTest.ButtonID2 == INVALID_ENT_REFERENCE) iAbility = 0;
						
						// Base delay on the wait time of the button (button is locked for this duration)
						int waitTime = 0;
						if (HasEntProp(iButton, Prop_Data, "m_flWait"))
							waitTime = RoundToFloor(GetEntPropFloat(iButton, Prop_Data, "m_flWait"));

						if(waitTime < 0) waitTime = 0;
						
						if(iAbility!=2)
						{
							switch (ItemTest.Mode)
							{
								case 2: 
									if(ItemTest.CheckCoolDown() <= 0)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										if(waitTime > ItemTest.CoolDown) ItemTest.SetWaitTime(waitTime);
										ItemTest.SetCoolDown(ItemTest.CoolDown);
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										return Plugin_Changed;
									}
								case 3:
									if(ItemTest.Uses < ItemTest.MaxUses)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										ItemTest.SetWaitTime(waitTime);
										ItemTest.Uses++;
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										return Plugin_Changed;
									}
								case 4:
									if(ItemTest.Uses < ItemTest.MaxUses && ItemTest.CheckCoolDown() <= 0)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										if(waitTime > ItemTest.CoolDown) ItemTest.SetWaitTime(waitTime);
										ItemTest.SetCoolDown(ItemTest.CoolDown);
										ItemTest.Uses++;
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										return Plugin_Changed;
									}
								case 5:
									if(ItemTest.CheckCoolDown() <= 0)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										ItemTest.SetWaitTime(waitTime);
										ItemTest.Uses++;
										if(ItemTest.Uses >= ItemTest.MaxUses)
										{
											ItemTest.SetCoolDown(ItemTest.CoolDown);
											ItemTest.Uses = 0;
										}
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										return Plugin_Changed;
									}
								case 6,7:
								{
									if(ItemTest.CoolDown > 0)
									{
										if(ItemTest.CheckCoolDown() <= 0)
										{
											#if defined EW_MODULE_FORWARDS
											Call_StartForward(g_hOnUseItem);
											Call_PushString(ItemTest.Name);
											Call_PushCell(iActivator);
											Call_PushCell(iAbility);
											Call_Finish();
											#endif
											#if defined EW_MODULE_CHAT
											if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
											#endif
											
											if(waitTime > ItemTest.CoolDown) ItemTest.SetWaitTime(waitTime);
											ItemTest.SetCoolDown(ItemTest.CoolDown);
											g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										}else return Plugin_Handled;
									}
									return Plugin_Changed;
								}
								default: return Plugin_Changed;
							}
						}else
						{
							switch (ItemTest.Mode2)
							{
								case 2: 
									if(ItemTest.CheckCoolDown2() <= 0)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif

										if(waitTime > ItemTest.CoolDown2) ItemTest.SetWaitTime2(waitTime);
										ItemTest.SetCoolDown2(ItemTest.CoolDown2);
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										return Plugin_Changed;
									}
								case 3:
									if(ItemTest.Uses2 < ItemTest.MaxUses2)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										ItemTest.SetWaitTime2(waitTime);
										ItemTest.Uses2++;
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										return Plugin_Changed;
									}
								case 4:
									if(ItemTest.Uses2 < ItemTest.MaxUses2 && ItemTest.CheckCoolDown2() <= 0)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										if(waitTime > ItemTest.CoolDown2) ItemTest.SetWaitTime2(waitTime);
										ItemTest.SetCoolDown2(ItemTest.CoolDown2);
										ItemTest.Uses2++;
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										return Plugin_Changed;
									}
								case 5:
									if(ItemTest.CheckCoolDown2() <= 0)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										ItemTest.SetWaitTime2(waitTime);
										ItemTest.Uses2++;
										if(ItemTest.Uses2 >= ItemTest.MaxUses2)
										{
											ItemTest.SetCoolDown2(ItemTest.CoolDown2);
											ItemTest.Uses2 = 0;
										}
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										return Plugin_Changed;
									}
								case 6,7:
								{
									if(ItemTest.CoolDown2 > 0)
									{
										if(ItemTest.CheckCoolDown2() <= 0)
										{
											#if defined EW_MODULE_FORWARDS
											Call_StartForward(g_hOnUseItem);
											Call_PushString(ItemTest.Name);
											Call_PushCell(iActivator);
											Call_PushCell(iAbility);
											Call_Finish();
											#endif
											#if defined EW_MODULE_CHAT
											if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
											#endif
											
											if(waitTime > ItemTest.CoolDown2) ItemTest.SetWaitTime2(waitTime);
											ItemTest.SetCoolDown2(ItemTest.CoolDown2);
											g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
										}else return Plugin_Handled;
									}
									return Plugin_Changed;
								}
								default: return Plugin_Changed;
							}
						}
						//~~~~ Is this return needed (?) ~~~~
						return Plugin_Handled;
					}
				}
			}
		}
	}
	return Plugin_Handled;
}

//-------------------------------------------------------
//Purpose: Update item energy from math counter
//-------------------------------------------------------
public void Event_OutValue(const char[] sOutput, int iCaller, int iActivator, float Delay)
{
	for(int i = 0; i < g_ItemList.Length; i++)
	{
		class_ItemList ItemTest;
		g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
		if (ItemTest.MathID == iCaller)
		{
			int max = RoundFloat(GetEntPropFloat(iCaller, Prop_Data, "m_flMax"));
			int value = GetCounterValue(iCaller);
			if (ItemTest.Mode == 6) ItemTest.MathValue = value;
			else if (ItemTest.Mode == 7) ItemTest.MathValue = (max - value);
			ItemTest.MathValueMax = max;
			g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			return;
		}else if (ItemTest.MathID2 == iCaller)
		{
			int max = RoundFloat(GetEntPropFloat(iCaller, Prop_Data, "m_flMax"));
			int value = GetCounterValue(iCaller);
			if (ItemTest.Mode2 == 6) ItemTest.MathValue2 = value;
			else if (ItemTest.Mode2 == 7) ItemTest.MathValue2 = (max - value);
			ItemTest.MathValueMax2 = max;
			g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			return;
		}
	}
}

//-------------------------------------------------------
//Purpose: Notify when they use a special weapon
//-------------------------------------------------------
public Action Event_GameUI_RightClick(const char[] sOutput, int iCaller, int iActivator, float Delay)
{
	if(g_bConfigLoaded)
	{
		for(int i = 0; i < g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			
			if(IsValidEdict(ItemTest.WeaponID) && ItemTest.OwnerID==iActivator)
			{
				int iAbility = -1;
				if(ItemTest.ButtonID == -5)
				{
					if(ItemTest.LockButton) continue;
					iAbility = 1;
				}
				else if(ItemTest.ButtonID2 == -5)
				{
					if(ItemTest.LockButton2) continue;
					iAbility = 2;
				}
				if(iAbility == 1 && ItemTest.ButtonID2 == INVALID_ENT_REFERENCE) iAbility = 0;
				if(iAbility > -1)
				{
					if(!(StrEqual(ItemTest.FilterName,""))) DispatchKeyValue(iActivator, "targetname", ItemTest.FilterName);
					UpdateTime();
					if(ItemTest.CheckDelay() > 0.0) continue;
					if(iAbility != 2)
					{
						switch (ItemTest.Mode)
						{
							case 2: 
								if(ItemTest.CheckCoolDown() <= 0)
								{
									#if defined EW_MODULE_FORWARDS
									Call_StartForward(g_hOnUseItem);
									Call_PushString(ItemTest.Name);
									Call_PushCell(iActivator);
									Call_PushCell(iAbility);
									Call_Finish();
									#endif
									#if defined EW_MODULE_CHAT
									if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
									#endif
									
									ItemTest.SetCoolDown(ItemTest.CoolDown);
									g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									continue;
								}
							case 3:
								if(ItemTest.Uses < ItemTest.MaxUses)
								{
									#if defined EW_MODULE_FORWARDS
									Call_StartForward(g_hOnUseItem);
									Call_PushString(ItemTest.Name);
									Call_PushCell(iActivator);
									Call_PushCell(iAbility);
									Call_Finish();
									#endif
									#if defined EW_MODULE_CHAT
									if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
									#endif
									
									ItemTest.Uses++;
									g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									continue;
								}
							case 4:
								if(ItemTest.Uses < ItemTest.MaxUses && ItemTest.CheckCoolDown() <= 0)
								{
									#if defined EW_MODULE_FORWARDS
									Call_StartForward(g_hOnUseItem);
									Call_PushString(ItemTest.Name);
									Call_PushCell(iActivator);
									Call_PushCell(iAbility);
									Call_Finish();
									#endif
									#if defined EW_MODULE_CHAT
									if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
									#endif
									
									ItemTest.SetCoolDown(ItemTest.CoolDown);
									ItemTest.Uses++;
									g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									continue;
								}
							case 5:
								if(ItemTest.CheckCoolDown() <= 0)
								{
									#if defined EW_MODULE_FORWARDS
									Call_StartForward(g_hOnUseItem);
									Call_PushString(ItemTest.Name);
									Call_PushCell(iActivator);
									Call_PushCell(iAbility);
									Call_Finish();
									#endif
									#if defined EW_MODULE_CHAT
									if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
									#endif
									
									ItemTest.Uses++;
									if(ItemTest.Uses >= ItemTest.MaxUses)
									{
										ItemTest.SetCoolDown(ItemTest.CoolDown);
										ItemTest.Uses = 0;
									}
									g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									continue;
								}
							case 6,7:
							{
								if(ItemTest.CoolDown > 0)
								{
									if(ItemTest.CheckCoolDown() <= 0)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										ItemTest.SetCoolDown(ItemTest.CoolDown);
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									}else continue;
								}
								continue;
							}
							default: continue;
						}
					}else
					{
						switch (ItemTest.Mode2)
						{
							case 2: 
								if(ItemTest.CheckCoolDown2() <= 0)
								{
									#if defined EW_MODULE_FORWARDS
									Call_StartForward(g_hOnUseItem);
									Call_PushString(ItemTest.Name);
									Call_PushCell(iActivator);
									Call_PushCell(iAbility);
									Call_Finish();
									#endif
									#if defined EW_MODULE_CHAT
									if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
									#endif
									
									ItemTest.SetCoolDown2(ItemTest.CoolDown2);
									g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									continue;
								}
							case 3:
								if(ItemTest.Uses2 < ItemTest.MaxUses2)
								{
									#if defined EW_MODULE_FORWARDS
									Call_StartForward(g_hOnUseItem);
									Call_PushString(ItemTest.Name);
									Call_PushCell(iActivator);
									Call_PushCell(iAbility);
									Call_Finish();
									#endif
									#if defined EW_MODULE_CHAT
									if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
									#endif
									
									ItemTest.Uses2++;
									g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									continue;
								}
							case 4:
								if(ItemTest.Uses2 < ItemTest.MaxUses2 && ItemTest.CheckCoolDown2() <= 0)
								{
									#if defined EW_MODULE_FORWARDS
									Call_StartForward(g_hOnUseItem);
									Call_PushString(ItemTest.Name);
									Call_PushCell(iActivator);
									Call_PushCell(iAbility);
									Call_Finish();
									#endif
									#if defined EW_MODULE_CHAT
									if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
									#endif
									
									ItemTest.SetCoolDown2(ItemTest.CoolDown2);
									ItemTest.Uses2++;
									g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									continue;
								}
							case 5:
								if(ItemTest.CheckCoolDown2() <= 0)
								{
									#if defined EW_MODULE_FORWARDS
									Call_StartForward(g_hOnUseItem);
									Call_PushString(ItemTest.Name);
									Call_PushCell(iActivator);
									Call_PushCell(iAbility);
									Call_Finish();
									#endif
									#if defined EW_MODULE_CHAT
									if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
									#endif
									
									ItemTest.Uses2++;
									if(ItemTest.Uses2 >= ItemTest.MaxUses2)
									{
										ItemTest.SetCoolDown2(ItemTest.CoolDown2);
										ItemTest.Uses2 = 0;
									}
									g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									continue;
								}
							case 6,7:
							{
								if(ItemTest.CoolDown2 > 0)
								{
									if(ItemTest.CheckCoolDown2() <= 0)
									{
										#if defined EW_MODULE_FORWARDS
										Call_StartForward(g_hOnUseItem);
										Call_PushString(ItemTest.Name);
										Call_PushCell(iActivator);
										Call_PushCell(iAbility);
										Call_Finish();
										#endif
										#if defined EW_MODULE_CHAT
										if(ItemTest.Chat || ItemTest.Chat_Uses) EWM_Chat_Use(ItemTest, iActivator, iAbility);
										#endif
										
										ItemTest.SetCoolDown2(ItemTest.CoolDown2);
										g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
									}else continue;
								}
								continue;
							}
							default: continue;
						}
					}
				}
			}
		}
	}
	return Plugin_Continue;
}

//-------------------------------------------------------
//Purpose: Notify when they drop a special weapon
//-------------------------------------------------------
public void OnWeaponDrop(int iClient, int iWeapon)
{
	if(g_bConfigLoaded && IsValidEdict(iWeapon))
	{
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.WeaponID == iWeapon)
			{
				ItemTest.OwnerID = INVALID_ENT_REFERENCE;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				
				#if defined EW_MODULE_BLINK
				EWM_Blink_BlinkWeapon(ItemTest);
				#endif
				
				#if defined EW_MODULE_GLOW
				EWM_Glow_GlowWeapon(ItemTest, i, false);
				#endif
				
				#if defined EW_MODULE_HIGHLIGHT
				if(g_evGameEngine == Engine_CSGO)
				{
					EWM_HLight_PRemove(iClient);
					EWM_HLight_Set(ItemTest);
				}
				#endif
				
				#if defined EW_MODULE_FORWARDS
				Call_StartForward(g_hOnDropItem);
				Call_PushString(ItemTest.Name);
				Call_PushCell(iClient);
				Call_Finish();
				#endif
				#if defined EW_MODULE_CHAT
				if(ItemTest.Chat) EWM_Chat_Drop(ItemTest, iClient);
				#endif
				
				#if defined EW_MODULE_CLANTAG
				EWM_Clantag_Reset(iClient);
				#endif
					
				break;
			}
		}
	}
}

//-------------------------------------------------------
//Purpose: Prevent banned players from picking up special weapons
//-------------------------------------------------------
public Action OnWeaponCanUse(int iClient, int iWeapon)
{
	//if (IsFakeClient(iClient)) return Plugin_Handled;	
	if(g_bConfigLoaded && IsValidEdict(iWeapon))
	{
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.WeaponID == iWeapon)
			{
				#if defined EW_MODULE_EBAN
				if(g_bGlobalBlock || ItemTest.BlockPickup || g_EbanClients[iClient].Banned || ((GetClientButtons(iClient) & IN_USE) && g_bBlockEPick)) return Plugin_Handled;
				#else
				if(g_bGlobalBlock || ItemTest.BlockPickup || ((GetClientButtons(iClient) & IN_USE) && g_bBlockEPick)) return Plugin_Handled;
				#endif
				
				return Plugin_Continue;
			}
		}
	}
	return Plugin_Continue;
}

//-------------------------------------------------------
//Purpose: Notify when they pick up a special weapon
//-------------------------------------------------------
public void OnWeaponEquip(int iClient, int iWeapon)
{
	if(g_bConfigLoaded && IsValidEdict(iWeapon))
	{
		for(int i = 0; i < g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.WeaponID == iWeapon)
			{
				ItemTest.OwnerID = iClient;
				UpdateTime();
				ItemTest.SetDelay(g_fDelayUse);
				ItemTest.Team = GetClientTeam(iClient);
				
				#if defined EW_MODULE_BLINK
				EWM_Blink_DisableBlink(ItemTest);
				#endif
				
				#if defined EW_MODULE_GLOW
				EWM_Glow_DisableGlow(ItemTest);
				#endif
				
				#if defined EW_MODULE_HIGHLIGHT
				if(g_evGameEngine == Engine_CSGO)
				{
					EWM_HLight_WRemove(ItemTest.WeaponID);
					EWM_HLight_Set(ItemTest);
				}
				#endif
				
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				
				#if defined EW_MODULE_FORWARDS
				Call_StartForward(g_hOnPickUpItem);
				Call_PushString(ItemTest.Name);
				Call_PushCell(iClient);
				Call_Finish();
				#endif
				#if defined EW_MODULE_CHAT
				if(ItemTest.Chat) EWM_Chat_PickUp(ItemTest, iClient);
				#endif
				
				#if defined EW_MODULE_OFFLINE_EBAN
				EWM_OfflineEban_UpdateItemName(iClient, ItemTest.Name);
				#endif
				
				break;
			}
		}
		#if defined EW_MODULE_PHYSBOX
		EWM_Physbox_Pickedup(iClient, iWeapon);
		#endif
	}
}

// Handlers Commands
public Action EW_Command_ReloadConfig(int iClient, int iArgs)
{	
	#if defined EW_MODULE_CLANTAG
	EWM_Clantag_Mass_Reset();
	#endif
	
	CleanData();
	LoadConfig();
	LoadScheme();

	CReplyToCommand(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Reload Configs");

	return Plugin_Handled;
}

public Action EW_Command_Cooldown(int iClient, int iArgs)
{
	if (iArgs != 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_setcooldown <hammerid> <cooldown>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32], sCooldown[10];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sCooldown, sizeof(sCooldown));

	int iHammerID = StringToInt(sHammerID);
	int iCooldown = StringToInt(sCooldown);
	
	if(iCooldown < 0) iCooldown = 0;

	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				ItemTest.CoolDown = iCooldown;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Cooldown2(int iClient, int iArgs)
{
	if (iArgs != 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_setcooldown2 <hammerid> <cooldown>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32], sCooldown[10];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sCooldown, sizeof(sCooldown));

	int iHammerID = StringToInt(sHammerID);
	int iCooldown = StringToInt(sCooldown);
	
	if(iCooldown < 0) iCooldown = 0;

	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				ItemTest.CoolDown2 = iCooldown;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Setmaxuses(int iClient, int iArgs)
{
	if (iArgs < 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_setmaxuses <hammerid> <maxuses> [<even if over>]", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32], sMaxUses[10];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sMaxUses, sizeof(sMaxUses));
	
	bool bOver = false;
	if(iArgs >= 3)
	{
		char sOver[10];
		GetCmdArg(3, sOver, sizeof(sOver));
		int iOver = StringToInt(sOver);
		if(iOver == 1) bOver = true;
	}

	int iHammerID = StringToInt(sHammerID);
	int iMaxUses = StringToInt(sMaxUses);
	
	if(iMaxUses < 0) iMaxUses = 0;

	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				if(ItemTest.MaxUses > ItemTest.Uses || bOver)
				{
					ItemTest.MaxUses = iMaxUses;
					g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				}
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Setmaxuses2(int iClient, int iArgs)
{
	if (iArgs < 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_setmaxuses2 <hammerid> <maxuses> [<even if over>]", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32], sMaxUses[10];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sMaxUses, sizeof(sMaxUses));
	
	bool bOver = false;
	if(iArgs >= 3)
	{
		char sOver[10];
		GetCmdArg(3, sOver, sizeof(sOver));
		int iOver = StringToInt(sOver);
		if(iOver == 1) bOver = true;
	}

	int iHammerID = StringToInt(sHammerID);
	int iMaxUses = StringToInt(sMaxUses);
	
	if(iMaxUses < 0) iMaxUses = 0;

	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				if(ItemTest.MaxUses2 > ItemTest.Uses2 || bOver)
				{
					ItemTest.MaxUses2 = iMaxUses;
					g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				}
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Addmaxuses(int iClient, int iArgs)
{
	if (iArgs < 1)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_addmaxuses <hammerid> [<even if over>]", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	
	bool bOver = false;
	if(iArgs >= 2)
	{
		char sOver[10];
		GetCmdArg(2, sOver, sizeof(sOver));
		int iOver = StringToInt(sOver);
		if(iOver == 1) bOver = true;
	}

	int iHammerID = StringToInt(sHammerID);

	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				if(ItemTest.MaxUses > ItemTest.Uses || bOver)
				{
					ItemTest.MaxUses++;
					g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				}
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Addmaxuses2(int iClient, int iArgs)
{
	if (iArgs < 1)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_addmaxuses2 <hammerid> [<even if over>]", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	
	bool bOver = false;
	if(iArgs >= 2)
	{
		char sOver[10];
		GetCmdArg(2, sOver, sizeof(sOver));
		int iOver = StringToInt(sOver);
		if(iOver == 1) bOver = true;
	}

	int iHammerID = StringToInt(sHammerID);

	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				if(ItemTest.MaxUses2 > ItemTest.Uses2 || bOver)
				{
					ItemTest.MaxUses2++;
					g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				}
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Setmode(int iClient, int iArgs)
{
	if (iArgs < 4)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_ewsetmode <hammerid> <newmode> <cooldown> <maxuses> [<even if over>]", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32], sNewMode[10], sCooldown[10], sMaxUses[10];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sNewMode, sizeof(sNewMode));
	GetCmdArg(3, sCooldown, sizeof(sCooldown));
	GetCmdArg(4, sMaxUses, sizeof(sMaxUses));
	
	bool bOver = false;
	if(iArgs >= 5)
	{
		char sOver[10];
		GetCmdArg(5, sOver, sizeof(sOver));
		int iOver = StringToInt(sOver);
		if(iOver == 1) bOver = true;
	}

	int iHammerID = StringToInt(sHammerID);
	int iNewMode = StringToInt(sNewMode);
	int iCooldown = StringToInt(sCooldown);
	int iMaxUses = StringToInt(sMaxUses);
	
	if(iNewMode < 1 || iNewMode > 7) iNewMode = 1;
	if(iCooldown < 0) iCooldown = 0;
	if(iMaxUses < 0) iMaxUses = 0;

	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				if(ItemTest.MaxUses > ItemTest.Uses || bOver || iNewMode == 7 || iNewMode == 6 || iNewMode == 2 || iNewMode == 1)
				{
					ItemTest.Mode = iNewMode;
					ItemTest.CoolDown = iCooldown;
					ItemTest.MaxUses = iMaxUses;
					g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				}
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Setmode2(int iClient, int iArgs)
{
	if (iArgs < 4)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_ewsetmode2 <hammerid> <newmode> <cooldown> <maxuses> [<even if over>]", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32], sNewMode[10], sCooldown[10], sMaxUses[10];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sNewMode, sizeof(sNewMode));
	GetCmdArg(3, sCooldown, sizeof(sCooldown));
	GetCmdArg(4, sMaxUses, sizeof(sMaxUses));
	
	bool bOver = false;
	if(iArgs >= 5)
	{
		char sOver[10];
		GetCmdArg(5, sOver, sizeof(sOver));
		int iOver = StringToInt(sOver);
		if(iOver == 1) bOver = true;
	}

	int iHammerID = StringToInt(sHammerID);
	int iNewMode = StringToInt(sNewMode);
	int iCooldown = StringToInt(sCooldown);
	int iMaxUses = StringToInt(sMaxUses);
	
	if(iNewMode < 1 || iNewMode > 7) iNewMode = 1;
	if(iCooldown < 0) iCooldown = 0;
	if(iMaxUses < 0) iMaxUses = 0;

	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				if(ItemTest.MaxUses2 > ItemTest.Uses2 || bOver || iNewMode == 7 || iNewMode == 6 || iNewMode == 2 || iNewMode == 1)
				{
					ItemTest.Mode2 = iNewMode;
					ItemTest.CoolDown2 = iCooldown;
					ItemTest.MaxUses2 = iMaxUses;
					g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				}
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Setname(int iClient, int iArgs)
{
	if (iArgs < 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_ewsetname <hammerid> <newname>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32], sNewName[32];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sNewName, sizeof(sNewName));

	int iHammerID = StringToInt(sHammerID);

	TrimString(sNewName);
	
	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				FormatEx(ItemTest.Name, sizeof(ItemTest.Name), "%s", sNewName);
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_Setshortname(int iClient, int iArgs)
{
	if (iArgs < 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_ewsetshortname <hammerid> <newname>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}

	char sHammerID[32], sNewName[32];

	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sNewName, sizeof(sNewName));

	int iHammerID = StringToInt(sHammerID);

	TrimString(sNewName);
	
	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				FormatEx(ItemTest.ShortName, sizeof(ItemTest.ShortName), "%s", sNewName);
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_BlockItem(int iClient, int iArgs)
{
	if (iArgs < 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_ewblock <hammerid> <0/1>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}
	
	char sHammerID[32], sBlock[2];
	
	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sBlock, sizeof(sBlock));
	
	int iHammerID = StringToInt(sHammerID);
	bool bBlock = false;
	int iBlock = StringToInt(sBlock);
	if(iBlock == 1) bBlock = true;
	
	if (g_bConfigLoaded)
	{
		for(int i = 0; i<g_ItemList.Length; i++) //Blocking All Spawned Items
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				ItemTest.BlockPickup = bBlock;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
				if (IsValidEdict(ItemTest.WeaponID) && ItemTest.OwnerID != INVALID_ENT_REFERENCE)
				{
					char sClassnameBuf[64];
					GetEdictClassname(ItemTest.WeaponID, sClassnameBuf, sizeof(sClassnameBuf));
					CS_DropWeapon(ItemTest.OwnerID, ItemTest.WeaponID, false);
					GivePlayerItem(ItemTest.OwnerID, sClassnameBuf);
				}
			}
		}
		
		for(int i = 0; i<g_ItemConfig.Length; i++) //Blocking Items in Config Array
		{
			class_ItemConfig ItemConfigTest;
			g_ItemConfig.GetArray(i, ItemConfigTest, sizeof(ItemConfigTest));
			if(ItemConfigTest.HammerID == iHammerID)
			{
				ItemConfigTest.BlockPickup = bBlock;
				g_ItemConfig.SetArray(i, ItemConfigTest, sizeof(ItemConfigTest));
			}
		}
	}
	return Plugin_Handled;
}

public Action EW_Command_LockButton(int iClient, int iArgs)
{
	if (iArgs < 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_ewlockbutton <hammerid> <0/1>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}
	
	char sHammerID[32], sLock[2];
	
	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sLock, sizeof(sLock));
	
	bool bLock = false;
	int iLock = StringToInt(sLock);
	if(iLock == 1) bLock = true;
	
	int iHammerID = StringToInt(sHammerID);
	
	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				ItemTest.LockButton = bLock;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}

	return Plugin_Handled;
}

public Action EW_Command_LockButton2(int iClient, int iArgs)
{
	if (iArgs < 2)
	{
		CReplyToCommand(iClient, "%s%t %s%t: sm_ewlockbutton2 <hammerid> <0/1>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
		return Plugin_Handled;
	}
	
	char sHammerID[32], sLock[2];
	
	GetCmdArg(1, sHammerID, sizeof(sHammerID));
	GetCmdArg(2, sLock, sizeof(sLock));
	
	bool bLock = false;
	int iLock = StringToInt(sLock);
	if(iLock == 1) bLock = true;
	
	int iHammerID = StringToInt(sHammerID);
	
	if (g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.HammerID == iHammerID)
			{
				ItemTest.LockButton2 = bLock;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}

	return Plugin_Handled;
}