#define EW_MODULE_BLINK

// Purpose: Blink effect for highlighting items (CS:S)

// Module settings
ConVar	g_hCvar_Blink;

// Module Local settings
bool	g_bBlink = true;
bool	g_bMapRunningBlink = false;
bool	g_bThinkPostHooked[2048] = { false, ... };

stock void EWM_Blink_OnPluginStart()
{
	g_hCvar_Blink	= CreateConVar("entwatch_blink", "0", "Enable/Disable the Blink Global.", _, true, 0.0, true, 1.0);

	//Initialize values
	g_bBlink = GetConVarBool(g_hCvar_Blink);

	//Hook CVARs
	HookConVarChange(g_hCvar_Blink, Cvar_BLINK_Changed);
}

stock void EWM_Blink_OnMapStart()
{
	g_bMapRunningBlink = true;
}

stock void EWM_Blink_OnMapEnd()
{
	g_bMapRunningBlink = false;
}

public void Cvar_BLINK_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	g_bBlink = GetConVarBool(convar);

	if(!g_bBlink && g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));

			EWM_Blink_DisableBlink(ItemTest);

			if(ItemTest.GlowEnt != INVALID_ENT_REFERENCE && IsValidEdict(ItemTest.GlowEnt))
			{
				AcceptEntityInput(ItemTest.GlowEnt, "Kill");
				ItemTest.GlowEnt = INVALID_ENT_REFERENCE;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}
}

// Init Blink weapon on Drop weapons
stock void EWM_Blink_BlinkWeapon(class_ItemList ItemTest)
{
	if(g_bBlink && g_bMapRunningBlink)
	{
		SetEntProp(ItemTest.WeaponID, Prop_Send, "m_fEffects", GetEntProp(ItemTest.WeaponID, Prop_Send, "m_fEffects") & ~256);
		SetEntityRenderColor(ItemTest.WeaponID, ItemTest.GlowColor[0], ItemTest.GlowColor[1], ItemTest.GlowColor[2], ItemTest.GlowColor[3]);

		if (!g_bThinkPostHooked[ItemTest.WeaponID])
		{
			SDKHook(ItemTest.WeaponID, SDKHook_ThinkPost, EWM_Blink_SDKHook_OnThinkPost);
			g_bThinkPostHooked[ItemTest.WeaponID] = true;
		}
	}
}

// Disable Blink
stock void EWM_Blink_DisableBlink(class_ItemList ItemTest)
{
	if (g_bBlink)
		if (g_bThinkPostHooked[ItemTest.WeaponID])
		{
			SDKUnhook(ItemTest.WeaponID, SDKHook_ThinkPost, EWM_Blink_SDKHook_OnThinkPost);
			g_bThinkPostHooked[ItemTest.WeaponID] = false;
		}
}

// Source 2009 Blinking effect
public void EWM_Blink_SDKHook_OnThinkPost(int iEntity)
{
	SetEntProp(iEntity, Prop_Send, "m_fEffects", GetEntProp(iEntity, Prop_Send, "m_fEffects") | 256);
	for(int i = 0; i<g_ItemList.Length; i++)
	{
		class_ItemList ItemTest;
		g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
		if(ItemTest.WeaponID == iEntity)
		{
			SetEntityRenderColor(iEntity, ItemTest.GlowColor[0], ItemTest.GlowColor[1], ItemTest.GlowColor[2], ItemTest.GlowColor[3]);
			return;
		}
	}
}