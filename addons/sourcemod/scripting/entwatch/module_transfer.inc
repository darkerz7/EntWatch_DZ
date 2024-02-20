#define EW_MODULE_TRANSFER

// Purpose: Allows the administrator to transfer an item from the player or dropped to the ground

stock void EWM_Transfer_OnPluginStart()
{
	RegAdminCmd("sm_etransfer", EWM_Transfer_Command_Transfer, ADMFLAG_BAN);
}

// Interface transfer between clients
public void EWM_Transfer_Client(int iClient, int iTarget, int iReceiver)
{
	if (GetClientTeam(iTarget) != GetClientTeam(iReceiver)) // The team that owns the item is different from the team that gets the item
	{
		CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Differs team");
		return;
	}
	
	if (iTarget == iReceiver) // Sending and receiving same client
	{
		CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Already owns");
		return;
	}
	
	// Transfer of all items
	int iWeaponCount = 0;
	for(int i = 0; i<g_ItemList.Length; i++)
	{
		class_ItemList ItemTest;
		g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
		if(ItemTest.OwnerID == iTarget && ItemTest.AllowTransfer)
		{
			iWeaponCount++;
			EWM_Transfer_Function(iClient, i, iReceiver, false);
		}
	}
	if(iWeaponCount <= 0)
	{
		//Not Found
		CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Target has no items");
	} else
	{
		#if defined EW_MODULE_FORWARDS
		Call_StartForward(g_hOnAdminTransferedAllItems);
		Call_PushCell(iClient);
		Call_PushCell(iTarget);
		Call_PushCell(iReceiver);
		Call_Finish();
		#endif
		CPrintToChatAll("%s%t %s%N %s%t %s%N %s%t %s%N", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Warning, "Transfered all items", g_SchemeConfig.Color_Name, iTarget, g_SchemeConfig.Color_Warning, "EW to", g_SchemeConfig.Color_Name, iReceiver);
	}
}

// Interface transfer by item name
void EWM_Transfer_ItemName(int iClient, char[] sItemName, int iReceiver)
{
	int iWeaponCount = 0,
		iItemIndex = -1;
	for(int i = 0; i<g_ItemList.Length; i++)
	{
		class_ItemList ItemTest;
		g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
		if((StrContains(ItemTest.Name, sItemName, false) != -1 || StrContains(ItemTest.ShortName, sItemName, false) != -1) && ItemTest.AllowTransfer)
		{
			iWeaponCount++;
			iItemIndex = i;
		}
	}
	if(iWeaponCount <= 0)
	{
		//Not Found
		CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Invalid item name");
	} else if(iWeaponCount == 1)
	{
		//Only 1 weapon
		EWM_Transfer_Function(iClient, iItemIndex, iReceiver, true);
	} else
	{
		//More 1 weapon
		Menu hEdictMenu = CreateMenu(EWM_Transfer_EdictMenu_Handler);
		char sMenuTemp[64], sIndexTemp[32];
		FormatEx(sMenuTemp, sizeof(sMenuTemp), "%T %T:", "EW_Tag", iClient, "Item targets", iClient);
		hEdictMenu.SetTitle(sMenuTemp);
		for(int i = 0; i<g_ItemList.Length; i++)
		{
			class_ItemList ItemTest;
			g_ItemList.GetArray(i, ItemTest, sizeof(ItemTest));
			if((StrContains(ItemTest.Name, sItemName, false) != -1 || StrContains(ItemTest.ShortName, sItemName, false) != -1) && ItemTest.AllowTransfer && IsValidEdict(ItemTest.WeaponID))
			{
				FormatEx(sIndexTemp, sizeof(sIndexTemp), "%i/%i", i, g_iUserIDs[iReceiver]);
				if(ItemTest.OwnerID != INVALID_ENT_REFERENCE)
					FormatEx(sMenuTemp, sizeof(sMenuTemp), "%s (%N|#%i)", ItemTest.Name, ItemTest.OwnerID, g_iUserIDs[ItemTest.OwnerID]);
				else FormatEx(sMenuTemp, sizeof(sMenuTemp), "%s (%T)", ItemTest.Name, "No owner", iClient);
				hEdictMenu.AddItem(sIndexTemp, sMenuTemp, ITEMDRAW_DEFAULT);
			}
		}
		hEdictMenu.Display(iClient, MENU_TIME_FOREVER);
	}
}

public int EWM_Transfer_EdictMenu_Handler(Menu hEdictMenu, MenuAction hAction, int iClient, int iParam2)
{
	switch (hAction)
	{
		case MenuAction_End:delete hEdictMenu;
		case MenuAction_Select:
		{
			char sSelected[32], Explode_sParam[2][32];
			GetMenuItem(hEdictMenu, iParam2, sSelected, sizeof(sSelected));
			ExplodeString(sSelected, "/", Explode_sParam, 2, 32);
			int iItemIndex = StringToInt(Explode_sParam[0]);
			int iReceiver = GetClientOfUserId(StringToInt(Explode_sParam[1]));
			if(!IsValidClient(iReceiver))
			{
				CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Receiver is not valid anymore");
				return 0;
			}
			EWM_Transfer_Function(iClient, iItemIndex, iReceiver, true);
		}
	}
	return 0;
}

// Main transfer function
public void EWM_Transfer_Function(int iClient, int iItemIndex, int iReceiver, bool bMessageTransfer)
{
	if(iItemIndex >= 0 && g_ItemList.Length > iItemIndex)
	{
		#if defined EW_MODULE_EBAN
		if(g_EbanClients[iReceiver].Banned)
		{
			if(IsValidClient(iClient) && IsClientConnected(iClient)) CPrintToChat(iClient, "%s%t %s%N %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, iReceiver, g_SchemeConfig.Color_Disabled, "Has a Restrict");
			return;
		}
		#endif
		class_ItemList ItemTest;
		g_ItemList.GetArray(iItemIndex, ItemTest, sizeof(ItemTest));
		if(IsValidEdict(ItemTest.WeaponID))
		{
			if(ItemTest.OwnerID != INVALID_ENT_REFERENCE)
			{
				int iCurOwner = ItemTest.OwnerID;
				if(iCurOwner != iReceiver)
				{
					if(GetClientTeam(iCurOwner) == GetClientTeam(iReceiver))
					{
						char sClassnameBuf[64];
						GetEdictClassname(ItemTest.WeaponID, sClassnameBuf, sizeof(sClassnameBuf));
						CS_DropWeapon(iCurOwner, ItemTest.WeaponID, false);
						GivePlayerItem(iCurOwner, sClassnameBuf);
						/*if(ItemTest.Chat)
						{
							ItemTest.Chat = false;
							g_ItemList.SetArray(iItemIndex, ItemTest, sizeof(ItemTest));
							FixedEquipPlayerWeapon(iReceiver, ItemTest.WeaponID);
							ItemTest.Chat = true;
							g_ItemList.SetArray(iItemIndex, ItemTest, sizeof(ItemTest));
						}*/
						FixedEquipPlayerWeapon(iReceiver, ItemTest.WeaponID);
						if(bMessageTransfer)
						{
							#if defined EW_MODULE_FORWARDS
							Call_StartForward(g_hOnAdminTransferedAllItems);
							Call_PushCell(iClient);
							Call_PushCell(iCurOwner);
							Call_PushCell(iReceiver);
							Call_Finish();
							#endif
							CPrintToChatAll("%s%t %s%N %s%t %s%N %s%t %s%N", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Warning, "Transfered all items", g_SchemeConfig.Color_Name, iCurOwner, g_SchemeConfig.Color_Warning, "EW to", g_SchemeConfig.Color_Name, iReceiver);
						}
					}else if(bMessageTransfer) CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Differs team");
				}else if(bMessageTransfer) CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Already owns");
			}else
			{
				/*if(ItemTest.Chat)
				{
					ItemTest.Chat = false;
					g_ItemList.SetArray(iItemIndex, ItemTest, sizeof(ItemTest));
					FixedEquipPlayerWeapon(iReceiver, ItemTest.WeaponID);
					ItemTest.Chat = true;
					g_ItemList.SetArray(iItemIndex, ItemTest, sizeof(ItemTest));
				}*/
				FixedEquipPlayerWeapon(iReceiver, ItemTest.WeaponID);
				if(bMessageTransfer)
				{
					#if defined EW_MODULE_FORWARDS
					Call_StartForward(g_hOnAdminTransferedItem);
					Call_PushCell(iClient);
					Call_PushString(ItemTest.Name);
					Call_PushCell(iReceiver);
					Call_Finish();
					#endif
					CPrintToChatAll("%s%t %s%N %s%t %s%s %s%t %s%N", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Name, iClient, g_SchemeConfig.Color_Warning, "Transfered", ItemTest.Color, ItemTest.Name, g_SchemeConfig.Color_Warning, "EW to", g_SchemeConfig.Color_Name, iReceiver);
				}
			}
		} else CPrintToChat(iClient, "%s%t %s%t", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Item not available");
	}
}

public Action EWM_Transfer_Command_Transfer(int iClient, int iArgs)
{
	if((IsClientConnected(iClient) && IsClientInGame(iClient)) && g_bConfigLoaded)
	{
		if (GetCmdArgs() != 2)
		{
			CReplyToCommand(iClient, "%s%t %s%t: sm_etransfer <owner>/$<itemname> <receiver>", g_SchemeConfig.Color_Tag, "EW_Tag", g_SchemeConfig.Color_Warning, "Usage");
			return Plugin_Handled;
		}

		int iTarget = -1,
			iReceiver = -1;
		
		char sTarget[64], sReceiver[64];
		GetCmdArg(1, sTarget, sizeof(sTarget));
		GetCmdArg(2, sReceiver, sizeof(sReceiver));
		
		if ((iReceiver = FindTarget(iClient, sReceiver, true, false)) == -1) return Plugin_Handled;
		
		if(sTarget[0] == '$')
		{
			strcopy(sTarget, sizeof(sTarget), sTarget[1]);
			EWM_Transfer_ItemName(iClient, sTarget, iReceiver);
		} else
		{
			if ((iTarget = FindTarget(iClient, sTarget, true, false)) == -1) return Plugin_Handled;
			EWM_Transfer_Client(iClient, iTarget, iReceiver);
		}
	}
	return Plugin_Handled;
}