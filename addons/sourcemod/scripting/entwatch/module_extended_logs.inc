#define EW_MODULE_ELOGS
#define WEBHOOK_URL_MAX_SIZE	1000

ConVar	g_hCvar_ELog_Type;
ConVar	g_hCvar_ELog_Webhook;
ConVar g_hCvar_ELog_WebhookRetry;

stock void EWM_ELogs_OnPluginStart()
{
	g_hCvar_ELog_Type		= CreateConVar("entwatch_elog_type", "0", "How should elogs be notified (-1 = Disabled, 0 = Server, 1 = Discord)");
	g_hCvar_ELog_Webhook	= CreateConVar("entwatch_elog_webhook", "", "The webhook URL of your Discord channel.", FCVAR_PROTECTED);
	g_hCvar_ELog_WebhookRetry = CreateConVar("entwatch_elog_webhook_retry", "3", "Number of retries if webhook fails.", FCVAR_PROTECTED);
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
	else if (g_hCvar_ELog_Type.IntValue == 1)
	{
		char sWebhookURL[WEBHOOK_URL_MAX_SIZE];
		g_hCvar_ELog_Webhook.GetString(sWebhookURL, sizeof sWebhookURL);
		if(!sWebhookURL[0])
		{
			LogError("[EntWatch-Elogs] No webhook found or specified.");
			return;
		}

		char sMessage[4096];
		char sTime[64];
		int iTime = GetTime();
		FormatTime(sTime, sizeof(sTime), "%m/%d/%Y @ %H:%M:%S", iTime);

		char currentMap[PLATFORM_MAX_PATH];
		GetCurrentMap(currentMap, sizeof(currentMap));

		Format(sMessage, sizeof(sMessage), "*%s - %s* ```%s```", currentMap, sTime, message);

		if(StrContains(sMessage, "\"") != -1)
			ReplaceString(sMessage, sizeof(sMessage), "\"", "");

		SendWebHook(sMessage, sWebhookURL);
	}
}

stock void SendWebHook(char sMessage[4096], char sWebhookURL[WEBHOOK_URL_MAX_SIZE])
{
	Webhook webhook = new Webhook(sMessage);

	DataPack pack = new DataPack();
	pack.WriteString(sMessage);
	pack.WriteString(sWebhookURL);

	webhook.Execute(sWebhookURL, OnWebHookExecuted, pack);
	delete webhook;
}

public void OnWebHookExecuted(HTTPResponse response, DataPack pack)
{
	static int retries = 0;

	pack.Reset();

	char sMessage[4096];
	pack.ReadString(sMessage, sizeof(sMessage));

	char sWebhookURL[WEBHOOK_URL_MAX_SIZE];
	pack.ReadString(sWebhookURL, sizeof(sWebhookURL));

	delete pack;

	if (response.Status != HTTPStatus_OK)
	{
		if (retries < g_hCvar_ELog_WebhookRetry.IntValue)
		{
			PrintToServer("[EntWatch-ELogs] Failed to send the webhook. Resending it .. (%d/%d)", retries, g_hCvar_ELog_WebhookRetry.IntValue);

			SendWebHook(sMessage, sWebhookURL);
			retries++;
			return;
		}
		else
		{
			LogError("[EntWatch-Elogs] Failed to send the webhook after %d retries, aborting.", retries);
		}
	}

	retries = 0;
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
