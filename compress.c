#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <lz4frame.h>

#define PAGES(n) (4096 * (n))

static void *ubuf;
static void *cbuf;
static size_t ucap, ulen;
static size_t ccap, clen;

static void panic(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(2);
}

static void panic_memory(void) { panic("out of memory"); }

static void free_buffers(void) {
  free(ubuf);
  free(cbuf);
  ubuf = cbuf = 0;
  ucap = ulen = 0;
  ccap = clen = 0;
}

static void allocate_buffers(size_t new_ucap, size_t new_ccap) {
  free_buffers();
  ucap = new_ucap;
  ccap = new_ccap;
  if (!(ubuf = calloc(1, ucap))) {
    panic_memory();
  }
  if (!(cbuf = calloc(1, ccap))) {
    panic_memory();
  }
}

static int read_uncompressed_bytes(void) {
  ssize_t n;

  memset(ubuf, 0, ucap);
  memset(cbuf, 0, ccap);
  n = read(STDIN_FILENO, ubuf, ucap);
  if (n == (ssize_t)-1) {
    panic("read()");
  }
  if (!n) {
    return 0;
  }
  ulen = (size_t)n;
  return 1;
}

static void write_compressed_bytes(void) {
  if (write(STDOUT_FILENO, cbuf, clen) != (ssize_t)clen) {
    panic("write()");
  }
}

static void compress_bzip2(void) {}

static void compress_gzip(void) {}

static void compress_lz4_step(const char *func, size_t n) {
  if (LZ4F_isError(n)) {
    panic(func);
  }
  clen = n;
  write_compressed_bytes();
}

static void compress_lz4(void) {
  LZ4F_cctx *ctx;

  allocate_buffers(PAGES(2), LZ4F_compressBound(PAGES(2), 0));
  if (LZ4F_createCompressionContext(&ctx, LZ4F_VERSION) != 0) {
    panic("LZ4F_createCompressionContext");
  }
  compress_lz4_step("LZ4F_compressBegin",
                    LZ4F_compressBegin(ctx, cbuf, ccap, 0));
  while (read_uncompressed_bytes()) {
    compress_lz4_step("LZ4F_compressUpdate",
                      LZ4F_compressUpdate(ctx, cbuf, ccap, ubuf, ulen, 0));
  }
  compress_lz4_step("LZ4F_compressEnd", LZ4F_compressEnd(ctx, cbuf, ccap, 0));
  if (LZ4F_freeCompressionContext(ctx) != 0) {
    panic("LZ4F_freeCompressionContext()");
  }
}

static void compress_lzma(void) {}

static void compress_zstd(void) {}

int main(int argc, char **argv) {
  const char *which;

  if (argc != 2) {
    panic("usage");
  }
  which = argv[1];
  if (!strcmp(which, "bzip2")) {
    compress_bzip2();
  } else if (!strcmp(which, "gzip")) {
    compress_gzip();
  } else if (!strcmp(which, "lz4")) {
    compress_lz4();
  } else if (!strcmp(which, "lzma")) {
    compress_lzma();
  } else if (!strcmp(which, "zstd")) {
    compress_zstd();
  } else {
    panic("what?");
  }
  return 0;
}
