#define EW_MODULE_GLOW

//-------------------------------------------------------
// Purpose: Plugin settings
//-------------------------------------------------------
ConVar	g_hCvar_Glow,
		g_hCvar_Glow_Spawn,
		g_hCvar_Glow_Spawn_Type,
		g_hCvar_Glow_Drop_Type;

//-------------------------------------------------------
// Purpose: Plugin Local settings
//-------------------------------------------------------
bool	g_bGlow = true,
		g_bGlow_Spawn = true;

int		g_iGlow_Spawn_Type = 0,
		g_iGlow_Drop_Type = 0;
		
bool g_bMapRunning = false;

bool g_bThinkPostHooked[2048] = { false, ... };

stock void EWM_Glow_OnPluginStart()
{
	g_hCvar_Glow              = CreateConVar("entwatch_glow", "0", "Enable/Disable the glow Global.", _, true, 0.0, true, 1.0);
	g_hCvar_Glow_Spawn        = CreateConVar("entwatch_glow_spawn", "0", "Enable/Disable the glow after Spawn Items.", _, true, 0.0, true, 1.0);
	g_hCvar_Glow_Spawn_Type   = CreateConVar("entwatch_glow_spawn_type", "0", "Glow Type after Spawn Items.", _, true, -1.0, true, 3.0);
	g_hCvar_Glow_Drop_Type    = CreateConVar("entwatch_glow_drop_type", "0", "Glow Type after Drop Items.", _, true, -1.0, true, 3.0);

	HookConVarChange(g_hCvar_Glow, Cvar_GLOW_Changed);
	HookConVarChange(g_hCvar_Glow_Spawn, Cvar_GLOW_Changed);
	HookConVarChange(g_hCvar_Glow_Spawn_Type, Cvar_GLOW_Changed);
	HookConVarChange(g_hCvar_Glow_Drop_Type, Cvar_GLOW_Changed);
}

stock void EWM_Glow_OnConfigsExecuted()
{
	EWM_Glow_GetConVars();
}

stock void EWM_Glow_OnMapStart()
{
	g_bMapRunning = true;
}

stock void EWM_Glow_OnMapEnd()
{
	g_bMapRunning = false;
}

stock void EWM_Glow_GetConVars()
{
	g_bGlow = GetConVarBool(g_hCvar_Glow);
	g_bGlow_Spawn = GetConVarBool(g_hCvar_Glow_Spawn);
	g_iGlow_Spawn_Type = GetConVarInt(g_hCvar_Glow_Spawn_Type);
	g_iGlow_Drop_Type = GetConVarInt(g_hCvar_Glow_Drop_Type);
}

public void Cvar_GLOW_Changed(ConVar convar, const char[] oldValue, const char[] newValue)
{
	EWM_Glow_GetConVars();

	if(!g_bGlow && g_bConfigLoaded)
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));

			EWM_Glow_DisableGlow_Internal(ItemTest);

			if(ItemTest.GlowEnt != INVALID_ENT_REFERENCE && IsValidEdict(ItemTest.GlowEnt))
			{
				AcceptEntityInput(ItemTest.GlowEnt, "Kill");
				ItemTest.GlowEnt = INVALID_ENT_REFERENCE;
				g_ItemList.SetArray(i, ItemTest, sizeof(ItemTest));
			}
		}
}

//----------------------------------------------------------------------------------------------------
// Purpose: init Glow weapon on Drop weapons
//----------------------------------------------------------------------------------------------------
stock void EWM_Glow_GlowWeapon(class_ItemList ItemTest, int index, bool bSpawn)
{
	if(!g_bGlow || !g_bMapRunning)
		return;

	EWM_Glow_GlowWeapon_Internal(ItemTest, index, bSpawn);
}
//----------------------------------------------------------------------------------------------------
// Purpose: Disable glow
//----------------------------------------------------------------------------------------------------
stock void EWM_Glow_DisableGlow(class_ItemList ItemTest)
{
	if (!g_bGlow)
		return;
	
	EWM_Glow_DisableGlow_Internal(ItemTest);
}

stock void EWM_Glow_GlowWeapon_Internal(class_ItemList ItemTest, int index, bool bSpawn)
{
	switch (g_evGameEngine)
	{
		case Engine_CSS:
		{
			SetEntProp(ItemTest.WeaponID, Prop_Send, "m_fEffects", GetEntProp(ItemTest.WeaponID, Prop_Send, "m_fEffects") & ~256);
			SetEntityRenderColor(ItemTest.WeaponID, 255, 255, 255, 255);

			if (!g_bThinkPostHooked[ItemTest.WeaponID])
			{
				SDKHook(ItemTest.WeaponID, SDKHook_ThinkPost, EWM_Glow_SDKHook_OnThinkPost);
				g_bThinkPostHooked[ItemTest.WeaponID] = true;
			}
		}
		case Engine_CSGO:
		{
			if (!bSpawn)
			{
				if (g_iGlow_Drop_Type < 0)
				{
					if (ItemTest.GlowEnt != INVALID_ENT_REFERENCE && IsValidEdict(ItemTest.GlowEnt))
						AcceptEntityInput(ItemTest.GlowEnt, "Kill");
					return;
				}
			}
			else
			{
				//WESKER EDIT -- Set spawn type negative to disable glow on new items
				if (g_iGlow_Spawn_Type < 0)
					return;
			}
			if(ItemTest.GlowEnt == INVALID_ENT_REFERENCE || !IsValidEdict(ItemTest.GlowEnt))
			{
				// Give glowing effect to the entity
				char sModelPath[PLATFORM_MAX_PATH];
				// Get the original model path
				GetEntPropString(ItemTest.WeaponID, Prop_Data, "m_ModelName", sModelPath, sizeof(sModelPath));
				ReplaceString(sModelPath, sizeof(sModelPath), "_dropped", "", false);
				
				// Find the location of the weapon
				float fWOrigin[3], fWAngle[3];
				float fForward[3], fRight[3], fUp[3];
				float fOffset[3] = {0.0, -5.0, 0.0};
				
				GetEntPropVector(ItemTest.WeaponID, Prop_Send, "m_vecOrigin", fWOrigin);
				GetEntPropVector(ItemTest.WeaponID, Prop_Send, "m_angRotation", fWAngle);
				GetAngleVectors(fWAngle, fForward, fRight, fUp);
				
				fWOrigin[0] += fRight[0]*fOffset[0]+fForward[0]*fOffset[1]+fUp[0]*fOffset[2];
				fWOrigin[1] += fRight[1]*fOffset[0]+fForward[1]*fOffset[1]+fUp[1]*fOffset[2];
				fWOrigin[2] += fRight[2]*fOffset[0]+fForward[2]*fOffset[1]+fUp[2]*fOffset[2];
				
				// Create & set dynamic glow entity and give properties
				ItemTest.GlowEnt = CreateEntityByName("prop_dynamic");
				
				DispatchKeyValue(ItemTest.GlowEnt, "model", sModelPath);
				DispatchKeyValue(ItemTest.GlowEnt, "disablereceiveshadows", "1");
				DispatchKeyValue(ItemTest.GlowEnt, "disableshadows", "1");
				DispatchKeyValue(ItemTest.GlowEnt, "solid", "0");
				DispatchKeyValue(ItemTest.GlowEnt, "spawnflags", "256");
				SetEntProp(ItemTest.GlowEnt, Prop_Send, "m_CollisionGroup", 11);
				
				// Spawn and teleport the entity
				DispatchSpawn(ItemTest.GlowEnt);
				TeleportEntity(ItemTest.GlowEnt, fWOrigin, fWAngle, NULL_VECTOR);

				SetEntProp(ItemTest.GlowEnt, Prop_Send, "m_bShouldGlow", true, true);
				SetEntPropFloat(ItemTest.GlowEnt, Prop_Send, "m_flGlowMaxDist", 10000000.0);
				if(bSpawn)
					SetEntProp(ItemTest.GlowEnt, Prop_Send, "m_nGlowStyle", g_iGlow_Spawn_Type);
				else SetEntProp(ItemTest.GlowEnt, Prop_Send, "m_nGlowStyle", g_iGlow_Drop_Type);
				
				SetVariantColor(ItemTest.GlowColor);
				AcceptEntityInput(ItemTest.GlowEnt, "SetGlowColor");

				// Set the activator and group the entity
				SetVariantString("!activator");
				AcceptEntityInput(ItemTest.GlowEnt, "SetParent", ItemTest.WeaponID);

				AcceptEntityInput(ItemTest.GlowEnt, "TurnOn");
				AcceptEntityInput(ItemTest.GlowEnt, "SetGlowEnabled");
			}
			else
			{
				if (g_bGlow_Spawn)
					SetEntProp(ItemTest.GlowEnt, Prop_Send, "m_nGlowStyle", g_iGlow_Drop_Type);

				AcceptEntityInput(ItemTest.GlowEnt, "TurnOn");
				AcceptEntityInput(ItemTest.GlowEnt, "SetGlowEnabled");
			}
		}
	}
}

stock void EWM_Glow_DisableGlow_Internal(class_ItemList ItemTest)
{
	switch (g_evGameEngine)
	{
		case Engine_CSS:
		{
			if (g_bThinkPostHooked[ItemTest.WeaponID])
			{
				SDKUnhook(ItemTest.WeaponID, SDKHook_ThinkPost, EWM_Glow_SDKHook_OnThinkPost);
				g_bThinkPostHooked[ItemTest.WeaponID] = false;
			}
		}
		case Engine_CSGO:
		{
			if (ItemTest.GlowEnt != INVALID_ENT_REFERENCE && IsValidEdict(ItemTest.GlowEnt))
			{
				AcceptEntityInput(ItemTest.GlowEnt, "SetGlowDisabled");
				AcceptEntityInput(ItemTest.GlowEnt, "TurnOff");
			}
		}
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
