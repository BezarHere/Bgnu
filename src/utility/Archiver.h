#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


#define ARCHIVER_SIGN_LEN 8

#define ARCHIVER_FILE_NAME_LEN 16

#define ARCHIVER_MOD_TIMESTAMP_LEN 12
#define ARCHIVER_OWNER_ID_LEN 6
#define ARCHIVER_GROUP_ID_LEN 6
#define ARCHIVER_FILE_MODE_LEN 8
#define ARCHIVER_FILE_SIZE_LEN 10

#define ARCHIVER_END_CHR_LEN 2

#define ARCHIVER_FILE_HEADER_LEN 60

typedef struct archive_file_s
{
  uint64_t offset;

  char file_name[ARCHIVER_FILE_NAME_LEN + 1];

  uint64_t modification_time_stamp;

  uint32_t owner_id;
  uint32_t group_id;

  uint32_t file_mode_flags;
  uint64_t file_size;

  char _ending_chars[ARCHIVER_END_CHR_LEN];

  uint64_t data_offset;
  char *data;
} archive_file_t;

typedef struct archive_container_s
{
  char signature[ARCHIVER_SIGN_LEN];

  bool x64_symbol_table;
  const char *const *symbol_table;
  const char *const *file_table;

  unsigned files_count;
  archive_file_t *files;
} archive_t;

#define ARCHIVER_IO_BUF_DEF(type, name) struct archiver_io_## name { \
  size_t length; type data; size_t pos; }

typedef ARCHIVER_IO_BUF_DEF(const void *, read_buf_s) archiver_io_read_buf_t;
typedef ARCHIVER_IO_BUF_DEF(void *, write_buf_s) archiver_io_write_buf_t;

// a read-write io buffer
typedef archiver_io_write_buf_t archiver_io_rw_buf_t;

#undef ARCHIVER_IO_BUF_DEF

typedef union archiver_io_data_u
{
  FILE *file;
  void *custom;

  union archiver_io_data_buffers_u
  {
    archiver_io_read_buf_t read_buf;
    archiver_io_write_buf_t write_buf;
  } buffer;

} archiver_io_data_t;

typedef int (*archiver_read_func_t)(void *buffer, size_t bytes,
                                       size_t *out_count, archiver_io_data_t data);

typedef int (*archiver_write_func_t)(const void *buffer, size_t bytes,
                                        size_t *out_count, archiver_io_data_t data);

typedef int (*archiver_seek_func_t)(int state, size_t pos, archiver_io_data_t data);
typedef size_t(*archiver_tell_func_t)(archiver_io_data_t data);

typedef struct archiver_io_s
{
  archiver_read_func_t read_proc;
  archiver_write_func_t write_proc;
  archiver_seek_func_t seek_proc;
  archiver_tell_func_t tell_proc;

  archiver_io_data_t data;

} archiver_io_t;

#ifdef __cplusplus
extern "C" {
#endif

  extern archiver_io_t Archiver_FileOpenPath(const char *path, bool read);
  extern archiver_io_t Archiver_FileOpen(FILE *file);
  extern int Archiver_FileClose(const archiver_io_t *io_state);

  extern archiver_io_t ArchiverIO_BufferOpenW(archiver_io_write_buf_t buffer);
  extern archiver_io_t ArchiverIO_BufferOpenR(archiver_io_read_buf_t buffer);
  extern int ArchiverIO_BufferClose(const archiver_io_t *io_state);

  extern int Archiver_ReadIO(const archiver_io_t *io_state, archive_t *container);


  extern int Archiver_Close(archive_t *archive);

#ifdef __cplusplus
}
#endif
