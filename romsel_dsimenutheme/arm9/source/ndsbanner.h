/*! \file ndsbanner.h
\brief Defines the Nintendo DS file header and icon/title structs.
*/

#ifndef __ROMSEL_DSITHEME_NDS_HEADER_H__
#define __ROMSEL_DSITHEME_NDS_HEADER_H__

#include <nds.h>

extern char gameTid[40][5];
extern u16 headerCRC[40];

extern char bnriconTile[41][0x23C0];

// bnriconframenum[]
extern int bnriconPalLine[41];
extern int bnriconframenumY[41];
extern int bannerFlip[41];

// bnriconisDSi[]
extern bool isDirectory[40];
extern bool bnrSysSettings[41];
extern int bnrRomType[41];
extern bool bnriconisDSi[41];
extern int bnrWirelessIcon[41];	// 0 = None, 1 = Local, 2 = WiFi
extern bool isDSiWare[41];
extern int isHomebrew[41];		// 0 = No, 1 = Yes with no DSi-Extended header, 2 = Yes with DSi-Extended header

/**
 * Get banner sequence from banner file.
 * @param binFile Banner file.
 */
void grabBannerSequence(int iconnum);

/**
 * Clear loaded banner sequence.
 */
void clearBannerSequence(int iconnum);

/**
 * Play banner sequence.
 * @param binFile Banner file.
 */
void playBannerSequence(int iconnum);

#endif /* __ROMSEL_DSITHEME_NDS_HEADER_H__ */
