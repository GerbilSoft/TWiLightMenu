#include <nds.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include "common/gl2d.h"

#include "ndsbanner.h"
#include "ndsheader.h"
#include "module_params.h"

extern sNDSBannerExt ndsBanner;

char gameTid[40][5] = {0};
u16 headerCRC[40] = {0};

typedef enum
{
	GL_FLIP_BOTH = (1 << 3)
} GL_FLIP_MODE_XTRA;

char bnriconTile[41][0x23C0];

// bnriconframeseq[]
static u16 bnriconframeseq[41][64] = {0x0000};

// bnriconframenum[]
int bnriconPalLine[41] = {0};
int bnriconframenumY[41] = {0};
int bannerFlip[41] = {GL_FLIP_NONE};

// bnriconisDSi[]
bool isDirectory[40] = {false};
bool bnrSysSettings[41] = {false};
int bnrRomType[41] = {0};
bool bnriconisDSi[41] = {false};
int bnrWirelessIcon[41] = {0}; // 0 = None, 1 = Local, 2 = WiFi
bool isDSiWare[41] = {true};
int isHomebrew[41] = {0}; // 0 = No, 1 = Yes with no DSi-Extended header, 2 = Yes with DSi-Extended header

static u16 bannerDelayNum[41] = {0x0000};
int currentbnriconframeseq[41] = {0};

/**
 * Get banner sequence from banner file.
 * @param binFile Banner file.
 */
void grabBannerSequence(int iconnum)
{
	for (int i = 0; i < 64; i++)
	{
		bnriconframeseq[iconnum][i] = ndsBanner.dsi_seq[i];
	}
	currentbnriconframeseq[iconnum] = 0;
}

/**
 * Clear loaded banner sequence.
 */
void clearBannerSequence(int iconnum)
{
	for (int i = 0; i < 64; i++)
	{
		bnriconframeseq[iconnum][i] = 0x0000;
	}
	currentbnriconframeseq[iconnum] = 0;
}

/**
 * Play banner sequence.
 * @param binFile Banner file.
 */
void playBannerSequence(int iconnum)
{
	if (bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]] == 0x0001 
	&& bnriconframeseq[iconnum][currentbnriconframeseq[iconnum] + 1] == 0x0100)
	{
		// Do nothing if icon isn't animated
		bnriconPalLine[iconnum] = 0;
		bnriconframenumY[iconnum] = 0;
		bannerFlip[iconnum] = GL_FLIP_NONE;
	}
	else
	{
		u16 setframeseq = bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]];
		bnriconPalLine[iconnum] = SEQ_PAL(setframeseq);
		bnriconframenumY[iconnum] = SEQ_BMP(setframeseq);
		bool flipH = SEQ_FLIPH(setframeseq);
		bool flipV = SEQ_FLIPV(setframeseq);

		if (flipH && flipV)
		{
			bannerFlip[iconnum] = GL_FLIP_BOTH;
		}
		else if (!flipH && !flipV)
		{
			bannerFlip[iconnum] = GL_FLIP_NONE;
		}
		else if (flipH && !flipV)
		{
			bannerFlip[iconnum] = GL_FLIP_H;
		}
		else if (!flipH && flipV)
		{
			bannerFlip[iconnum] = GL_FLIP_V;
		}

		bannerDelayNum[iconnum]++;
		if (bannerDelayNum[iconnum] >= (setframeseq & 0x00FF))
		{
			bannerDelayNum[iconnum] = 0x0000;
			currentbnriconframeseq[iconnum]++;
			if (bnriconframeseq[iconnum][currentbnriconframeseq[iconnum]] == 0x0000)
			{
				currentbnriconframeseq[iconnum] = 0; // Reset sequence
			}
		}
	}
}
