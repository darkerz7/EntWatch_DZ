#define EW_MODULE_USE_PRIORITY

// Purpose: Needed when the player cannot press the button while in the crowd. Forcefully press the button

ConVar	g_hCvar_UsePriority;

bool	g_bUsePriority = true;

bool	g_bUsePriorityClient[MAXPLAYERS+1] = {true,...};

Handle	g_hCookie_UP = null;

stock void EWM_Use_Priority_OnPluginStart()
{
	g_hCvar_UsePriority		= CreateConVar("entwatch_use_priority", "1", "Enable/Disable forced pressing of the button.", _, true, 0.0, true, 1.0);
	g_bUsePriority 			= GetConVarBool(g_hCvar_UsePriority);
	HookConVarChange(g_hCvar_UsePriority, Cvar_UsePriority_Changed);
	RegConsoleCmd("sm_usepr", EWM_Use_Priority_Command);
	g_hCookie_UP			= RegClientCookie("entwatch_up_cookie", "", CookieAccess_Private);
}

public void Cvar_UsePriority_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	g_bUsePriority = GetConVarBool(convar);
}

stock void EWM_Use_Priority_OnClientCookiesCached(int iClient)
{
	if (!IsValidClient(iClient) || IsFakeClient(iClient))
		return;

	char sBuffer_cookie[4];
	GetClientCookie(iClient, g_hCookie_UP, sBuffer_cookie, sizeof(sBuffer_cookie));
	if (strcmp(sBuffer_cookie,"", false) == 0)
	{
		SetClientCookie(iClient, g_hCookie_UP, "1");
		strcopy(sBuffer_cookie, sizeof(sBuffer_cookie), "1");
	}
	g_bUsePriorityClient[iClient] = view_as<bool>(StringToInt(sBuffer_cookie));
}

stock void EWM_Use_Priority_LoadDefaultClientSettings(int iClient)
{
	g_bUsePriorityClient[iClient] = true;
}

// Catching button click use
public Action OnPlayerRunCmd(int iClient, int& iButtons, int& iImpulse, float aVel[3], float aAngles[3], int& iWeapon)
{
	if(iButtons & IN_USE)
	{
		if (g_bConfigLoaded && g_bUsePriority && g_bUsePriorityClient[iClient])
		{
			int iCountButtons = 0, iCurCountButton = 0, iMode = 0;
			int iButton = INVALID_ENT_REFERENCE;
			bool bHasNotDelay = true;
			for(int i = 0; i<g_ItemList.Length; i++)
			{
				class_ItemList ItemTest;
				g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
				if(ItemTest.OwnerID == iClient)
				{
					if(!ItemTest.UsePriority || ItemTest.CheckDelay() > 0) // Forcibly disabled in the config or the timer has not passed after selection
					{
						bHasNotDelay = false;
						break;
					}
					// Counting the total number of buttons a player has
					iCurCountButton = ItemTest.ButtonsArray.Length;
					iCountButtons+=iCurCountButton;
					if(iCountButtons > 1) break;
					if(iCountButtons == 1 && iCurCountButton == 1)
					{
						int iBtnBuff = ItemTest.ButtonsArray.Get(0);
						if(IsValidEdict(iBtnBuff) && ItemTest.ButtonID == Entity_GetHammerID(iBtnBuff)) 
						{
							iButton = iBtnBuff;
							iMode = ItemTest.Mode;
						}
					}
				}
			}
			// Checking for the number of buttons on the item or if the player has several items or ignore mod 1,6,7
			if(bHasNotDelay && iCountButtons == 1 && iMode!=1 && iMode!=6 && iMode!=7 && IsValidEdict(iButton)) AcceptEntityInput(iButton, "Use", iClient, iClient); 
		}
	}
	return Plugin_Continue;
}

// Client command to disable forced button press
public Action EWM_Use_Priority_Command(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)))
	{
		if(g_bUsePriorityClient[iClient])
		{
			g_bUsePriorityClient[iClient] = false;
			SetClientCookie(iClient, g_hCookie_UP, "0");
			CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Use Priority", g_SchemeConfig.Color_Disabled, "Disabled");
		} else
		{
			g_bUsePriorityClient[iClient] = true;
			SetClientCookie(iClient, g_hCookie_UP, "1");
			CPrintToChat(iClient, "%s%t %s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Use Priority", g_SchemeConfig.Color_Enabled, "Enabled");
		}
	}
	return Plugin_Handled;
}