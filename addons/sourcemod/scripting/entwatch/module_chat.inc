#define EW_MODULE_CHAT

// Purpose: Displays actions with items in the chat

ConVar	g_hCvar_Chat_Admins_See;
ConVar	g_hCvar_Player_Format;
int		g_iChatASMode = 0;
int		g_iPlayerFormat = 3;

stock void EWM_Chat_OnPluginStart()
{
	g_hCvar_Chat_Admins_See	= CreateConVar("entwatch_adminchat_mode", "0", "Change AdminChat Mode (0 - All Messages, 1 - Only Pickup/Drop Items, 2 - Nothing)", _, true, 0.0, true, 2.0);
	g_hCvar_Player_Format = CreateConVar("entwatch_player_format", "3", "Changes the way player information is displayed by default (0 - Only Nickname, 1 - Nickname and UserID, 2 - Nickname and SteamID, 3 - Nickname, UserID and SteamID)", _, true, 0.0, true, 3.0);
	g_iChatASMode = GetConVarInt(g_hCvar_Chat_Admins_See);
	g_iPlayerFormat = GetConVarInt(g_hCvar_Player_Format);
	HookConVarChange(g_hCvar_Chat_Admins_See, Cvar_Chat_Changed);
	HookConVarChange(g_hCvar_Player_Format, Cvar_Chat_Changed);
}

public void Cvar_Chat_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_Chat_Admins_See)
		g_iChatASMode = GetConVarInt(convar);
	else if(convar==g_hCvar_Player_Format)
		g_iPlayerFormat = GetConVarInt(convar);
}

stock void FormatPlayerInfo(int iClient, char[] buffer, int maxlen)
{
	switch(g_iPlayerFormat)
	{
		case 0: // Only Nickname
			Format(buffer, maxlen, "%s%N", g_SchemeConfig.Color_Name, iClient);
		case 1: // Nickname and UserID
			Format(buffer, maxlen, "%s%N %s(%s#%d%s)", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Warning, g_SchemeConfig.Color_SteamID, g_iUserIDs[iClient], g_SchemeConfig.Color_Warning);
		case 2: // Nickname and SteamID
			Format(buffer, maxlen, "%s%N %s(%s%s%s)", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Warning, g_SchemeConfig.Color_SteamID, g_sSteamIDs_short[iClient], g_SchemeConfig.Color_Warning);
		default: // Nickname, UserID and SteamID
			Format(buffer, maxlen, "%s%N %s(%s#%d|%s%s)", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Warning, g_SchemeConfig.Color_SteamID, g_iUserIDs[iClient], g_sSteamIDs_short[iClient], g_SchemeConfig.Color_Warning);
	}
}

// Called when the ability is used
stock void EWM_Chat_Use(class_ItemList ItemTest, int iClient, int iAbility)
{
	char sPlayerInfo[128];
	FormatPlayerInfo(iClient, sPlayerInfo, sizeof(sPlayerInfo));

	for(int iPly = 1; iPly <= MaxClients; iPly++)
		if(IsClientConnected(iPly) && IsClientInGame(iPly))
			if(!g_bTeamOnly || (g_bTeamOnly && GetClientTeam(iPly) == ItemTest.Team || !IsPlayerAlive(iPly) || (g_iChatASMode == 0 && g_bIsAdmin[iPly])))
			{
				if(iAbility == 0 || ItemTest.Mode2 < 2) CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Use, "Use", ItemTest.Color, ItemTest.Name);
				else if(iAbility == 1) CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Use, "Use First", ItemTest.Color, ItemTest.Name);
				else CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Use, "Use Second", ItemTest.Color, ItemTest.Name);
			}
}

// Called when an item is picked up
stock void EWM_Chat_PickUp(class_ItemList ItemTest, int iClient)
{
	char sPlayerInfo[128];
	FormatPlayerInfo(iClient, sPlayerInfo, sizeof(sPlayerInfo));

	for(int iPly = 1; iPly <= MaxClients; iPly++)
		if(IsClientConnected(iPly) && IsClientInGame(iPly))
			if(!g_bTeamOnly || (g_bTeamOnly && GetClientTeam(iPly) == ItemTest.Team || !IsPlayerAlive(iPly) || (g_iChatASMode < 2 && g_bIsAdmin[iPly])))
				CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Pickup, "Pickup", ItemTest.Color, ItemTest.Name);
}

// Called when an item is dropped
stock void EWM_Chat_Drop(class_ItemList ItemTest, int iClient)
{
	char sPlayerInfo[128];
	FormatPlayerInfo(iClient, sPlayerInfo, sizeof(sPlayerInfo));

	for(int iPly = 1; iPly <= MaxClients; iPly++)
		if(IsClientConnected(iPly) && IsClientInGame(iPly))
			if(!g_bTeamOnly || (g_bTeamOnly && GetClientTeam(iPly) == ItemTest.Team || !IsPlayerAlive(iPly) || (g_iChatASMode < 2 && g_bIsAdmin[iPly])))
				CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Drop, "Drop", ItemTest.Color, ItemTest.Name);
}

// Called when a player disconnects with an item and the item is destroyed (ex. weapon_knife)
stock void EWM_Chat_Disconnect(class_ItemList ItemTest, int iClient)
{
	char sPlayerInfo[128];
	FormatPlayerInfo(iClient, sPlayerInfo, sizeof(sPlayerInfo));

	for(int iPly = 1; iPly <= MaxClients; iPly++)
		if(IsClientConnected(iPly) && IsClientInGame(iPly))
			if (!g_bTeamOnly || (g_bTeamOnly && GetClientTeam(iPly) == ItemTest.Team || !IsPlayerAlive(iPly) || (g_iChatASMode < 2 && g_bIsAdmin[iPly])))
				CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Disconnect, "Disconnect", ItemTest.Color, ItemTest.Name);
}

// Called when a player disconnects with an item and the item is discarded (ex. weapon_elite)
stock void EWM_Chat_Disconnect_Drop(class_ItemList ItemTest, int iClient)
{
	char sPlayerInfo[128];
	FormatPlayerInfo(iClient, sPlayerInfo, sizeof(sPlayerInfo));

	for(int iPly = 1; iPly <= MaxClients; iPly++)
		if(IsClientConnected(iPly) && IsClientInGame(iPly))
			if (!g_bTeamOnly || (g_bTeamOnly && GetClientTeam(iPly) == ItemTest.Team || !IsPlayerAlive(iPly) || (g_iChatASMode < 2 && g_bIsAdmin[iPly])))
				CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Disconnect, "Disconnect and drop", ItemTest.Color, ItemTest.Name);
}

// Called when the player died with the item and the item is destroyed (ex. weapon_knife)
stock void EWM_Chat_PlayerDeath(class_ItemList ItemTest, int iClient)
{
	char sPlayerInfo[128];
	FormatPlayerInfo(iClient, sPlayerInfo, sizeof(sPlayerInfo));

	for(int iPly = 1; iPly <= MaxClients; iPly++)
		if(IsClientConnected(iPly) && IsClientInGame(iPly))
			if(!g_bTeamOnly || (g_bTeamOnly && GetClientTeam(iPly) == ItemTest.Team || !IsPlayerAlive(iPly) || (g_iChatASMode < 2 && g_bIsAdmin[iPly])))
				CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Death, "Death", ItemTest.Color, ItemTest.Name);
}

// Called when a player died with an item and the item is discarded (ex. weapon_elite)
stock void EWM_Chat_PlayerDeath_Drop(class_ItemList ItemTest, int iClient)
{
	char sPlayerInfo[128];
	FormatPlayerInfo(iClient, sPlayerInfo, sizeof(sPlayerInfo));

	for(int iPly = 1; iPly <= MaxClients; iPly++)
		if(IsClientConnected(iPly) && IsClientInGame(iPly))
			if(!g_bTeamOnly || (g_bTeamOnly && GetClientTeam(iPly) == ItemTest.Team || !IsPlayerAlive(iPly) || (g_iChatASMode < 2 && g_bIsAdmin[iPly])))
				CPrintToChat(iPly,"%s%t %s %s%t %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", sPlayerInfo, g_SchemeConfig.Color_Death, "Death and drop", ItemTest.Color, ItemTest.Name);
}