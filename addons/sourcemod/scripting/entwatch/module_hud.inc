#define EW_MODULE_HUD

// Purpose: Displays items information in HUD or KeyHint
#define MAXSHOWHUDITEMS 12
#define MAXSIZESTRINGFORITEM 40
#define TIMEROTATIONHUD 3
#define MINPOS_HUD -1.0
#define MAXPOS_HUD 1.0

// Include to display more items information in the HUD
#include <bighud>

char colorsList[][] = {
	"255 0 0 255 Red", 
	"128 0 128 255 Purple", 
	"0 128 0 255 Green", 
	"0 255 0 255 Lime", 
	"0 0 255 255 Blue",
	"0 255 255 255 Cyan", 
	"255 215 0 255 Gold",
	"255 255 255 255 White"
};

// Purpose: Module settings
ConVar	g_hCvar_DisplayEnabled,
		g_hCvar_DisplayCooldowns,
		g_hCvar_Admins_See,
		g_hCvar_HUD_Channel,
		g_hCvar_ZM_pry,
		g_hCvar_DefaultHudType,
		g_hCvar_MaxHudItemsCount,
		g_hCvar_HudRotationDefault;

// Purpose: Module Client settings
Handle	g_hCookie_Display		  = null,
		g_hCookie_HudType         = null,
		g_hCookie_HudColor		  = null,
		g_hCookie_HudPos		  = null,
		g_hCookie_HudName		  = null,
		g_hCookie_HudItemsCount   = null,
		g_hCookie_HudRotationTime    = null;

// Purpose: Module Local settings
bool	g_bDispEnabled = true,
		g_bDispCooldowns = true,
		g_bAdminsSee = true,
		g_bZombieNoItemPry = false;
		bPluginDynamic = false,
		bNative_Dynamic = false,
		bNative_MsgTypeProtobuf = false;

int		g_iHUDChannel = 5;
int		g_iDefaultRotationTime = 3;
int		g_iMaxItemsCount = 10;

int			g_iRotation[MAXPLAYERS+1][3][2]; // [client][0] - Humans, [client][1] - Zombies, [client][2] - Admins, [x][0] - count Items, [client][x][1] - current # list
ArrayList	g_iHItemsArray;
ArrayList	g_iZMItemsArray;

class_ClientSettings_Hud g_CSettings_Hud[MAXPLAYERS+1];

stock void EWM_Hud_OnPluginStart()
{
	g_iHItemsArray = new ArrayList(32);
	g_iZMItemsArray = new ArrayList(32);
	g_hCvar_DisplayEnabled		= CreateConVar("entwatch_display_enable", "1", "Enable/Disable the display.", _, true, 0.0, true, 1.0);
	g_hCvar_DisplayCooldowns	= CreateConVar("entwatch_display_cooldowns", "1", "Show/Hide the cooldowns on the display.", _, true, 0.0, true, 1.0);
	g_hCvar_Admins_See			= CreateConVar("entwatch_admins_see", "1", "Enable/Disable admins see everything Items", _, true, 0.0, true, 1.0);
	g_hCvar_HUD_Channel			= CreateConVar("entwatch_hud_channel", "5", "Change HUD Channel/Group Dynamic channel.", _, true, 0.0, true, 5.0);
	g_hCvar_ZM_pry				= CreateConVar("entwatch_zm_noitem_pry", "0", "Enable/Disable zm pry human Items if zms without items", _, true, 0.0, true, 1.0);
	g_hCvar_DefaultHudType		= CreateConVar("entwatch_hud_type_default", "0", "Default type of hud (0 = KeyHintText, 1 = HUD).", _, true, 0.0, true, 1.0);
	g_hCvar_MaxHudItemsCount	= CreateConVar("entwatch_hud_items_count", "10", "Max/Default Items Count in hud to display.", _, true, 1.0, true, 10.0);
	g_hCvar_HudRotationDefault	= CreateConVar("entwatch_hud_rotation_time_default", "3", "Default Rotation time in seconds for hud.", _, true, 1.0, true, 5.0);
	
	g_bDispEnabled = GetConVarBool(g_hCvar_DisplayEnabled);
	g_bDispCooldowns = GetConVarBool(g_hCvar_DisplayCooldowns);
	g_bAdminsSee = GetConVarBool(g_hCvar_Admins_See);
	g_iHUDChannel = GetConVarInt(g_hCvar_HUD_Channel);
	g_bZombieNoItemPry = GetConVarBool(g_hCvar_ZM_pry);

	RegConsoleCmd("sm_hud", EWM_Hud_Command_ToggleHUD);
	RegConsoleCmd("sm_hudtype", EWM_Hud_Command_ToggleHUDType);
	RegConsoleCmd("sm_hudname", EWM_Hud_Command_ToggleHUDName);
	RegConsoleCmd("sm_hudpos", EWM_Hud_Command_Hudpos);
	RegConsoleCmd("sm_hudcolor", EWM_Hud_Command_HudColor);
	RegConsoleCmd("sm_huditems", EWM_Hud_Command_HudItems);
	RegConsoleCmd("sm_hudtime", EWM_Hud_Command_HudRotation);

	HookConVarChange(g_hCvar_DisplayEnabled, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_DisplayCooldowns, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_Admins_See, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_HUD_Channel, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_ZM_pry, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_DefaultHudType, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_MaxHudItemsCount, Cvar_HUD_Changed);
	HookConVarChange(g_hCvar_HudRotationDefault, Cvar_HUD_Changed);

	SetCookieMenuItem(CookieHandler, 0, "EntWatch Settings");

	g_hCookie_Display     	= RegClientCookie("entwatch_display", "", CookieAccess_Private);
	g_hCookie_HudType     	= RegClientCookie("entwatch_hudtype", "", CookieAccess_Private);
	g_hCookie_HudName     	= RegClientCookie("entwatch_hudname", "", CookieAccess_Private);
	g_hCookie_HudColor    	= RegClientCookie("entwatch_hudcolor_rgba", "", CookieAccess_Private);
	g_hCookie_HudPos      	= RegClientCookie("entwatch_hudpos", "", CookieAccess_Private);
	g_hCookie_HudItemsCount	= RegClientCookie("entwatch_huditems_count", "", CookieAccess_Private);
	g_hCookie_HudRotationTime = RegClientCookie("entwatch_hudrotation_time", "", CookieAccess_Private);
	
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
	else if(convar==g_hCvar_HUD_Channel)
		g_iHUDChannel = GetConVarInt(convar);
	else if(convar==g_hCvar_ZM_pry)
		g_bZombieNoItemPry = GetConVarBool(convar);
	else if(convar==g_hCvar_MaxHudItemsCount)
		g_iMaxItemsCount = GetConVarInt(convar);
	else if(convar==g_hCvar_HudRotationDefault)
		g_iDefaultRotationTime = GetConVarInt(convar);

	// Prevent this kind of issue: https://github.com/srcdslab/sm-plugin-EntWatch-old/issues/1#issuecomment-1445196178
	if (g_iMaxItemsCount > MAXSHOWHUDITEMS)
		g_iMaxItemsCount = MAXSHOWHUDITEMS;

	if (g_iDefaultRotationTime > TIMEROTATIONHUD)
		g_iDefaultRotationTime = TIMEROTATIONHUD;
}

public void EWM_Hud_OnAllPluginsLoaded()
{
	VerifyNatives();
}

public void EWM_Hud_OnLibraryRemoved(const char[] name)
{
	if (strcmp(name, "DynamicChannels", false) == 0)
	{
		bPluginDynamic = false;
		VerifyNatives_DynamicChannels();
	}
}
 
public void EWM_Hud_OnLibraryAdded(const char[] name)
{
	if (strcmp(name, "DynamicChannels", false) == 0)
	{
		bPluginDynamic = true;
		VerifyNatives_DynamicChannels();
	}
}

stock VerifyNatives()
{
	VerifyNatives_DynamicChannels();
	VerifyNative_MessageTypeProtobuf();
}

stock void VerifyNatives_DynamicChannels()
{
	bNative_Dynamic = bPluginDynamic && CanTestFeatures() && GetFeatureStatus(FeatureType_Native, "GetDynamicChannel") == FeatureStatus_Available;
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

	char sBuffer_cookie[32];
	
	// Display
	GetClientCookie(iClient, g_hCookie_Display, sBuffer_cookie, sizeof(sBuffer_cookie));
	if (strcmp(sBuffer_cookie, "") == 0)
	{
		SetClientCookie(iClient, g_hCookie_Display, "1");
		strcopy(sBuffer_cookie, sizeof(sBuffer_cookie), "1");
	}
	g_CSettings_Hud[iClient].Display = view_as<bool>(StringToInt(sBuffer_cookie));

	//Hud Type
	GetClientCookie(iClient, g_hCookie_HudType, sBuffer_cookie, sizeof(sBuffer_cookie));
	if (strcmp(sBuffer_cookie, "") == 0)
	{
		char defaultValue[32];
		g_hCvar_DefaultHudType.GetString(defaultValue, sizeof(defaultValue));
		SetClientCookie(iClient, g_hCookie_HudType, defaultValue);
		strcopy(sBuffer_cookie, sizeof(sBuffer_cookie), defaultValue);
	}
	g_CSettings_Hud[iClient].Type = StringToInt(sBuffer_cookie);

	//Hud Names
	GetClientCookie(iClient, g_hCookie_HudName, sBuffer_cookie, sizeof(sBuffer_cookie));
	if (strcmp(sBuffer_cookie, "") == 0)
	{
		SetClientCookie(iClient, g_hCookie_HudName, "1");
		strcopy(sBuffer_cookie, sizeof(sBuffer_cookie), "1");
	}
	g_CSettings_Hud[iClient].Name = view_as<bool>(StringToInt(sBuffer_cookie));
	
	// Colors RBGA
	GetClientCookie(iClient, g_hCookie_HudColor, sBuffer_cookie, sizeof(sBuffer_cookie));
	if (strcmp(sBuffer_cookie, "") == 0)
	{
		FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", g_SchemeConfig.Color_HUD[0], g_SchemeConfig.Color_HUD[1], g_SchemeConfig.Color_HUD[2], g_SchemeConfig.Color_HUD[3]);
		SetClientCookie(iClient, g_hCookie_HudColor, sBuffer_cookie);
		g_CSettings_Hud[iClient].Color[0] = g_SchemeConfig.Color_HUD[0];
		g_CSettings_Hud[iClient].Color[1] = g_SchemeConfig.Color_HUD[1];
		g_CSettings_Hud[iClient].Color[2] = g_SchemeConfig.Color_HUD[2];
		g_CSettings_Hud[iClient].Color[3] = g_SchemeConfig.Color_HUD[3];
	}else 
	{
		char Explode_HudColor[4][32];
		ExplodeString(sBuffer_cookie, "/", Explode_HudColor, 4, 32);
		
		int Color_validate = StringToInt(Explode_HudColor[0]);
		if(Color_validate >= 0 && Color_validate <= 255) g_CSettings_Hud[iClient].Color[0] = Color_validate;
			else g_CSettings_Hud[iClient].Color[0] = g_SchemeConfig.Color_HUD[0];
			
		Color_validate = StringToInt(Explode_HudColor[1]);
		if(Color_validate >= 0 && Color_validate <= 255) g_CSettings_Hud[iClient].Color[1] = Color_validate;
			else g_CSettings_Hud[iClient].Color[1] = g_SchemeConfig.Color_HUD[1];
		
		Color_validate = StringToInt(Explode_HudColor[2]);
		if(Color_validate >= 0 && Color_validate <= 255) g_CSettings_Hud[iClient].Color[2] = Color_validate;
			else g_CSettings_Hud[iClient].Color[2] = g_SchemeConfig.Color_HUD[2];
			
		Color_validate = StringToInt(Explode_HudColor[3]);
		if(Color_validate >= 0 && Color_validate <= 255) g_CSettings_Hud[iClient].Color[3] = Color_validate;
			else g_CSettings_Hud[iClient].Color[3] = g_SchemeConfig.Color_HUD[3];
	}

	// Position
	GetClientCookie(iClient, g_hCookie_HudPos, sBuffer_cookie, sizeof(sBuffer_cookie));
	if (strcmp(sBuffer_cookie, "") == 0)
	{
		FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", g_SchemeConfig.Pos_HUD_X, g_SchemeConfig.Pos_HUD_Y);
		SetClientCookie(iClient, g_hCookie_HudPos, sBuffer_cookie);
		g_CSettings_Hud[iClient].Pos_X = g_SchemeConfig.Pos_HUD_X;
		g_CSettings_Hud[iClient].Pos_Y = g_SchemeConfig.Pos_HUD_Y;
	}else 
	{
		char Explode_HudPosition[2][32];
		ExplodeString(sBuffer_cookie, "/", Explode_HudPosition, 2, 32);
		
		float Pos_validate = StringToFloat(Explode_HudPosition[0]);
		if((Pos_validate >= 0.0 && Pos_validate <= 1.0) || Pos_validate == -1.0) g_CSettings_Hud[iClient].Pos_X = Pos_validate;
			else g_CSettings_Hud[iClient].Pos_X = g_SchemeConfig.Pos_HUD_X;
			
		Pos_validate = StringToFloat(Explode_HudPosition[1]);
		if((Pos_validate >= 0.0 && Pos_validate <= 1.0) || Pos_validate == -1.0) g_CSettings_Hud[iClient].Pos_Y = Pos_validate;
			else g_CSettings_Hud[iClient].Pos_Y = g_SchemeConfig.Pos_HUD_Y;
	}

	// Items count
	GetClientCookie(iClient, g_hCookie_HudItemsCount, sBuffer_cookie, sizeof(sBuffer_cookie));
	if (strcmp(sBuffer_cookie, "") == 0) 
	{
		FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%d", g_iMaxItemsCount);
		SetClientCookie(iClient, g_hCookie_HudItemsCount, sBuffer_cookie);
		g_CSettings_Hud[iClient].ItemsCount = g_iMaxItemsCount;
	}else
	{
		int count = StringToInt(sBuffer_cookie);
		g_CSettings_Hud[iClient].ItemsCount = count;
	}
	
	// Rotation time
	GetClientCookie(iClient, g_hCookie_HudRotationTime, sBuffer_cookie, sizeof(sBuffer_cookie));
	if (strcmp(sBuffer_cookie, "") == 0) 
	{
		FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%d", g_iDefaultRotationTime);
		SetClientCookie(iClient, g_hCookie_HudRotationTime, sBuffer_cookie);
		g_CSettings_Hud[iClient].RotationTime = g_iDefaultRotationTime;
	}else
	{
		int time = StringToInt(sBuffer_cookie);
		g_CSettings_Hud[iClient].RotationTime = time;
	}
}

stock void EWM_Hud_LoadDefaultClientSettings(int iClient)
{
	g_CSettings_Hud[iClient].Display = true;
	g_CSettings_Hud[iClient].Color[0] = g_SchemeConfig.Color_HUD[0];
	g_CSettings_Hud[iClient].Color[1] = g_SchemeConfig.Color_HUD[1];
	g_CSettings_Hud[iClient].Color[2] = g_SchemeConfig.Color_HUD[2];
	g_CSettings_Hud[iClient].Color[3] = g_SchemeConfig.Color_HUD[3];
	g_CSettings_Hud[iClient].Pos_X = g_SchemeConfig.Pos_HUD_X;
	g_CSettings_Hud[iClient].Pos_Y = g_SchemeConfig.Pos_HUD_Y;
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

public void EWM_Hud_DisplayCustomHUD(int client, int iHUDChannel, const char[] msg)
{
#if defined _DynamicChannels_included_
	if (bNative_Dynamic)
		iHUDChannel = GetDynamicChannel(iHUDChannel);
#endif

	// Display function in KeyHint
	if (g_CSettings_Hud[client].Type <= 0)
	{
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
	else if (g_CSettings_Hud[client].Type >= 1)
	{
		switch (g_evGameEngine)
		{
			case Engine_CSGO:
			{
				BigHud_ShowHudText(client, iHUDChannel, msg);
			}
			default:
			{
				ShowHudText(client, iHUDChannel, msg);
			}
		}
	}
}

public Action EWM_Hud_Timer_DisplayHUD(Handle timer, int client)
{
	static int iRotationTime[MAXPLAYERS + 1];
	
	if (g_bDispEnabled && g_bConfigLoaded)
	{
		g_iHItemsArray.Clear();
		g_iZMItemsArray.Clear();
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.Hud && ItemTest.OwnerID != INVALID_ENT_REFERENCE && IsValidEdict(ItemTest.WeaponID))
			{
				int iTeamOwner = GetClientTeam(ItemTest.OwnerID);
				if(iTeamOwner == CS_TEAM_CT) g_iHItemsArray.Push(i);
				else if(iTeamOwner == CS_TEAM_T) g_iZMItemsArray.Push(i);
			}
		}
		
		// Based on client preferences, we split the entire list into sheets
		int iCountHItems = g_iHItemsArray.Length;
		int iCountZMItems = g_iZMItemsArray.Length;
		int iCountAItems = iCountHItems + iCountZMItems;
		
		// We display information in the corresponding HUD
		if(iCountHItems > 0 || iCountZMItems > 0)
		{
			for(int i = 1; i <= MaxClients; i++)
			{
				//if(IsClientInGame(i) && !IsFakeClient(i) && g_CSettings_Hud[i].Display && (!IsVoteInProgress() || !IsClientInVotePool(i)) && GetClientMenu(i) == MenuSource_None)
				if(IsClientInGame(i) && !IsFakeClient(i) && g_CSettings_Hud[i].Display && GetClientMenu(i) == MenuSource_None)
				{
					bool bRotation = false;
					int iTeam = GetClientTeam(i);
					iRotationTime[i]++;
					if(iRotationTime[i]>=g_CSettings_Hud[i].RotationTime)
					{
						iRotationTime[i] = 0;
						bRotation = true;
					}
					
					float fItemCount = float(g_CSettings_Hud[i].ItemsCount);
					int iMaxHList = RoundToCeil(float(iCountHItems)/fItemCount);
					int iMaxZMList = RoundToCeil(float(iCountZMItems)/fItemCount);
					int iMaxAList = RoundToCeil(float(iCountAItems)/fItemCount);
					
					if(iCountHItems != g_iRotation[i][0][0])
					{
						g_iRotation[i][0][0] = iCountHItems;
						g_iRotation[i][0][1] = 0;
					}else
					{
						if(bRotation)
						{
							g_iRotation[i][0][1] += 1;
							if(g_iRotation[i][0][1] > iMaxHList-1) g_iRotation[i][0][1] = 0;
						}
					}
					if(iCountZMItems != g_iRotation[i][1][0])
					{
						g_iRotation[i][1][0] = iCountZMItems;
						g_iRotation[i][1][1] = 0;
					}else
					{
						if(bRotation)
						{
							g_iRotation[i][1][1] += 1;
							if(g_iRotation[i][1][1] > iMaxZMList-1) g_iRotation[i][1][1] = 0;
						}
					}
					if(iCountAItems != g_iRotation[i][2][0])
					{
						g_iRotation[i][2][0] = iCountAItems;
						g_iRotation[i][2][1] = 0;
					}else
					{
						if(bRotation)
						{
							g_iRotation[i][2][1] += 1;
							if(g_iRotation[i][2][1] > iMaxAList-1) g_iRotation[i][2][1] = 0;
						}
					}
					
					int maxBufferSize = 1024;
					// KeyHintText is maximum 255 bytes
					if (g_evGameEngine != Engine_CSGO)
						maxBufferSize = 254;

					char[] sMes_Humans = new char[maxBufferSize];
					char[] sMes_Humans_woname = new char[maxBufferSize];
					char[] sMes_Zombies = new char[maxBufferSize];
					char[] sMes_Zombies_woname = new char[maxBufferSize];
					char[] sMes_Admins = new char[maxBufferSize];
					char[] sMes_Admins_woname = new char[maxBufferSize];
					EWM_Hud_Construct_String(i, sMes_Humans, maxBufferSize, sMes_Humans_woname, maxBufferSize, iCountHItems, iMaxHList, 0);
					EWM_Hud_Construct_String(i, sMes_Zombies, maxBufferSize, sMes_Zombies_woname, maxBufferSize, iCountZMItems, iMaxZMList, 1);
					EWM_Hud_Construct_String(i, sMes_Admins, maxBufferSize, sMes_Admins_woname, maxBufferSize, iCountAItems, iMaxAList, 2, iCountHItems);
					
					switch (g_evGameEngine)
					{
						case Engine_CSGO:
						{
							BigHud_SetHudTextParams(g_CSettings_Hud[i].Pos_X, g_CSettings_Hud[i].Pos_Y, 1.1, g_CSettings_Hud[i].Color[0], g_CSettings_Hud[i].Color[1], g_CSettings_Hud[i].Color[2], g_CSettings_Hud[i].Color[3], 0, 0.0, 0.0, 0.0);
						}
						default:
						{
							SetHudTextParams(g_CSettings_Hud[i].Pos_X, g_CSettings_Hud[i].Pos_Y, 1.1, g_CSettings_Hud[i].Color[0], g_CSettings_Hud[i].Color[1], g_CSettings_Hud[i].Color[2], g_CSettings_Hud[i].Color[3], 0, 0.0, 0.0, 0.0);
						}
					}

					if(!IsClientSourceTV(i)) // Always display full information to the SourceTV
					{
						if(g_bAdminsSee && g_bIsAdmin[i])
						{
							if(g_CSettings_Hud[i].Name)
							{
								EWM_Hud_DisplayCustomHUD(i, g_iHUDChannel, sMes_Admins);
							}else
							{
								EWM_Hud_DisplayCustomHUD(i, g_iHUDChannel, sMes_Admins_woname);
							}
						}else if((g_bTeamOnly && iTeam == CS_TEAM_CT) || (g_bZombieNoItemPry && iCountZMItems<=0 && iTeam == CS_TEAM_T))
						{
							if(iCountHItems > 0)
							{
								if(g_CSettings_Hud[i].Name)
								{
									EWM_Hud_DisplayCustomHUD(i, g_iHUDChannel, sMes_Humans);
								}else
								{
									EWM_Hud_DisplayCustomHUD(i, g_iHUDChannel, sMes_Humans_woname);
								}
							}
						}else if(g_bTeamOnly && iTeam == CS_TEAM_T)
						{
							if(iCountZMItems > 0)
							{
								if(g_CSettings_Hud[i].Name)
								{
									EWM_Hud_DisplayCustomHUD(i, g_iHUDChannel, sMes_Zombies);
								}else
								{
									EWM_Hud_DisplayCustomHUD(i, g_iHUDChannel, sMes_Zombies_woname);
								}
							}
						}else
						{
							if(g_CSettings_Hud[i].Name)
							{
								EWM_Hud_DisplayCustomHUD(i, g_iHUDChannel, sMes_Admins);
							}else
							{
								EWM_Hud_DisplayCustomHUD(i, g_iHUDChannel, sMes_Admins_woname);
							}
						}
					}
				}else
				{
					iRotationTime[i] = 0;
				}
			}
		}
	}
	return Plugin_Continue;
}

// A function that collects information about items based on parameters
void EWM_Hud_Construct_String(int iClient, char[] sResult, int iSizeResult, char[] sResult_woname, int iSizeResult_woname, int iCountItems, int iMaxList, int iType, int iCountHItems = 0)
{
	if(iCountItems > 0)
	{
		int count = g_CSettings_Hud[iClient].ItemsCount;
		char[][] sBuf = new char[count][MAXSIZESTRINGFORITEM];
		char[][] sBuf_woname = new char[count][MAXSIZESTRINGFORITEM];
		int iLastIndex = (g_iRotation[iClient][iType][1]+1) * count;
		if(iLastIndex > iCountItems) iLastIndex = iCountItems;
		int iIndex = -1;
		for(int i = g_iRotation[iClient][iType][1]*count; i < iLastIndex; i++)
		{
			iIndex++;
			class_ItemList ItemTest;
			int iFixIndex = 0;
			if(iType==0) iFixIndex = g_iHItemsArray.Get(i);
			else if(iType==1) iFixIndex = g_iZMItemsArray.Get(i);
			else
			{
				if(i < iCountHItems) iFixIndex = g_iHItemsArray.Get(i);
				else iFixIndex = g_iZMItemsArray.Get(i-iCountHItems); 
			}
			g_ItemList.GetArray(iFixIndex, ItemTest, sizeof(ItemTest));
			UpdateTime();
			if(ItemTest.CheckDelay() > 0.0)
			{
				FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[-%d]", ItemTest.ShortName, ItemTest.RoundDelay());
				FormatEx(sBuf[iIndex], MAXSIZESTRINGFORITEM, "%s: %N", sBuf_woname[iIndex], ItemTest.OwnerID);
			}
			else
			{
				if(g_bDispCooldowns)
				{
					if(ItemTest.LockButton) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[L]", ItemTest.ShortName);
					//else if(ItemTest.CheckWaitTime() > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", ItemTest.ShortName, ItemTest.RoundWaitTime());
					else
					{
						switch(ItemTest.Mode)
						{
							case 2:
							{
								if (ItemTest.CheckCoolDown() > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", ItemTest.ShortName, ItemTest.RoundCoolDown());
								else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[R]", ItemTest.ShortName);
							}
							case 3:
							{
								if (ItemTest.Uses < ItemTest.MaxUses) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d/%d]", ItemTest.ShortName, ItemTest.Uses, ItemTest.MaxUses);
								else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[E]", ItemTest.ShortName);
							}
							case 4:
							{
								if (ItemTest.CheckCoolDown() > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", ItemTest.ShortName, ItemTest.RoundCoolDown());
								else
									if (ItemTest.Uses < ItemTest.MaxUses) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d/%d]", ItemTest.ShortName, ItemTest.Uses, ItemTest.MaxUses);
									else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[E]", ItemTest.ShortName);
							}
							case 5:
							{
								if (ItemTest.CheckCoolDown() > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", ItemTest.ShortName, ItemTest.RoundCoolDown());
								else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d/%d]", ItemTest.ShortName, ItemTest.Uses, ItemTest.MaxUses);
							}
							case 6, 7:
							{
								if (ItemTest.MathValue > 0)
								{
									if(ItemTest.CoolDown > 0 && ItemTest.CheckCoolDown() > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", ItemTest.ShortName, ItemTest.RoundCoolDown());
									else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d/%d]", ItemTest.ShortName, ItemTest.MathValue, ItemTest.MathValueMax);
								}
								else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[E]", ItemTest.ShortName);
							}
							default: FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[+]", ItemTest.ShortName);
						}
					}
					if(ItemTest.LockButton2) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[L]", sBuf_woname[iIndex]);
					else if(ItemTest.CheckWaitTime(true) > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", sBuf_woname[iIndex], ItemTest.RoundWaitTime(true));
					else
					{
						switch(ItemTest.Mode2)
						{
							case 2:
							{
								if (ItemTest.CheckCoolDown(true) > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", sBuf_woname[iIndex], ItemTest.RoundCoolDown(true));
								else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[R]", sBuf_woname[iIndex]);

								ReplaceString(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "][", "|", false);
							}
							case 3:
							{
								if (ItemTest.Uses2 < ItemTest.MaxUses2) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d/%d]", sBuf_woname[iIndex], ItemTest.Uses2, ItemTest.MaxUses2);
								else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[E]", sBuf_woname[iIndex]);
							}
							case 4:
							{
								if (ItemTest.CheckCoolDown(true) > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", sBuf_woname[iIndex], ItemTest.RoundCoolDown(true));
								else
									if (ItemTest.Uses < ItemTest.MaxUses) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d/%d]", sBuf_woname[iIndex], ItemTest.Uses2, ItemTest.MaxUses2);
									else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[E]", sBuf_woname[iIndex]);
							}
							case 5:
							{
								if (ItemTest.CheckCoolDown(true) > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", sBuf_woname[iIndex], ItemTest.RoundCoolDown(true));
								else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d/%d]", sBuf_woname[iIndex], ItemTest.Uses2, ItemTest.MaxUses2);
							}
							case 6, 7:
							{
								if (ItemTest.MathValue2 > 0)
								{
									if(ItemTest.CoolDown2 > 0 && ItemTest.CheckCoolDown(true) > 0) FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d]", sBuf_woname[iIndex], ItemTest.RoundCoolDown(true));
									else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[%d/%d]", sBuf_woname[iIndex], ItemTest.MathValue2, ItemTest.MathValueMax2);
								}
								else FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s[E]", sBuf_woname[iIndex]);
							}
					}
					}
					FormatEx(sBuf[iIndex], MAXSIZESTRINGFORITEM, "%s: %N", sBuf_woname[iIndex], ItemTest.OwnerID);
				}
				else
				{
					FormatEx(sBuf[iIndex], MAXSIZESTRINGFORITEM, "%s: %N", ItemTest.ShortName, ItemTest.OwnerID);
					FormatEx(sBuf_woname[iIndex], MAXSIZESTRINGFORITEM, "%s picked up", ItemTest.ShortName);
				}
			}
		}
		for(int i=0; i <= iIndex; i++)
		{
			StrCat(sResult, iSizeResult, sBuf[i]);
			StrCat(sResult_woname, iSizeResult_woname, sBuf_woname[i]);
			if(i != iIndex)
			{
				StrCat(sResult, iSizeResult, "\n");
				StrCat(sResult_woname, iSizeResult_woname, "\n");
			}else
			{
				if(iMaxList>1)
				{
					char sCurrentList[MAXSIZESTRINGFORITEM];
					FormatEx(sCurrentList, sizeof(sCurrentList), "\n\nList: %i/%i", g_iRotation[iClient][iType][1]+1, iMaxList);
					StrCat(sResult, iSizeResult , sCurrentList);
					StrCat(sResult_woname, iSizeResult_woname, sCurrentList);
				}
			}
		}
	}
}

// Client command handlers
public Action EWM_Hud_Command_ToggleHUD(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		char sStatus[32], sColor[32];
		g_CSettings_Hud[iClient].Display = !g_CSettings_Hud[iClient].Display;
		SetClientCookie(iClient, g_hCookie_Display, g_CSettings_Hud[iClient].Display ? "1" : "0");

		sStatus = g_CSettings_Hud[iClient].Display ? "Enabled" : "Disabled";
		sColor = g_CSettings_Hud[iClient].Display ? g_SchemeConfig.Color_Enabled : g_SchemeConfig.Color_Disabled;        
		CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud", sColor, sStatus);
	}
	return Plugin_Handled;
}

public Action EWM_Hud_Command_ToggleHUDType(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		g_CSettings_Hud[iClient].Type = !g_CSettings_Hud[iClient].Type;
		SetClientCookie(iClient, g_hCookie_HudType, g_CSettings_Hud[iClient].Type ? "1" : "0");
		CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Type", g_SchemeConfig.Color_Enabled, g_CSettings_Hud[iClient].Type ?"KeyHintText" : "HUD");
	}
	return Plugin_Handled;
}

public Action EWM_Hud_Command_ToggleHUDName(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		char sStatus[32], sColor[32];
		g_CSettings_Hud[iClient].Name = !g_CSettings_Hud[iClient].Name;
		SetClientCookie(iClient, g_hCookie_HudName, g_CSettings_Hud[iClient].Name ? "1" : "0");

		sStatus = g_CSettings_Hud[iClient].Name ? "Enabled" : "Disabled";
		sColor = g_CSettings_Hud[iClient].Name ? g_SchemeConfig.Color_Enabled : g_SchemeConfig.Color_Disabled;
		CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Names", sColor, sStatus);
	}
	return Plugin_Handled;
}

public Action EWM_Hud_Command_Hudpos(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		if (GetCmdArgs() < 2)
		{
			CReplyToCommand(iClient, "%s%t %s%t: sm_hudpos <x> <y>. %t: %.3f %.3f", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage", "Default", g_SchemeConfig.Pos_HUD_X, g_SchemeConfig.Pos_HUD_Y);
			return Plugin_Handled;
		}
		char sBuffer[128];
		float HudPosX_validate;
		float HudPosY_validate;
		GetCmdArg(1, sBuffer, sizeof(sBuffer));
		HudPosX_validate = StringToFloat(sBuffer);
		
		GetCmdArg(2, sBuffer, sizeof(sBuffer));
		HudPosY_validate = StringToFloat(sBuffer);
		
		if(((HudPosX_validate >= 0.0 && HudPosX_validate <= 1.0) || HudPosX_validate == -1.0) && ((HudPosY_validate >= 0.0 && HudPosY_validate <= 1.0) || HudPosY_validate == -1.0))
		{
			g_CSettings_Hud[iClient].Pos_X = HudPosX_validate;
			g_CSettings_Hud[iClient].Pos_Y = HudPosY_validate;
			
			char sBuffer_cookie[32];
			FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", HudPosX_validate, HudPosY_validate);
			SetClientCookie(iClient, g_hCookie_HudPos, sBuffer_cookie);
		} else 
		{
			CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Position", g_SchemeConfig.Color_Disabled, "Wrong");
			return Plugin_Handled;
		}
		
		CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Position", g_SchemeConfig.Color_Enabled, "Saved");
	}
	return Plugin_Handled;
}

public Action EWM_Hud_Command_HudColor(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		if (GetCmdArgs() < 4)
		{
			DisplayMenuColorHUD(iClient);
			CReplyToCommand(iClient, "%s%t %s%t: sm_hudcolor <R> <G> <B> <A>. %t: %i %i %i %i", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage", "Default", g_SchemeConfig.Color_HUD[0], g_SchemeConfig.Color_HUD[1], g_SchemeConfig.Color_HUD[2], g_SchemeConfig.Color_HUD[3]);
			return Plugin_Handled;
		}
		char sBuffer[128];
		int iColor_validate[4];
		GetCmdArg(1, sBuffer, sizeof(sBuffer));
		iColor_validate[0] = StringToInt(sBuffer);
		
		GetCmdArg(2, sBuffer, sizeof(sBuffer));
		iColor_validate[1] = StringToInt(sBuffer);
		
		GetCmdArg(3, sBuffer, sizeof(sBuffer));
		iColor_validate[2] = StringToInt(sBuffer);
		
		GetCmdArg(4, sBuffer, sizeof(sBuffer));
		iColor_validate[3] = StringToInt(sBuffer);
		
		if(iColor_validate[0] >= 0 && iColor_validate[0] <= 255 && 
			iColor_validate[1] >= 0 && iColor_validate[1] <= 255 && 
			iColor_validate[2] >= 0 && iColor_validate[2] <= 255 && 
			iColor_validate[3] >= 0 && iColor_validate[3] <= 255)
		{
			g_CSettings_Hud[iClient].Color[0] = iColor_validate[0];
			g_CSettings_Hud[iClient].Color[1] = iColor_validate[1];
			g_CSettings_Hud[iClient].Color[2] = iColor_validate[2];
			g_CSettings_Hud[iClient].Color[3] = iColor_validate[3];
			char sBuffer_cookie[32];
			FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", iColor_validate[0], iColor_validate[1], iColor_validate[2], iColor_validate[3]);
			SetClientCookie(iClient, g_hCookie_HudColor, sBuffer_cookie);
		} else 
		{
			CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Color", g_SchemeConfig.Color_Disabled, "Wrong");
			return Plugin_Handled;
		}
		
		CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud Color", g_SchemeConfig.Color_Enabled, "Saved");
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
			CReplyToCommand(iClient, "%s%t %s%t: sm_huditems <1-%d>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage", MAXSHOWHUDITEMS);
			return Plugin_Handled;
		}
		
		char sArg[32];
		GetCmdArg(1, sArg, sizeof(sArg));
		
		int iCount;
		if (!StringToIntEx(sArg, iCount) || iCount <= 0 || iCount > MAXSHOWHUDITEMS)
		{
			CReplyToCommand(iClient, "%s%t %s%t <1-%d>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Invalid Amount", MAXSHOWHUDITEMS);
			return Plugin_Handled;
		}
		
		char sBuffer_cookie[32];
		IntToString(iCount, sBuffer_cookie, sizeof(sBuffer_cookie));
		SetClientCookie(iClient, g_hCookie_HudItemsCount, sBuffer_cookie);
		
		g_CSettings_Hud[iClient].ItemsCount = iCount;
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
		
		char sBuffer_cookie[32];
		IntToString(iTime, sBuffer_cookie, sizeof(sBuffer_cookie));
		SetClientCookie(iClient, g_hCookie_HudRotationTime, sBuffer_cookie);
		
		g_CSettings_Hud[iClient].RotationTime = iTime;
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

	char hudisplay[64], hudtype[64], hudname[64], hudcolor[64], hudpos[64], huditems[64], hudtime[64];
	FormatEx(hudisplay, sizeof(hudisplay), "%T", "Status", iClient);
	FormatEx(hudtype, sizeof(hudtype), "%T", "Hud Type", iClient);
	FormatEx(hudname, sizeof(hudname), "%T", "Hud Names", iClient);
	FormatEx(hudcolor, sizeof(hudcolor), "%T", "Hud Color", iClient);
	FormatEx(hudpos, sizeof(hudpos), "%T", "Hud Position", iClient);
	FormatEx(huditems, sizeof(huditems), "%T", "Hud Item Number", iClient);
	FormatEx(hudtime, sizeof(hudtime), "%T", "Hud Rotation Time", iClient); 

	menu.AddItem("display", hudisplay);
	menu.AddItem("type", hudtype);
	menu.AddItem("name", hudname);
	menu.AddItem("items", huditems);
	menu.AddItem("time", hudtime);
	menu.AddItem("pos", hudpos);
	menu.AddItem("color", hudcolor);

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
			else if (strcmp(info, "type", false) == 0)
			{
				char Type[32];
				if (g_CSettings_Hud[param1].Type)
					FormatEx(Type, sizeof(Type), "%T", "KeyHintText", param1);
				else
					FormatEx(Type, sizeof(Type), "%T", "HUD", param1);

				FormatEx(display, sizeof(display), "%T: %s", "Hud Type", param1, Type);
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
				SetClientCookie(param1, g_hCookie_Display, g_CSettings_Hud[param1].Display ? "1" : "0");

				color = g_CSettings_Hud[param1].Display ? g_SchemeConfig.Color_Enabled : g_SchemeConfig.Color_Disabled;
				status = g_CSettings_Hud[param1].Display ? "Enabled" : "Disabled";

				CPrintToChat(param1, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Hud", color, status);
				DisplayCookieMenu(param1);
			}
			else if (strcmp(info, "type", false) == 0)
			{
				g_CSettings_Hud[param1].Type = !g_CSettings_Hud[param1].Type;
				SetClientCookie(param1, g_hCookie_HudType, g_CSettings_Hud[param1].Type ? "1" : "0");
				status = g_CSettings_Hud[param1].Type ? "KeyHintText" : "HUD";

				CPrintToChat(param1, "%s%t %s%t %t: %s%s", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Type", g_SchemeConfig.Color_Enabled, status);
				DisplayCookieMenu(param1);
			}
			else if (strcmp(info, "name", false) == 0)
			{
				g_CSettings_Hud[param1].Name = !g_CSettings_Hud[param1].Name;
				SetClientCookie(param1, g_hCookie_HudName, g_CSettings_Hud[param1].Name ? "1" : "0");

				color = g_CSettings_Hud[param1].Name ? g_SchemeConfig.Color_Enabled : g_SchemeConfig.Color_Disabled;
				status = g_CSettings_Hud[param1].Name ? "Enabled" : "Disabled";

				CPrintToChat(param1, "%s%t %s%t %t: %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Names", color, status);
				DisplayCookieMenu(param1);
			}
			else if (strcmp(info, "color", false) == 0)
			{
				DisplayMenuColorHUD(param1);
			}
			else if (strcmp(info, "pos", false) == 0)
			{
				DisplayMenuPositionMenu(param1);
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

public void DisplayMenuColorHUD(int iClient)
{
	Menu menu = new Menu(DisplayMenuColorHUDMenuHandler);

	menu.SetTitle("%T %t", "EW_Tag", iClient, "Hud Color");

	char sColor[20];
	Format(sColor, sizeof(sColor), "%i %i %i %i", g_SchemeConfig.Color_HUD[0], g_SchemeConfig.Color_HUD[1], g_SchemeConfig.Color_HUD[2], g_SchemeConfig.Color_HUD[3]);

	menu.AddItem(sColor, "Default", (g_CSettings_Hud[iClient].Color[0] == g_SchemeConfig.Color_HUD[0] &&
									g_CSettings_Hud[iClient].Color[1] == g_SchemeConfig.Color_HUD[1] &&
									g_CSettings_Hud[iClient].Color[2] == g_SchemeConfig.Color_HUD[2] &&
									g_CSettings_Hud[iClient].Color[3] == g_SchemeConfig.Color_HUD[3]) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);

	for(int i = 0; i < sizeof(colorsList); i++)
	{
		char buffers[5][64];
		ExplodeString(colorsList[i], " ", buffers, 5, sizeof(buffers[]));
		
		int r = StringToInt(buffers[0]);
		int g = StringToInt(buffers[1]);
		int b = StringToInt(buffers[2]);
		int a = StringToInt(buffers[3]);

		menu.AddItem(colorsList[i], buffers[4], (g_CSettings_Hud[iClient].Color[0] == r &&
											g_CSettings_Hud[iClient].Color[1] == g &&
											g_CSettings_Hud[iClient].Color[2] == b &&
											g_CSettings_Hud[iClient].Color[3] == a) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	}

	menu.ExitBackButton = true;
	menu.ExitButton = true;
	menu.Display(iClient, MENU_TIME_FOREVER);
}

public int DisplayMenuColorHUDMenuHandler(Menu menu, MenuAction action, int param1, int param2)
{
	switch (action)
	{
		case MenuAction_Select:
		{
			char color[64], sBuffer_cookie[32];
			menu.GetItem(param2, color, sizeof(color));

			char Explode_HudColor[5][64];
			ExplodeString(color, " ", Explode_HudColor, 5, sizeof(Explode_HudColor[]));
			int r = StringToInt(Explode_HudColor[0]);
			int g = StringToInt(Explode_HudColor[1]);
			int b = StringToInt(Explode_HudColor[2]);
			int a = StringToInt(Explode_HudColor[3]);
			
			g_CSettings_Hud[param1].Color[0] = r;
			g_CSettings_Hud[param1].Color[1] = g;
			g_CSettings_Hud[param1].Color[2] = b;
			g_CSettings_Hud[param1].Color[3] = a;
	
			FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%i/%i/%i/%i", 
				g_CSettings_Hud[param1].Color[0], g_CSettings_Hud[param1].Color[1], g_CSettings_Hud[param1].Color[2], g_CSettings_Hud[param1].Color[3]);

			SetClientCookie(param1, g_hCookie_HudColor, sBuffer_cookie);
			CPrintToChat(param1, "%s%t %s%t %t: %s%t (%s)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Color", g_SchemeConfig.Color_Enabled, "Saved", Explode_HudColor[4]);
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

public void DisplayMenuItemsNumberHUD(int iClient)
{
	Menu menu = new Menu(DisplayMenuItemsNumberHUDMenuHandler);

	menu.SetTitle("%T %T", "EW_Tag", iClient, "Hud Items", iClient);

	for(int i = MAXSHOWHUDITEMS; i >= 1; i--)
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
			char sBuffer[10], sBuffer_cookie[32];
			menu.GetItem(param2, sBuffer, sizeof(sBuffer));
			int count = StringToInt(sBuffer);

			g_CSettings_Hud[param1].ItemsCount = count;
			FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%d", count);
			SetClientCookie(param1, g_hCookie_HudItemsCount, sBuffer_cookie);

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
			char sBuffer[10], sBuffer_cookie[32];
			menu.GetItem(param2, sBuffer, sizeof(sBuffer));
			int time = StringToInt(sBuffer);

			g_CSettings_Hud[param1].RotationTime = time;
			FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%d", time);
			SetClientCookie(param1, g_hCookie_HudRotationTime, sBuffer_cookie);

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

stock bool IsValidHudPos(float position, float MaxValue)
{
	switch (MaxValue)
	{
		case MAXPOS_HUD:
			if (position < MAXPOS_HUD) return true;
		case MINPOS_HUD:
			if (position > MINPOS_HUD) return true;
		default:
			return false;
	}
	return false;
}

public void DisplayMenuPositionMenu(int iClient)
{
	Menu menu = new Menu(Hud_Position_Handler);

	menu.SetTitle("%T %T", "EW_Tag", iClient, "Hud Position", iClient);

	char Y[64], X[64], Top[64], Bottom[64], Preset[64];
	FormatEx(Y, sizeof(Y), "%T", "Y axis", iClient, g_CSettings_Hud[iClient].Pos_Y);
	FormatEx(X, sizeof(X), "%T", "X axis", iClient, g_CSettings_Hud[iClient].Pos_X);
	FormatEx(Top, sizeof(Top), "+0.05");
	FormatEx(Bottom, sizeof(Bottom), "-0.05");
	FormatEx(Preset, sizeof(Preset), "%T", "HUD Presets", iClient);

	float fClientX = g_CSettings_Hud[iClient].Pos_X;
	float fClientY = g_CSettings_Hud[iClient].Pos_Y;

	menu.AddItem("preset", Preset);
	menu.AddItem("", Y, ITEMDRAW_DISABLED);
	menu.AddItem("Y++", Top, IsValidHudPos(fClientY, MAXPOS_HUD) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
	menu.AddItem("Y--", Bottom, IsValidHudPos(fClientY, MINPOS_HUD) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
	menu.AddItem("", X, ITEMDRAW_DISABLED);
	menu.AddItem("X++", Top, IsValidHudPos(fClientX, MAXPOS_HUD) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
	menu.AddItem("X--", Bottom, IsValidHudPos(fClientX, MINPOS_HUD) ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);

	menu.ExitBackButton = true;
	menu.ExitButton = true;
	menu.Display(iClient, MENU_TIME_FOREVER);
}

public int Hud_Position_Handler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			char info[64];
			float fBufferY = g_CSettings_Hud[param1].Pos_Y;
			float fBufferX = g_CSettings_Hud[param1].Pos_X;

			menu.GetItem(param2, info, sizeof(info));
			if (strcmp(info, "preset", false) == 0)
			{
				DisplayMenuPresetPositionMenu(param1);
				return 0;
			}
			else if (strcmp(info, "Y++", false) == 0)
				fBufferY += 0.05;
			else if (strcmp(info, "Y--", false) == 0)
				fBufferY -= 0.05;
			else if (strcmp(info, "X++", false) == 0)
				fBufferX += 0.05;
			else if (strcmp(info, "X--", false) == 0)
				fBufferX -= 0.05;

			// Clamp the position to the valid range
			if (fBufferY > MAXPOS_HUD) fBufferY = MAXPOS_HUD;
			if (fBufferY < MINPOS_HUD) fBufferY = MINPOS_HUD;
			if (fBufferX > MAXPOS_HUD) fBufferX = MAXPOS_HUD;
			if (fBufferX < MINPOS_HUD) fBufferX = MINPOS_HUD;

			g_CSettings_Hud[param1].Pos_Y = fBufferY;
			g_CSettings_Hud[param1].Pos_X = fBufferX;

			char sBuffer_cookie[32];
			FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", g_CSettings_Hud[param1].Pos_X, g_CSettings_Hud[param1].Pos_Y);
			SetClientCookie(param1, g_hCookie_HudPos, sBuffer_cookie);
			CPrintToChat(param1, "%s%t %s%t %t: %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved");

			// Refresh the menu
			DisplayMenuPositionMenu(param1);
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

public void DisplayMenuPresetPositionMenu(int iClient)
{
	Menu menu = new Menu(Hud_Preset_Position_Handler);

	menu.SetTitle("%T %T", "EW_Tag", iClient, "Hud Position", iClient);

	char TopLeft[64], TopCenter[64], TopRight[64], MiddleLeft[64], MiddleRight[64], BottomLeft[64], BottomCenter[64], BottomRight[64];
	FormatEx(TopLeft, sizeof(TopLeft), "%T", "Hud Top Left", iClient);
	FormatEx(TopCenter, sizeof(TopCenter), "%T", "Hud Top Center", iClient);
	FormatEx(TopRight, sizeof(TopRight), "%T", "Hud Top Right", iClient);
	FormatEx(MiddleLeft, sizeof(MiddleLeft), "%T", "Hud Middle Left", iClient);
	FormatEx(MiddleRight, sizeof(MiddleRight), "%T", "Hud Middle Right", iClient);
	FormatEx(BottomLeft, sizeof(BottomLeft), "%T", "Hud Bottom Left", iClient);
	FormatEx(BottomCenter, sizeof(BottomCenter), "%T", "Hud Bottom Center", iClient);
	FormatEx(BottomRight, sizeof(BottomRight), "%T", "Hud Bottom Right", iClient);

	float fClientX = g_CSettings_Hud[iClient].Pos_X;
	float fClientY = g_CSettings_Hud[iClient].Pos_Y;

	menu.AddItem("TopLeft", TopLeft, (fClientX == 0.0 && fClientY == 0.225) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	menu.AddItem("TopCenter", TopCenter, (fClientX == 0.435 && fClientY == 0.0) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	menu.AddItem("TopRight", TopRight, (fClientX == 0.890 && fClientY == 0.0) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	menu.AddItem("MiddleLeft", MiddleLeft, (fClientX == 0.4 && fClientY == 0.890) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	menu.AddItem("MiddleRight", MiddleRight, (fClientX == 0.890 && fClientY == 0.4) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	menu.AddItem("BottomLeft", BottomLeft, (fClientX == 0.0 && fClientY == 1.0) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	menu.AddItem("BottomCenter", BottomCenter, (fClientX == 0.435 && fClientY == 1.0) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);
	menu.AddItem("BottomRight", BottomRight, (fClientX == 0.890 && fClientY == 1.0) ? ITEMDRAW_DISABLED : ITEMDRAW_DEFAULT);

	menu.ExitBackButton = true;
	menu.ExitButton = true;
	menu.Display(iClient, MENU_TIME_FOREVER);
}

public int Hud_Preset_Position_Handler(Menu menu, MenuAction action, int param1, int param2)
{
	switch(action)
	{
		case MenuAction_Select:
		{
			char info[64], sBuffer_cookie[32];
			menu.GetItem(param2, info, sizeof(info));
			if (strcmp(info, "TopLeft", false) == 0)
			{
				g_CSettings_Hud[param1].Pos_X = 0.0;
				g_CSettings_Hud[param1].Pos_Y = 0.225;
				CPrintToChat(param1, "%s%t %s%t %t: %s%t (%t)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved", "Hud Top Left");
			}
			else if (strcmp(info, "TopCenter", false) == 0)
			{
				g_CSettings_Hud[param1].Pos_X = 0.435;
				g_CSettings_Hud[param1].Pos_Y = 0.0;
				CPrintToChat(param1, "%s%t %s%t %t: %s%t (%t)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved", "Hud Top Center");
			}
			else if (strcmp(info, "TopRight", false) == 0)
			{
				g_CSettings_Hud[param1].Pos_X = 0.890;
				g_CSettings_Hud[param1].Pos_Y = 0.0;
				CPrintToChat(param1, "%s%t %s%t %t: %s%t (%t)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved", "Hud Top Right");
			}
			else if (strcmp(info, "MiddleLeft", false) == 0)
			{
				g_CSettings_Hud[param1].Pos_X = 0.4;
				g_CSettings_Hud[param1].Pos_Y = 0.890;
				CPrintToChat(param1, "%s%t %s%t %t: %s%t (%t)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved", "Hud Middle Left");
			}
			else if (strcmp(info, "MiddleRight", false) == 0)
			{
				g_CSettings_Hud[param1].Pos_X = 0.890;
				g_CSettings_Hud[param1].Pos_Y = 0.4;
				CPrintToChat(param1, "%s%t %s%t %t: %s%t (%t)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved", "Hud Middle Right");
			}
			else if (strcmp(info, "BottomLeft", false) == 0)
			{
				g_CSettings_Hud[param1].Pos_X = 0.0;
				g_CSettings_Hud[param1].Pos_Y = 1.0;
				CPrintToChat(param1, "%s%t %s%t %t: %s%t (%t)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved", "Hud Bottom Left");
			}
			else if (strcmp(info, "BottomCenter", false) == 0)
			{
				g_CSettings_Hud[param1].Pos_X = 0.435;
				g_CSettings_Hud[param1].Pos_Y = 1.0;
				CPrintToChat(param1, "%s%t %s%t %t: %s%t (%t)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved", "Hud Bottom Center");
			}
			else if (strcmp(info, "BottomRight", false) == 0)
			{
				g_CSettings_Hud[param1].Pos_X = 0.890;
				g_CSettings_Hud[param1].Pos_Y = 1.0;
				CPrintToChat(param1, "%s%t %s%t %t: %s%t (%t)", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "HUD", "Hud Position", g_SchemeConfig.Color_Enabled, "Saved", "Hud Bottom Right");
			}

			FormatEx(sBuffer_cookie, sizeof(sBuffer_cookie), "%f/%f", g_CSettings_Hud[param1].Pos_X, g_CSettings_Hud[param1].Pos_Y);
			SetClientCookie(param1, g_hCookie_HudPos, sBuffer_cookie);

			// Always return to EntWatch settings menu after a selection
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
