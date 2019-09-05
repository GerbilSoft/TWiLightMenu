#ifndef __ROMSEL_COMMON_FILE_COPY_H__
#define __ROMSEL_COMMON_FILE_COPY_H__

#ifdef __cplusplus
extern "C" {
#endif

off_t getFileSize(const char *fileName);
int fcopy(const char *sourcePath, const char *destinationPath);

#ifdef __cplusplus
}
#endif

#endif /* __ROMSEL_COMMON_FILE_COPY_H__ */
