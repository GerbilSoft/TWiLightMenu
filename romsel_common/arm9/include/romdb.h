/*! \file romdb.h
\brief Defines ROM database lookup functions.
*/

#ifndef __ROMSEL_COMMON_ROMDB_H__
#define __ROMSEL_COMMON_ROMDB_H__

#include <nds.h>

/**
 * Get the title ID.
 * @param ndsFile DS ROM image.
 * @param buf Output buffer for title ID. (Must be at least 4 characters.)
 * @return 0 on success; non-zero on error.
 */
int grabTID(FILE* ndsFile, char *buf);

/**
 * Get SDK version from an NDS file.
 * @param ndsFile NDS file.
 * @param filename NDS ROM filename.
 * @param allowDSi If true, allow DSi-enhanced ROMs; if not, return 0 for DSi-enhanced ROMs.
 * @return 0 on success; non-zero on error.
 */
u32 getSDKVersion(FILE *ndsFile, bool allowDSi);

/**
 * Check if an NDS game has AP.
 * @param game_TID Title ID. (ID4)
 * @param headerCRC16 Header CRC.
 * @return true on success; false if no AP.
 */
bool checkRomAP(const char *game_TID, u16 headerCRC16);

/**
 * Check if an NDS game has AP.
 * @param ndsFile NDS file.
 * @param filename NDS ROM filename.
 * @return true on success; false if no AP.
 */
bool checkRomAP(FILE *ndsFile);

#endif /* __ROMSEL_COMMON_ROMDB_H__ */
