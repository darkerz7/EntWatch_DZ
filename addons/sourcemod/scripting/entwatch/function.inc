// Purpose: Global Function Variables
float		g_fGameTime;
int			g_iOutValueOffset = -1;

// Structure for items loaded from the configuration file
enum struct class_ItemConfig
{
	char		Name[32];
	char		ShortName[32];
	char		Color[16];
	char		ButtonClass[32];
	char		FilterName[32];
	bool		BlockPickup;
	bool		AllowTransfer;
	bool		ForceDrop;
	bool		Chat;
	bool		Chat_Uses;
	bool		Hud;
	int			HammerID;
	int 		EnergyID;
	int			Mode;
	int			MaxUses;
	int			CoolDown;
	int			ButtonID;
	int			GlowColor[4];
	int			Trigger;
	char		Spawner[64];
	bool		PhysBox;
	bool		UsePriority;
	
	char		ButtonClass2[32];
	int 		EnergyID2;
	int			Mode2;
	int			MaxUses2;
	int			CoolDown2;
	int			ButtonID2;
}

// Structure for the next item on the map
enum struct class_ItemList
{
	char		Name[32];			// String:	Full Name of the Item
	char		ShortName[32];		// String:	Short Name of Item for HUD
	char		Color[16];			// String:	Color for visual highlighting in chat and highlight color definitions
	char		ButtonClass[32];	// String:	Entity class name to activate item actions (func_button/func_door/game_ui)
	char		FilterName[32];		// String:	Targetname of the player that can activate the item (filter_activator_name)
	bool		BlockPickup;		// Bool:	Disable item selection
	bool		AllowTransfer;		// Bool:	Allow transferring an item to another player
	bool		ForceDrop;			// Bool:	Necessary for proper handling of the drop function
	bool		Chat;				// Bool:	Display all actions performed on the item in the chat
	bool		Chat_Uses;			// Bool:	Display only item usage in chat
	bool		Hud;				// Bool:	Display item in HUD
	int			HammerID;			// Integer:	HammerID of the weapon to which the ability is bound
	int 		EnergyID;			// Integer:	HammerID of the math_counter which is used for items with energy (ex. Minigun)
	int 		MathID;				// Integer:	Entity Index of the next item instance
	int 		MathValue;			// Integer:	The current value of the math_counter
	int 		MathValueMax;		// Integer:	Maximum value of math_counter
	int			Mode;				// Integer:	Current item mode
	int			Uses;				// Integer: Item's current number of uses
	int			MaxUses;			// Integer:	Maximum number of uses of the item
	int			CoolDown;			// Integer:	Cooldown after using an item
	int			ButtonID;			// Integer:	HammerID Entity activating actions(-5 - game_ui)
	int			WeaponID;			// Integer:	Entity Index of the weapon to which the item is attached
	ArrayList	ButtonsArray;		// Array:	List of triggering entities attached to the weapon
	int			OwnerID;			// Integer:	Entity Index of the Client that owns the item
	float		CoolDownTime;		// Float:	Timestamp when the item rolls back
	float		Delay;				// Float:	Timestamp when the item can be used after picking
	int			GlowEnt;			// Integer:	Entity Index that glows
	int			GlowColor[4];		// Array:	Glow color RGBA
	bool		PhysBox;			// Bool:	Does the item have a PhysBox?
	bool		UsePriority;		// Bool:	Whether to force click on the button
	int			Team;				// Integer:	Which team owns the item (-1 - No Team, 1 - Spec, 2 - T, 3 - CT)
	bool		LockButton;			// Bool:	Item cannot be used
	float		WaitTime;			// Float:	Timestamp when the button is rolled back
	
	// Parameters for the entity activating the second ability
	char		ButtonClass2[32];
	int 		EnergyID2;
	int 		MathID2;
	int 		MathValue2;
	int 		MathValueMax2;
	int			Mode2;
	int			Uses2;
	int			MaxUses2;
	int			CoolDown2;
	int			ButtonID2;
	float		CoolDownTime2;
	bool		LockButton2;
	float		WaitTime2;
	
	//Purpose: Base cooldowns on GameTime rather than Timer
	void SetCoolDown(int time)
	{
		this.CoolDownTime = g_fGameTime + time;
	}
	float CheckCoolDown()
	{
		return this.CoolDownTime - g_fGameTime;
	}
	int RoundCoolDown()
	{
		return RoundToCeil(this.CoolDownTime - g_fGameTime);
	}
	void SetCoolDown2(int time)
	{
		this.CoolDownTime2 = g_fGameTime + time;
	}
	float CheckCoolDown2()
	{
		return this.CoolDownTime2 - g_fGameTime;
	}
	int RoundCoolDown2()
	{
		return RoundToCeil(this.CoolDownTime2 - g_fGameTime);
	}
	//Purpose: Cooldown when item is picked up
	void SetDelay(float time)
	{
		if(time<0.0) time = 0.0;
		this.Delay = g_fGameTime + time;
	}
	float CheckDelay()
	{
		return this.Delay - g_fGameTime;
	}
	int RoundDelay()
	{
		return RoundToCeil(this.Delay - g_fGameTime);
	}
	//Purpose: Correct deletion of handles. Memory leak fix
	void ClearHandles()
	{
		if(this.ButtonsArray != null)
		{
			CloseHandle(this.ButtonsArray);
			this.ButtonsArray = null;
		}
	}
	//WaitTime for first button
	void SetWaitTime(int time)
	{
		if(time<1) time = 1;
		this.WaitTime = g_fGameTime + time;
	}
	float CheckWaitTime()
	{
		return this.WaitTime - g_fGameTime;
	}
	int RoundWaitTime()
	{
		return RoundToCeil(this.WaitTime - g_fGameTime);
	}
	//WaitTime for second button
	void SetWaitTime2(int time)
	{
		if(time<1) time = 1;
		this.WaitTime2 = g_fGameTime + time;
	}
	float CheckWaitTime2()
	{
		return this.WaitTime2 - g_fGameTime;
	}
	int RoundWaitTime2()
	{
		return RoundToCeil(this.WaitTime2 - g_fGameTime);
	}
}

//Purpose: Update cached GameTime
void UpdateTime()
{
	g_fGameTime = GetGameTime();
}

// Structure for loading scheme config
enum struct class_Scheme
{
	char		Color_Tag[16];			// String:	Tag color from the CColor properties list (csgocolors_fix.inc)
	char		Color_Name[16];			// String:	Player name color
	char		Color_SteamID[16];		// String:	Player SteamID color
	char		Color_Use[16];			// String:	Item usage text color
	char		Color_Pickup[16];		// String:	Item pickup text color
	char		Color_Drop[16];			// String:	Item drop text color
	char		Color_Disconnect[16];	// String:	Text color of disconnected player with item
	char		Color_Death[16];		// String:	Text color of the deceased player with item
	char		Color_Warning[16];		// String:	Default text color
	char		Color_Enabled[16];		// String:	Text color of Enable
	char		Color_Disabled[16];		// String:	Text color of Disable
	char		Server_Name[64];		// String:	Unique server name to use the same database for different servers
	int			Color_HUD[4];			// Array:	Default HUD color
	float		Pos_HUD_X;				// Float:	Default HUD position in X
	float		Pos_HUD_Y;				// Float:	Default HUD position in Y
}

// Structure for client settings
enum struct class_ClientSettings_Hud
{
	bool		Display;	// Bool:	Show HUD on client
	int			Color[4];	// Array:	Color of HUD on client
	float		Pos_X;		// Float:	HUD position in X on client
	float		Pos_Y;		// Float:	HUD position in Y on client
	bool		Name;		// Bool:	Show player nicknames on items
}

// Structure for player e-ban information
enum struct class_ClientSettings_EBan
{
	bool		Banned;				// Bool:	Is the player e-banned?
	char		Admin_Name[32];		// String:	The name of the admin who e-banned the player
	char		Admin_SteamID[64];	// String:	The SteamID of the admin who e-banned the player
	int			Duration;			// Integer:	E-ban Time
	int			TimeStamp_Issued;	// Integer:	Timestamp when e-ban issued
	char		Reason[64];			// String:	The reason for the e-ban
}

// Structure for item with physbox
enum struct class_Physbox
{
	int		Entity;		// Integer:	Physbox Entity Index
	int		Team;		// Integer:	The team of the player who owns the physbox item
	int		WeaponID;	// Integer:	Weaponid to which the physbox is attached
}

// Structure for storing information about disconnected players
enum struct class_Offline_Eban
{
	int			UserID;					// Integer:	UserID of the disconnected player
	char		PlayerName[32];			// String:	The last name of the player before he left
	char		Player_SteamID[64];		// String:	SteamID of the disconnected player
	int			TimeStamp;				// Integer:	Timestamp when a disconnected player can be ignored
	int			TimeStamp_Start;		// Integer:	Timestamp when the player left
	char		LastItem[32];			// String:	The last item picked up by the disconnected player
}

// Checks the player for validity
stock bool IsValidClient(int iClient) 
{ 
	if (iClient > 0 && iClient <= MaxClients && IsValidEdict(iClient) && IsClientInGame(iClient)) return true; 
	return false; 
}

// Correct transfer of items to the player
stock void FixedEquipPlayerWeapon(int iClient, int iWeapon)
{
	int wSlot = GetSlotCSGO(iWeapon);
	if((wSlot>=0) && (wSlot<=2))
	{
		int TargetWInSlot = GetPlayerWeaponSlot(iClient, wSlot);
		if(TargetWInSlot != -1) CS_DropWeapon(iClient, TargetWInSlot, false);
		float iClientOrigin[3];
		GetClientAbsOrigin(iClient,iClientOrigin);
		TeleportEntity(iWeapon, iClientOrigin, NULL_VECTOR, NULL_VECTOR);
	}
}

// Correctly getting the value in the math_counter
stock int GetCounterValue(int counter) {
    char szType[64];
    GetEntityClassname(counter, szType, sizeof(szType));

    if(!StrEqual(szType, "math_counter", false)) {
        return -1;
    }

    if(g_iOutValueOffset == -1)
        g_iOutValueOffset = FindDataMapInfo(counter, "m_OutValue");
    
    return RoundFloat(GetEntDataFloat(counter, g_iOutValueOffset));
}

// Kludge, Getting a weapon slot without using a gamedata
stock int GetSlotCSGO(int eWeapon)
{
	/*Slots:
	Invalid/Unknown: -1
	Primary: 0
	Secondary: 1
	Melee(Knife): 2
	Projectile(Grenades): 3
	Explosive(c4): 4
	*/
	char wClassName[64];
	GetEntityClassname(eWeapon,wClassName,sizeof(wClassName));
	
	/* Old function https://hlmod.net/threads/analogi-funkcij-na-bolee-bystrye.49613/
	if((StrEqual(wClassName,"weapon_elite",false))||(StrEqual(wClassName,"weapon_deagle",false))||(StrEqual(wClassName,"weapon_fiveseven",false))||
		(StrEqual(wClassName,"weapon_glock",false))||(StrEqual(wClassName,"weapon_revolver",false))||(StrEqual(wClassName,"weapon_usp_silencer",false))||
		(StrEqual(wClassName,"weapon_cz75a",false))||(StrEqual(wClassName,"weapon_hkp2000",false))||(StrEqual(wClassName,"weapon_p250",false))||
		(StrEqual(wClassName,"weapon_tec9",false)))
	{
		return 1;
	} else if((StrEqual(wClassName,"weapon_knife",false))||(StrEqual(wClassName,"weapon_knifegg",false))||(StrEqual(wClassName,"weapon_taser",false))||
		(StrEqual(wClassName,"weapon_axe",false))||(StrEqual(wClassName,"weapon_hammer",false))||(StrEqual(wClassName,"weapon_spanner",false)))
	{
		return 2;
	} else if((StrEqual(wClassName,"weapon_decoy",false))||(StrEqual(wClassName,"weapon_flashbang",false))||(StrEqual(wClassName,"weapon_hegrenade",false))||
		(StrEqual(wClassName,"weapon_incgrenade",false))||(StrEqual(wClassName,"weapon_molotov",false))||(StrEqual(wClassName,"weapon_smokegrenade",false))||
		(StrEqual(wClassName,"weapon_tagrenade",false))||(StrEqual(wClassName,"weapon_diversion",false))||(StrEqual(wClassName,"weapon_snowball",false)))
	{
		return 3;
	} else if((StrEqual(wClassName,"weapon_ak47",false))||(StrEqual(wClassName,"weapon_aug",false))||(StrEqual(wClassName,"weapon_awp",false))||
		(StrEqual(wClassName,"weapon_bizon",false))||(StrEqual(wClassName,"weapon_famas",false))||(StrEqual(wClassName,"weapon_g3sg1",false))||
		(StrEqual(wClassName,"weapon_galilar",false))||(StrEqual(wClassName,"weapon_m4a1",false))||(StrEqual(wClassName,"weapon_m4a1_silencer",false))||
		(StrEqual(wClassName,"weapon_m249",false))||(StrEqual(wClassName,"weapon_negev",false))||(StrEqual(wClassName,"weapon_p90",false))||
		(StrEqual(wClassName,"weapon_scar20",false))||(StrEqual(wClassName,"weapon_ssg08",false))||(StrEqual(wClassName,"weapon_ump45",false))||
		(StrEqual(wClassName,"weapon_xm1014",false))||(StrEqual(wClassName,"weapon_mac10",false))||(StrEqual(wClassName,"weapon_mag7",false))||
		(StrEqual(wClassName,"weapon_mp7",false))||(StrEqual(wClassName,"weapon_mp9",false))||(StrEqual(wClassName,"weapon_nova",false))||
		(StrEqual(wClassName,"weapon_sawedoff",false))||(StrEqual(wClassName,"weapon_sg556",false)))
	{
		return 0;
	} else if((StrEqual(wClassName,"weapon_c4",false))||(StrEqual(wClassName,"weapon_breachcharge",false))||(StrEqual(wClassName,"weapon_bumpmine",false))||
		(StrEqual(wClassName,"weapon_healthshot",false))||(StrEqual(wClassName,"weapon_shield",false)))
	{
		return 4;
	} else return -1;
	*/
	if((strcmp(wClassName,"weapon_elite",false)==0)||(strcmp(wClassName,"weapon_deagle",false)==0)||(strcmp(wClassName,"weapon_fiveseven",false)==0)||
	(strcmp(wClassName,"weapon_glock",false)==0)||(strcmp(wClassName,"weapon_revolver",false)==0)||(strcmp(wClassName,"weapon_usp_silencer",false)==0)||
	(strcmp(wClassName,"weapon_cz75a",false)==0)||(strcmp(wClassName,"weapon_hkp2000",false)==0)||(strcmp(wClassName,"weapon_p250",false)==0)||
	(strcmp(wClassName,"weapon_tec9",false)==0)||(strcmp(wClassName,"weapon_p228",false)==0)||(strcmp(wClassName,"weapon_usp",false)==0))
		return 1;
	else if((strcmp(wClassName,"weapon_knife",false)==0)||(strcmp(wClassName,"weapon_knifegg",false)==0)||(strcmp(wClassName,"weapon_taser",false)==0)||
	(strcmp(wClassName,"weapon_axe",false)==0)||(strcmp(wClassName,"weapon_hammer",false)==0)||(strcmp(wClassName,"weapon_spanner",false)==0))
		return 2;
	else if((strcmp(wClassName,"weapon_hegrenade",false)==0)||(strcmp(wClassName,"weapon_decoy",false)==0)||(strcmp(wClassName,"weapon_molotov",false)==0)||
	(strcmp(wClassName,"weapon_incgrenade",false)==0)||(strcmp(wClassName,"weapon_flashbang",false)==0)||(strcmp(wClassName,"weapon_smokegrenade",false)==0)||
	(strcmp(wClassName,"weapon_tagrenade",false)==0)||(strcmp(wClassName,"weapon_diversion",false)==0)||(strcmp(wClassName,"weapon_snowball",false)==0))
		return 3;
	else if((strcmp(wClassName,"weapon_p90",false)==0)||(strcmp(wClassName,"weapon_ak47",false)==0)||(strcmp(wClassName,"weapon_m4a1",false)==0)||
	(strcmp(wClassName,"weapon_m249",false)==0)||(strcmp(wClassName,"weapon_bizon",false)==0)||(strcmp(wClassName,"weapon_negev",false)==0)||
	(strcmp(wClassName,"weapon_m4a1_silencer",false)==0)||(strcmp(wClassName,"weapon_aug",false)==0)||(strcmp(wClassName,"weapon_awp",false)==0)||
	(strcmp(wClassName,"weapon_famas",false)==0)||(strcmp(wClassName,"weapon_g3sg1",false)==0)||(strcmp(wClassName,"weapon_galilar",false)==0)||
	(strcmp(wClassName,"weapon_scar20",false)==0)||(strcmp(wClassName,"weapon_ssg08",false)==0)||(strcmp(wClassName,"weapon_ump45",false)==0)||
	(strcmp(wClassName,"weapon_xm1014",false)==0)||(strcmp(wClassName,"weapon_mac10",false)==0)||(strcmp(wClassName,"weapon_mag7",false)==0)||
	(strcmp(wClassName,"weapon_mp7",false)==0)||(strcmp(wClassName,"weapon_mp9",false)==0)||(strcmp(wClassName,"weapon_nova",false)==0)||
	(strcmp(wClassName,"weapon_sawedoff",false)==0)||(strcmp(wClassName,"weapon_sg556",false)==0)||(strcmp(wClassName,"weapon_galil",false)==0)||
	(strcmp(wClassName,"weapon_m3",false)==0)||(strcmp(wClassName,"weapon_mp5navy",false)==0)||(strcmp(wClassName,"weapon_scout",false)==0)||
	(strcmp(wClassName,"weapon_sg550",false)==0)||(strcmp(wClassName,"weapon_sg552",false)==0)||(strcmp(wClassName,"weapon_tmp",false)==0))
		return 0;
	else if((strcmp(wClassName,"weapon_c4",false)==0)||(strcmp(wClassName,"weapon_breachcharge",false)==0)||(strcmp(wClassName,"weapon_bumpmine",false)==0)||
	(strcmp(wClassName,"weapon_healthshot",false)==0)||(strcmp(wClassName,"weapon_shield",false)==0))
		return 4;
	else return -1;
}
