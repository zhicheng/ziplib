#ifndef _ZIP_H_
#define _ZIP_H_

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <zlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

enum bool { false = 0, true };
typedef enum bool bool;

#pragma pack(1) // set data align

struct FileHeaderData
{
    unsigned int   signature;
    unsigned short extract_needed;
    unsigned short bit_flag;
    unsigned short compression_method;
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned int   crc32;
    unsigned int   compressed_size;
    unsigned int   uncompressed_size;
    unsigned short filename_length;
    unsigned short extra_length;
};

struct FileDescData
{
    unsigned int crc32;
    unsigned int compressed_size;
    unsigned int uncompressed_size;
};

struct CentralDirData 
{
    unsigned int   signature;
    unsigned short made_by;
    unsigned short extract_needed;
    unsigned short bit_flag;
    unsigned short compression_method;
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned int   crc32;
    unsigned int   compressed_size;
    unsigned int   uncompressed_size;
    unsigned short filename_length;
    unsigned short extra_length;
    unsigned short comment_length;
    unsigned short disk_num_start;
    unsigned short internal_attr;
    unsigned int   external_attr;
    unsigned int   file_header_offset;
};

struct ArchiveEndData
{
    unsigned int   signature;
    unsigned short disk_number;
    unsigned short disk_number_start;
    unsigned short dir_number_on_disk;
    unsigned short dir_number;
    unsigned int   dir_length;
    unsigned int   dir_offset;
    unsigned short file_comment;
};

#pragma pack()

typedef struct FileHeaderData FileHeader;
typedef struct FileDescData   FileDesc;
typedef struct CentralDirData CentralDir;
typedef struct ArchiveEndData ArchiveEnd;

extern FileHeader *
get_file_header(FILE *fp, size_t offset); // c_size is compressed_size

extern char *
get_file_content(FILE *fp, size_t offset, size_t c_size); // c_size is compressed_size

extern ArchiveEnd *
get_archive_end(FILE *fp);

extern CentralDir *
get_central_dir(FILE *fp, size_t offset);

extern size_t      
get_central_dir_size(FILE *fp, size_t offset);

extern int         
verify_zip(FILE *fp);

bool 
uncompress_buffer(char *source, size_t source_len, char *dest, size_t dest_len);

bool
verify_crc32(unsigned int source_crc32, char *source, size_t len);

#endif /* _ZIP_H_ */
