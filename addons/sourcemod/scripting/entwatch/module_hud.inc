#define EW_MODULE_HUD

// Purpose: Displays items information in HUD or KeyHint
#define MAX_ShowHudItems 12
#define MAX_SizeStringForItem 40
#define MAX_KeyHintText 254
#define TIMEROTATIONHUD 3
#define MAX_HUD_BUFFERS 6  // Number of static buffers needed (Humans, Humans_wo_name, Zombies, Zombies_wo_name, Admins, Admins_wo_name)

// Purpose: Module settings
ConVar	g_hCvar_DisplayEnabled,
		g_hCvar_DisplayCooldowns,
		g_hCvar_Admins_See,
		g_hCvar_ZM_pry,
		g_hCvar_MaxHudItemsCount,
		g_hCvar_HudRotationDefault;

// Purpose: Module Client settings
Cookie	g_cCookie_HudSettings;

// Purpose: Module Local settings
bool	g_bDispEnabled = true,
		g_bDispCooldowns = true,
		g_bAdminsSee = true,
		g_bZombieNoItemPry = false,
		bNative_MsgTypeProtobuf = false;

int		g_iDefaultRotationTime = 3;
int		g_iMaxItemsCount = 10;

// Cache for HUD calculations
int g_iCachedItemCounts[3]; // [0] = Humans, [1] = Zombies, [2] = Total
bool g_bCachedHUDStates[2]; // [0] = Humans, [1] = Zombies
int g_iCachedMaxLists[3]; // [0] = Humans, [1] = Zombies, [2] = Total
bool g_bCacheDirty = true;

// Static buffers for string operations
char g_sStaticBuffers[MAX_HUD_BUFFERS][MAX_KeyHintText]; // Reusable buffers for HUD strings

int			g_iRotation[MAXPLAYERS+1][3][2]; // [client][0] - Humans, [client][1] - Zombies, [client][2] - Admins, [x][0] - count Items, [client][x][1] - current # list
ArrayList	g_iHItemsArray;
ArrayList	g_iZMItemsArray;

class_ClientSettings_Hud g_CSettings_Hud[MAXPLAYERS+1];

// Helper functions for cookie management
void SaveHudSettings(int client)
{
	// Only save if settings differ from defaults
	if (g_CSettings_Hud[client].Display == true &&
		g_CSettings_Hud[client].Name == true &&
		g_CSettings_Hud[client].ItemsCount == g_iMaxItemsCount &&
		g_CSettings_Hud[client].RotationTime == g_iDefaultRotationTime)
	{
		// Settings are at defaults, delete the cookie to save DB space
		g_cCookie_HudSettings.Set(client, "");
		return;
	}

	char sCookieData[32];
	FormatEx(sCookieData, sizeof(sCookieData), "%d|%d|%d|%d",
		g_CSettings_Hud[client].Display ? 1 : 0,
		g_CSettings_Hud[client].Name ? 1 : 0,
		g_CSettings_Hud[client].ItemsCount,
		g_CSettings_Hud[client].RotationTime);
	g_cCookie_HudSettings.Set(client, sCookieData);
}

void LoadHudSettings(int client)
{
	char sCookieData[32];
	g_cCookie_HudSettings.Get(client, sCookieData, sizeof(sCookieData));

	if (strcmp(sCookieData, "") == 0)
	{
		// No cookie found, use default values (don't save them)
		g_CSettings_Hud[client].Display = true;
		g_CSettings_Hud[client].Name = true;
		g_CSettings_Hud[client].ItemsCount = g_iMaxItemsCount;
		g_CSettings_Hud[client].RotationTime = g_iDefaultRotationTime;
		return;
	}

	// Parse the cookie data: "1|1|10|3" format
	char sParts[4][8];
	int iPartCount = ExplodeString(sCookieData, "|", sParts, sizeof(sParts), sizeof(sParts[]));

	if (iPartCount >= 4)
	{
		// Parse all values as integers
		g_CSettings_Hud[client].Display = (StringToInt(sParts[0]) == 1);
		g_CSettings_Hud[client].Name = (StringToInt(sParts[1]) == 1);
		g_CSettings_Hud[client].ItemsCount = StringToInt(sParts[2]);
		g_CSettings_Hud[client].RotationTime = StringToInt(sParts[3]);

		// Validate ranges
		if (g_CSettings_Hud[client].ItemsCount <= 0 || g_CSettings_Hud[client].ItemsCount > MAX_ShowHudItems)
			g_CSettings_Hud[client].ItemsCount = g_iMaxItemsCount;
		if (g_CSettings_Hud[client].RotationTime <= 0 || g_CSettings_Hud[client].RotationTime > TIMEROTATIONHUD)
			g_CSettings_Hud[client].RotationTime = g_iDefaultRotationTime;
	}
	else
	{
		// Corrupted data, use defaults (don't save them)
		g_CSettings_Hud[client].Display = true;
		g_CSettings_Hud[client].Name = true;
		g_CSettings_Hud[client].ItemsCount = g_iMaxItemsCount;
		g_CSettings_Hud[client].RotationTime = g_iDefaultRotationTime;
	}
}

stock void EWM_Hud_OnClientPutInServer(int iClient)
{
	if (!IsClientSourceTV(iClient))
		return;

	// Initialize HUD settings for SourceTV
	g_CSettings_Hud[iClient].Display = true;
	g_CSettings_Hud[iClient].Name = true;
	g_CSettings_Hud[iClient].ItemsCount = g_iMaxItemsCount;
	g_CSettings_Hud[iClient].RotationTime = g_iDefaultRotationTime;
}

void UpdateCache()
{
	if (!g_bCacheDirty)
		return;

	int iCountHItems = g_iHItemsArray.Length;
	int iCountZMItems = g_iZMItemsArray.Length;
	int iCountAItems = iCountHItems + iCountZMItems;

	g_iCachedItemCounts[0] = iCountHItems;
	g_iCachedItemCounts[1] = iCountZMItems;
	g_iCachedItemCounts[2] = iCountAItems;

	g_bCachedHUDStates[0] = (iCountHItems > 0) && (g_bTeamOnly || (g_bZombieNoItemPry && iCountZMItems <= 0));
	g_bCachedHUDStates[1] = (iCountZMItems > 0) && g_bTeamOnly;

	// Pre-calculate max lists for default item count
	float fDefaultItemCount = float(g_iMaxItemsCount);
	g_iCachedMaxLists[0] = RoundToCeil(float(iCountHItems) / fDefaultItemCount);
	g_iCachedMaxLists[1] = RoundToCeil(float(iCountZMItems) / fDefaultItemCount);
	g_iCachedMaxLists[2] = RoundToCeil(float(iCountAItems) / fDefaultItemCount);

	g_bCacheDirty = false;
}

void MarkCacheDirty()
{
	g_bCacheDirty = true;
}

stock void EWM_Hud_OnPluginStart()
{
	g_iHItemsArray = new ArrayList(32);
	g_iZMItemsArray = new ArrayList(32);
	g_hCvar_DisplayEnabled		= CreateConVar("entwatch_display_enable", "1", "Enable/Disable the display.", _, true, 0.0, true, 1.0);
	g_hCvar_DisplayCooldowns	= CreateConVar("entwatch_display_cooldowns", "1", "Show/Hide the cooldowns on the display.", _, true, 0.0, true, 1.0);
	g_hCvar_Admins_See			= CreateConVar("entwatch_admins_see", "1", "Enable/Disable admins see everything Items", _, true, 0.0, true, 1.0);
	g_hCvar_ZM_pry				= CreateConVar("entwatch_zm_noitem_pry", "0", "Enable/Disable zm pry human Items if zms without items", _, true, 0.0, true, 1.0);
	g_hCvar_MaxHudItemsCount	= CreateConVar("entwatch_hud_items_count", "10", "Max/Default Items Count in hud to display.", _, true, 1.0, true, 10.0);
	g_hCvar_HudRotationDefault	= CreateConVar("entwatch_hud_rotation_time_default", "3", "Default Rotation time in seconds for hud.", _, true, 1.0, true, 5.0);

	g_bDispEnabled = GetConVarBool(g_hCvar_DisplayEnabled);
	g_bDispCooldowns = GetConVarBool(g_hCvar_DisplayCooldowns);
	g_bAdminsSee = GetConVarBool(g_hCvar_Admins_See);
	g_bZombieNoItemPry = GetConVarBool(g_hCvar_ZM_pry);

	RegConsoleCmd("sm_hud", EWM_Hud_Command_ToggleHUD);
	RegConsoleCmd("sm_hudname", EWM_Hud_Command_ToggleHUDName);
	RegConsoleCmd("sm_huditems", EWM_Hud_Command_HudItems);
	RegConsoleCmd("sm_hudtime", EWM_Hud_Command_HudRotation);

	HookConVarChange(g_hCvar_DisplayEnabled, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_DisplayCooldowns, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_Admins_See, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_ZM_pry, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_MaxHudItemsCount, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_HudRotationDefault, Cvar_HUD_Changed);

	SetCookieMenuItem(CookieHandler, 0, "EntWatch Settings");

	g_cCookie_HudSettings = new Cookie("entwatch_hud_settings", "Combined settings (display|name|items|time)", CookieAccess_Private);

	CreateTimer(1.0, EWM_Hud_Timer_DisplayHUD, _, TIMER_REPEAT);
}

public void Cvar_HUD_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_DisplayEnabled)
		g_bDispEnabled = GetConVarBool(convar);
	else if(convar==g_hCvar_DisplayCooldowns)
		g_bDispCooldowns = GetConVarBool(convar);
	else if(convar==g_hCvar_Admins_See)
		g_bAdminsSee = GetConVarBool(convar);
	else if(convar==g_hCvar_ZM_pry)
		g_bZombieNoItemPry = GetConVarBool(convar);
	else if(convar==g_hCvar_MaxHudItemsCount)
		g_iMaxItemsCount = GetConVarInt(convar);
	else if(convar==g_hCvar_HudRotationDefault)
		g_iDefaultRotationTime = GetConVarInt(convar);

	// Prevent this kind of issue: https://github.com/srcdslab/sm-plugin-EntWatch-old/issues/1#issuecomment-1445196178
	if (g_iMaxItemsCount > MAX_ShowHudItems)
		g_iMaxItemsCount = MAX_ShowHudItems;

	if (g_iDefaultRotationTime > TIMEROTATIONHUD)
		g_iDefaultRotationTime = TIMEROTATIONHUD;

	MarkCacheDirty();
}

public void EWM_Hud_OnAllPluginsLoaded()
{
	VerifyNatives();
}

public void EWM_Hud_OnLibraryRemoved(const char[] name)
{
	// Nothing yet
}

public void EWM_Hud_OnLibraryAdded(const char[] name)
{
	// Nothing yet
}

stock VerifyNatives()
{
	VerifyNative_MessageTypeProtobuf();
}

stock void VerifyNative_MessageTypeProtobuf()
{
	bNative_MsgTypeProtobuf = CanTestFeatures() && GetFeatureStatus(FeatureType_Native, "GetUserMessageType") == FeatureStatus_Available && GetUserMessageType() == UM_Protobuf;
}

stock void EWM_Hud_OnClientDisconnect(int iClient)
{
	g_iRotation[iClient][0][0] = 0;
	g_iRotation[iClient][0][1] = 0;

	g_iRotation[iClient][1][0] = 0;
	g_iRotation[iClient][1][1] = 0;

	g_iRotation[iClient][2][0] = 0;
	g_iRotation[iClient][2][1] = 0;
}

stock void EWM_Hud_OnClientCookiesCached(int iClient)
{
	if (!IsValidClient(iClient) || IsFakeClient(iClient))
		return;

	LoadHudSettings(iClient);
}

stock void EWM_Hud_LoadDefaultClientSettings(int iClient)
{
	g_CSettings_Hud[iClient].Display = true;
	g_CSettings_Hud[iClient].ItemsCount = g_iMaxItemsCount;
	g_CSettings_Hud[iClient].RotationTime = g_iDefaultRotationTime;
}

public void EWM_Hud_OnMapStart()
{
	g_iHItemsArray.Clear();
	g_iZMItemsArray.Clear();
	for(int j = 1; j <= MaxClients; j++)
	{
		if(!IsClientInGame(j) || IsClientSourceTV(j)) continue;

		for(int i = 0; i < 3; i++)
		{
			g_iRotation[j][i][0] = 0;
			g_iRotation[j][i][1] = 0;
		}
	}
}

public void EWM_Hud_DisplayCustomHUD(int client, const char[] sMes_Names, const char[] sMes_NoNames)
{
	char msg[MAX_KeyHintText];
	if (g_CSettings_Hud[client].Name)
		Format(msg, sizeof(msg), "%s", sMes_Names);
	else
		Format(msg, sizeof(msg), "%s", sMes_NoNames);

	// Display function in KeyHint
	Handle hBuffer = StartMessageOne("KeyHintText", client);
	if (bNative_MsgTypeProtobuf)
	{
		PbAddString(hBuffer, "hints", msg);
	}
	else
	{
		BfWriteByte(hBuffer, 1);
		BfWriteString(hBuffer, msg);
	}
	EndMessage();
}

public Action EWM_Hud_Timer_DisplayHUD(Handle timer, int client)
{
	// Nothing to display
	if (!g_bDispEnabled || !g_bConfigLoaded)
		return Plugin_Continue;

	static int iRotationTime[MAXPLAYERS + 1];

	g_iHItemsArray.Clear();
	g_iZMItemsArray.Clear();
	for (int i = 0; i < g_ItemList.Length; i++)
	{
		class_ItemList ItemTest;
		g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
		if (ItemTest.Hud && ItemTest.OwnerID != INVALID_ENT_REFERENCE && IsValidEdict(ItemTest.WeaponID))
		{
			int iTeamOwner = GetClientTeam(ItemTest.OwnerID);
			if (iTeamOwner == CS_TEAM_CT)
				g_iHItemsArray.Push(i);
			else if (iTeamOwner == CS_TEAM_T)
				g_iZMItemsArray.Push(i);
		}
	}

	// Update cache with new item counts
	MarkCacheDirty();
	UpdateCache();

	// No items to display
	if (g_iCachedItemCounts[2] == 0)
		return Plugin_Continue;

	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i))
			continue;

		// Skip normal bots but keep SourceTV, and check Display setting for non-SourceTV clients
		if ((IsFakeClient(i) && !IsClientSourceTV(i)) || (!IsClientSourceTV(i) && !g_CSettings_Hud[i].Display))
		{
			iRotationTime[i] = 0;
			continue;
		}

		bool bRotation = false;
		int iTeam = GetClientTeam(i);
		iRotationTime[i]++;
		if (iRotationTime[i] >= g_CSettings_Hud[i].RotationTime)
		{
			iRotationTime[i] = 0;
			bRotation = true;
		}

		// Use cached max lists if client uses default item count
		int iMaxHList = (g_CSettings_Hud[i].ItemsCount == g_iMaxItemsCount) ? g_iCachedMaxLists[0] : RoundToCeil(float(g_iCachedItemCounts[0]) / float(g_CSettings_Hud[i].ItemsCount));
		int iMaxZMList = (g_CSettings_Hud[i].ItemsCount == g_iMaxItemsCount) ? g_iCachedMaxLists[1] : RoundToCeil(float(g_iCachedItemCounts[1]) / float(g_CSettings_Hud[i].ItemsCount));
		int iMaxAList = (g_CSettings_Hud[i].ItemsCount == g_iMaxItemsCount) ? g_iCachedMaxLists[2] : RoundToCeil(float(g_iCachedItemCounts[2]) / float(g_CSettings_Hud[i].ItemsCount));

		UpdateRotation(i, 0, g_iCachedItemCounts[0], iMaxHList, bRotation);
		UpdateRotation(i, 1, g_iCachedItemCounts[1], iMaxZMList, bRotation);
		UpdateRotation(i, 2, g_iCachedItemCounts[2], iMaxAList, bRotation);

		// Clear static buffers before use
		for (int j = 0; j < MAX_HUD_BUFFERS; j++)
		{
			g_sStaticBuffers[j][0] = '\0';
		}

		ConstructHudStrings(i, g_iCachedItemCounts[0], iMaxHList, g_iCachedItemCounts[1], iMaxZMList, g_iCachedItemCounts[2], iMaxAList, g_sStaticBuffers[0], g_sStaticBuffers[1], g_sStaticBuffers[2], g_sStaticBuffers[3], g_sStaticBuffers[4], g_sStaticBuffers[5]);

		// Always display full information to the SourceTV
		if (IsClientSourceTV(i))
		{
			EWM_Hud_DisplayCustomHUD(i, g_sStaticBuffers[4], g_sStaticBuffers[5]);
		}
		else
		{
			bool bIsAdmin = g_bAdminsSee && g_bIsAdmin[i];
			bool bShowHumansHUD = g_bCachedHUDStates[0] && iTeam == CS_TEAM_CT;
			bool bShowZombiesHUD = g_bCachedHUDStates[1] && iTeam == CS_TEAM_T;

			if (bIsAdmin)
				EWM_Hud_DisplayCustomHUD(i, g_sStaticBuffers[4], g_sStaticBuffers[5]);
			else if (bShowHumansHUD)
				EWM_Hud_DisplayCustomHUD(i, g_sStaticBuffers[0], g_sStaticBuffers[1]);
			else if (bShowZombiesHUD)
				EWM_Hud_DisplayCustomHUD(i, g_sStaticBuffers[2], g_sStaticBuffers[3]);
			else
				EWM_Hud_DisplayCustomHUD(i, g_sStaticBuffers[4], g_sStaticBuffers[5]);
		}
	}

	return Plugin_Continue;
}

void UpdateRotation(int client, int type, int count, int maxList, bool bRotation)
{
	if (count != g_iRotation[client][type][0])
	{
		g_iRotation[client][type][0] = count;
		g_iRotation[client][type][1] = 0;
	}
	else
	{
		if (bRotation)
		{
			g_iRotation[client][type][1] += 1;
			if (g_iRotation[client][type][1] > maxList - 1)
				g_iRotation[client][type][1] = 0;
		}
	}
}

void ConstructHudStrings(int client, int iCountHItems, int iMaxHList, int iCountZMItems, int iMaxZMList, int iCountAItems, int iMaxAList, char[] sMes_Humans, char[] sMes_Humans_woname, char[] sMes_Zombies, char[] sMes_Zombies_woname, char[] sMes_Admins, char[] sMes_Admins_woname)
{
	EWM_Hud_Construct_String(client, sMes_Humans, MAX_KeyHintText, sMes_Humans_woname, MAX_KeyHintText, iCountHItems, iMaxHList, 0);
	EWM_Hud_Construct_String(client, sMes_Zombies, MAX_KeyHintText, sMes_Zombies_woname, MAX_KeyHintText, iCountZMItems, iMaxZMList, 1);
	EWM_Hud_Construct_String(client, sMes_Admins, MAX_KeyHintText, sMes_Admins_woname, MAX_KeyHintText, iCountAItems, iMaxAList, 2, iCountHItems);
}

// A function that collects information about items based on parameters
void EWM_Hud_Construct_String(int iClient, char[] sResult, int iSizeResult, char[] sResult_woname, int iSizeResult_woname, int iCountItems, int iMaxList, int iType, int iCountHItems = 0) {
	// No items was found.
	if (iCountItems < 1)
		return;

	int count = g_CSettings_Hud[iClient].ItemsCount;
	static char sBuf[MAX_ShowHudItems][MAX_SizeStringForItem];
	static char sBuf_woname[MAX_ShowHudItems][MAX_SizeStringForItem];
	int iLastIndex = (g_iRotation[iClient][iType][1] + 1) * count;
	if (iLastIndex > iCountItems)
		iLastIndex = iCountItems;
	int iIndex = -1;

	for (int i = g_iRotation[iClient][iType][1] * count; i < iLastIndex; i++)
	{
		iIndex++;
		class_ItemList ItemTest;
		int iFixIndex = (iType == 0) ? g_iHItemsArray.Get(i) : (iType == 1) ? g_iZMItemsArray.Get(i) : (i < iCountHItems) ? g_iHItemsArray.Get(i) : g_iZMItemsArray.Get(i - iCountHItems);

		g_ItemList.GetArray(iFixIndex, ItemTest, sizeof(ItemTest));
		UpdateTime();

		if (ItemTest.CheckDelay() > 0.0)
		{
			FormatEx(sBuf_woname[iIndex], MAX_SizeStringForItem, "%s[-%d]", ItemTest.ShortName, ItemTest.RoundDelay());
			FormatEx(sBuf[iIndex], MAX_SizeStringForItem, "%s: %N", sBuf_woname[iIndex], ItemTest.OwnerID);
		}
		else
		{
			if (g_bDispCooldowns)
			{
				if (ItemTest.LockButton)
				{
					FormatEx(sBuf_woname[iIndex], MAX_SizeStringForItem, "%s[L]", ItemTest.ShortName);
				}
				else
				{
					switch (ItemTest.Mode)
					{
						case 2:
							FormatItemWithCooldown(sBuf_woname[iIndex], ItemTest);
						case 3:
							FormatItemWithUses(sBuf_woname[iIndex], ItemTest);
						case 4:
							FormatItemWithUsesAndCoolodown(sBuf_woname[iIndex], ItemTest);
						case 5:
							FormatItemCooldownAfterUses(sBuf_woname[iIndex], ItemTest);
						case 6, 7:
							FormatItemWithMathValue(sBuf_woname[iIndex], ItemTest);
						default:
							FormatEx(sBuf_woname[iIndex], MAX_SizeStringForItem, "%s[+]", ItemTest.ShortName);
					}
				}

				if (ItemTest.LockButton2)
					FormatEx(sBuf_woname[iIndex], MAX_SizeStringForItem, "%s[L]", sBuf_woname[iIndex]);
				else if (ItemTest.CheckWaitTime(true) > 0)
					FormatEx(sBuf_woname[iIndex], MAX_SizeStringForItem, "%s[%d]", sBuf_woname[iIndex], ItemTest.RoundWaitTime(true));
				else
				{
					switch (ItemTest.Mode2)
					{
						case 2:
						{
							FormatItemWithCooldown(sBuf_woname[iIndex], ItemTest, true);
							ReplaceString(sBuf_woname[iIndex], MAX_SizeStringForItem, "][", "|", false);
						}
						case 3:
							FormatItemWithUses(sBuf_woname[iIndex], ItemTest, true);
						case 4:
							FormatItemWithUsesAndCoolodown(sBuf_woname[iIndex], ItemTest, true);
						case 5:
							FormatItemCooldownAfterUses(sBuf_woname[iIndex], ItemTest, true);
						case 6, 7:
							FormatItemWithMathValue(sBuf_woname[iIndex], ItemTest, true);
					}
				}
				FormatEx(sBuf[iIndex], MAX_SizeStringForItem, "%s: %N", sBuf_woname[iIndex], ItemTest.OwnerID);
			}
			else
			{
				FormatEx(sBuf[iIndex], MAX_SizeStringForItem, "%s: %N", ItemTest.ShortName, ItemTest.OwnerID);
				FormatEx(sBuf_woname[iIndex], MAX_SizeStringForItem, "%s picked up", ItemTest.ShortName);
			}
		}
	}

	for (int i = 0; i <= iIndex; i++)
	{
		StrCat(sResult, iSizeResult, sBuf[i]);
		StrCat(sResult_woname, iSizeResult_woname, sBuf_woname[i]);
		if (i != iIndex)
		{
			StrCat(sResult, iSizeResult, "\n");
			StrCat(sResult_woname, iSizeResult_woname, "\n");
		}
		else if (iMaxList > 1)
		{
			char sCurrentList[MAX_SizeStringForItem];
			FormatEx(sCurrentList, sizeof(sCurrentList), "\n\nList: %i/%i", g_iRotation[iClient][iType][1] + 1, iMaxList);
			StrCat(sResult, iSizeResult, sCurrentList);
			StrCat(sResult_woname, iSizeResult_woname, sCurrentList);
		}
	}
}

void FormatItemWithCooldown(char[] sBuf_woname, class_ItemList ItemTest, bool bSecond = false)
{
	if (!bSecond)
	{
		if (ItemTest.CheckCoolDown() > 0)
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d]", ItemTest.ShortName, ItemTest.RoundCoolDown());
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[R]", ItemTest.ShortName);
	}
	else
	{
		if (ItemTest.CheckCoolDown(true) > 0)
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d]", sBuf_woname, ItemTest.RoundCoolDown(true));
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[R]", sBuf_woname);
	}
}

void FormatItemWithUses(char[] sBuf_woname, class_ItemList ItemTest, bool bSecond = false)
{
	if (!bSecond)
	{
		if (ItemTest.Uses < ItemTest.MaxUses)
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d/%d]", ItemTest.ShortName, ItemTest.Uses, ItemTest.MaxUses);
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[E]", ItemTest.ShortName);
	}
	else
	{
		if (ItemTest.Uses2 < ItemTest.MaxUses2)
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d/%d]", sBuf_woname, ItemTest.Uses2, ItemTest.MaxUses2);
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[E]", sBuf_woname);
	}
}

void FormatItemWithUsesAndCoolodown(char[] sBuf_woname, class_ItemList ItemTest, bool bSecond = false)
{
	if (!bSecond)
	{
		if (ItemTest.Uses < ItemTest.MaxUses)
		{
			if (ItemTest.CoolDown > 0 && ItemTest.CheckCoolDown() > 0)
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d]", ItemTest.ShortName, ItemTest.RoundCoolDown());
			else
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d/%d]", ItemTest.ShortName, ItemTest.Uses, ItemTest.MaxUses);
		}
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[E]", ItemTest.ShortName);
	}
	else
	{
		if (ItemTest.Uses2 < ItemTest.MaxUses2)
		{
			if (ItemTest.CoolDown2 > 0 && ItemTest.CheckCoolDown(true) > 0)
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d]", sBuf_woname, ItemTest.RoundCoolDown(true));
			else
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d/%d]", sBuf_woname, ItemTest.Uses2, ItemTest.MaxUses2);
		}
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[E]", sBuf_woname);
	}
}

void FormatItemCooldownAfterUses(char[] sBuf_woname, class_ItemList ItemTest, bool bSecond = false)
{
	if (!bSecond)
	{
		if (ItemTest.Uses < ItemTest.MaxUses)
		{
			if (ItemTest.CoolDown > 0 && ItemTest.CheckCoolDown() > 0)
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d]", ItemTest.ShortName, ItemTest.RoundCoolDown());
			else
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d/%d]", ItemTest.ShortName, ItemTest.Uses, ItemTest.MaxUses);
		}
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[E]", ItemTest.ShortName);
	}
	else
	{
		if (ItemTest.Uses2 < ItemTest.MaxUses2)
		{
			if (ItemTest.CoolDown2 > 0 && ItemTest.CheckCoolDown(true) > 0)
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d]", sBuf_woname, ItemTest.RoundCoolDown(true));
			else
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d/%d]", sBuf_woname, ItemTest.Uses2, ItemTest.MaxUses2);
		}
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[E]", sBuf_woname);
	}
}

void FormatItemWithMathValue(char[] sBuf_woname, class_ItemList ItemTest, bool bSecond = false)
{
	if (!bSecond)
	{
		if (ItemTest.MathValue > 0)
		{
			if (ItemTest.CoolDown > 0 && ItemTest.CheckCoolDown() > 0)
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d]", ItemTest.ShortName, ItemTest.RoundCoolDown());
			else
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d/%d]", ItemTest.ShortName, ItemTest.MathValue, ItemTest.MathValueMax);
		}
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[E]", ItemTest.ShortName);
	}
	else
	{
		if (ItemTest.MathValue2 > 0)
		{
			if (ItemTest.CoolDown2 > 0 && ItemTest.CheckCoolDown(true) > 0)
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d]", sBuf_woname, ItemTest.RoundCoolDown(true));
			else
				FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[%d/%d]", sBuf_woname, ItemTest.MathValue2, ItemTest.MathValueMax2);
		}
		else
			FormatEx(sBuf_woname, MAX_SizeStringForItem, "%s[E]", sBuf_woname);
	}
}

// Client command handlers
public Action EWM_Hud_Command_ToggleHUD(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		char sStatus[32], sColor[32];
		g_CSettings_Hud[iClient].Display = !g_CSettings_Hud[iClient].Display;
		SaveHudSettings(iClient);

		sStatus = g_CSettings_Hud[iClient].Display ? "Enabled" : "Disabled";
		sColor = g_CSettings_Hud[iClient].Display ? g_SchemeConfig.Color_Enabled : g_SchemeConfig.Color_Disabled;
		CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud", sColor, sStatus);
	}
	return Plugin_Handled;
}

public Action EWM_Hud_Command_ToggleHUDName(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		char sStatus[32], sColor[32];
		g_CSettings_Hud[iClient].Name = !g_CSettings_Hud[iClient].Name;
		SaveHudSettings(iClient);

		sStatus = g_CSettings_Hud[iClient].Name ? "Enabled" : "Disabled";
		sColor = g_CSettings_Hud[iClient].Name ? g_SchemeConfig.Color_Enabled : g_SchemeConfig.Color_Disabled;
		CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Names", sColor, sStatus);
	}
	return Plugin_Handled;
}

public Action EWM_Hud_Command_HudItems(int iClient, int iArgs)
{
	if(!iClient) return Plugin_Handled;

	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		if (!AreClientCookiesCached(iClient)) return Plugin_Handled;

		if (GetCmdArgs() < 1)
		{
			DisplayMenuItemsNumberHUD(iClient);
			CReplyToCommand(iClient, "%s%t %s%t: sm_huditems <1-%d>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage", MAX_ShowHudItems);
			return Plugin_Handled;
		}

		char sArg[32];
		GetCmdArg(1, sArg, sizeof(sArg));

		int iCount;
		if (!StringToIntEx(sArg, iCount) || iCount <= 0 || iCount > MAX_ShowHudItems)
		{
			CReplyToCommand(iClient, "%s%t %s%t <1-%d>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Invalid Amount", MAX_ShowHudItems);
			return Plugin_Handled;
		}

		g_CSettings_Hud[iClient].ItemsCount = iCount;
		SaveHudSettings(iClient);
		CPrintToChat(iClient, "%s%t %s%t %s%t (%d)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Items", g_SchemeConfig.Color_Enabled, "Saved", g_CSettings_Hud[iClient].RotationTime);
	}

	return Plugin_Handled;
}

public Action EWM_Hud_Command_HudRotation(int iClient, int iArgs)
{
	if(!iClient) return Plugin_Handled;

	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		if (!AreClientCookiesCached(iClient)) return Plugin_Handled;

		if (GetCmdArgs() < 1)
		{
			DisplayMenuRotationTimeHUD(iClient);
			CReplyToCommand(iClient, "%s%t %s%t: sm_hudtime <1-%d>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage", TIMEROTATIONHUD);
			return Plugin_Handled;
		}

		char sArg[32];
		GetCmdArg(1, sArg, sizeof(sArg));

		int iTime;
		if (!StringToIntEx(sArg, iTime) || iTime <= 0 || iTime > TIMEROTATIONHUD)
		{
			CReplyToCommand(iClient, "%s%t %s%t <1-%d>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Invalid Amount", TIMEROTATIONHUD);
			return Plugin_Handled;
		}

		g_CSettings_Hud[iClient].RotationTime = iTime;
		SaveHudSettings(iClient);
		CPrintToChat(iClient, "%s%t %s%t %s%t (%d)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Rotation Time", g_SchemeConfig.Color_Enabled, "Saved", g_CSettings_Hud[iClient].RotationTime);
	}

	return Plugin_Handled;
}

// Cookies Handler Menu
public void CookieHandler(int iClient, CookieMenuAction action, any info, char[] buffer, int maxlen)
{
	switch (action)
	{
		case CookieMenuAction_SelectOption:
		{
			DisplayCookieMenu(iClient);
		}
	}
}

// Cookie Settings Menu
public void DisplayCookieMenu(int iClient)
{
	Menu menu = new Menu(CookieMenuHandler, MENU_ACTIONS_ALL);

	menu.SetTitle("%T %T %T", "EW_Tag", iClient, "HUD", iClient, "Client Settings", iClient);

	char hudisplay[64], hudname[64], huditems[64], hudtime[64];
	FormatEx(hudisplay, sizeof(hudisplay), "%T", "Status", iClient);
	FormatEx(hudname, sizeof(hudname), "%T", "Hud Names", iClient);
	FormatEx(huditems, sizeof(huditems), "%T", "Hud Item Number", iClient);
	FormatEx(hudtime, sizeof(hudtime), "%T", "Hud Rotation Time", iClient);

	menu.AddItem("display", hudisplay);
	menu.AddItem("name", hudname);
	menu.AddItem("items", huditems);
	menu.AddItem("time", hudtime);

	menu.ExitBackButton = true;
	menu.ExitButton = true;
	menu.Display(iClient, MENU_TIME_FOREVER);
}

// Cookies Settings Menu Handler
public int CookieMenuHandler(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_DisplayItem:
		{
			char info[64];
			char display[64];
			menu.GetItem(param2, info, sizeof(info));
			if (strcmp(info, "display", false) == 0)
			{
				if (g_CSettings_Hud[param1].Display)
					FormatEx(display, sizeof(display), "%T: %T", "Status", param1, "Enabled", param1);
				else
					FormatEx(display, sizeof(display), "%T: %T", "Status", param1, "Disabled", param1);

				return RedrawMenuItem(display);
			}
			else if (strcmp(info, "name", false) == 0)
			{
				if (g_CSettings_Hud[param1].Name)
					FormatEx(display, sizeof(display), "%T: %T", "Hud Names", param1, "Enabled", param1);
				else
					FormatEx(display, sizeof(display), "%T: %T", "Hud Names", param1, "Disabled", param1);

				return RedrawMenuItem(display);
			}
		}

		case MenuAction_Select:
		{
			char info[64], status[32], color[32];
			menu.GetItem(param2, info, sizeof(info));
			if (strcmp(info, "display", false) == 0)
			{
				g_CSettings_Hud[param1].Display = !g_CSettings_Hud[param1].Display;
				SaveHudSettings(param1);

				color = g_CSettings_Hud[param1].Display ? g_SchemeConfig.Color_Enabled : g_SchemeConfig.Color_Disabled;
				status = g_CSettings_Hud[param1].Display ? "Enabled" : "Disabled";

				CPrintToChat(param1, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud", color, status);
				DisplayCookieMenu(param1);
			}
			else if (strcmp(info, "name", false) == 0)
			{
				g_CSettings_Hud[param1].Name = !g_CSettings_Hud[param1].Name;
				SaveHudSettings(param1);

				color = g_CSettings_Hud[param1].Name ? g_SchemeConfig.Color_Enabled : g_SchemeConfig.Color_Disabled;
				status = g_CSettings_Hud[param1].Name ? "Enabled" : "Disabled";

				CPrintToChat(param1, "%s%t %s%t %t: %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Names", color, status);
				DisplayCookieMenu(param1);
			}
			else if (strcmp(info, "items", false) == 0)
			{
				DisplayMenuItemsNumberHUD(param1);
			}
			else if (strcmp(info, "time", false) == 0)
			{
				DisplayMenuRotationTimeHUD(param1);
			}
		}

		case MenuAction_Cancel: {
			if(param2 == MenuCancel_ExitBack)
			{
				ShowCookieMenu(param1);
				return 0;
			}
		}
		case MenuAction_End: {
			delete menu;
		}
	}
	return 0;
}

public void DisplayMenuItemsNumberHUD(int iClient)
{
	Menu menu = new Menu(DisplayMenuItemsNumberHUDMenuHandler);

	menu.SetTitle("%T %T", "EW_Tag", iClient, "Hud Items", iClient);

	for(int i = MAX_ShowHudItems; i >= 1; i--)
	{
		char buf[3];
		IntToString(i, buf, sizeof(buf));
		menu.AddItem(buf, buf, (g_CSettings_Hud[iClient].ItemsCount == i) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	}

	menu.ExitBackButton = true;
	menu.ExitButton = true;
	menu.Display(iClient, MENU_TIME_FOREVER);
}

public int DisplayMenuItemsNumberHUDMenuHandler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			char sBuffer[10];
			menu.GetItem(param2, sBuffer, sizeof(sBuffer));
			int count = StringToInt(sBuffer);

			g_CSettings_Hud[param1].ItemsCount = count;
			SaveHudSettings(param1);
			CPrintToChat(param1, "%s%t %s%t %t: %s%t (%d)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Items", g_SchemeConfig.Color_Enabled, "Saved", g_CSettings_Hud[param1].ItemsCount);
			DisplayCookieMenu(param1);
		}
		case MenuAction_Cancel:
		{
			if(param2 == MenuCancel_ExitBack)
			{
				DisplayCookieMenu(param1);
				return 0;
			}
		}
		case MenuAction_End:
		{
			delete menu;
		}
	}

	return 0;
}

public void DisplayMenuRotationTimeHUD(int iClient)
{
	Menu menu = new Menu(DisplayMenuRotationTimeHUDMenuHandler);
	menu.SetTitle("%T %T", "EW_Tag", iClient, "Hud Rotation Time", iClient);

	for(int i = 1; i <= TIMEROTATIONHUD; i++)
	{
		char buf[3];
		IntToString(i, buf, sizeof(buf));
		menu.AddItem(buf, buf, (g_CSettings_Hud[iClient].RotationTime == i) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	}

	menu.ExitBackButton = true;
	menu.ExitButton = true;
	menu.Display(iClient, MENU_TIME_FOREVER);
}

public int DisplayMenuRotationTimeHUDMenuHandler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			char sBuffer[10];
			menu.GetItem(param2, sBuffer, sizeof(sBuffer));
			int time = StringToInt(sBuffer);

			g_CSettings_Hud[param1].RotationTime = time;
			SaveHudSettings(param1);
			CPrintToChat(param1, "%s%t %s%t %t: %s%t (%d)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Rotation Time", g_SchemeConfig.Color_Enabled, "Saved", g_CSettings_Hud[param1].RotationTime);
			DisplayCookieMenu(param1);
		}
		case MenuAction_Cancel:
		{
			if(param2 == MenuCancel_ExitBack)
			{
				DisplayCookieMenu(param1);
				return 0;
			}
		}
		case MenuAction_End:
		{
			delete menu;
		}
	}

	return 0;
}
