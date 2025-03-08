#if defined _EntWatch_include
  #endinput
#endif
#define _EntWatch_include

#define EW_V_MAJOR   "3"
#define EW_V_MINOR   "73"

#define EW_VERSION   EW_V_MAJOR...".DZ."...EW_V_MINOR

enum struct class_NativeClientSettings_Hud
{
	bool		Display;
	int			Color[4];
	float		Pos_X;
	float		Pos_Y;
	bool		Name;
}

/**
 * Checks if a client is currently banned, if an integer variable is referenced the time of unban will be assigned to it.
 *
 * @param iClient		Client index to check for ban
 * @param iTimeStamp	Pass an integer variable by reference and it will contain the UNIX timestamp when the player will be unbanned/ when a player was banned if ban = Permamently/Temporary
 * @param iDuration		Duration of the ban -1 - Temporary, 0 - Permamently, Positive value - time in minutes
 * @param sReason		The reason why the player was banned
 * @return				True if user is banned, false otherwise
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_IsClientBanned(int iClient, int &iTimeStamp, int &iDuration, char[] sReason);

/**
 * Bans a client from using special items.
 *
 * @param iClient		Client index to ban
 * @param iAdmin		Admin index which ban (if IsValidClient - Client, else Console)
 * @param iDuration		Duration of the ban -1 - Temporary, 0 - Permamently, Positive value - time in minutes
 * @param sReason		The reason why the player will be banned
 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_BanClient(int iClient, int iAdmin = 0, int iDuration = 0, const char[] sReason = "No Reason");

/**
 * Unbans a previously ebanned Client.
 *
 * @param iClient		Client index to unban
 * @param iAdmin		Admin index which ban (if IsValidClient - Client, else Console)
 * @param sReason		The reason why the player will be unbanned
 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_UnbanClient(int iClient, int iAdmin = 0, const char[] sReason = "No Reason");

/**
* Gets client Ebans Number
*
* @param iClient		Client index to get
* @return				Client's Ebans number
*
* On error/errors:		Invalid client index
*/
native int EntWatch_GetClientEbansNumber(int iCient);

/**
 * Gets client HUD settings
 *
 * @param iClient		Client index to get
 * @param cSettings		An instance of the class in which the information will be saved

 * @return				True and client settings on success, false and default settings otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_GetHudSettings(int iClient, any cSettings[sizeof(class_NativeClientSettings_Hud)]);

/**
 * Sets client HUD Display
 *
 * @param iClient		Client index to set
 * @param iDisplay		Parameter to Display. 0 - Disable, otherwsie - Enable

 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index/Client cookies are not yet loaded
 */
native bool EntWatch_SetHudDisplay(int iClient, int iDisplay);

/**
 * Sets client HUD Name Display
 *
 * @param iClient		Client index to set
 * @param iName			Parameter to Name Display. 0 - Disable, otherwsie - Enable

 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index/Client cookies are not yet loaded
 */
native bool EntWatch_SetHudName(int iClient, int iName);

/**
 * Checks if an entity is a special item.
 *
 * @param iEntity		Entity index to check
 * @return				True if entity is a special item, false otherwsie
 */
native bool EntWatch_IsSpecialItem(int iEntity);

/**
 * Checks if a client has a special item.
 *
 * @param iClient		Client index to check
 * @return				True if client has a special item, false otherwsie
 */
native bool EntWatch_HasSpecialItem(int iClient);

/**
 * Sets the HighLighting Weapons privilege for an array of clients.
 *
 * @param iCount		Count of clients in the array
 * @param iClients		Array of clients for which to set the HighLighting Weapons privilege
 *
 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_SetHLWeapon(int iCount, int[] iClients);

/**
 * Sets the HighLighting Players privilege for an array of clients.
 *
 * @param iCount		Count of clients in the array
 * @param iClients		Array of clients for which to set the HighLighting Players privilege
 *
 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_SetHLPlayer(int iCount, int[] iClients);

/**
 * Sets the HighLighting Weapons privilege for client.
 *
 * @param iClient		Client for which to set the HighLighting Weapons privilege
 *
 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_SetHLWeaponOne(int iClient);

/**
 * Sets the HighLighting Players privilege for client.
 *
 * @param iClient		Client for which to set the HighLighting Players privilege
 *
 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_SetHLPlayerOne(int iClient);

/**
 * Remove the HighLighting Weapons privilege for client.
 *
 * @param iClient		Client for which to remove the HighLighting Weapons privilege
 *
 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_RemoveHLWeaponOne(int iClient);

/**
 * Remove the HighLighting Players privilege for client.
 *
 * @param iClient		Client for which to remove the HighLighting Players privilege
 *
 * @return				True on success, false otherwsie
 *
 * On error/errors:		Invalid client index
 */
native bool EntWatch_RemoveHLPlayerOne(int iClient);

/**
 * Called when a client is e-banned by any means
 *
 * @param iAdmin		Admin index that issued the ban
 * @param iDuration		Duration of the ban
 * @param iClient		Client index that was banned
 * @param sReason		The reason why the player was banned
 *
 * @return				None
 */
forward void EntWatch_OnClientBanned(int iAdmin, int iDuration, int iClient, const char[] sReason);

/**
 * Called when a client is e-unbanned by any means
 *
 * @param iAdmin		Admin index that removed the ban
 * @param iClient		Client index that was unbanned
 * @param sReason		The reason why the player was unbanned
 * @return				None
 */
forward void EntWatch_OnClientUnbanned(int iAdmin, int iClient, const char[] sReason);

/**
 * Сalled when the EntWatch is ready to accept requests for privileges for HighLight Weapon
 *
 * @return				None
 */
forward void EntWatch_OnHLWeaponReady();

/**
 * Сalled when the EntWatch is ready to accept requests for privileges for HighLight Player
 *
 * @return				None
 */
forward void EntWatch_OnHLPlayerReady();

/**
 * Сalled when a client is use item
 *
 * @param sItemName		The name of the item that was used
 * @param iClient		Client index that was used item
 * @param iAbility		The ability of the item that was used (0-Standart, 1 - First Ability, Other - Second Ability)
 * @return				None
 */
forward void EntWatch_OnUseItem(const char[] sItemName, int iClient, int iAbility);

/**
 * Сalled when a client is pickup item
 *
 * @param sItemName		The name of the item that was picked
 * @param iClient		Client index that was picked item
 * @return				None
 */
forward void EntWatch_OnPickUpItem(const char[] sItemName, int iClient);

/**
 * Сalled when a client is drop item
 *
 * @param sItemName		The name of the item that was dropped
 * @param iClient		Client index that was dropped item
 * @return				None
 */
forward void EntWatch_OnDropItem(const char[] sItemName, int iClient);

/**
 * Сalled when a client is disconnect with item
 *
 * @param sItemName		The name of the item that the disconnected player had
 * @param iClient		Client index that was disconnected with item
 * @return				None
 */
forward void EntWatch_OnPlayerDisconnectWithItem(const char[] sItemName, int iClient);

/**
 * Сalled when a client is death with item
 *
 * @param sItemName		The name of the item that player had when death
 * @param iClient		Client index that was death with item
 * @return				None
 */
forward void EntWatch_OnPlayerDeathWithItem(const char[] sItemName, int iClient);

/**
 * Called when the EntWatch tries to find a configuration file
 *
 * @param sFileName		The name of the configuration file the EntWatch is trying to load
 * @return				None
 */
forward void EntWatch_OnCfgLoading(const char[] sFileName);

/**
 * Called when the EntWatch cannot find the configuration file
 *
 * @param sFileName		The name of the configuration file the EntWatch was trying to load
 * @return				None
 */
forward void EntWatch_OnCfgNotFound(const char[] sFileName);

/**
 * Called when the EntWatch has loaded the Server Name from the scheme
 *
 * @param sServerName	Server name loaded from scheme
 * @return				None
 */
forward void EntWatch_OnSchemeServerName(const char[] sServerName);

/**
 * Called when the EntWatch cannot find the scheme
 *
 * @param sFileName		The name of the scheme file the EntWatch was trying to load
 * @return				None
 */
forward void EntWatch_OnSchemeNotFound(const char[] sFileName);

/**
 * Called when the EntWatch cannot connect to the database
 *
 * @param sError		Error text
 * @return				None
 */
forward void EntWatch_OnDatabaseFailure(const char[] sError);

/**
 * Called when the EntWatch has connected to the database
 *
 * @return				None
 */
forward void EntWatch_OnDatabaseSuccess();

/**
 * Called when the EntWatch does not recognize the database driver
 *
 * @param sDriverDB		Name of driver
 * @return				None
 */
forward void EntWatch_OnDatabaseUnknownDriver(const char[] sDriverDB);

/**
 * Called when the EntWatch is ready
 *
 * @return				None
 */
forward void EntWatch_OnDatabaseReady();

/**
 * Called when EntWatch is unable to create tables in the database
 *
 * @param sError		Error text
 * @return				None
 */
forward void EntWatch_OnDatabaseCreateTablesError(const char[] sError);

/**
 * Called when the EntWatch cannot ban a client from the database
 *
 * @param sError		Error text
 * @return				None
 */
forward void EntWatch_OnDatabaseBanError(const char[] sError);

/**
 * Called when EntWatch has successfully e-banned a client.
 *
 * @return				None
 */
forward void EntWatch_OnDatabaseBanSuccess();

/**
 * Called when the EntWatch cannot e-unban a client from the database
 *
 * @param sError		Error text
 * @return				None
 */
forward void EntWatch_OnDatabaseUnBanError(const char[] sError);

/**
 * Called when EntWatch has successfully e-unbanned a client.
 *
 * @return				None
 */
forward void EntWatch_OnDatabaseUnBanSuccess();

/**
 * Called when the EntWatch cannot e-update a client from the database
 *
 * @param sError		Error text
 * @return				None
 */
forward void EntWatch_OnDatabaseBanUpdateError(const char[] sError);

/**
 * Called when EntWatch fails to e-unban offline players
 *
 * @param sError		Error text
 * @return				None
 */
forward void EntWatch_OnDatabaseOfflineUnBanError(const char[] sError);

/**
 * Called when a client is offline e-banned by any means
 *
 * @param iAdmin			Admin index that issued the ban
 * @param iDuration			Duration of the ban
 * @param sClientName		Client name that was banned
 * @param sClientSteamID	Client SteamID that was banned
 * @param sReason			The reason why the player was banned
 *
 * @return				None
 */
forward void EntWatch_OnClientOfflineBanned(int iAdmin, int iDuration, const char[] sClientName, const char[] sClientSteamID, const char[] sReason);

/**
 * Called when the EntWatch cannot offline e-banned a client in the database
 *
 * @param sError		Error text
 * @return				None
 */
forward void EntWatch_OnOfflineBanError(const char[] sError);

/**
 * Called when EntWatch has successfully offline e-banned a client.
 *
 * @return				None
 */
forward void EntWatch_OnOfflineBanSuccess();

/**
 * Called when a admin was spawned item
 *
 * @param iAdmin		Admin index that was spawned item
 * @param sItemName		The name of the item that was spawned
 * @param iTarget		Target index that received the item
 *
 * @return				None
 */
forward void EntWatch_OnAdminSpawnItem(int iAdmin, const char[] sItemName, int iTarget);

/**
 * Called when a admin was transfered all items from target to receiver
 *
 * @param iAdmin		Admin index that was transfered all items
 * @param iTarget		Target index from whom all items was taken
 * @param iReceiver		Receiver index that received all items
 *
 * @return				None
 */
forward void EntWatch_OnAdminTransferedAllItems(int iAdmin, int iTarget, int iReceiver);

/**
 * Called when a admin was transfered item to receiver
 *
 * @param iAdmin		Admin index that was transfered item
 * @param sItemName		The name of the item transfered to the receiver
 * @param iReceiver		Receiver index that received the item
 *
 * @return				None
 */
forward void EntWatch_OnAdminTransferedItem(int iAdmin, const char[] sItemName, int iReceiver);

public SharedPlugin __pl_EntWatch =
{
	name = "EntWatch",
	file = "entwatch_dz.smx",
#if defined REQUIRE_PLUGIN
	required = 1
#else
	required = 0
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_EntWatch_SetNTVOptional()
{
	MarkNativeAsOptional("EntWatch_IsClientBanned");
	MarkNativeAsOptional("EntWatch_BanClient");
	MarkNativeAsOptional("EntWatch_UnbanClient");
	MarkNativeAsOptional("EntWatch_IsSpecialItem");
	MarkNativeAsOptional("EntWatch_HasSpecialItem");
}
#endif
