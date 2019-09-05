#ifndef __QUICKMENU_NDS_BANNER_H__
#define __QUICKMENU_NDS_BANNER_H__

#include <nds.h>

// bnriconframenum[]
extern int bnriconPalLine;
extern int bnriconframenumY;
extern int bannerFlip;

// bnriconisDSi[]
extern bool isDirectory;
extern int bnrRomType;
extern bool bnriconisDSi;
extern int bnrWirelessIcon;	// 0 = None, 1 = Local, 2 = WiFi
extern bool isDSiWare;
extern int isHomebrew;		// 0 = No, 1 = Yes with no DSi-Extended header, 2 = Yes with DSi-Extended header

/**
 * Get banner sequence from banner file.
 * @param binFile Banner file.
 */
void grabBannerSequence();

/**
 * Clear loaded banner sequence.
 */
void clearBannerSequence();

/**
 * Play banner sequence.
 * @param binFile Banner file.
 */
void playBannerSequence();

#endif /* __QUICKMENU_NDS_BANNER_H__ */
