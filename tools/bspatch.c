/* Minimal bspatch for bsdiff4-compatible patches (BSDIFF40 format)
 * Compile: aarch64-linux-gnu-gcc -static -o bspatch bspatch.c -lbz2
 * Usage:   bspatch old_file new_file patch_file
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bzlib.h>

#define MAGIC "BSDIFF40"

static int64_t read_int64(FILE *f) {
    unsigned char buf[8];
    if (fread(buf, 1, 8, f) != 8) return -1;
    return (int64_t)buf[0] << 56 |
           (int64_t)buf[1] << 48 |
           (int64_t)buf[2] << 40 |
           (int64_t)buf[3] << 32 |
           (int64_t)buf[4] << 24 |
           (int64_t)buf[5] << 16 |
           (int64_t)buf[6] << 8  |
           (int64_t)buf[7];
}

int main(int argc, char *argv[]) {
    FILE *f, *fout;
    unsigned char *old_data, *new_data;
    unsigned char *dbuf, *ebuf, *cbuf;
    int64_t old_size, new_size, len_control, len_diff;
    int64_t n_control, i, j;
    int ret;

    if (argc != 4) {
        fprintf(stderr, "Usage: bspatch oldfile newfile patchfile\n");
        return 1;
    }

    /* Read old file */
    f = fopen(argv[1], "rb");
    if (!f) { perror("oldfile"); return 1; }
    fseek(f, 0, SEEK_END);
    old_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    old_data = malloc(old_size + 1);
    if (!old_data) { fprintf(stderr, "malloc failed\n"); return 1; }
    fread(old_data, 1, old_size, f);
    fclose(f);

    /* Read patch */
    f = fopen(argv[3], "rb");
    if (!f) { perror("patchfile"); return 1; }

    /* Read magic */
    char magic[9] = {0};
    fread(magic, 1, 8, f);
    if (strncmp(magic, MAGIC, 7) != 0) {
        fprintf(stderr, "Invalid patch magic: expected BSDIFF40\n");
        return 1;
    }

    /* Read lengths */
    len_control = read_int64(f);
    len_diff = read_int64(f);
    new_size = read_int64(f);

    /* Read compressed blocks */
    cbuf = malloc(len_control);
    dbuf = malloc(len_diff);
    fread(cbuf, 1, len_control, f);
    fread(dbuf, 1, len_diff, f);
    /* Rest of file is extra block */
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long file_end = ftell(f);
    long len_extra = file_end - pos;
    fseek(f, pos, SEEK_SET);
    ebuf = malloc(len_extra);
    fread(ebuf, 1, len_extra, f);
    fclose(f);

    /* Decompress blocks */
    unsigned int dest_len;

    /* Decompress control block */
    /* Control block is 24-byte tuples: (diff_len, extra_len, seek), all int64 */
    /* We need the decompressed size which is not stored separately */
    /* Decompress with buffer expansion */
    unsigned int cbuf_decomp_len = len_control * 4 + 1024; /* conservative estimate */
    unsigned char *cbuf_decomp = malloc(cbuf_decomp_len);
    dest_len = cbuf_decomp_len;
    ret = BZ2_bzBuffToBuffDecompress((char*)cbuf_decomp, &dest_len,
                                     (char*)cbuf, len_control, 0, 0);
    if (ret != BZ_OK) {
        fprintf(stderr, "bz2 decompress control failed: %d\n", ret);
        return 1;
    }
    n_control = dest_len / 24;

    /* Decompress diff block */
    unsigned int dbuf_decomp_len = new_size;
    unsigned char *dbuf_decomp = malloc(dbuf_decomp_len);
    dest_len = dbuf_decomp_len;
    ret = BZ2_bzBuffToBuffDecompress((char*)dbuf_decomp, &dest_len,
                                     (char*)dbuf, len_diff, 0, 0);
    if (ret != BZ_OK) {
        fprintf(stderr, "bz2 decompress diff failed: %d\n", ret);
        return 1;
    }

    /* Decompress extra block */
    unsigned int ebuf_decomp_len = new_size;
    unsigned char *ebuf_decomp = malloc(ebuf_decomp_len);
    dest_len = ebuf_decomp_len;
    ret = BZ2_bzBuffToBuffDecompress((char*)ebuf_decomp, &dest_len,
                                     (char*)ebuf, len_extra, 0, 0);
    if (ret != BZ_OK) {
        fprintf(stderr, "bz2 decompress extra failed: %d\n", ret);
        return 1;
    }

    /* Apply patch */
    new_data = malloc(new_size + 1);
    if (!new_data) { fprintf(stderr, "malloc failed\n"); return 1; }

    int64_t old_pos = 0, new_pos = 0;
    unsigned char *ctrl = cbuf_decomp;

    while (new_pos < new_size) {
        int64_t diff_len = (int64_t)ctrl[0]  << 56 | (int64_t)ctrl[1]  << 48 |
                           (int64_t)ctrl[2]  << 40 | (int64_t)ctrl[3]  << 32 |
                           (int64_t)ctrl[4]  << 24 | (int64_t)ctrl[5]  << 16 |
                           (int64_t)ctrl[6]  << 8  | (int64_t)ctrl[7];
        int64_t extra_len = (int64_t)ctrl[8]  << 56 | (int64_t)ctrl[9]  << 48 |
                            (int64_t)ctrl[10] << 40 | (int64_t)ctrl[11] << 32 |
                            (int64_t)ctrl[12] << 24 | (int64_t)ctrl[13] << 16 |
                            (int64_t)ctrl[14] << 8  | (int64_t)ctrl[15];
        int64_t seek =     (int64_t)ctrl[16] << 56 | (int64_t)ctrl[17] << 48 |
                           (int64_t)ctrl[18] << 40 | (int64_t)ctrl[19] << 32 |
                           (int64_t)ctrl[20] << 24 | (int64_t)ctrl[21] << 16 |
                           (int64_t)ctrl[22] << 8  | (int64_t)ctrl[23];
        ctrl += 24;

        /* Apply diff block */
        for (i = 0; i < diff_len; i++) {
            if (old_pos + i < old_size)
                new_data[new_pos + i] = old_data[old_pos + i] + dbuf_decomp[new_pos + i];
            else
                new_data[new_pos + i] = dbuf_decomp[new_pos + i];
        }
        new_pos += diff_len;
        old_pos += diff_len;

        /* Copy extra block */
        for (i = 0; i < extra_len; i++)
            new_data[new_pos + i] = ebuf_decomp[new_pos - diff_len + i];
        new_pos += extra_len;

        /* Seek in old file */
        old_pos += seek;
    }

    /* Write output */
    fout = fopen(argv[2], "wb");
    if (!fout) { perror("newfile"); return 1; }
    fwrite(new_data, 1, new_size, fout);
    fclose(fout);

    fprintf(stderr, "bspatch: %lld bytes written to %s\n", (long long)new_size, argv[2]);

    free(old_data); free(new_data);
    free(dbuf_decomp); free(ebuf_decomp); free(cbuf_decomp);
    free(cbuf); free(dbuf); free(ebuf);
    return 0;
}
