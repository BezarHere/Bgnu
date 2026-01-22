#include "Archiver.h"

#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#define ERROR_MSG_SZ 512
#define NAMED_PERM(value) #value, value
#define NAMED_PERM_F(format) "%s=%" #format
#define BOOL_ALPHA(value) ((value) ? "true" : "false")

#define ASSERT_ERR(cond, ...) if (!(cond)) { g_DoErrorLogging = true; ErrPrintF(__VA_ARGS__); abort(); }
#define PARSER_ASSERT(cond, ...) if (!(cond)) { g_DoErrorLogging = true; ParserErrorF(parser,__VA_ARGS__); abort(); }

typedef archiver_io_t io_state_t;
typedef struct parser_s parser_t, *ref_parser_t;

typedef enum
{
  eReading,
  eWriting,
  eReadingAdvanced,
  eWritingAdvanced,
} io_op_type_t;

typedef char error_msg_t[ERROR_MSG_SZ];
static error_msg_t g_ErrorMSG = {0};
static bool g_DoErrorLogging = true;

#pragma region(IO Checks)

static inline bool IO_OPHasWriting(io_op_type_t type);
static inline bool IO_OPHasReading(io_op_type_t type);
static inline bool IO_OPIsAdvanced(io_op_type_t type);

static bool IsIOStateFullyFunctional(const io_state_t *state, io_op_type_t io_op);
static bool IsIOStateAdvancedCapable(const io_state_t *state);
static bool IsIOFile(const io_state_t *state);

#pragma endregion

#pragma region(IO Procs)

static int IO_FileRead(void *buffer, size_t bytes, size_t *read_count, archiver_io_data_t data);
static int IO_FileWrite(const void *buffer, size_t bytes, size_t *write_count, archiver_io_data_t data);
static int IO_FileSeek(int state, size_t pos, archiver_io_data_t data);
static size_t IO_FileTell(archiver_io_data_t data);

static int IO_BufRead(void *buffer, size_t bytes, size_t *read_count, archiver_io_data_t data);
static int IO_BufWrite(const void *buffer, size_t bytes, size_t *write_count, archiver_io_data_t data);
static int IO_BufSeek(int state, size_t pos, archiver_io_data_t data);
static size_t IO_BufTell(archiver_io_data_t data);

#pragma endregion

#pragma region(Error stuff)

static void CopyErrMsg(error_msg_t *dst);
static void ErrPrintF(const char *format, ...);
static void VErrPrintF(const char *format, va_list list);
static void ParserErrorF(ref_parser_t parser, const char *format, ...);

#pragma endregion

#pragma region(IO Processing)

static const io_state_t *PreprocessIOState(const io_state_t *state, io_op_type_t io_op);
static int SetupBufferIOState(io_state_t *p_state, const void *io_buffer_data, bool write);

#pragma endregion

#pragma region(Parser stuff)

#define PARSER_FUNC_HEAD() {parser->error = 0;}
#define PARSER_FUNC_ERR_BREAK() if (parser->error != 0){ return parser->error; }
#define PARSER_FUNC_ERR_BREAK_EXP(exp) if (parser->error != 0){ exp; return parser->error; }

#define PARSER_CONTEXT(operation, step) { \
char buffer[c_parser_context_sz] = {0};\
snprintf(buffer, c_parser_context_sz, "%s::%s", operation, step);\
strncpy(parser->context, buffer, c_parser_context_sz);}

#define PARSER_CONTEXT_FUNC(step) PARSER_CONTEXT(__FUNCTION__, step)
#define PARSER_READ_TO_VALIDATE(name, default) if (ParserReadBuf(parser, &(name), sizeof(name)) != sizeof(name)) { parser->error = ENODATA; return default; }

enum
{
  c_parser_context_sz = 64,
};

typedef struct parser_s
{
  const io_state_t *const io;
  archive_t *const output;

  char context[c_parser_context_sz];

  bool tiny;
  int error;
} parser_t, *ref_parser_t;

static inline unsigned ParserReadBuf(ref_parser_t parser, void *buffer, unsigned count) {
  PARSER_FUNC_HEAD();

  size_t read_count = 0;
  parser->error = parser->io->read_proc(buffer, count, &read_count, parser->io->data);
  return read_count;
}

static inline int ParserReadBufFully(ref_parser_t parser, void *buffer, unsigned count) {
  if (ParserReadBuf(parser, buffer, count) != count)
  {
    return EOF;
  }

  return 0;
}

static inline char ParserReadChar(ref_parser_t parser) {
  char chr = 0;
  PARSER_READ_TO_VALIDATE(chr, 0);
  return chr;
}

static inline void ParserAdvanceBy(ref_parser_t parser, unsigned offset) {
  parser->io->seek_proc(SEEK_CUR, offset, parser->io->data);
}

static inline void ParserAdvance(ref_parser_t parser) {
  ParserAdvanceBy(parser, 1);
}

static inline size_t ParserTellPos(ref_parser_t parser) {
  return parser->io->tell_proc(parser->io->data);
}

static size_t ParserSpaceLeft(ref_parser_t parser);

static uint64_t ParserReadIntStr(ref_parser_t parser, unsigned num_str_len);

static int ReadSignature(ref_parser_t parser);
static int ReadFileHeader(ref_parser_t parser, archive_file_t *file);
static int ReadFile(ref_parser_t parser, archive_file_t *file);

static int ReadAllFiles(ref_parser_t parser);


#pragma endregion

archiver_io_t Archiver_FileOpenPath(const char *path, bool read) {
  FILE *file = fopen(path, read ? "rb" : "wb");
  return Archiver_FileOpen(file);
}

archiver_io_t Archiver_FileOpen(FILE *file) {
  archiver_io_t state = {0};

  state.data.file = file;

  state.read_proc = IO_FileRead;
  state.write_proc = IO_FileWrite;
  state.seek_proc = IO_FileSeek;
  state.tell_proc = IO_FileTell;

  return state;
}

int Archiver_FileClose(const archiver_io_t *io_state) {
  if (!IsIOFile(io_state))
  {
    ErrPrintF("invalid FILE io state");
    return EBADF;
  }

  fclose(io_state->data.file);

  return 0;
}

archiver_io_t ArchiverIO_BufferOpenW(archiver_io_write_buf_t buffer) {
  io_state_t state = {0};
  int error = 0;

  error = SetupBufferIOState(&state, &buffer, true);
  if (error != 0)
  {
    error_msg_t old_err_msg = {0};
    CopyErrMsg(&old_err_msg);

    ErrPrintF("Couldn't setup the Buffer IO state (WRITE) [%X]: %s", error, old_err_msg);
    return state;
  }

  return state;
}

archiver_io_t ArchiverIO_BufferOpenR(archiver_io_read_buf_t buffer) {
  io_state_t state = {0};
  int error = 0;


  error = SetupBufferIOState(&state, &buffer, false);
  if (error != 0)
  {
    error_msg_t old_err_msg = {0};
    CopyErrMsg(&old_err_msg);

    ErrPrintF("Couldn't setup the Buffer IO state (READ) [%X]: %s", error, old_err_msg);
    return state;
  }

  return state;
}

int ArchiverIO_BufferClose(const archiver_io_t *io_state) {


  return 0;
}

int Archiver_ReadIO(const archiver_io_t *io_state, archive_t *container) {
  if (io_state == NULL)
  {
    ErrPrintF("null 'io_state'");
    return EINVAL;
  }

  if (container == NULL)
  {
    ErrPrintF("null 'container'");
    return EINVAL;
  }
  *container = (archive_t){0};

  io_state = PreprocessIOState(io_state, eReadingAdvanced);
  if (io_state == NULL)
  {
    error_msg_t old_err_msg = {0};
    CopyErrMsg(&old_err_msg);

    ErrPrintF("Failed to validate/preprocess io state: %s", old_err_msg);
    return EBADF;
  }

  parser_t parser = {
    io_state,
    container
  };

  ReadSignature(&parser);
  ReadAllFiles(&parser);

  return 0;
}

int Archiver_Close(archive_t *archive) {
  if (archive == NULL)
  {
    ErrPrintF("Null archive to close!");
    return EINVAL;
  }

  // either:
  // 1- files data == null BUT files count is greater then zero
  // 2- files data != null BUT files count == zero
  if ((archive->files == NULL) != (archive->files_count <= 0))
  {
    ErrPrintF(
      "Invalid/Illegal files data in archive, %s=%p, %s=%u",
      NAMED_PERM(archive->files), NAMED_PERM(archive->files_count)
    );

    return EBADF;
  }

  for (unsigned i = 0; i < archive->files_count; i++)
  {
    free(archive->files[i].data);
  }
  free(archive->files);



  return 0;
}

inline bool IO_OPHasWriting(io_op_type_t type) {
  return ((int)type) % 2 == 1;
}

inline bool IO_OPHasReading(io_op_type_t type) {
  return !IO_OPHasWriting(type);
}

inline bool IO_OPIsAdvanced(io_op_type_t type) {
  return (int)type >= eReadingAdvanced;
}

int IO_FileRead(void *buffer, size_t bytes, size_t *read_count, archiver_io_data_t data) {

  errno = 0;
  const size_t read_count_v = fread(buffer, bytes, 1, data.file);

  if (read_count) { *read_count = read_count_v; }

  return errno;
}

int IO_FileWrite(const void *buffer, size_t bytes, size_t *write_count, archiver_io_data_t data) {

  errno = 0;
  const size_t write_count_v = fwrite(buffer, bytes, 1, data.file);

  if (write_count) { *write_count = write_count_v; }

  return errno;
}

int IO_FileSeek(int state, size_t pos, archiver_io_data_t data) {

  if (state != SEEK_SET && state != SEEK_CUR && state != SEEK_END)
  {
    return EINVAL;
  }

  errno = 0;
  fseek(data.file, pos, state);

  return errno;
}

size_t IO_FileTell(archiver_io_data_t data) {
  return ftell(data.file);
}

int IO_BufRead(void *buffer, size_t bytes, size_t *read_count, archiver_io_data_t data) {
  archiver_io_read_buf_t *read_buf = &data.buffer.read_buf;

  if (read_buf->data == NULL)
  {
    return ENODATA;
  }

  if (read_buf->pos > read_buf->length)
  {
    ErrPrintF(
      "Reading Buffer: buffer pos is overflowing it's length: %s=%llu, %s=%llu",
      NAMED_PERM(read_buf->length),
      NAMED_PERM(read_buf->pos)
    );
    return EBADF;
  }

  const size_t space_left = read_buf->length - read_buf->pos;
  const size_t valid_read_bytes = space_left < bytes ? space_left : bytes;
  const void *read_buf_pos = read_buf->data + read_buf->pos;

  if (read_count != NULL)
  {
    *read_count = valid_read_bytes;
  }

  memcpy(
    buffer, read_buf_pos, valid_read_bytes
  );

  read_buf->pos += valid_read_bytes;

  return 0;
}

int IO_BufWrite(const void *buffer, size_t bytes, size_t *write_count, archiver_io_data_t data) {
  archiver_io_write_buf_t *write_buf = &data.buffer.write_buf;

  if (write_buf->data == NULL)
  {
    return ENODATA;
  }

  if (write_buf->pos > write_buf->length)
  {
    ErrPrintF(
      "Writing Buffer: buffer pos is overflowing it's length: %s=%llu, %s=%llu",
      NAMED_PERM(write_buf->length),
      NAMED_PERM(write_buf->pos)
    );
    return EBADF;
  }

  const size_t space_left = write_buf->length - write_buf->pos;
  const size_t valid_write_bytes = space_left < bytes ? space_left : bytes;
  void *write_buf_pos = write_buf->data + write_buf->pos;

  if (write_count != NULL)
  {
    *write_count = valid_write_bytes;
  }

  memcpy(
    write_buf_pos, buffer, valid_write_bytes
  );

  write_buf->pos += valid_write_bytes;

  return 0;
}

int IO_BufSeek(int state, size_t pos, archiver_io_data_t data) {

  switch (state)
  {
  case SEEK_SET:
    data.buffer.read_buf.pos = pos;
    break;
  case SEEK_CUR:
    data.buffer.read_buf.pos += pos;
    break;
  case SEEK_END:
    data.buffer.read_buf.pos = data.buffer.read_buf.length - pos;
    break;
  default:
    {
      ErrPrintF("Invalid state id: %d, only SEEK_SET/SEEK_CUR/SEEK_END are valid states", state);
      return EBADF;
    }
  }

  if (data.buffer.read_buf.pos > data.buffer.read_buf.length)
  {
    ErrPrintF(
      "Seeking Buffer: Pos overflowing buffer length: %s=%llu, %s=%llu",
      NAMED_PERM(data.buffer.read_buf.length),
      NAMED_PERM(data.buffer.read_buf.pos)
    );

    return EINVAL;
  }

  return 0;
}

size_t IO_BufTell(archiver_io_data_t data) {
  return data.buffer.read_buf.pos;
}

void CopyErrMsg(error_msg_t *dst) {
  strncpy(*dst, g_ErrorMSG, ERROR_MSG_SZ);
}

void ErrPrintF(const char *format, ...) {
  va_list list;
  va_start(list, format);

  VErrPrintF(
    format, list
  );




  va_end(list);


}

void VErrPrintF(const char *format, va_list list) {
  vsnprintf(
    g_ErrorMSG, ERROR_MSG_SZ, format, list
  );

  if (g_DoErrorLogging)
  {
    fprintf(stderr, "%s", g_ErrorMSG);
    fputc('\n', stderr);
    fflush(stderr);
  }
}

void ParserErrorF(ref_parser_t parser, const char *format, ...) {
  char buffer[ERROR_MSG_SZ] = {0};
  snprintf(buffer, ERROR_MSG_SZ, "Parser[%s]: %s", parser->context, format);

  va_list list;

  va_start(list, format);

  VErrPrintF(
    buffer, list
  );

  va_end(list);
}

const io_state_t *PreprocessIOState(const io_state_t *state, io_op_type_t io_op) {
  if (state == NULL)
  {
    return NULL;
  }

  if (!IsIOStateFullyFunctional(state, io_op))
  {
    return NULL;
  }

  return state;
}

bool IsIOStateFullyFunctional(const io_state_t *state, io_op_type_t io_op) {
  if (IO_OPHasReading(io_op) && state->read_proc == NULL)
  {
    ErrPrintF("reading operation requires the io state to have the `read` procedure");
    return false;
  }

  if (IO_OPHasWriting(io_op) && state->read_proc == NULL)
  {
    ErrPrintF("writing operation requires the io state to have the `write` procedure");
    return false;
  }

  if (IO_OPIsAdvanced(io_op) && !IsIOStateAdvancedCapable(state))
  {
    ErrPrintF(
      "advanced read/write operations require the io state to have the `seek` and `tell` procedures"
    );
    return false;
  }

  return true;
}

bool IsIOStateAdvancedCapable(const io_state_t *state) {
  return state->seek_proc != NULL && state->tell_proc != NULL;
}

bool IsIOFile(const io_state_t *state) {
  if (state->read_proc != IO_FileRead)
  {
    return false;
  }
  if (state->write_proc != IO_FileWrite)
  {
    return false;
  }
  if (state->seek_proc != IO_FileSeek)
  {
    return false;
  }
  if (state->tell_proc != IO_FileTell)
  {
    return false;
  }

  if (state->data.file == NULL)
  {
    return false;
  }

  return true;
}

int SetupBufferIOState(io_state_t *p_state, const void *io_buffer_data, bool write) {
  if (!p_state || !io_buffer_data)
  {
    return ECANCELED;
  }

  const archiver_io_write_buf_t *p_buf = (const archiver_io_write_buf_t *)io_buffer_data;

  if (p_buf->data == NULL)
  {
    ErrPrintF("io buffer data has a `NULL` data!");
    return ENODATA;
  }

  if (p_buf->length >= UINT32_MAX)
  {
    ErrPrintF("IO buffer data is TOO LONG: %s=%llu", NAMED_PERM(p_buf->length));
    return ENODATA;
  }

  if (p_buf->pos > p_buf->length)
  {
    ErrPrintF(
      "IO buffer POS is overflowing the buffer length: %s=%llu, %s=%llu",
      NAMED_PERM(p_buf->length),
      NAMED_PERM(p_buf->pos)
    );
    return ENODATA;
  }

  if (write)
  {
    p_state->data.buffer.write_buf = *(const archiver_io_write_buf_t *)io_buffer_data;
    p_state->write_proc = IO_BufWrite;
  }
  else
  {
    p_state->data.buffer.read_buf = *(const archiver_io_read_buf_t *)io_buffer_data;
    p_state->read_proc = IO_BufRead;
  }

  p_state->seek_proc = IO_BufSeek;
  p_state->tell_proc = IO_BufTell;

  return 0;
}

size_t ParserSpaceLeft(ref_parser_t parser) {
  const io_state_t *const io_state = parser->io;

  const size_t current = ParserTellPos(parser);

  ASSERT_ERR(io_state->seek_proc(SEEK_END, 0, io_state->data) == 0, "failed to jump to IO end");

  const size_t end = ParserTellPos(parser);

  ASSERT_ERR(io_state->seek_proc(SEEK_SET, current, io_state->data) == 0, "failed to return jump to IO last current pos");

  return (end > current) ? (end - current) : (current - end);
}

uint64_t ParserReadIntStr(ref_parser_t parser, unsigned num_str_len) {
  enum
  {
    c_max_num_str_length = 20
  };

  if (num_str_len > c_max_num_str_length)
  {
    ParserErrorF(
      parser,
      "IntStr is too big! %s=%d, %s=%u",
      NAMED_PERM(c_max_num_str_length),
      NAMED_PERM(num_str_len)
    );
    parser->error = E2BIG;
    return 0;
  }

  char num_str[c_max_num_str_length + 1];

  ParserReadBufFully(parser, num_str, num_str_len);
  PARSER_FUNC_ERR_BREAK_EXP(
    ParserErrorF(
      parser,
      "Couldn't read the number with %s=%u at %s=%llu",
      NAMED_PERM(num_str_len),
      NAMED_PERM(ParserTellPos(parser))
    );
  );

  return strtoull(num_str, NULL, 0);
}

int ReadSignature(ref_parser_t parser) {
  PARSER_CONTEXT_FUNC("file signature");
  ParserReadBufFully(parser, parser->output->signature, ARCHIVER_SIGN_LEN);
  return parser->error;
}

int ReadFileHeader(ref_parser_t parser, archive_file_t *file) {

  file->offset = ParserTellPos(parser);

  PARSER_CONTEXT_FUNC("file_name");
  ParserReadBufFully(parser, file->file_name, ARCHIVER_FILE_NAME_LEN);
  PARSER_FUNC_ERR_BREAK_EXP(
    ParserErrorF(
      parser,
      "couldn't read the file name/identifier for %s=%p at IO[%s=%llu]",
      NAMED_PERM(file),
      NAMED_PERM(ParserTellPos(parser))
    )
  );

  PARSER_CONTEXT_FUNC("modification_time_stamp");
  file->modification_time_stamp = ParserReadIntStr(parser, ARCHIVER_MOD_TIMESTAMP_LEN);
  PARSER_FUNC_ERR_BREAK();

  PARSER_CONTEXT_FUNC("owner_id");
  file->owner_id = ParserReadIntStr(parser, ARCHIVER_OWNER_ID_LEN);
  PARSER_FUNC_ERR_BREAK();
  PARSER_CONTEXT_FUNC("group_id");
  file->group_id = ParserReadIntStr(parser, ARCHIVER_GROUP_ID_LEN);
  PARSER_FUNC_ERR_BREAK();

  PARSER_CONTEXT_FUNC("file_mode_flags");
  file->file_mode_flags = ParserReadIntStr(parser, ARCHIVER_FILE_MODE_LEN);
  PARSER_FUNC_ERR_BREAK();
  PARSER_CONTEXT_FUNC("file_size");
  file->file_size = ParserReadIntStr(parser, ARCHIVER_FILE_SIZE_LEN);
  PARSER_FUNC_ERR_BREAK();

  PARSER_CONTEXT_FUNC("_ending_chars");
  ParserReadBufFully(parser, file->_ending_chars, ARCHIVER_END_CHR_LEN);
  PARSER_FUNC_ERR_BREAK_EXP(
    ParserErrorF(
      parser,
      "couldn't read the file header's ending chars '`\\n' for %s=%p at IO[%s=%llu]",
      NAMED_PERM(file),
      NAMED_PERM(ParserTellPos(parser))
    )
  );


  file->data_offset = ParserTellPos(parser);

  return 0;
}

int ReadFile(ref_parser_t parser, archive_file_t *file) {
  if (ReadFileHeader(parser, file) != 0)
  {
    return parser->error;
  }

  file->data = malloc(file->file_size);
  PARSER_ASSERT(
    file->data != NULL,

    "Failed to allocate file data: " NAMED_PERM_F(llu) ", " NAMED_PERM_F(llu) ", " NAMED_PERM_F(llu),
    NAMED_PERM(file->offset),
    NAMED_PERM(file->data_offset),
    NAMED_PERM(file->file_size)
  );

  PARSER_CONTEXT_FUNC("file_data");
  parser->error = ParserReadBufFully(parser, file->data, file->file_size);
  PARSER_FUNC_ERR_BREAK();

  PARSER_CONTEXT_FUNC("final_new_line");
  const char new_line = ParserReadChar(parser);

  if (new_line != '\n')
  {
    size_t final_new_line_pos = file->data_offset + file->file_size;
    ParserErrorF(
      parser,
      "Expecting a new line at " NAMED_PERM_F(llu) " for file %s='%s', found [0x%X] " NAMED_PERM_F(c),
      NAMED_PERM(final_new_line_pos),
      NAMED_PERM(file->file_name),
      new_line, NAMED_PERM(new_line)
    );
  }

  return parser->error;
}

int ReadAllFiles(ref_parser_t parser) {
  size_t files_arr_cap = 32;
  size_t files_arr_len = 0;
  archive_file_t *files_arr = calloc(files_arr_cap, sizeof(archive_file_t));

  PARSER_ASSERT(
    files_arr != NULL,
    "Allocating files array failed! alloc_size=%llu bytes",
    files_arr_cap * sizeof(archive_file_t)
  );

  while (true)
  {
    // read the entire archive
    if (ParserSpaceLeft(parser) == 0)
    {
      break;
    }

    archive_file_t file = {0};
    if (ReadFile(parser, &file) != 0)
    {
      error_msg_t msg = {0};
      CopyErrMsg(&msg);

      ParserErrorF(parser, "Failed to load file index %llu: %s", files_arr_len, msg);
      break;
    }

    if (files_arr_len == files_arr_cap)
    {
      files_arr_cap *= 2;
      archive_file_t *new_arr = realloc(files_arr, files_arr_cap * sizeof(archive_file_t));
      PARSER_ASSERT(
        new_arr != NULL,

        "Reallocating files array failed! alloc_size=%llu bytes",
        files_arr_cap * sizeof(archive_file_t)
      );

      files_arr = new_arr;
    }

    files_arr[files_arr_len++] = file;
  }

  parser->output->files = files_arr;
  parser->output->files_count = files_arr_len;

  return parser->error;
}


