#include <nds.h>
#include <stdio.h>
#include <string.h>

#include "ndsheader.h"
#include "module_params.h"
#include "romdb.h"

extern sNDSBannerExt ndsBanner;

/**
 * Get the title ID.
 * @param ndsFile DS ROM image.
 * @param buf Output buffer for title ID. (Must be at least 4 characters.)
 * @return 0 on success; non-zero on error.
 */
int grabTID(FILE *ndsFile, char *buf)
{
	fseek(ndsFile, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
	size_t size = fread(buf, 1, 4, ndsFile);
	return !(size == 4);
}

/**
 * Get SDK version from an NDS file.
 * @param ndsFile NDS file.
 * @param filename NDS ROM filename.
 * @param allowDSi If true, allow DSi-enhanced ROMs; if not, return 0 for DSi-enhanced ROMs.
 * @return 0 on success; non-zero on error.
 */
u32 getSDKVersion(FILE *ndsFile, bool allowDSi)
{
	sNDSHeaderExt NDSHeader;
	fseek(ndsFile, 0, SEEK_SET);
	fread(&NDSHeader, 1, sizeof(NDSHeader), ndsFile);
	if (NDSHeader.arm7destination >= 0x037F8000)
		return 0;
	if (!allowDSi) {
		char tidBuf[4];
		if (grabTID(ndsFile, tidBuf) != 0)
			return 0;
	}
	return getModuleParams(&NDSHeader, ndsFile)->sdk_version;
}

/**
 * Check if an NDS game has AP.
 * @param game_TID Title ID. (ID4)
 * @param headerCRC16 Header CRC.
 * @return true on success; false if no AP.
 */
bool checkRomAP(const char *game_TID, u16 headerCRC16)
{
	char ipsPath[256];
	snprintf(ipsPath, sizeof(ipsPath), "sd:/_nds/TWiLightMenu/apfix/%s-%X.ips", game_TID, headerCRC16);

	if (access(ipsPath, F_OK) == 0) {
		return false;
	}

	// Check for SDK4-5 ROMs that don't have AP measures.
	if ((memcmp(game_TID, "AZLJ", 4) == 0)  // Girls Mode (JAP version of Style Savvy)
	 || (memcmp(game_TID, "YEEJ", 4) == 0)  // Inazuma Eleven (J)
	 || (memcmp(game_TID, "VSO",  3) == 0)  // Sonic Classic Collection
	 || (memcmp(game_TID, "B2D",  3) == 0)  // Doctor Who: Evacuation Earth
	 || (memcmp(game_TID, "BWB",  3) == 0)  // Plants vs Zombies
	 || (memcmp(game_TID, "VDX",  3) == 0)	  // Daniel X: The Ultimate Power
	 || (memcmp(game_TID, "BUD",  3) == 0)	  // River City Super Sports Challenge
	 || (memcmp(game_TID, "B3X",  3) == 0)	  // River City Soccer Hooligans
	 || (memcmp(game_TID, "BZX",  3) == 0)	  // Puzzle Quest 2
	 || (memcmp(game_TID, "BRFP", 4) == 0)	 // Rune Factory 3 - A Fantasy Harvest Moon
	 || (memcmp(game_TID, "BDX",  3) == 0)   // Minna de Taikan Dokusho DS: Choo Kowaai!: Gakkou no Kaidan
	 || (memcmp(game_TID, "TFB",  3) == 0)  // Frozen: Olaf's Quest
	 || (memcmp(game_TID, "B88",  3) == 0)) // DS WiFi Settings
	{
		return false;
	}
	else
	// Check for ROMs that have AP measures.
	if ((memcmp(game_TID, "B", 1) == 0)
	 || (memcmp(game_TID, "T", 1) == 0)
	 || (memcmp(game_TID, "V", 1) == 0)) {
		return true;
	} else {
		static const char ap_list[][4] = {
			"ABT",	// Bust-A-Move DS
			"YHG",	// Houkago Shounen
			"YWV",	// Taiko no Tatsujin DS: Nanatsu no Shima no Daibouken!
			"AS7",	// Summon Night: Twin Age
			"YFQ",	// Nanashi no Geemu
			"AFX",	// Final Fantasy Crystal Chronicles: Ring of Fates
			"YV5",	// Dragon Quest V: Hand of the Heavenly Bride
			"CFI",	// Final Fantasy Crystal Chronicles: Echoes of Time
			"CCU",	// Tomodachi Collection
			"CLJ",	// Mario & Luigi: Bowser's Inside Story
			"YKG",	// Kindgom Hearts: 358/2 Days
			"COL",	// Mario & Sonic at the Olympic Winter Games
			"C24",	// Phantasy Star 0
			"AZL",	// Style Savvy
			"CS3",	// Sonic and Sega All Stars Racing
			"IPK",	// Pokemon HeartGold Version
			"IPG",	// Pokemon SoulSilver Version
			"YBU",	// Blue Dragon: Awakened Shadow
			"YBN",	// 100 Classic Books
			"YVI",	// Dragon Quest VI: Realms of Revelation
			"YDQ",	// Dragon Quest IX: Sentinels of the Starry Skies
			"C3J",	// Professor Layton and the Unwound Future
			"IRA",	// Pokemon Black Version
			"IRB",	// Pokemon White Version
			"CJR",	// Dragon Quest Monsters: Joker 2
			"YEE",	// Inazuma Eleven
			"UZP",	// Learn with Pokemon: Typing Adventure
			"IRE",	// Pokemon Black Version 2
			"IRD",	// Pokemon White Version 2
		};

		// TODO: If the list gets large enough, switch to bsearch().
		for (unsigned int i = 0; i < sizeof(ap_list)/sizeof(ap_list[0]); i++) {
			if (memcmp(game_TID, ap_list[i], 3) == 0) {
				// Found a match.
				return true;
				break;
			}
		}

	}

	return false;
}

/**
 * Check if NDS game has AP.
 * @param ndsFile NDS file.
 * @param filename NDS ROM filename.
 * @return true on success; false if no AP.
 */
bool checkRomAP(FILE *ndsFile)
{
	char game_TID[5];
	u16 headerCRC16 = 0;
	fseek(ndsFile, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
	fread(game_TID, 1, 4, ndsFile);
	fseek(ndsFile, offsetof(sNDSHeaderExt, headerCRC16), SEEK_SET);
	fread(&headerCRC16, sizeof(u16), 1, ndsFile);
	game_TID[4] = 0;

	return checkRomAP(game_TID, headerCRC16);
}
