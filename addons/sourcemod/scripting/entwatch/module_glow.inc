#define EW_MODULE_GLOW

// Purpose: Glows items on the map using a prop (Old method)

// Module settings
ConVar	g_hCvar_Glow,
		g_hCvar_Glow_Spawn;

// Module Local settings
bool	g_bGlow = true,
		g_bGlow_Spawn = true;
		
bool g_bMapRunning = false;
bool g_bThinkPostHooked[2048] = { false, ... };

stock void EWM_Glow_OnPluginStart()
{
	g_hCvar_Glow              = CreateConVar("entwatch_glow", "1", "Enable/Disable the glow Global.", _, true, 0.0, true, 1.0);
	g_hCvar_Glow_Spawn        = CreateConVar("entwatch_glow_spawn", "1", "Enable/Disable the glow after Spawn Items.", _, true, 0.0, true, 1.0);

	HookConVarChange(g_hCvar_Glow, Cvar_GLOW_Changed);
	HookConVarChange(g_hCvar_Glow_Spawn, Cvar_GLOW_Changed);
}

stock void EWM_Glow_OnMapStart()
{
	g_bMapRunning = true;
}

stock void EWM_Glow_OnMapEnd()
{
	g_bMapRunning = false;
}

public void Cvar_GLOW_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	if(convar==g_hCvar_Glow)
		g_bGlow = GetConVarBool(convar);
	else if(convar==g_hCvar_Glow_Spawn)
		g_bGlow_Spawn = GetConVarBool(convar);
	
	if(!g_bGlow && g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if(ItemTest.GlowEnt != INVALID_ENT_REFERENCE && IsValidEdict(ItemTest.GlowEnt))
			{
				AcceptEntityInput(ItemTest.GlowEnt, "Kill");
				ItemTest.GlowEnt = INVALID_ENT_REFERENCE;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}
}

// Init Glow weapon on Drop weapons
stock void EWM_Glow_GlowWeapon(class_ItemList ItemTest)
{
	if(g_bGlow && g_bMapRunning)
	{
		if (!IsValidEntity(ItemTest.WeaponID))
			return;

		SetEntProp(ItemTest.WeaponID, Prop_Send, "m_fEffects", GetEntProp(ItemTest.WeaponID, Prop_Send, "m_fEffects") & ~256);
		SetEntityRenderColor(ItemTest.WeaponID, 255, 255, 255, 255);

		if (!g_bThinkPostHooked[ItemTest.WeaponID])
		{
			SDKHook(ItemTest.WeaponID, SDKHook_ThinkPost, EWM_Glow_SDKHook_OnThinkPost);
			g_bThinkPostHooked[ItemTest.WeaponID] = true;
		}
	}
}

// Disable glow
stock void EWM_Glow_DisableGlow(class_ItemList ItemTest)
{
	if (!g_bGlow)
		return;

	if (g_bThinkPostHooked[ItemTest.WeaponID])
	{
		SDKUnhook(ItemTest.WeaponID, SDKHook_ThinkPost, EWM_Glow_SDKHook_OnThinkPost);
		g_bThinkPostHooked[ItemTest.WeaponID] = false;
	}
}

//----------------------------------------------------------------------------------------------------
// Purpose: Source 2009 Blinking effect
//----------------------------------------------------------------------------------------------------
public void EWM_Glow_SDKHook_OnThinkPost(int entity)
{
	SetEntProp(entity, Prop_Send, "m_fEffects", GetEntProp(entity, Prop_Send, "m_fEffects") | 256);
	SetEntityRenderColor(entity, 255, 165, 0, 255);
}
