#include <nds.h>
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include "common/gl2d.h"

#include "ndsbanner.h"
#include "ndsheader.h"
#include "module_params.h"

extern sNDSBannerExt ndsBanner;

typedef enum
{
	GL_FLIP_BOTH = (1 << 3)
} GL_FLIP_MODE_XTRA;

// bnriconframeseq[]
static u16 bnriconframeseq[64] = {0x0000};

// bnriconframenum[]
int bnriconPalLine = 0;
int bnriconframenumY = 0;
int bannerFlip = GL_FLIP_NONE;

// bnriconisDSi[]
bool isDirectory = false;
int bnrRomType = 0;
bool bnriconisDSi = false;
int bnrWirelessIcon = 0; // 0 = None, 1 = Local, 2 = WiFi
bool isDSiWare = false;
int isHomebrew = 0; // 0 = No, 1 = Yes with no DSi-Extended header, 2 = Yes with DSi-Extended header

/**
 * Get banner sequence from banner file.
 * @param binFile Banner file.
 */
void grabBannerSequence()
{
	for (int i = 0; i < 64; i++)
	{
		bnriconframeseq[i] = ndsBanner.dsi_seq[i];
	}
}

/**
 * Clear loaded banner sequence.
 */
void clearBannerSequence()
{
	for (int i = 0; i < 64; i++)
	{
		bnriconframeseq[i] = 0x0000;
	}
}

static u16 bannerDelayNum = 0x0000;
int currentbnriconframeseq = 0;

/**
 * Play banner sequence.
 * @param binFile Banner file.
 */
void playBannerSequence()
{
	if (bnriconframeseq[currentbnriconframeseq] == 0x0001 && bnriconframeseq[currentbnriconframeseq + 1] == 0x0100)
	{
		// Do nothing if icon isn't animated
		bnriconPalLine = 0;
		bnriconframenumY = 0;
		bannerFlip = GL_FLIP_NONE;
	}
	else
	{
		u16 setframeseq = bnriconframeseq[currentbnriconframeseq];
		bnriconPalLine = SEQ_PAL(setframeseq);
		bnriconframenumY =  SEQ_BMP(setframeseq);
		bool flipH = SEQ_FLIPH(setframeseq);
		bool flipV = SEQ_FLIPV(setframeseq);

		if (flipH && flipV)
		{
			bannerFlip = GL_FLIP_BOTH;
		}
		else if (!flipH && !flipV)
		{
			bannerFlip = GL_FLIP_NONE;
		}
		else if (flipH && !flipV)
		{
			bannerFlip = GL_FLIP_H;
		}
		else if (!flipH && flipV)
		{
			bannerFlip = GL_FLIP_V;
		}

		bannerDelayNum++;
		if (bannerDelayNum >= (setframeseq & 0x00FF))
		{
			bannerDelayNum = 0x0000;
			currentbnriconframeseq++;
			if (bnriconframeseq[currentbnriconframeseq] == 0x0000)
			{
				currentbnriconframeseq = 0; // Reset sequence
			}
		}
	}
}
