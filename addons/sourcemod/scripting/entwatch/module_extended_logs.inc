#define EW_MODULE_ELOGS

ConVar	g_hCvar_ELog_Type;

stock void EWM_ELogs_OnPluginStart()
{
	g_hCvar_ELog_Type		= CreateConVar("entwatch_elog_type", "0", "How should elogs be notified (-1 = Disabled, 0 = Server)");
}

stock void EWM_ELogs_OnConfigsExecuted()
{
	
}

stock void EWM_ELogs_Log(int client, int target, const char[] message)
{
	if (g_hCvar_ELog_Type.IntValue == 0)
	{
		LogAction(client, target, message);
	}
}

stock void EWM_ELogs_Use(class_ItemList ItemTest, int iClient, int iAbility)
{
	char sMessage[4096];

	if(iAbility == 0) Format(sMessage, sizeof(sMessage), "\"%L\" used item \"%s\"", iClient, ItemTest.Name);
	else if(iAbility == 1) Format(sMessage, sizeof(sMessage), "\"%L\" used First Ability of item \"%s\"", iClient, ItemTest.Name);
	else Format(sMessage, sizeof(sMessage), "\"%L\" used Second Ability of item \"%s\"", iClient, ItemTest.Name);

	EWM_ELogs_Log(iClient, -1, sMessage);
}

stock void EWM_ELogs_PickUp(class_ItemList ItemTest, int iClient)
{
	char sMessage[4096];
	Format(sMessage, sizeof(sMessage), "\"%L\" has picked up item \"%s\"", iClient, ItemTest.Name);
	EWM_ELogs_Log(iClient, -1, sMessage);
}

stock void EWM_ELogs_Drop(class_ItemList ItemTest, int iClient)
{
	char sMessage[4096];
	Format(sMessage, sizeof(sMessage), "\"%L\" has dropped item \"%s\"", iClient, ItemTest.Name);
	EWM_ELogs_Log(iClient, -1, sMessage);
}

stock void EWM_ELogs_Disconnect(class_ItemList ItemTest, int iClient)
{
	char sMessage[4096];
	Format(sMessage, sizeof(sMessage), "\"%L\" disconnected with item \"%s\"", iClient, ItemTest.Name);
	EWM_ELogs_Log(iClient, -1, sMessage);
}

stock void EWM_ELogs_PlayerDeath(class_ItemList ItemTest, int iClient)
{
	char sMessage[4096];
	Format(sMessage, sizeof(sMessage), "\"%L\" has died with item \"%s\"", iClient, ItemTest.Name);
	EWM_ELogs_Log(iClient, -1, sMessage);
}
