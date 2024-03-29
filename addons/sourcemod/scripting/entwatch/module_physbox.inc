#define EW_MODULE_PHYSBOX

// Purpose: Sometimes when using items with physbox it is impossible to shoot/throw a grenade them for your teammates. It module fix it

#include <collisionhook>

ArrayList g_PhysboxArray;

stock void EWM_Physbox_OnPluginStart()
{
	if(g_PhysboxArray == INVALID_HANDLE) g_PhysboxArray = new ArrayList(512);
}

stock void EWM_Physbox_Event_RoundEnd()
{
	g_PhysboxArray.Clear();
}

// Function to properly clean up the physbox array
stock void EWM_Physbox_CleanData()
{
	if(g_PhysboxArray != null)
	{
		g_PhysboxArray.Clear();
		CloseHandle(g_PhysboxArray);
	}
	g_PhysboxArray = new ArrayList(512);
}

// Function clears the list of physboxes when they are destroyed
stock void EWM_Physbox_OnEntityDestroyed(int iEntity)
{
	for(int i = 0; i < g_PhysboxArray.Length; i++)
	{
		class_Physbox PhysBoxTest;
		g_PhysboxArray.GetArray(i, PhysBoxTest, sizeof(PhysBoxTest));
		if(PhysBoxTest.Entity == iEntity)
		{
			g_PhysboxArray.Erase(i);
			return;
		}
	}
}

// Function of updating the parameters in the array when pickup items with a physbox
stock void EWM_Physbox_Pickedup(int iClient, int iWeapon)
{
	for(int i = 0; i < g_PhysboxArray.Length; i++)
	{
		class_Physbox PhysBoxTest;
		g_PhysboxArray.GetArray(i, PhysBoxTest, sizeof(PhysBoxTest));
		if(PhysBoxTest.WeaponID == iWeapon)
		{
			PhysBoxTest.Team = GetClientTeam(iClient);
			g_PhysboxArray.SetArray(i, PhysBoxTest, sizeof(PhysBoxTest));
		}
	}
}

// Registering a new physbox when it spawns
public bool RegisterPhysbox(class_ItemList ItemInstance, int iEntity)
{
	if(IsValidEntity(ItemInstance.WeaponID) && ItemInstance.PhysBox)
	{
		char Item_Weapon_Targetname[64], Item_Weapon_Parent[64];
		Entity_GetTargetName(ItemInstance.WeaponID, Item_Weapon_Targetname, sizeof(Item_Weapon_Targetname));
		Entity_GetParentName(iEntity, Item_Weapon_Parent, sizeof(Item_Weapon_Parent));
		if ((strcmp(Item_Weapon_Targetname,"", false) != 0) && strcmp(Item_Weapon_Targetname, Item_Weapon_Parent, false) == 0)
		{
			class_Physbox NewPhysbox;
			NewPhysbox.Entity = iEntity;
			NewPhysbox.Team = -1;
			NewPhysbox.WeaponID = ItemInstance.WeaponID;
			g_PhysboxArray.PushArray(NewPhysbox, sizeof(NewPhysbox));
			return true;
		}
	}
	return false;
}

// Checking if the physbox belongs to an item
public void OnPhysboxSpawned(int iEntity)
{
	if(!IsValidEntity(iEntity) || !g_bConfigLoaded) return;
	
	for(int i = 0; i<g_ItemList.Length; i++)
	{
		class_ItemList ItemTest;
		g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
		if(RegisterPhysbox(ItemTest,iEntity)) return;
	}
}

// Filter on the attacker and grenades
public Action CH_PassFilter(int iEntBP, int iAttacker, bool &bResult)
{
	char sClassName[64];
	GetEntityClassname(iAttacker,sClassName,sizeof(sClassName));
	bool bDetectNade = false;
	if(StrContains(sClassName, "_projectile") != -1) bDetectNade = true;
	if (bDetectNade || 1 <= iAttacker <= MaxClients && IsClientInGame(iAttacker) && IsPlayerAlive(iAttacker))
	{
		for(int i = 0; i < g_PhysboxArray.Length; i++)
		{
			class_Physbox PhysBoxTest;
			g_PhysboxArray.GetArray(i, PhysBoxTest, sizeof(PhysBoxTest));
			if(PhysBoxTest.Entity == iEntBP)
			{
				if(bDetectNade)
				{
					bResult = false;
					return Plugin_Handled;
				}else
				{
					if(PhysBoxTest.Team == GetClientTeam(iAttacker))
					{
						bResult = false;
						return Plugin_Handled;
					}
				}
			}
		}
	}

	return Plugin_Continue;
}