/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/
#include <maxmod9.h>
#include <nds.h>
#include <nds/arm9/dldi.h>

#include <fat.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>

#include "common/gl2d.h"
#include <string.h>
#include <unistd.h>

#include "date.h"
#include "fileCopy.h"

#include "graphics/graphics.h"

#include "common/dsimenusettings.h"
#include "common/flashcard.h"
#include "common/nitrofs.h"
#include "common/systemdetails.h"
#include "graphics/ThemeConfig.h"
#include "graphics/ThemeTextures.h"
#include "graphics/themefilenames.h"

#include "fileBrowse.h"
#include "nds_loader_arm9.h"
#include "gbaswitch.h"
#include "perGameSettings.h"

#include "ndsheader.h"
#include "ndsbanner.h"

#include "graphics/fontHandler.h"
#include "graphics/iconHandler.h"

#include "common/inifile.h"
#include "tonccpy.h"

#include "sound.h"
#include "language.h"

#include "cheat.h"
#include "crc.h"

#include "sr_data_srllastran.h"		 // For rebooting into the game

bool whiteScreen = true;
bool fadeType = false; // false = out, true = in
bool fadeSpeed = true; // false = slow (for DSi launch effect), true = fast
bool fadeColor = true; // false = black, true = white
bool controlTopBright = true;
bool controlBottomBright = true;

extern void ClearBrightness();
extern bool showProgressIcon;

const char *settingsinipath = "sd:/_nds/TWiLightMenu/settings.ini";
const char *bootstrapinipath = "sd:/_nds/nds-bootstrap.ini";

const char *unlaunchAutoLoadID = "AutoLoadInfo";
static char hiyaNdsPath[14] = {'s', 'd', 'm', 'c', ':', '/', 'h', 'i', 'y', 'a', '.', 'd', 's', 'i'};
char unlaunchDevicePath[256];

/**
 * Remove trailing slashes from a pathname, if present.
 * @param path Pathname to modify.
 */
void RemoveTrailingSlashes(std::string &path) {
	if (path.size() == 0) return;
	while (!path.empty() && path[path.size() - 1] == '/') {
		path.resize(path.size() - 1);
	}
}

/**
 * Remove trailing spaces from a cheat code line, if present.
 * @param path Code line to modify.
 */
/*static void RemoveTrailingSpaces(std::string& code)
{
	while (!code.empty() && code[code.size()-1] == ' ') {
		code.resize(code.size()-1);
	}
}*/

// These are used by flashcard functions and must retain their trailing slash.
static const std::string slashchar = "/";
static const std::string woodfat = "fat0:/";
static const std::string dstwofat = "fat1:/";

typedef DSiMenuPlusPlusSettings::TLaunchType Launch;

int donorSdkVer = 0;

bool gameSoftReset = false;

int mpuregion = 0;
int mpusize = 0;
bool ceCached = true;

bool applaunch = false;
bool startMenu = false;

bool gbaBiosFound[2] = {false};

bool useBackend = false;

using namespace std;

bool dropDown = false;
bool redoDropDown = false;
int currentBg = 0;
bool showSTARTborder = false;
bool buttonArrowTouched[2] = {false};
bool scrollWindowTouched = false;

bool titleboxXmoveleft = false;
bool titleboxXmoveright = false;

bool applaunchprep = false;

int spawnedtitleboxes = 0;

s16 usernameRendered[11] = {0};
bool usernameRenderedDone = false;

bool showColon = true;

touchPosition touch;

//---------------------------------------------------------------------------------
void stop(void) {
	//---------------------------------------------------------------------------------
	while (1) {
		swiWaitForVBlank();
	}
}

/**
 * Set donor SDK version for a specific game.
 */
void SetDonorSDK(const char *filename) {
	donorSdkVer = 0;

	// Check for ROM hacks that need an SDK version.
	static const char sdk2_list[][4] = {
	    "AMQ", // Mario vs. Donkey Kong 2 - March of the Minis
	    "AMH", // Metroid Prime Hunters
	    "ASM", // Super Mario 64 DS
	};

	static const char sdk3_list[][4] = {
	    "AMC", // Mario Kart DS
	    "EKD", // Ermii Kart DS (Mario Kart DS hack)
	    "A2D", // New Super Mario Bros.
	    "ADA", // Pokemon Diamond
	    "APA", // Pokemon Pearl
	    "ARZ", // Rockman ZX/MegaMan ZX
	    "YZX", // Rockman ZX Advent/MegaMan ZX Advent
	};

	static const char sdk4_list[][4] = {
	    "YKW", // Kirby Super Star Ultra
	    "A6C", // MegaMan Star Force: Dragon
	    "A6B", // MegaMan Star Force: Leo
	    "A6A", // MegaMan Star Force: Pegasus
	    "B6Z", // Rockman Zero Collection/MegaMan Zero Collection
	    "YT7", // SEGA Superstars Tennis
	    "AZL", // Style Savvy
	    "BKI", // The Legend of Zelda: Spirit Tracks
	    "B3R", // Pokemon Ranger: Guardian Signs
	};

	static const char sdk5_list[][4] = {
	    "B2D", // Doctor Who: Evacuation Earth
	    "BH2", // Super Scribblenauts
	    "BSD", // Lufia: Curse of the Sinistrals
	    "BXS", // Sonic Colo(u)rs
	    "BOE", // Inazuma Eleven 3: Sekai heno Chousen! The Ogre
	    "BQ8", // Crafting Mama
	    "BK9", // Kingdom Hearts: Re-Coded
	    "BRJ", // Radiant Historia
	    "IRA", // Pokemon Black Version
	    "IRB", // Pokemon White Version
	    "VI2", // Fire Emblem: Shin Monshou no Nazo Hikari to Kage no Eiyuu
	    "BYY", // Yu-Gi-Oh 5Ds World Championship 2011: Over The Nexus
	    "UZP", // Learn with Pokemon: Typing Adventure
	    "B6F", // LEGO Batman 2: DC Super Heroes
	    "IRE", // Pokemon Black Version 2
	    "IRD", // Pokemon White Version 2
	};

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(sdk2_list) / sizeof(sdk2_list[0]); i++) {
		if (memcmp(gameTid[CURPOS], sdk2_list[i], 3) == 0) {
			// Found a match.
			donorSdkVer = 2;
			break;
		}
	}

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(sdk3_list) / sizeof(sdk3_list[0]); i++) {
		if (memcmp(gameTid[CURPOS], sdk3_list[i], 3) == 0) {
			// Found a match.
			donorSdkVer = 3;
			break;
		}
	}

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(sdk4_list) / sizeof(sdk4_list[0]); i++) {
		if (memcmp(gameTid[CURPOS], sdk4_list[i], 3) == 0) {
			// Found a match.
			donorSdkVer = 4;
			break;
		}
	}

	if (strncmp(gameTid[CURPOS], "V", 1) == 0) {
		donorSdkVer = 5;
	} else {
		// TODO: If the list gets large enough, switch to bsearch().
		for (unsigned int i = 0; i < sizeof(sdk5_list) / sizeof(sdk5_list[0]); i++) {
			if (memcmp(gameTid[CURPOS], sdk5_list[i], 3) == 0) {
				// Found a match.
				donorSdkVer = 5;
				break;
			}
		}
	}
}

/**
 * Disable soft-reset, in favor of non OS_Reset one, for a specific game.
 */
void SetGameSoftReset(const char *filename) {
	scanKeys();
	if(keysHeld() & KEY_R){
		gameSoftReset = true;
		return;
	}

	gameSoftReset = false;

	// Check for games that have it's own reset function (OS_Reset not used).
	static const char list[][4] = {
	    "NTR", // Download Play ROMs
	    "ASM", // Super Mario 64 DS
	    "SMS", // Super Mario Star World, and Mario's Holiday
	    "AMC", // Mario Kart DS
	    "EKD", // Ermii Kart DS
	    "A2D", // New Super Mario Bros.
	    "ARZ", // Rockman ZX/MegaMan ZX
	    "AKW", // Kirby Squeak Squad/Mouse Attack
	    "YZX", // Rockman ZX Advent/MegaMan ZX Advent
	    "B6Z", // Rockman Zero Collection/MegaMan Zero Collection
	};

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(list) / sizeof(list[0]); i++) {
		if (memcmp(gameTid[CURPOS], list[i], 3) == 0) {
			// Found a match.
			gameSoftReset = true;
			break;
		}
	}
}

/**
 * Set MPU settings for a specific game.
 */
void SetMPUSettings(const char *filename) {
	scanKeys();
	int pressed = keysHeld();

	if (pressed & KEY_B) {
		mpuregion = 1;
	} else if (pressed & KEY_X) {
		mpuregion = 2;
	} else if (pressed & KEY_Y) {
		mpuregion = 3;
	} else {
		mpuregion = 0;
	}
	if (pressed & KEY_RIGHT) {
		mpusize = 3145728;
	} else if (pressed & KEY_LEFT) {
		mpusize = 1;
	} else {
		mpusize = 0;
	}

	// Check for games that need an MPU size of 3 MB.
	static const char mpu_3MB_list[][4] = {
	    "A7A", // DS Download Station - Vol 1
	    "A7B", // DS Download Station - Vol 2
	    "A7C", // DS Download Station - Vol 3
	    "A7D", // DS Download Station - Vol 4
	    "A7E", // DS Download Station - Vol 5
	    "A7F", // DS Download Station - Vol 6 (EUR)
	    "A7G", // DS Download Station - Vol 6 (USA)
	    "A7H", // DS Download Station - Vol 7
	    "A7I", // DS Download Station - Vol 8
	    "A7J", // DS Download Station - Vol 9
	    "A7K", // DS Download Station - Vol 10
	    "A7L", // DS Download Station - Vol 11
	    "A7M", // DS Download Station - Vol 12
	    "A7N", // DS Download Station - Vol 13
	    "A7O", // DS Download Station - Vol 14
	    "A7P", // DS Download Station - Vol 15
	    "A7Q", // DS Download Station - Vol 16
	    "A7R", // DS Download Station - Vol 17
	    "A7S", // DS Download Station - Vol 18
	    "A7T", // DS Download Station - Vol 19
	    "ARZ", // Rockman ZX/MegaMan ZX
	    "YZX", // Rockman ZX Advent/MegaMan ZX Advent
	    "B6Z", // Rockman Zero Collection/MegaMan Zero Collection
	    "A2D", // New Super Mario Bros.
	};

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(mpu_3MB_list) / sizeof(mpu_3MB_list[0]); i++) {
		if (memcmp(gameTid[CURPOS], mpu_3MB_list[i], 3) == 0) {
			// Found a match.
			mpuregion = 1;
			mpusize = 3145728;
			break;
		}
	}
}

/**
 * Exclude moving nds-bootstrap's cardEngine_arm9 to cached memory region for some games.
 */
void SetSpeedBumpExclude(const char *filename) {
	scanKeys();
	if(keysHeld() & KEY_L){
		ceCached = false;
		return;
	}

	ceCached = true;

	static const char list[][5] = {
		"AWRP",	// Advance Wars: Dual Strike (EUR)
		"YFTP",	// Pokemon Mystery Dungeon: Explorers of Time (EUR)
		"YFYP",	// Pokemon Mystery Dungeon: Explorers of Darkness (EUR)
		"AH9P",	// Tony Hawk's American Sk8land (EUR)
	};

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(list)/sizeof(list[0]); i++) {
		if (memcmp(gameTid[CURPOS], list[i], 4) == 0) {
			// Found a match.
			ceCached = false;
			break;
		}
	}

	static const char list2[][4] = {
		"AEK",	// Age of Empires: The Age of Kings
		"ALC",	// Animaniacs: Lights, Camera, Action!
		"YAH",	// Assassin's Creed: Alta�r's Chronicles
		"B6R",	// Bakugan: Battle Brawlers
		"AB2",	// Battles of Prince of Persia
		"YB4",	// Bee Movie
		"CBK",	// Bolt
		"CBD",	// Bolt: Be-Awesome Edition
		//"ACV",	// Castlevania: Dawn of Sorrow	(fixed on nds-bootstrap side)
		"BIG",	// Combat/Battle of Giants: Mutant Insects
		"BDB",	// Dragon Ball: Origins 2
		"YIV",	// Dragon Quest IV: Chapters of the Chosen
		"AE4",	// Eyeshield 21 Max Devil Power
		"APR",	// Feel the Magic - XY XX
		"A26",	// Feel the Magic - XY XX (Demo)
		"A5P",	// Harry Potter and the Order of the Phoenix
		"CQ7",	// Henry Hatsworth
		"AR2",	// Kirarin * Revolution: Naasan to Issho
		"B3X",	// Kunio-kun no Chou Nekketsu!: Soccer League Plus - World Hyper Cup Hen
		"AVC",	// Magical Starsign
		"ARM",	// Mario & Luigi: Partners in Time
		"CLJ",	// Mario & Luigi: Bowser's Inside Story
		"COL",	// Mario & Sonic at the Olympic Winter Games
		"AMQ",	// Mario vs. Donkey Kong 2: March of the Minis
		"AMH",	// Metroid Prime Hunters
		"A2D",	// New Super Mario Bros.
		"B2K",	// Ni no Kuni: Shikkoku no Madoushi
		"C2S",	// Pokemon Mystery Dungeon: Explorers of Sky
		"Y6S",	// Pokemon Mystery Dungeon: Explorers of Sky (Demo)
		"CPU",	// Pokemon Platinum
		"B3R",	// Pokemon Ranger: Guardian Signs
		"BPP",	// PostPet DS - Yumemiru Momo to Fushigi no Pen
		"APU",	// Puyo Puyo!! 15th Anniversary
		"BYO",	// Puyo Puyo 7
		"BQ2",	// Quiz Magic Academy DS: Futatsu no Jikuuseki
	    "B3X",	// River City: Soccer Hooligans
	    "ARZ",	// Rockman ZX/MegaMan ZX
		"YZX",	// Rockman ZX Advent/MegaMan ZX Advent
		"B6X",	// Rockman EXE: Operate Shooting Star
		"B6Z",	// Rockman Zero Collection/MegaMan Zero Collection
		"AKA",	// The Rub Rabbits!
		"ARF",	// Rune Factory: A Fantasy Harvest Moon
		"A6N",	// Rune Factory 2: A Fantasy Harvest Moon
		"A3S",	// Shrek the Third
		"AIR",	// Space Invaders DS
		"AIS",	// Space Invaders Revolution
		"YV4",  // Spectrobes: Beyond the Portals
		"AS2",  // Spider-Man 2
		"AQ3",	// Spider-Man 3
		"AST",	// Star Wars Episode III: Revenge of the Sith
		"CS7",	// Summon Night X: Tears Crown
		"AYT",	// Tales of Innocence
		"YT9",	// Tony Hawk's Proving Ground
		"YYK",	// Trauma Center: Under the Knife 2
		"CY8",	// Yu-Gi-Oh! World Championship 2009
		"BYX",	// Yu-Gi-Oh! World Championship 2010
	};

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(list2)/sizeof(list2[0]); i++) {
		if (memcmp(gameTid[CURPOS], list2[i], 3) == 0) {
			// Found match
			ceCached = false;
			break;
		}
	}
}

/**
 * Fix AP for some games.
 */
std::string setApFix(const char *filename) {
	remove("fat:/_nds/nds-bootstrap/apFix.ips");

	bool ipsFound = false;
	char ipsPath[256];
	snprintf(ipsPath, sizeof(ipsPath), "sd:/_nds/TWiLightMenu/apfix/%s.ips", filename);
	ipsFound = (access(ipsPath, F_OK) == 0);

	if (!ipsFound) {
		snprintf(ipsPath, sizeof(ipsPath), "sd:/_nds/TWiLightMenu/apfix/%s-%X.ips", gameTid[CURPOS], headerCRC[CURPOS]);
		ipsFound = (access(ipsPath, F_OK) == 0);
	}

	if (ipsFound) {
		if (ms().secondaryDevice) {
			mkdir("fat:/_nds", 0777);
			mkdir("fat:/_nds/nds-bootstrap", 0777);
			fcopy(ipsPath, "fat:/_nds/nds-bootstrap/apFix.ips");
			return "fat:/_nds/nds-bootstrap/apFix.ips";
		}
		return ipsPath;
	}

	return "";
}

sNDSHeader ndsCart;

/**
 * Enable widescreen for some games.
 */
TWL_CODE void SetWidescreen(const char *filename) {
	remove("/_nds/nds-bootstrap/wideCheatData.bin");

	if (sys().arm7SCFGLocked() || ms().consoleModel < 2 || !ms().wideScreen
	|| (access("sd:/_nds/TWiLightMenu/TwlBg/Widescreen.cxi", F_OK) != 0)) {
		return;
	}
	
	bool wideCheatFound = false;
	char wideBinPath[256];
	if (ms().launchType == Launch::ESDFlashcardLaunch) {
		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s.bin", filename);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	}

	if (ms().launchType == Launch::ESlot1) {
		// Reset Slot-1 to allow reading card header
		sysSetCardOwner (BUS_OWNER_ARM9);
		disableSlot1();
		for(int i = 0; i < 25; i++) { swiWaitForVBlank(); }
		enableSlot1();
		for(int i = 0; i < 15; i++) { swiWaitForVBlank(); }

		cardReadHeader((uint8*)&ndsCart);

		char s1GameTid[5];
		tonccpy(s1GameTid, ndsCart.gameCode, 4);
		s1GameTid[4] = 0;

		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s-%X.bin", s1GameTid, ndsCart.headerCRC16);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	} else if (!wideCheatFound) {
		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s-%X.bin", gameTid[CURPOS], headerCRC[CURPOS]);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	}

	if (wideCheatFound) {
		const char* resultText1;
		const char* resultText2;
		mkdir("/_nds", 0777);
		mkdir("/_nds/nds-bootstrap", 0777);
		if (fcopy(wideBinPath, "/_nds/nds-bootstrap/wideCheatData.bin") == 0) {
			// Prepare for reboot into 16:10 TWL_FIRM
			mkdir("sd:/luma", 0777);
			mkdir("sd:/luma/sysmodules", 0777);
			if ((access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) == 0)
			&& (rename("sd:/luma/sysmodules/TwlBg.cxi", "sd:/luma/sysmodules/TwlBg_bak.cxi") != 0)) {
				resultText1 = "Failed to backup custom";
				resultText2 = "TwlBg.";
			} else {
				if (rename("sd:/_nds/TWiLightMenu/TwlBg/Widescreen.cxi", "sd:/luma/sysmodules/TwlBg.cxi") == 0) {
					irqDisable(IRQ_VBLANK);				// Fix the throwback to 3DS HOME Menu bug
					tonccpy((u32 *)0x02000300, sr_data_srllastran, 0x020);
					fifoSendValue32(FIFO_USER_02, 1); // Reboot in 16:10 widescreen
					stop();
				} else {
					resultText1 = "Failed to reboot TwlBg";
					resultText2 = "in widescreen.";
				}
			}
			rename("sd:/luma/sysmodules/TwlBg_bak.cxi", "sd:/luma/sysmodules/TwlBg.cxi");
		} else {
			resultText1 = "Failed to copy widescreen";
			resultText2 = "code for the game.";
		}
		remove("/_nds/nds-bootstrap/wideCheatData.bin");
		int textXpos[2] = {0};
		if (ms().theme == 4) {
			textXpos[0] = 24;
			textXpos[1] = 40;
		} else {
			textXpos[0] = 72;
			textXpos[1] = 88;
		}
		clearText();
		printLargeCentered(false, textXpos[0], resultText1);
		printLargeCentered(false, textXpos[1], resultText2);
		if (ms().theme != 4) {
			fadeType = true; // Fade in from white
		}
		for (int i = 0; i < 60 * 3; i++) {
			swiWaitForVBlank(); // Wait 3 seconds
		}
		if (ms().theme != 4) {
			fadeType = false;	   // Fade to white
			for (int i = 0; i < 25; i++) {
				swiWaitForVBlank();
			}
		}
	}
}

char filePath[PATH_MAX];

void doPause() {
	while (1) {
		scanKeys();
		if (keysDown() & KEY_START)
			break;
		snd().updateStream();
		swiWaitForVBlank();
	}
	scanKeys();
}

std::string ReplaceAll(const std::string str, const std::string &from, const std::string &to) {
	size_t start_pos = 0;
	std::string newStr = std::string(str);
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		newStr.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return newStr;
}

void loadGameOnFlashcard(const char *ndsPath, std::string filename, bool usePerGameSettings) {
	bool runNds_boostCpu = false;
	bool runNds_boostVram = false;
	if (isDSiMode() && usePerGameSettings) {
		loadPerGameSettings(filename);

		runNds_boostCpu = perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu;
		runNds_boostVram = perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram;
	}
	std::string path;
	int err = 0;
	snd().stopStream();

	if (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0) {
		CIniFile fcrompathini("fat:/_wfwd/lastsave.ini");
		path = ReplaceAll(ndsPath, "fat:/", woodfat);
		fcrompathini.SetString("Save Info", "lastLoaded", path);
		fcrompathini.SaveIniFile("fat:/_wfwd/lastsave.ini");
		err = runNdsFile("fat:/Wfwd.dat", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
	} else if (memcmp(io_dldi_data->friendlyName, "Acekard AK2", 0xB) == 0) {
		CIniFile fcrompathini("fat:/_afwd/lastsave.ini");
		path = ReplaceAll(ndsPath, "fat:/", woodfat);
		fcrompathini.SetString("Save Info", "lastLoaded", path);
		fcrompathini.SaveIniFile("fat:/_afwd/lastsave.ini");
		err = runNdsFile("fat:/Afwd.dat", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
	} else if (memcmp(io_dldi_data->friendlyName, "DSTWO(Slot-1)", 0xD) == 0) {
		CIniFile fcrompathini("fat:/_dstwo/autoboot.ini");
		path = ReplaceAll(ndsPath, "fat:/", dstwofat);
		fcrompathini.SetString("Dir Info", "fullName", path);
		fcrompathini.SaveIniFile("fat:/_dstwo/autoboot.ini");
		err = runNdsFile("fat:/_dstwo/autoboot.nds", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
	} else if (memcmp(io_dldi_data->friendlyName, "R4(DS) - Revolution for DS (v2)", 0xB) == 0) {
		CIniFile fcrompathini("fat:/__rpg/lastsave.ini");
		path = ReplaceAll(ndsPath, "fat:/", woodfat);
		fcrompathini.SetString("Save Info", "lastLoaded", path);
		fcrompathini.SaveIniFile("fat:/__rpg/lastsave.ini");
		// Does not support autoboot; so only nds-bootstrap launching works.
		err = runNdsFile(path.c_str(), 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
	}

	char text[32];
	snprintf(text, sizeof(text), "Start failed. Error %i", err);
	ClearBrightness();
	printLarge(false, 4, 4, text);
	if (err == 0) {
		printLarge(false, 4, 20, "Flashcard may be unsupported.");
		printLarge(false, 4, 52, "Flashcard name:");
		printLarge(false, 4, 68, io_dldi_data->friendlyName);
	}
	stop();
}

void unlaunchSetHiyaBoot(void) {
	snd().stopStream();
	tonccpy((u8 *)0x02000800, unlaunchAutoLoadID, 12);
	*(u16 *)(0x0200080C) = 0x3F0;			   // Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16 *)(0x0200080E) = 0;			   // Unlaunch CRC16 (empty)
	*(u32 *)(0x02000810) = (BIT(0) | BIT(1));	  // Load the title at 2000838h
							   // Use colors 2000814h
	*(u16 *)(0x02000814) = 0x7FFF;			   // Unlaunch Upper screen BG color (0..7FFFh)
	*(u16 *)(0x02000816) = 0x7FFF;			   // Unlaunch Lower screen BG color (0..7FFFh)
	toncset((u8 *)0x02000818, 0, 0x20 + 0x208 + 0x1C0); // Unlaunch Reserved (zero)
	int i2 = 0;
	for (int i = 0; i < 14; i++) {
		*(u8 *)(0x02000838 + i2) =
		    hiyaNdsPath[i]; // Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
		i2 += 2;
	}
	while (*(vu16 *)(0x0200080E) == 0) { // Keep running, so that CRC16 isn't 0
		*(u16 *)(0x0200080E) = swiCRC16(0xFFFF, (void *)0x02000810, 0x3F0); // Unlaunch CRC16
	}
}

void dsCardLaunch() {
	snd().stopStream();
	*(u32 *)(0x02000300) = 0x434E4C54; // Set "CNLT" warmboot flag
	*(u16 *)(0x02000304) = 0x1801;
	*(u32 *)(0x02000308) = 0x43415254; // "CART"
	*(u32 *)(0x0200030C) = 0x00000000;
	*(u32 *)(0x02000310) = 0x43415254; // "CART"
	*(u32 *)(0x02000314) = 0x00000000;
	*(u32 *)(0x02000318) = 0x00000013;
	*(u32 *)(0x0200031C) = 0x00000000;
	while (*(u16 *)(0x02000306) == 0x0000) { // Keep running, so that CRC16 isn't 0
		*(u16 *)(0x02000306) = swiCRC16(0xFFFF, (void *)0x02000308, 0x18);
	}

	unlaunchSetHiyaBoot();

	fifoSendValue32(FIFO_USER_02, 1); // Reboot into DSiWare title, booted via Launcher
	for (int i = 0; i < 15; i++) {
		swiWaitForVBlank();
	}
}

bool extention(const std::string& filename, const char* ext) {
	if(strcasecmp(filename.c_str() + filename.size() - strlen(ext), ext)) {
		return false;
	} else {
		return true;
	}
}

int main(int argc, char **argv) {
	/*SetBrightness(0, 0);
	SetBrightness(1, 0);
	consoleDemoInit();*/

	defaultExceptionHandler();
	sys().initFilesystem();
	sys().initArm7RegStatuses();

	if (access(settingsinipath, F_OK) != 0 && flashcardFound()) {
		settingsinipath =
		    "fat:/_nds/TWiLightMenu/settings.ini"; // Fallback to .ini path on flashcard, if not found on
							   // SD card, or if SD access is disabled
	}

	ms().loadSettings();
	tfn(); //
	tc().loadConfig();
	tex().videoSetup(); // allocate texture pointers

	fontInit();

	if (ms().theme == 4) {
		tex().loadSaturnTheme();
	} else if (ms().theme == 1) {
		tex().load3DSTheme();
	} else {
		tex().loadDSiTheme();
	}

	printf("Username copied\n");
	tonccpy(usernameRendered, PersonalData->name, sizeof(s16) * 10);

	if (!sys().fatInitOk()) {
		graphicsInit();
		fontInit();
		whiteScreen = false;
		fadeType = true;
		for (int i = 0; i < 5; i++)
			swiWaitForVBlank();
		if (!dropDown && ms().theme == 0) {
			dropDown = true;
			for (int i = 0; i < 72; i++) 
				swiWaitForVBlank();
		} else {
			for (int i = 0; i < 25; i++)
				swiWaitForVBlank();
		}
		currentBg = 1;
		printLargeCentered(false, 32, "fatInitDefault failed!");

		// Control the DSi Menu, but can't launch anything.
		int pressed = 0;

		while (1) {
			// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing
			// else to do
			do {
				scanKeys();
				pressed = keysDownRepeat();
				snd().updateStream();
				swiWaitForVBlank();
			} while (!pressed);

			if ((pressed & KEY_LEFT) && !titleboxXmoveleft && !titleboxXmoveright) {
				CURPOS -= 1;
				if (CURPOS >= 0)
					titleboxXmoveleft = true;
			} else if ((pressed & KEY_RIGHT) && !titleboxXmoveleft && !titleboxXmoveright) {
				CURPOS += 1;
				if (CURPOS <= 39)
					titleboxXmoveright = true;
			}
			if (CURPOS < 0) {
				CURPOS = 0;
			} else if (CURPOS > 39) {
				CURPOS = 39;
			}
		}
	}

	if (ms().theme == 4) {
		whiteScreen = false;
		fadeColor = false;
	}

	langInit();

	std::string filename;

	if ((access("sd:/bios.bin", F_OK) == 0) || (access("sd:/gba/bios.bin", F_OK) == 0) || (access("sd:/_gba/bios.bin", F_OK) == 0)) {
		gbaBiosFound[0] = true;
	}
	if ((access("fat:/bios.bin", F_OK) == 0) || (access("fat:/gba/bios.bin", F_OK) == 0) || (access("fat:/_gba/bios.bin", F_OK) == 0)) {
		gbaBiosFound[1] = true;
	}

	if (isDSiMode() && sdFound() && ms().consoleModel < 2 && ms().launcherApp != -1) {
		u8 setRegion = 0;

		if (ms().sysRegion == -1) {
			// Determine SysNAND region by searching region of System Settings on SDNAND
			char tmdpath[256] = {0};
			for (u8 i = 0x41; i <= 0x5A; i++) {
				snprintf(tmdpath, sizeof(tmdpath), "sd:/title/00030015/484e42%x/content/title.tmd", i);
				if (access(tmdpath, F_OK) == 0) {
					setRegion = i;
					break;
				}
			}
		} else {
			switch (ms().sysRegion) {
			case 0:
			default:
				setRegion = 0x4A; // JAP
				break;
			case 1:
				setRegion = 0x45; // USA
				break;
			case 2:
				setRegion = 0x50; // EUR
				break;
			case 3:
				setRegion = 0x55; // AUS
				break;
			case 4:
				setRegion = 0x43; // CHN
				break;
			case 5:
				setRegion = 0x4B; // KOR
				break;
			}
		}

		snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath),
			 "nand:/title/00030017/484E41%x/content/0000000%i.app", setRegion, ms().launcherApp);
	}

	graphicsInit();
	iconManagerInit();

	keysSetRepeat(10, 2);

	vector<string> extensionList;
	if (ms().showNds) {
		extensionList.emplace_back(".nds");
		extensionList.emplace_back(".dsi");
		extensionList.emplace_back(".ids");
		extensionList.emplace_back(".argv");
	}
	if (memcmp(io_dldi_data->friendlyName, "DSTWO(Slot-1)", 0xD) == 0) {
		extensionList.emplace_back(".plg");
	}
	if (ms().showRvid) {
		extensionList.emplace_back(".rvid");
	}
	/*if (!ms().useGbarunner) {
		extensionList.emplace_back(".gba");
	}*/
	if (ms().showGb) {
		extensionList.emplace_back(".gb");
		extensionList.emplace_back(".sgb");
		extensionList.emplace_back(".gbc");
	}
	if (ms().showNes) {
		extensionList.emplace_back(".nes");
		extensionList.emplace_back(".fds");
	}
	if (ms().showSmsGg) {
		extensionList.emplace_back(".sms");
		extensionList.emplace_back(".gg");
	}
	if (ms().showMd) {
		extensionList.emplace_back(".gen");
	}
	if (ms().showSnes) {
		extensionList.emplace_back(".smc");
		extensionList.emplace_back(".sfc");
	}
	srand(time(NULL));

	char path[256] = {0};

	snd();

	if (ms().theme == 4) {
		snd().playStartup();
	} else if (ms().dsiMusic != 0) {
		if ((ms().theme == 1 && ms().dsiMusic == 1) || ms().dsiMusic == 2 || (ms().dsiMusic == 3 && tc().playStartupJingle())) {
			snd().playStartup();
			snd().setStreamDelay(snd().getStartupSoundLength() - tc().startupJingleDelayAdjust());
		}
		snd().beginStream();
	}

	if ((ms().consoleModel < 2 && ms().previousUsedDevice && bothSDandFlashcard() && ms().launchType == Launch::EDSiWareLaunch &&
	     access(ms().dsiWarePubPath.c_str(), F_OK) == 0 && extention(ms().dsiWarePubPath.c_str(), ".pub")) ||
	    (ms().consoleModel < 2 && ms().previousUsedDevice && bothSDandFlashcard() && ms().launchType == Launch::EDSiWareLaunch &&
	     access(ms().dsiWarePrvPath.c_str(), F_OK) == 0 && extention(ms().dsiWarePrvPath.c_str(), ".prv"))) {
		fadeType = true; // Fade in from white
		printSmallCentered(false, 20, "If this takes a while, close and open");
		printSmallCentered(false, 34, "the console's lid.");
		printLargeCentered(false, (ms().theme == 4 ? 80 : 88), "Now copying data...");
		printSmallCentered(false, (ms().theme == 4 ? 96 : 104), "Do not turn off the power.");
		for (int i = 0; i < 15; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		reloadFontPalettes();
		for (int i = 0; i < 20; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		showProgressIcon = true;
		controlTopBright = false;
		if (access(ms().dsiWarePubPath.c_str(), F_OK) == 0) {
			fcopy("sd:/_nds/TWiLightMenu/tempDSiWare.pub", ms().dsiWarePubPath.c_str());
		}
		if (access(ms().dsiWarePrvPath.c_str(), F_OK) == 0) {
			fcopy("sd:/_nds/TWiLightMenu/tempDSiWare.prv", ms().dsiWarePrvPath.c_str());
		}
		showProgressIcon = false;
		if (ms().theme != 4) {
			fadeType = false; // Fade to white
			for (int i = 0; i < 25; i++) {
				snd().updateStream();
				swiWaitForVBlank();
			}
		}
		clearText();
	}

	while (1) {

		snprintf(path, sizeof(path), "%s", ms().romfolder[ms().secondaryDevice].c_str());
		// Set directory
		chdir(path);

		// Navigates to the file to launch
		filename = browseForFile(extensionList);

		////////////////////////////////////
		// Launch the item

		if (applaunch) {
			// Delete previously used DSiWare of flashcard from SD
			if (!ms().gotosettings && ms().consoleModel < 2 && ms().previousUsedDevice &&
			    bothSDandFlashcard()) {
				if (access("sd:/_nds/TWiLightMenu/tempDSiWare.dsi", F_OK) == 0) {
					remove("sd:/_nds/TWiLightMenu/tempDSiWare.dsi");
				}
				if (access("sd:/_nds/TWiLightMenu/tempDSiWare.pub", F_OK) == 0) {
					remove("sd:/_nds/TWiLightMenu/tempDSiWare.pub");
				}
				if (access("sd:/_nds/TWiLightMenu/tempDSiWare.prv", F_OK) == 0) {
					remove("sd:/_nds/TWiLightMenu/tempDSiWare.prv");
				}
			}

			// Construct a command line
			getcwd(filePath, PATH_MAX);
			int pathLen = strlen(filePath);
			vector<char *> argarray;

			bool isArgv = false;
			if (strcasecmp(filename.c_str() + filename.size() - 5, ".argv") == 0) {
				ms().romPath = std::string(filePath) + std::string(filename);

				FILE *argfile = fopen(filename.c_str(), "rb");
				char str[PATH_MAX], *pstr;
				const char seps[] = "\n\r\t ";

				while (fgets(str, PATH_MAX, argfile)) {
					// Find comment and end string there
					if ((pstr = strchr(str, '#')))
						*pstr = '\0';

					// Tokenize arguments
					pstr = strtok(str, seps);

					while (pstr != NULL) {
						argarray.push_back(strdup(pstr));
						pstr = strtok(NULL, seps);
					}
				}
				fclose(argfile);
				filename = argarray.at(0);
				isArgv = true;
			} else {
				argarray.push_back(strdup(filename.c_str()));
			}

			bool dstwoPlg = false;
			bool rvid = false;
			bool SNES = false;
			bool GENESIS = false;
			bool gameboy = false;
			bool nes = false;
			bool gamegear = false;

			// Launch DSiWare .nds via Unlaunch
			if (isDSiMode() && isDSiWare[CURPOS]) {
				const char *typeToReplace = ".nds";
				if (extention(filename, ".dsi")) {
					typeToReplace = ".dsi";
				} else if (extention(filename, ".ids")) {
					typeToReplace = ".ids";
				}

				char *name = argarray.at(0);
				strcpy(filePath + pathLen, name);
				free(argarray.at(0));
				argarray.at(0) = filePath;

				ms().dsiWareSrlPath = std::string(argarray[0]);
				ms().dsiWarePubPath = ReplaceAll(argarray[0], typeToReplace, ".pub");
				ms().dsiWarePrvPath = ReplaceAll(argarray[0], typeToReplace, ".prv");
				if (!isArgv) {
					ms().romPath = std::string(argarray[0]);
				}
				ms().launchType = Launch::EDSiWareLaunch;
				ms().previousUsedDevice = ms().secondaryDevice;
				ms().saveSettings();

				sNDSHeaderExt NDSHeader;

				FILE *f_nds_file = fopen(filename.c_str(), "rb");

				fread(&NDSHeader, 1, sizeof(NDSHeader), f_nds_file);
				fclose(f_nds_file);

				fadeSpeed = true; // Fast fading

				if ((getFileSize(ms().dsiWarePubPath.c_str()) == 0) && (NDSHeader.pubSavSize > 0)) {
					clearText();
					printSmallCentered(false, 20, "If this takes a while, close and open");
					printSmallCentered(false, 34, "the console's lid.");
					printLargeCentered(false, (ms().theme == 4 ? 80 : 88), "Creating public save file...");
					if (ms().theme != 4 && !fadeType) {
						fadeType = true; // Fade in from white
						for (int i = 0; i < 35; i++) {
							swiWaitForVBlank();
						}
					}
					showProgressIcon = true;

					static const int BUFFER_SIZE = 4096;
					char buffer[BUFFER_SIZE];
					toncset(buffer, 0, sizeof(buffer));
					bool bufferCleared = false;
					char savHdrPath[64];
					snprintf(savHdrPath, sizeof(savHdrPath), "nitro:/DSiWareSaveHeaders/%x.savhdr",
						 (unsigned int)NDSHeader.pubSavSize);
					FILE *hdrFile = fopen(savHdrPath, "rb");
					if (hdrFile)
						fread(buffer, 1, 0x200, hdrFile);
					fclose(hdrFile);

					FILE *pFile = fopen(ms().dsiWarePubPath.c_str(), "wb");
					if (pFile) {
						for (int i = NDSHeader.pubSavSize; i > 0; i -= BUFFER_SIZE) {
							fwrite(buffer, 1, sizeof(buffer), pFile);
							if (!bufferCleared) {
								toncset(buffer, 0, sizeof(buffer));
								bufferCleared = true;
							}
						}
						fclose(pFile);
					}
					showProgressIcon = false;
					clearText();
					printLargeCentered(false, (ms().theme == 4 ? 32 : 88), "Public save file created!");
					for (int i = 0; i < 60; i++) {
						swiWaitForVBlank();
					}
				}

				if ((getFileSize(ms().dsiWarePrvPath.c_str()) == 0) && (NDSHeader.prvSavSize > 0)) {
					clearText();
					printSmallCentered(false, 20, "If this takes a while, close and open");
					printSmallCentered(false, 34, "the console's lid.");
					printLargeCentered(false, (ms().theme == 4 ? 80 : 88), "Creating private save file...");
					if (ms().theme != 4 && !fadeType) {
						fadeType = true; // Fade in from white
						for (int i = 0; i < 35; i++) {
							swiWaitForVBlank();
						}
					}
					showProgressIcon = true;

					static const int BUFFER_SIZE = 4096;
					char buffer[BUFFER_SIZE];
					toncset(buffer, 0, sizeof(buffer));
					bool bufferCleared = false;
					char savHdrPath[64];
					snprintf(savHdrPath, sizeof(savHdrPath), "nitro:/DSiWareSaveHeaders/%x.savhdr",
						 (unsigned int)NDSHeader.prvSavSize);
					FILE *hdrFile = fopen(savHdrPath, "rb");
					if (hdrFile)
						fread(buffer, 1, 0x200, hdrFile);
					fclose(hdrFile);

					FILE *pFile = fopen(ms().dsiWarePrvPath.c_str(), "wb");
					if (pFile) {
						for (int i = NDSHeader.prvSavSize; i > 0; i -= BUFFER_SIZE) {
							fwrite(buffer, 1, sizeof(buffer), pFile);
							if (!bufferCleared) {
								toncset(buffer, 0, sizeof(buffer));
								bufferCleared = true;
							}
						}
						fclose(pFile);
					}
					showProgressIcon = false;
					clearText();
					printLargeCentered(false, (ms().theme == 4 ? 32 : 88), "Private save file created!");
					for (int i = 0; i < 60; i++) {
						swiWaitForVBlank();
					}
				}

				if (ms().theme != 4 && fadeType) {
					fadeType = false; // Fade to white
					for (int i = 0; i < 25; i++) {
						swiWaitForVBlank();
					}
				}

				if (ms().secondaryDevice) {
					clearText();
					printSmallCentered(false, 20, "If this takes a while, close and open");
					printSmallCentered(false, 34, "the console's lid.");
					printLargeCentered(false, (ms().theme == 4 ? 80 : 88), "Now copying data...");
					printSmallCentered(false, (ms().theme == 4 ? 96 : 104), "Do not turn off the power.");
					if (ms().theme != 4) {
						fadeType = true; // Fade in from white
						for (int i = 0; i < 35; i++) {
							swiWaitForVBlank();
						}
					}
					showProgressIcon = true;
					fcopy(ms().dsiWareSrlPath.c_str(), "sd:/_nds/TWiLightMenu/tempDSiWare.dsi");
					if ((access(ms().dsiWarePubPath.c_str(), F_OK) == 0) && (NDSHeader.pubSavSize > 0)) {
						fcopy(ms().dsiWarePubPath.c_str(),
						      "sd:/_nds/TWiLightMenu/tempDSiWare.pub");
					}
					if ((access(ms().dsiWarePrvPath.c_str(), F_OK) == 0) && (NDSHeader.prvSavSize > 0)) {
						fcopy(ms().dsiWarePrvPath.c_str(),
						      "sd:/_nds/TWiLightMenu/tempDSiWare.prv");
					}
					showProgressIcon = false;
					if (ms().theme != 4) {
						fadeType = false; // Fade to white
						for (int i = 0; i < 25; i++) {
							swiWaitForVBlank();
						}
					}

					if ((access(ms().dsiWarePubPath.c_str(), F_OK) == 0 && (NDSHeader.pubSavSize > 0))
					 || (access(ms().dsiWarePrvPath.c_str(), F_OK) == 0 && (NDSHeader.prvSavSize > 0))) {
						int afterSaveTextXpos[3] = {0};
						if (ms().theme == 4) {
							afterSaveTextXpos[0] = 16;
							afterSaveTextXpos[1] = 32;
							afterSaveTextXpos[2] = 48;
						} else {
							afterSaveTextXpos[0] = 72;
							afterSaveTextXpos[1] = 88;
							afterSaveTextXpos[2] = 104;
						}
						clearText();
						printLargeCentered(false, afterSaveTextXpos[0], "After saving, please re-start");
						printLargeCentered(false, afterSaveTextXpos[1], "TWiLight Menu++ to transfer your");
						printLargeCentered(false, afterSaveTextXpos[2], "save data back.");
						if (ms().theme != 4) {
							fadeType = true; // Fade in from white
						}
						for (int i = 0; i < 60 * 3; i++) {
							swiWaitForVBlank(); // Wait 3 seconds
						}
						if (ms().theme != 4) {
							fadeType = false;	   // Fade to white
							for (int i = 0; i < 25; i++) {
								swiWaitForVBlank();
							}
						}
					}
				}

				char unlaunchDevicePath[256];
				if (ms().secondaryDevice) {
					snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath),
						 "sdmc:/_nds/TWiLightMenu/tempDSiWare.dsi");
				} else {
					snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath), "__%s",
						 ms().dsiWareSrlPath.c_str());
					unlaunchDevicePath[0] = 's';
					unlaunchDevicePath[1] = 'd';
					unlaunchDevicePath[2] = 'm';
					unlaunchDevicePath[3] = 'c';
				}

				tonccpy((u8 *)0x02000800, unlaunchAutoLoadID, 12);
				*(u16 *)(0x0200080C) = 0x3F0;   // Unlaunch Length for CRC16 (fixed, must be 3F0h)
				*(u16 *)(0x0200080E) = 0;       // Unlaunch CRC16 (empty)
				*(u32 *)(0x02000810) = 0;       // Unlaunch Flags
				*(u32 *)(0x02000810) |= BIT(0); // Load the title at 2000838h
				*(u32 *)(0x02000810) |= BIT(1); // Use colors 2000814h
				*(u16 *)(0x02000814) = 0x7FFF;  // Unlaunch Upper screen BG color (0..7FFFh)
				*(u16 *)(0x02000816) = 0x7FFF;  // Unlaunch Lower screen BG color (0..7FFFh)
				toncset((u8 *)0x02000818, 0, 0x20 + 0x208 + 0x1C0); // Unlaunch Reserved (zero)
				int i2 = 0;
				for (int i = 0; i < (int)sizeof(unlaunchDevicePath); i++) {
					*(u8 *)(0x02000838 + i2) =
					    unlaunchDevicePath[i]; // Unlaunch Device:/Path/Filename.ext (16bit
								   // Unicode,end by 0000h)
					i2 += 2;
				}
				while (*(u16 *)(0x0200080E) == 0) { // Keep running, so that CRC16 isn't 0
					*(u16 *)(0x0200080E) =
					    swiCRC16(0xFFFF, (void *)0x02000810, 0x3F0); // Unlaunch CRC16
				}
				// Stabilization code to make DSiWare always boot successfully(?)
				clearText();
				for (int i = 0; i < 15; i++) {
					swiWaitForVBlank();
				}

				fifoSendValue32(FIFO_USER_02, 1); // Reboot into DSiWare title, booted via Unlaunch
				for (int i = 0; i < 15; i++) {
					swiWaitForVBlank();
				}
			}

			// Launch .nds directly or via nds-bootstrap
			if (extention(filename, ".nds") || extention(filename, ".dsi") ||
			    extention(filename, ".ids")) {
				bool dsModeSwitch = false;
				bool dsModeDSiWare = false;

				if (memcmp(gameTid[CURPOS], "HND", 3) == 0 || memcmp(gameTid[CURPOS], "HNE", 3) == 0) {
					dsModeSwitch = true;
					dsModeDSiWare = true;
					useBackend = false; // Bypass nds-bootstrap
					ms().homebrewBootstrap = true;
				} else if (isHomebrew[CURPOS] == 2) {
					useBackend = false; // Bypass nds-bootstrap
					ms().homebrewBootstrap = true;
				} else if (isHomebrew[CURPOS] == 1) {
					loadPerGameSettings(filename);
					if (perGameSettings_directBoot || (ms().useBootstrap && ms().secondaryDevice)) {
						useBackend = false; // Bypass nds-bootstrap
					} else {
						useBackend = true;
					}
					if (isDSiMode() && !perGameSettings_dsiMode) {
						dsModeSwitch = true;
					}
					ms().homebrewBootstrap = true;
				} else {
					loadPerGameSettings(filename);
					useBackend = true;
					ms().homebrewBootstrap = false;
				}

				char *name = argarray.at(0);
				strcpy(filePath + pathLen, name);
				free(argarray.at(0));
				argarray.at(0) = filePath;
				if (useBackend) {
					if (ms().useBootstrap || !ms().secondaryDevice) {
						if (!sdFound() && ms().secondaryDevice && (access("fat:/BTSTRP.TMP", F_OK) != 0)) {
							// Create temporary file for nds-bootstrap
							printLarge(false, 4, 4, "Creating \"BTSTRP.TMP\"...");

							if (ms().theme != 4) {
								fadeSpeed = true; // Fast fading
								fadeType = true; // Fade in from white
								for (int i = 0; i < 25; i++) {
									swiWaitForVBlank();
								}
							}
							showProgressIcon = true;

							FILE *pFile = fopen("fat:/BTSTRP.TMP", "wb");
							if (pFile) {
								fseek(pFile, 0x40000 - 1, SEEK_SET);
								fputc('\0', pFile);
								fclose(pFile);
							}

							showProgressIcon = false;
							printLarge(false, 4, 20, "Done!");
							for (int i = 0; i < 30; i++) {
								swiWaitForVBlank();
							}
							if (ms().theme != 4) {
								fadeType = false;	   // Fade to white
								for (int i = 0; i < 25; i++) {
									swiWaitForVBlank();
								}
							}
							clearText();
						}

						std::string path = argarray[0];
						std::string savename = ReplaceAll(filename, ".nds", getSavExtension());
						std::string ramdiskname = ReplaceAll(filename, ".nds", getImgExtension(perGameSettings_ramDiskNo));
						std::string romFolderNoSlash = ms().romfolder[ms().secondaryDevice];
						RemoveTrailingSlashes(romFolderNoSlash);
						mkdir((isHomebrew[CURPOS] == 1) ? "ramdisks" : "saves", 0777);
						std::string savepath = romFolderNoSlash + "/saves/" + savename;
						if (sdFound() && ms().secondaryDevice && ms().fcSaveOnSd) {
							savepath = ReplaceAll(savepath, "fat:/", "sd:/");
						}
						std::string ramdiskpath = romFolderNoSlash + "/ramdisks/" + ramdiskname;

						if (getFileSize(savepath.c_str()) == 0 &&
						    isHomebrew[CURPOS] == 0)
						{ // Create save if game isn't homebrew
							printSmallCentered(false, 20, "If this takes a while, close and open");
							printSmallCentered(false, 34, "the console's lid.");
							printLargeCentered(false, (ms().theme == 4 ? 80 : 88), "Creating save file...");

							if (ms().theme != 4) {
								fadeSpeed = true; // Fast fading
								fadeType = true; // Fade in from white
								for (int i = 0; i < 35; i++) {
									swiWaitForVBlank();
								}
							}
							showProgressIcon = true;

							int savesize = 524288; // 512KB (default size for most games)

							// Set save size to 8KB for the following games
							if (memcmp(gameTid[CURPOS], "ASC", 3) == 0) // Sonic Rush
							{
								savesize = 8192;
							}

							// Set save size to 256KB for the following games
							if (memcmp(gameTid[CURPOS], "AMH", 3) == 0) // Metroid Prime Hunters
							{
								savesize = 262144;
							}

							// Set save size to 1MB for the following games
							if (memcmp(gameTid[CURPOS], "AZL", 3) ==
								0 // Wagamama Fashion: Girls Mode/Style Savvy/Nintendo
								  // presents: Style Boutique/Namanui Collection: Girls
								  // Style
							    || memcmp(gameTid[CURPOS], "C6P", 3) ==
								   0 // Picross 3D
							    || memcmp(gameTid[CURPOS], "BKI", 3) ==
								   0) // The Legend of Zelda: Spirit Tracks
							{
								savesize = 1048576;
							}

							// Set save size to 32MB for the following games
							if (memcmp(gameTid[CURPOS], "UOR", 3) == 0	// WarioWare - D.I.Y. (Do It Yourself)
							 || memcmp(gameTid[CURPOS], "UXB", 3) == 0)	// Jam with the Band
							{
								savesize = 1048576 * 32;
							}

							FILE *pFile = fopen(savepath.c_str(), "wb");
							if (pFile) {
								fseek(pFile, savesize - 1, SEEK_SET);
								fputc('\0', pFile);
								fclose(pFile);
							}
							showProgressIcon = false;
							clearText();
							printLargeCentered(false, (ms().theme == 4 ? 32 : 88), "Save file created!");
							for (int i = 0; i < 30; i++) {
								swiWaitForVBlank();
							}
							if (ms().theme != 4) {
								fadeType = false;	   // Fade to white
								for (int i = 0; i < 25; i++) {
									swiWaitForVBlank();
								}
							}
							clearText();
						}

						SetDonorSDK(argarray[0]);
						SetGameSoftReset(argarray[0]);
						SetMPUSettings(argarray[0]);
						SetSpeedBumpExclude(argarray[0]);

						bootstrapinipath = (sdFound() ? "sd:/_nds/nds-bootstrap.ini" : "fat:/_nds/nds-bootstrap.ini");
						CIniFile bootstrapini(bootstrapinipath);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", path);
						bootstrapini.SetString("NDS-BOOTSTRAP", "SAV_PATH", savepath);
						if (isHomebrew[CURPOS] == 0) {
							bootstrapini.SetString("NDS-BOOTSTRAP", "AP_FIX_PATH", setApFix(argarray[0]));
						}
						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", (perGameSettings_ramDiskNo >= 0 && !ms().secondaryDevice) ? ramdiskpath : "sd:/null.img");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", perGameSettings_language == -2 ? ms().bstrap_language : perGameSettings_language);
						if (isDSiMode()) {
							bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", perGameSettings_dsiMode == -1 ? ms().bstrap_dsiMode : perGameSettings_dsiMode);
							bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu);
							bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram);
						}
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DONOR_SDK_VER", donorSdkVer);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "GAME_SOFT_RESET", gameSoftReset);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_REGION", mpuregion);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_SIZE", mpusize);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "CARDENGINE_CACHED", ceCached);
						if (ms().forceSleepPatch || (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0 && !sys().isRegularDS())) {
							bootstrapini.SetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", 1);
						} else {
							bootstrapini.SetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", 0);
						}
						bootstrapini.SaveIniFile(bootstrapinipath);

						CheatCodelist codelist;
						u32 gameCode, crc32;

						if (isHomebrew[CURPOS] == 0) {
							bool cheatsEnabled = true;
							const char* cheatDataBin = "/_nds/nds-bootstrap/cheatData.bin";
							mkdir("/_nds", 0777);
							mkdir("/_nds/nds-bootstrap", 0777);
							if(codelist.romData(path,gameCode,crc32)) {
								long cheatOffset; size_t cheatSize;
								FILE* dat=fopen(sdFound() ? "sd:/_nds/TWiLightMenu/extras/usrcheat.dat" : "fat:/_nds/TWiLightMenu/extras/usrcheat.dat","rb");
								if (dat) {
									if (codelist.searchCheatData(dat, gameCode, crc32, cheatOffset, cheatSize)) {
										codelist.parse(path);
										writeCheatsToFile(codelist.getCheats(), cheatDataBin);
										FILE* cheatData=fopen(cheatDataBin,"rb");
										if (cheatData) {
											u32 check[2];
											fread(check, 1, 8, cheatData);
											fclose(cheatData);
											if (check[1] == 0xCF000000 || getFileSize(cheatDataBin) > 0x8000) {
												cheatsEnabled = false;
											}
										}
									} else {
										cheatsEnabled = false;
									}
									fclose(dat);
								} else {
									cheatsEnabled = false;
								}
							} else {
								cheatsEnabled = false;
							}
							if (!cheatsEnabled) {
								remove(cheatDataBin);
							}
						}

						if (!isArgv) {
							ms().romPath = std::string(argarray[0]);
						}
						ms().launchType = Launch::ESDFlashcardLaunch; // 1
						ms().previousUsedDevice = ms().secondaryDevice;
						ms().saveSettings();

						if (isDSiMode()) {
							SetWidescreen(filename.c_str());
						}

						bool useNightly = (perGameSettings_bootstrapFile == -1 ? ms().bootstrapFile : perGameSettings_bootstrapFile);

						char ndsToBoot[256];
						sprintf(ndsToBoot, "sd:/_nds/nds-bootstrap-%s%s.nds", ms().homebrewBootstrap ? "hb-" : "", useNightly ? "nightly" : "release");
						if(access(ndsToBoot, F_OK) != 0) {
							sprintf(ndsToBoot, "fat:/_nds/nds-bootstrap-%s%s.nds", ms().homebrewBootstrap ? "hb-" : "", useNightly ? "nightly" : "release");
						}

						argarray.at(0) = (char *)ndsToBoot;
						snd().stopStream();
						int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], (ms().homebrewBootstrap ? false : true), true, false, true, true);
						char text[32];
						snprintf(text, sizeof(text), "Start failed. Error %i", err);
						clearText();
						fadeType = true;
						printLarge(false, 4, 4, text);
						if (err == 1) {
							printLarge(false, 4, 20, useNightly ? "nds-bootstrap (Nightly) not found." : "nds-bootstrap (Release) not found.");
						}
						stop();
					} else {
						ms().romPath = std::string(argarray[0]);
						ms().launchType = Launch::ESDFlashcardLaunch;
						ms().previousUsedDevice = ms().secondaryDevice;
						ms().saveSettings();
						loadGameOnFlashcard(argarray[0], filename, true);
					}
				} else {
					if (!isArgv) {
						ms().romPath = std::string(argarray[0]);
					}
					ms().launchType = Launch::ESDFlashcardDirectLaunch;
					ms().previousUsedDevice = ms().secondaryDevice;
					ms().saveSettings();
					bool runNds_boostCpu = false;
					bool runNds_boostVram = false;
					if (isDSiMode() && !dsModeDSiWare) {
						loadPerGameSettings(filename);

						runNds_boostCpu = perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu;
						runNds_boostVram = perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram;
					}
					snd().stopStream();
					int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, dsModeSwitch, runNds_boostCpu, runNds_boostVram);
					char text[32];
					snprintf(text, sizeof(text), "Start failed. Error %i", err);
					fadeType = true;
					printLarge(false, 4, 4, text);
					stop();
				}
			} else if (extention(filename, ".plg")) {
				dstwoPlg = true;
			} else if (extention(filename, ".rvid")) {
				rvid = true;
			} /*else if (extention(filename, ".gba")) {
				//ms().launchType = Launch::ESDFlashcardLaunch;
				//ms().previousUsedDevice = ms().secondaryDevice;
				ms().saveSettings();

				sysSetCartOwner(BUS_OWNER_ARM9);	// Allow arm9 to access GBA ROM in memory map

				// Load GBA ROM into EZ Flash 3-in-1
				FILE *gbaFile = fopen(filename.c_str(), "rb");
				fread((void*)0x08000000, 1, 0x1000000, gbaFile);
				fclose(gbaFile);
				gbaSwitch();
			}*/ else if (extention(filename, ".gb") || extention(filename, ".sgb") ||
				   extention(filename, ".gbc")) {
				gameboy = true;
			} else if (extention(filename, ".nes") || extention(filename, ".fds")) {
				nes = true;
			} else if (extention(filename, ".sms") || extention(filename, ".gg")) {
				mkdir(ms().secondaryDevice ? "fat:/data" : "sd:/data", 0777);
				mkdir(ms().secondaryDevice ? "fat:/data/s8ds" : "sd:/data/s8ds", 0777);

				gamegear = true;
			} else if (extention(filename, ".gen")) {
				GENESIS = true;
			} else if (extention(filename, ".smc") || extention(filename, ".sfc")) {
				SNES = true;
			}

			if (dstwoPlg || rvid || gameboy || nes || gamegear) {
				const char *ndsToBoot;
				std::string romfolderNoSlash = ms().romfolder[ms().secondaryDevice];
				RemoveTrailingSlashes(romfolderNoSlash);
				char ROMpath[256];
				snprintf(ROMpath, sizeof(ROMpath), "%s/%s", romfolderNoSlash.c_str(), filename.c_str());
				ms().romPath = std::string(ROMpath);
				ms().homebrewArg = std::string(ROMpath);

				if (gameboy) {
					ms().launchType = Launch::EGameYobLaunch;
				} else if (nes) {
					ms().launchType = Launch::ENESDSLaunch;
				} else {
					ms().launchType = Launch::ES8DSLaunch;
				}

				ms().previousUsedDevice = ms().secondaryDevice;
				ms().saveSettings();
				argarray.push_back(ROMpath);
				int err = 0;

				if (dstwoPlg) {
					ndsToBoot = "/_nds/TWiLightMenu/bootplg.srldr";

					// Print .plg path without "fat:" at the beginning
					char ROMpathDS2[256];
					for (int i = 0; i < 252; i++) {
						ROMpathDS2[i] = ROMpath[4+i];
						if (ROMpath[4+i] == '\x00') break;
					}

					CIniFile dstwobootini( "fat:/_dstwo/twlm.ini" );
					dstwobootini.SetString("boot_settings", "file", ROMpathDS2);
					dstwobootini.SaveIniFile( "fat:/_dstwo/twlm.ini" );
				} else if (rvid) {
					ndsToBoot = "sd:/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
					if(access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
					}
				} else if (gameboy) {
					ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/gameyob.nds";
					if(access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "/_nds/TWiLightMenu/emulators/gameyob.nds";
					}
				} else if (nes) {
					ndsToBoot = (ms().secondaryDevice ? "sd:/_nds/TWiLightMenu/emulators/nesds.nds" : "sd:/_nds/TWiLightMenu/emulators/nestwl.nds");
					if(access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "/_nds/TWiLightMenu/emulators/nesds.nds";
					}
				} else {
					ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/S8DS.nds";
					if(access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "/_nds/TWiLightMenu/emulators/S8DS.nds";
					}
				}
				argarray.at(0) = (char *)ndsToBoot;
				snd().stopStream();
				err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to emulator as argument

				char text[32];
				snprintf(text, sizeof(text), "Start failed. Error %i", err);
				fadeType = true;
				printLarge(false, 4, 4, text);
				stop();
			} else if (SNES || GENESIS) {
				const char *ndsToBoot;
				std::string romfolderNoSlash = ms().romfolder[ms().secondaryDevice];
				RemoveTrailingSlashes(romfolderNoSlash);
				char ROMpath[256];
				snprintf(ROMpath, sizeof(ROMpath), "%s/%s", romfolderNoSlash.c_str(), filename.c_str());
				ms().homebrewBootstrap = true;
				ms().romPath = std::string(ROMpath);
				ms().launchType = Launch::ESDFlashcardLaunch; // 1
				ms().previousUsedDevice = ms().secondaryDevice;
				ms().saveSettings();
				// SaveSettings();
				if (ms().secondaryDevice) {
					if (SNES) {
						ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.nds";
						if(access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = "/_nds/TWiLightMenu/emulators/SNEmulDS.nds";
						}
					} else {
						ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds";
						if(access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = "/_nds/TWiLightMenu/emulators/jEnesisDS.nds";
						}
					}
				} else {
					ndsToBoot =
					    (ms().bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
					CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

					bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", ms().bstrap_language);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
					if (SNES) {
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "fat:/snes/ROM.SMC");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);
					} else {
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "fat:/ROM.BIN");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
					}
					bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

					bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", ROMpath);
					bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
				}
				argarray.at(0) = (char *)ndsToBoot;
				snd().stopStream();
				int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], ms().secondaryDevice, true, ms().secondaryDevice, true, true);
				char text[32];
				snprintf(text, sizeof(text), "Start failed. Error %i", err);
				fadeType = true;
				printLarge(false, 4, 4, text);
				if (err == 1) {
					printLarge(false, 4, 20, ms().bootstrapFile ? "nds-bootstrap (Nightly) not found." : "nds-bootstrap (Release) not found.");
				}
				stop();
			}

			while (argarray.size() != 0) {
				free(argarray.at(0));
				argarray.erase(argarray.begin());
			}
		}
	}

	return 0;
}
