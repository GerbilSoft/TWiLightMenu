#include <nds.h>
#include <stdio.h>
#include "ndsheader.h"

#pragma once
#ifndef __MODULE_PARAM__
#define __MODULE_PARAM__

typedef struct
{
	u32 auto_load_list_offset;
	u32 auto_load_list_end;
	u32 auto_load_start;
	u32 static_bss_start;
	u32 static_bss_end;
	u32 compressed_static_end;
	u32 sdk_version;
	u32 nitro_code_be;
	u32 nitro_code_le;
} module_params_t;

/**
 * Gets a pointer to module params of a file, valid until
 * the next call of getModuleParams.
 *
 * Try to ensure that the NDS ROM actually has moduleparams,
 * (i.e. not homebrew), else this method will be slow.
 */
module_params_t* getModuleParams(const sNDSHeaderExt* ndsHeader, FILE* ndsFile);
#endif
