/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "frogfs/frogfs.h"


extern void *frogfs_bin;

static void usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [ACTION [...]]\n", argv0);
    fputs("\n", stderr);
    fputs("Actions:\n", stderr);
    fputs("    --help\n", stderr);
    fputs("    --load=BINARY\n", stderr);
    fputs("    --stat=PATH\n", stderr);
    fputs("    --open=PATH\n", stderr);
    fputs("    --seek-cur=N\n", stderr);
    fputs("    --seek-set=N\n", stderr);
    fputs("    --seek-end=N\n", stderr);
    fputs("    --read[=N]\n", stderr);
    fputs("    --ls[=PATH]\n", stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int c, fd = -1;
    struct stat sb;
    void *mmap_addr = NULL;
    frogfs_fs_t *fs = NULL;
    frogfs_f_t *f = NULL;

    frogfs_config_t config = {
        .addr = &frogfs_bin,
    };

    fs = frogfs_init(&config);
    if (fs == NULL) {
        fputs("frogfs_init failed\n", stderr);
        exit(EXIT_FAILURE);
    }

    while (1) {
        int option_index = 0;

        static struct option long_options[] = {
            {"help",     no_argument,       0, 0},
            {"load",     required_argument, 0, 0},
            {"stat",     required_argument, 0, 0},
            {"open",     required_argument, 0, 0},
            {"seek-cur", required_argument, 0, 0},
            {"seek-set", required_argument, 0, 0},
            {"seek-end", required_argument, 0, 0},
            {"read",     optional_argument, 0, 0},
            {"drain",    no_argument,       0, 0},
            {"ls",       optional_argument, 0, 0},
            {0,          0,                 0, 0}
        };

        c = getopt_long(argc, argv, "", long_options, &option_index);
        if (c == -1) {
            break;
        }

        if (c == 0) {
            if (strcmp("help", long_options[option_index].name) == 0) {
                usage(argv[0]);
            }

            if (strcmp("load", long_options[option_index].name) == 0) {
                if (f) {
                    frogfs_close(f);
                    f = NULL;
                }

                if (fs) {
                    frogfs_deinit(fs);
                    fs = NULL;
                }

                if (mmap_addr) {
                    munmap(mmap_addr, sb.st_size);
                    mmap_addr = NULL;
                }

                if (fd != -1) {
                    close(fd);
                    fd = -1;
                }

                fd = open(optarg, O_RDONLY);
                if (fd < 0) {
                    fputs("open failed\n", stderr);
                    exit(EXIT_FAILURE);
                }

                if (fstat(fd, &sb) == -1) {
                    fputs("fstat failed\n", stderr);
                    exit(EXIT_FAILURE);
                }

                mmap_addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd,
                        0);
                if (mmap_addr == MAP_FAILED) {
                    fputs("mmap failed\n", stderr);
                    close(fd);
                    exit(EXIT_FAILURE);
                }

                frogfs_config_t config = {
                    .addr = mmap_addr,
                };

                fs = frogfs_init(&config);
                if (fs == NULL) {
                    fputs("frogfs_init failed\n", stderr);
                    munmap(mmap_addr, sb.st_size);
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            }

            if (strcmp("stat", long_options[option_index].name) == 0) {
                const frogfs_entry_t *entry = frogfs_get_entry(fs, argv[2]);
                if (entry) {
                    frogfs_stat_t stat;
                    frogfs_stat(fs, entry, &stat);
                    if (stat.type == FROGFS_ENTRY_TYPE_FILE) {
                        fprintf(stderr, "Object '%s' is a file.\n", optarg);
                        if (stat.compression == 0) {
                            fputs("File is not compressed.\n", stderr);
                        } else if (stat.compression == 1) {
                            fputs("File is compressed with deflate.\n",
                                    stderr);
                        } else if (stat.compression == 2) {
                            fputs("File is compressed with heatsrhink.\n",
                                    stderr);
                        } else {
                            fputs("File is compressed with an unknown"
                                    "scheme.\n", stderr);
                        }
                        fprintf(stderr, "File is %d bytes.\n", stat.real_sz);
                        if (stat.compression != 0) {
                            fprintf(stderr, "File is %d bytes compressed.\n",
                                    stat.data_sz);
                        }
                    } else if (stat.type == FROGFS_ENTRY_TYPE_DIR) {
                        fprintf(stderr, "Object '%s' is a directory.\n",
                                optarg);
                    }
                } else {
                    fprintf(stderr, "Object '%s' does not exist.\n", optarg);
                }
            }

            if (strcmp("open", long_options[option_index].name) == 0) {
                if (f) {
                    frogfs_close(f);
                    f = NULL;
                }

                const frogfs_entry_t *entry = frogfs_get_entry(fs, optarg);
                if (entry == NULL) {
                    fprintf(stderr, "No such object '%s'.\n", optarg);
                    exit(EXIT_FAILURE);
                }

                if (entry->child_count < 0xFF00) {
                    fprintf(stderr, "Object '%s' is not a file.\n", optarg);
                    exit(EXIT_FAILURE);
                }

                f = frogfs_open(fs, entry, 0);
                if (f == NULL) {
                    fprintf(stderr, "Error opening '%s'.\n", optarg);
                    exit(EXIT_FAILURE);
                }
            }

            if (strcmp("seek-cur", long_options[option_index].name) == 0) {
                if (f == NULL) {
                    fputs("Error, no file open.\n", stderr);
                    exit(EXIT_FAILURE);
                }

                frogfs_seek(f, atoi(optarg), SEEK_CUR);
            }

            if (strcmp("seek-set", long_options[option_index].name) == 0) {
                if (f == NULL) {
                    fputs("Error, no file open.\n", stderr);
                    exit(EXIT_FAILURE);
                }

                frogfs_seek(f, atoi(optarg), SEEK_SET);
            }

            if (strcmp("seek-end", long_options[option_index].name) == 0) {
                if (f == NULL) {
                    fputs("Error, no file open.\n", stderr);
                    exit(EXIT_FAILURE);
                }

                frogfs_seek(f, atoi(optarg), SEEK_END);
            }

            if (strcmp("read", long_options[option_index].name) == 0) {
                if (f == NULL) {
                    fputs("Error, no file open.\n", stderr);
                    exit(EXIT_FAILURE);
                }

                char buf[16];
                size_t bytes_read = 0;
                if (optarg) {
                    size_t n = atoi(optarg);
                    while (bytes_read < n) {
                        size_t chunk = n - bytes_read < sizeof(buf) ?
                                n - bytes_read : sizeof(buf);
                        ssize_t ret = frogfs_read(f, buf, chunk);
                        if (ret < 0) {
                            fputs("Error reading file.\n", stderr);
                            exit(EXIT_FAILURE);
                        }
                        fwrite(buf, ret, 1, stdout);
                        bytes_read += ret;
                        if (ret < chunk) {
                            break;
                        }
                    }
                } else {
                    ssize_t ret;
                    while ((ret = frogfs_read(f, buf, sizeof(buf))) > 0) {
                        fwrite(buf, ret, 1, stdout);
                        bytes_read += ret;
                    }
                }
                fflush(stdout);
                fprintf(stderr, "Read %d bytes.\n", bytes_read);
            }

            if (strcmp("ls", long_options[option_index].name) == 0) {
                const frogfs_entry_t *entry;

                if (optarg) {
                    entry = frogfs_get_entry(fs, optarg);
                    if (entry == NULL) {
                        fprintf(stderr, "No such entry '%s'.\n", optarg);
                        exit(EXIT_FAILURE);
                    }

                    if (!FROGFS_ISDIR(entry)) {
                        fprintf(stderr, "Entry '%s' is not a directory.\n",
                                optarg);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    entry = NULL;
                }

                frogfs_d_t *d = frogfs_opendir(fs, entry);
                if (d == NULL) {
                    fprintf(stderr, "Error opening directory '%s'.\n");
                    exit(EXIT_FAILURE);
                }

                while (entry = frogfs_readdir(d)) {
                    const char *path = frogfs_get_path(fs, entry);
                    if (FROGFS_ISFILE(entry)) {
                        printf("F %s\n", path);
                    } else {
                        printf("D %s\n", path);
                    }
                }

                frogfs_closedir(d);
            }
        }
    }

    if (f) {
        frogfs_close(f);
    }

    if (fs) {
        frogfs_deinit(fs);
    }

    if (mmap_addr) {
        munmap(mmap_addr, sb.st_size);
    }

    if (fd != -1) {
        close(fd);
    }

    exit(EXIT_SUCCESS);
}
