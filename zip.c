/* zip.c */

#include <stdio.h>
#include "zip.h"

#define max(x, y)   (x) > (y) ? (x) : (y)

FileHeader *
get_file_header(FILE *fp, size_t offset)
{
    FileHeader *fileHeader = (FileHeader *)malloc(sizeof(FileHeader));
    fseek(fp, offset, SEEK_SET);
    fread(fileHeader, sizeof(FileHeader), 1, fp);
    if (fileHeader->signature != 0x04034b50) 
        return NULL;
    return fileHeader;
}

char *
get_file_content(FILE *fp, size_t offset, size_t c_size)
{
    FileHeader *fileHeader = (FileHeader *)malloc(sizeof(FileHeader));
    fileHeader = get_file_header(fp, offset);
    char *content = (char *)malloc(c_size);
    fseek(fp, offset + sizeof(FileHeader) + fileHeader->filename_length + fileHeader->extra_length, SEEK_SET);
    fread(content, c_size, 1, fp);
    return content;
}

ArchiveEnd *
get_archive_end(FILE *fp)
{
    int file_size = 0;
    int max_comment_start;
    int max_comment_length;
    int comment_length;
    int start;
    char *data;
    ArchiveEnd *archiveEnd = (ArchiveEnd *)malloc(sizeof(ArchiveEnd));

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);

    fseek(fp, -sizeof(ArchiveEnd), SEEK_END);
    fread(archiveEnd, sizeof(ArchiveEnd), 1, fp);

    if (   archiveEnd->signature    == 0x06054b50 
        && archiveEnd->file_comment == 0) {
        return archiveEnd;
    } else {
        max_comment_start  = file_size - (1 << 16) - sizeof(ArchiveEnd);
        max_comment_start  = max(max_comment_start, 0);
        max_comment_length = file_size - max_comment_start;
        fseek(fp, max_comment_start, SEEK_SET);
        data = (char *)malloc(max_comment_length);
        comment_length = fread(data, max_comment_length, 1, fp);
        for(data = data + comment_length; *(int *)data != 0x06054b50 && start > comment_length; data--) start++;
        if (start >= 0) {
            memcpy(archiveEnd, data, sizeof(ArchiveEnd));
        }
        return archiveEnd;
        free(data);
    }
    return NULL;
}

CentralDir *
get_central_dir(FILE *fp, size_t offset)
{
    CentralDir *centralDir = (CentralDir *)malloc(sizeof(CentralDir));
    fseek(fp, offset, SEEK_SET);
    fread(centralDir, sizeof(CentralDir), 1, fp);
    if (centralDir->signature != 0x02014b50)
        return NULL;
    return centralDir;
}

size_t
get_central_dir_size(FILE *fp, size_t offset)
{
    CentralDir centralDir;
    fseek(fp, offset, SEEK_SET);
    fread(&centralDir, sizeof(CentralDir), 1, fp);
    if (centralDir.signature != 0x02014b50)
        return 0;
    return sizeof(CentralDir) 
         + centralDir.filename_length 
         + centralDir.extra_length 
         + centralDir.comment_length;
}

int
verify_zip(FILE *fp)
{

}

bool
uncompress_buffer(char *source, size_t source_len, char *dest, size_t dest_len)
{
    char *zlib_content = (char *)malloc(source_len + 2);
    zlib_content[0]    = 0x78;             /* zlib head */
    zlib_content[1]    = 0x9c;             /* zlib head */

    memcpy(zlib_content + 2, source, source_len);

    int n = uncompress(dest, &dest_len, zlib_content, source_len + 2);
    if (n == Z_MEM_ERROR) {
        return false;
    } else if (n == Z_BUF_ERROR) {
        return false;
    } else if (n == Z_DATA_ERROR) {
        /* nothing */
        /* verify crc32 in next step */
    }
    free(zlib_content);
}

bool
verify_crc32(unsigned int source_crc32, char *source, size_t len)
{
    unsigned int crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, source, len);
    if (crc != source_crc32) {
        return false;
    }
    return true;
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("%s filename\n", argv[0]);
        return 0;
    }
    FILE *fp = fopen(argv[1], "rb");

    unsigned short centraldir_offset = 0;
    char *c_buffer;
    char *unc_buffer;
    char *filename;
    int fd;
    int i;

    CentralDir *centralDir;
    ArchiveEnd *archiveEnd;
    archiveEnd = get_archive_end(fp);

    for (i = 0; i < archiveEnd->dir_number; i++) {
        if ((centralDir = get_central_dir(fp, archiveEnd->dir_offset + centraldir_offset);) == NULL)
           break; 
        centraldir_offset += get_central_dir_size(fp, archiveEnd->dir_offset + centraldir_offset);
        if (centralDir->compression_method != 8 
         && centralDir->compression_method != 0) {
            printf("only support deflate compress or store\n");
            return 1;
        }

        FileHeader *fileHeader;
        if ((fileHeader = get_file_header(fp, centralDir->file_header_offset)) == NULL)
            continue;

        filename = (char *)malloc(fileHeader->filename_length + 1);
        fseek(fp, centralDir->file_header_offset + sizeof(FileHeader), SEEK_SET);
        fread(filename, fileHeader->filename_length, 1, fp);
        filename[fileHeader->filename_length] = '\0';
        printf("filename: %s\n", filename);

        c_buffer = get_file_content(fp, centralDir->file_header_offset, centralDir->compressed_size);
        unc_buffer = NULL;

        if (centralDir->compression_method == 8) {
            unc_buffer = (char *)malloc(centralDir->uncompressed_size + 10);
            if (!uncompress_buffer(c_buffer, centralDir->compressed_size, unc_buffer, centralDir->uncompressed_size + 10)) {
                printf("uncompress_buffer error\n");
                return 1;
            }
        } else if (centralDir->compression_method == 0) {
            unc_buffer = c_buffer;
        } 

        if (centralDir->compressed_size != 0) {
            if (!verify_crc32(centralDir->crc32, unc_buffer, centralDir->uncompressed_size)) {
                printf("crc verity fail\n");
                return 1;
            }
        }
        
        if (filename[fileHeader->filename_length - 1] == '/') {
            mkdir(filename, 0744);
        } else {
            fd = open(filename, O_WRONLY | O_CREAT);
            write(fd, unc_buffer, centralDir->uncompressed_size);
            close(fd);
            chmod(filename, 0744);
        }
        free(filename);
        free(unc_buffer);
        free(fileHeader);
        free(centralDir);
    }
    fclose(fp);
    return 0;
}

