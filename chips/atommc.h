#pragma once
/*#
    # atommc

    Header-only AtoMMC emulator written in C.

    Do this:
    ~~~C
    #define CHIPS_IMPL
    ~~~
    before you include this file in *one* C or C++ file to create the
    implementation.

    Optionally provide the following macros with your own implementation

    ~~~C
    CHIPS_ASSERT(c)
    ~~~
        your own assert macro (default: assert(c))

    ## Emulated Pins
    ## TODO

    ## NOT EMULATED

    Currently this just contains the minimal required functionality to make
    some games on the Acorn Atom work (basically just timers, and even those
    or likely not correct).

    ## zlib/libpng license

    Copyright (c) 2018 Andre Weissflog
    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from the
    use of this software.
    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:
        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software in a
        product, an acknowledgment in the product documentation would be
        appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not
        be misrepresented as being the original software.
        3. This notice may not be removed or altered from any source
        distribution.
#*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdint.h>
#include <stdbool.h>

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ATOMMC_DEBUG

/* control pins */
#define ATOMMC_RW    (1ULL<<24)      /* RW pin is on same location as M6502 RW pin */
#define ATOMMC_CS    (1ULL<<40)      /* chip-select, active high */

/* register select same as lower 4 shared address bus bits */
#define ATOMMC_A0   (1ULL<<0)
#define ATOMMC_A1   (1ULL<<1)
#define ATOMMC_A2   (1ULL<<2)
#define ATOMMC_A3   (1ULL<<3)
#define ATOMMC_A    (ATOMMC_A0|ATOMMC_A1|ATOMMC_A2|ATOMMC_A3)

/* data bus pins shared with CPU */
#define ATOMMC_D0    (1ULL<<16)
#define ATOMMC_D1    (1ULL<<17)
#define ATOMMC_D2    (1ULL<<18)
#define ATOMMC_D3    (1ULL<<19)
#define ATOMMC_D4    (1ULL<<20)
#define ATOMMC_D5    (1ULL<<21)
#define ATOMMC_D6    (1ULL<<22)
#define ATOMMC_D7    (1ULL<<23)

#define ATOMMC_CMD_REG                     (0)
#define ATOMMC_LATCH_REG                   (1)
#define ATOMMC_READ_DATA_REG               (2)
#define ATOMMC_WRITE_DATA_REG              (3)

// DIR_CMD_REG commands
#define ATOMMC_CMD_DIR_OPEN                (0x00)
#define ATOMMC_CMD_DIR_READ                (0x01)
#define ATOMMC_CMD_DIR_CWD                 (0x02)
#define ATOMMC_CMD_DIR_GETCWD              (0x03)
#define ATOMMC_CMD_DIR_MKDIR               (0x04)
#define ATOMMC_CMD_DIR_RMDIR               (0x05)

// CMD_REG_COMMANDS
#define ATOMMC_CMD_FILE_CLOSE              (0x10)
#define ATOMMC_CMD_FILE_OPEN_READ          (0x11)
#define ATOMMC_CMD_FILE_OPEN_IMG           (0x12)
#define ATOMMC_CMD_FILE_OPEN_WRITE         (0x13)
#define ATOMMC_CMD_FILE_DELETE             (0x14)
#define ATOMMC_CMD_FILE_GETINFO            (0x15)
#define ATOMMC_CMD_FILE_SEEK               (0x16)
#define ATOMMC_CMD_FILE_OPEN_RAF           (0x17)

#define ATOMMC_CMD_INIT_READ               (0x20)
#define ATOMMC_CMD_INIT_WRITE              (0x21)
#define ATOMMC_CMD_READ_BYTES              (0x22)
#define ATOMMC_CMD_WRITE_BYTES             (0x23)

// READ_DATA_REG "commands"

// EXEC_PACKET_REG "commands"
#define ATOMMC_CMD_EXEC_PACKET             (0x3F)

// SDOS_LBA_REG commands
#define ATOMMC_CMD_LOAD_PARAM              (0x40)
#define ATOMMC_CMD_GET_IMG_STATUS          (0x41)
#define ATOMMC_CMD_GET_IMG_NAME            (0x42)
#define ATOMMC_CMD_READ_IMG_SEC            (0x43)
#define ATOMMC_CMD_WRITE_IMG_SEC           (0x44)
#define ATOMMC_CMD_SER_IMG_INFO            (0x45)
#define ATOMMC_CMD_VALID_IMG_NAMES         (0x46)
#define ATOMMC_CMD_IMG_UNMOUNT             (0x47)

// Utility commands
#define ATOMMC_CMD_GET_CARD_TYPE           (0x80)
#define ATOMMC_CMD_GET_PORT_DDR            (0xA0)
#define ATOMMC_CMD_SET_PORT_DDR            (0xA1)
#define ATOMMC_CMD_READ_PORT               (0xA2)
#define ATOMMC_CMD_WRITE_PORT              (0xA3)
#define ATOMMC_CMD_GET_FW_VER              (0xE0)
#define ATOMMC_CMD_GET_BL_VER              (0xE1)
#define ATOMMC_CMD_GET_CFG_BYTE            (0xF0)
#define ATOMMC_CMD_SET_CFG_BYTE            (0xF1)
#define ATOMMC_CMD_READ_AUX                (0xFD)
#define ATOMMC_CMD_GET_HEARTBEAT           (0xFE)


// Status codes
#define ATOMMC_STATUS_OK                   (0x3F)
#define ATOMMC_STATUS_COMPLETE             (0x40)
#define ATOMMC_STATUS_BUSY                 (0x80)

#define ATOMMC_ERROR_MASK                  (0x3F)

// To be or'd with STATUS_COMPLETE
#define ATOMMC_ERROR_NO_DATA               (0x08)
#define ATOMMC_ERROR_INVALID_DRIVE         (0x09)
#define ATOMMC_ERROR_READ_ONLY             (0x0A)
#define ATOMMC_ERROR_ALREADY_MOUNT         (0x0A)
#define ERROR_TOO_MANY_OPEN                (0x12)

// Offset returned file numbers by 0x20, to disambiguate from errors
#define FILENUM_OFFSET		               (0x20)

#define ATOMMC_CT_MMC   0x01            /* MMC ver 3 */
#define ATOMMC_CT_SD1   0x02            /* SD ver 1 */
#define ATOMMC_CT_SD2   0x04            /* SD ver 2 */
#define ATOMMC_CT_SDC   (CT_SD1|CT_SD2) /* SD */
#define ATOMMC_CT_BLOCK 0x08            /* Block addressing */


typedef enum {
    FR_OK = 0,          /* 0 */
    FR_DISK_ERR,        /* 1 */
    FR_INT_ERR,         /* 2 */
    FR_NOT_READY,       /* 3 */
    FR_NO_FILE,         /* 4 */
    FR_NO_PATH,         /* 5 */
    FR_INVALID_NAME,    /* 6 */
    FR_DENIED,          /* 7 */
    FR_EXIST,           /* 8 */
    FR_INVALID_OBJECT,  /* 9 */
    FR_WRITE_PROTECTED, /* 10 */
    FR_INVALID_DRIVE,   /* 11 */
    FR_NOT_ENABLED,     /* 12 */
    FR_NO_FILESYSTEM,   /* 13 */
    FR_MKFS_ABORTED,    /* 14 */
    FR_TIMEOUT          /* 15 */
} FRESULT;

typedef uint8_t (*atommc_in_t)(int port_id, void* user_data);
typedef void (*atommc_out_t)(int port_id, uint8_t data, void* user_data);

/* atommc initialization parameters */
typedef struct {
    atommc_in_t in_cb;
    atommc_out_t out_cb;
    void* user_data;
} atommc_desc_t;

#define MAX_FILENAME  20
#define MAX_FILEPATH 100
#define MAX_DIRSIZE  100
#define MAX_FD         4

/* atommc state */
typedef struct {
   uint8_t cmd;
   uint8_t latch;
   uint8_t address;
   uint8_t heartbeat;
   uint8_t response;
   uint8_t cfg_byte;
   uint8_t port_tris;
   uint8_t port_data;
   uint8_t global_data[0x100];
   uint8_t global_index;
   FILE *fd[MAX_FD];
   char cwd[MAX_FILEPATH];
   // State about the currently loaded directory
   int dir_size;
   int dir_index;
   char *dirsorted[MAX_DIRSIZE];
   char dirlist[MAX_DIRSIZE][MAX_FILENAME];
   atommc_in_t in_cb;
   atommc_out_t out_cb;
   void* user_data;
   uint64_t pins;          /* last pin value for debug inspection */
} atommc_t;

/* extract 8-bit data bus from 64-bit pins */
#define ATOMMC_GET_DATA(p) ((uint8_t)(p>>16))
/* merge 8-bit data bus value into 64-bit pins */
#define ATOMMC_SET_DATA(p,d) {p=((p&~0xFF0000)|(((d)&0xFF)<<16));}
/* merge 4-bit address into 64-bit pins */
#define ATOMMC_SET_ADDR(p,d) {p=((p&~0xF)|((d)&0xF));}

/* initialize a new atommc instance */
void atommc_init(atommc_t* atommc, const atommc_desc_t* desc);

/* reset an existing atommc instance */
void atommc_reset(atommc_t* atommc);

/* perform an IO request */
uint64_t atommc_iorq(atommc_t* atommc, uint64_t pins);

/* tick the atommc */
void atommc_tick(atommc_t* atommc);

#ifdef __cplusplus
} /* extern "C" */
#endif

/*-- IMPLEMENTATION ----------------------------------------------------------*/
#ifdef CHIPS_IMPL
#include <string.h>
#ifndef CHIPS_ASSERT
#include <assert.h>
#define CHIPS_ASSERT(c) assert(c)
#endif

void atommc_init(atommc_t* atommc, const atommc_desc_t* desc) {
   CHIPS_ASSERT(atommc && desc);
   memset(atommc, 0, sizeof(*atommc));
   atommc->in_cb = desc->in_cb;
   atommc->out_cb = desc->out_cb;
   atommc->user_data = desc->user_data;
   atommc_reset(atommc);
   atommc->cfg_byte = 0xE0;
   // All the files are packaged in a subdirectory called mmc
   if (chdir("mmc")) {
#ifdef ATOMMC_DEBUG
      printf("failed to change to mmc subdirectory\n");
#endif
   }
}

void atommc_reset(atommc_t* atommc) {
   CHIPS_ASSERT(atommc);
   atommc->heartbeat = 0x55;
   strcpy(atommc->cwd, ".");
}

char *getfilename(atommc_t* atommc) {
   static char buffer[1000];
   if (atommc->global_data[0]) {
      sprintf(buffer, "%s/%s", atommc->cwd, (char *)atommc->global_data);
#ifdef ATOMMC_DEBUG
      printf("%s\n", buffer);
#endif
      return buffer;
   } else {
      return 0;
   }
}

static int cmpstringp(const void *p1, const void *p2) {
   /* The actual arguments to this function are "pointers to
      pointers to char", but strcmp(3) arguments are "pointers
      to char", hence the following cast plus dereference */
   return strcmp(* (char * const *) p1, * (char * const *) p2);
}

static int findFile(atommc_t* atommc, int filenum) {
   if (filenum > 0) {
      filenum = -1;
      if (!atommc->fd[1]) {
         filenum = 1;
      } else if (!atommc->fd[2]) {
         filenum = 2;
      } else if (!atommc->fd[3]) {
         filenum = 3;
      }
   }
   if (filenum == -1) {
      atommc->response = ATOMMC_STATUS_COMPLETE | ERROR_TOO_MANY_OPEN;
   }
   return filenum;
}

static void _atommc_write(atommc_t* atommc, uint8_t addr, uint8_t data) {
   int filenum = 0;
   FILE **fdp;

   // Latch the address only on writes
   atommc->address = addr & 3;

   switch (addr & 3) {

   case ATOMMC_CMD_REG:

      // Deal with random access files

      // File Group 0x10-0x17, 0x30-0x37, 0x50-0x57, 0x70-0x77
      // filenum = bits 6,5
      // mask1 = 10011000 (test for file group command)
      // mask2 = 10011111 (remove file number)
      if ((data & 0x98) == 0x10) {
         filenum = (data >> 5) & 3;
         data &= 0x9F;
      }

      // Data Group 0x20-0x23, 0x24-0x27, 0x28-0x2B, 0x2C-0x2F
      // filenum = bits 3,2
      // mask1 = 11110000 (test for data group command)
      // mask2 = 11110011 (remove file number)
      if ((data & 0xf0) == 0x20) {
         filenum = (data >> 2) & 3;
         data &= 0xF3;
      }

      fdp = &atommc->fd[filenum];

      atommc->cmd = data;

      // Assume all commands are slow commands
      atommc->response = ATOMMC_STATUS_BUSY;

#ifdef ATOMMC_DEBUG
      printf("atommc: cmd=%02x\n", atommc->cmd);
#endif

      switch (atommc->cmd) {

      case ATOMMC_CMD_DIR_OPEN:
         {
            // Cache the directory entries, in sorted order
            DIR *dir = opendir(atommc->cwd);
            if (dir) {
               struct dirent *entry;
               int i = 0;
               while (i < MAX_DIRSIZE && (entry = readdir(dir)) != NULL) {
                  char *ptr = atommc->dirlist[i];
                  if (entry->d_type == DT_DIR) {
                     *ptr++ = '<';
                  }
                  strcpy(ptr, entry->d_name);
                  ptr = ptr + strlen(entry->d_name);
                  if (entry->d_type == DT_DIR) {
                     *ptr++ = '>';
                     *ptr++ = 0;
                  }
#ifdef ATOMMC_DEBUG
                  printf("dir:%s\n", atommc->dirlist[i]);
#endif
                  atommc->dirsorted[i] = atommc->dirlist[i];
                  i++;
               }
               closedir(dir);
               qsort(atommc->dirsorted, i, sizeof(char *), cmpstringp);
               atommc->dir_size = i;
               atommc->dir_index = 0;
               atommc->response = ATOMMC_STATUS_OK;
            } else {
               atommc->response = ATOMMC_STATUS_COMPLETE | FR_NO_PATH;
            }
         }
         break;

      case ATOMMC_CMD_DIR_READ:
         {
            // Return the next name from the caches directory
            if (atommc->dir_index < atommc->dir_size) {
               memset(atommc->global_data, 0, 0x100);
               char *name = atommc->dirsorted[atommc->dir_index];
               strcpy((char *)atommc->global_data, name);
               // TODO: Attribute Byte + Length should follow the name
               atommc->dir_index++;
               atommc->response = ATOMMC_STATUS_OK;
#ifdef ATOMMC_DEBUG
               printf("dir: %d %s\n", atommc->dir_index, name);
#endif
            } else {
               atommc->global_data[0] = 0;
               atommc->response = ATOMMC_STATUS_COMPLETE;
            }
         }
         break;

      case ATOMMC_CMD_DIR_CWD:
         {
            // Form the new directory name
            char *dirname = getfilename(atommc);

            // Test if it's a directory
            DIR *dir = opendir(dirname);
            if (dir) {
               strcpy(atommc->cwd, dirname);
               closedir(dir);
               atommc->response = ATOMMC_STATUS_COMPLETE;
            } else {
               atommc->response = ATOMMC_STATUS_COMPLETE | FR_NO_PATH;
            }
         }
         break;

      case ATOMMC_CMD_DIR_GETCWD:
         // This is never used by the AtoMMC filesystem ROM
         atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
         break;

      case ATOMMC_CMD_DIR_MKDIR:
         if (mkdir(getfilename(atommc), 0x755)) {
            atommc->response = ATOMMC_STATUS_COMPLETE | FR_DENIED;
         } else {
            atommc->response = ATOMMC_STATUS_COMPLETE;
         }
         break;

      case ATOMMC_CMD_DIR_RMDIR:
         if (rmdir(getfilename(atommc))) {
            atommc->response = ATOMMC_STATUS_COMPLETE | FR_DENIED;
         } else {
            atommc->response = ATOMMC_STATUS_COMPLETE;
         }
         break;

      case ATOMMC_CMD_FILE_CLOSE:
         atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
         if (*fdp) {
            if (fclose(*fdp) == 0) {
               atommc->response = ATOMMC_STATUS_COMPLETE;
            }
         }
         *fdp = NULL;
         break;

      case ATOMMC_CMD_FILE_OPEN_READ:
         // Search for a free file handle, or respond with Too Many Open Files
         filenum = findFile(atommc, filenum);
         if (filenum >= 0) {
            fdp = &atommc->fd[filenum];
            *fdp = fopen(getfilename(atommc), "r");
            if (*fdp == 0) {
               // File doesn't exist, return an error
               atommc->response = ATOMMC_STATUS_COMPLETE | FR_NO_FILE;
            } else if (filenum) {
               atommc->response = ATOMMC_STATUS_COMPLETE | FILENUM_OFFSET | filenum;
            } else {
               atommc->response = ATOMMC_STATUS_COMPLETE;
            }
         }
         break;

      case ATOMMC_CMD_FILE_OPEN_RAF:
         // Search for a free file handle, or respond with Too Many Open Files
         filenum = findFile(atommc, filenum);
         if (filenum >= 0) {
            fdp = &atommc->fd[filenum];
            // First check if the file already exists
            *fdp = fopen(getfilename(atommc), "r+");
            if (*fdp == 0) {
               // File doesn't exist, create it
               *fdp = fopen(getfilename(atommc), "w+");
            }
            if (*fdp == 0) {
               // File doesn't exist, return an error
               atommc->response = ATOMMC_STATUS_COMPLETE | FR_DENIED;
            } else if (filenum) {
               atommc->response = ATOMMC_STATUS_COMPLETE | FILENUM_OFFSET | filenum;
            } else {
               atommc->response = ATOMMC_STATUS_COMPLETE;
            }
         }
         break;

      case ATOMMC_CMD_FILE_OPEN_WRITE:
         // Search for a free file handle, or respond with Too Many Open Files
         filenum = findFile(atommc, filenum);
         if (filenum >= 0) {
            fdp = &atommc->fd[filenum];
            // First check if the file already exists
            *fdp = fopen(getfilename(atommc), "r");
            if (*fdp == 0) {
               // File doesn't exist, create it
               *fdp = fopen(getfilename(atommc), "w");
               if (*fdp == 0) {
                  atommc->response = ATOMMC_STATUS_COMPLETE | FR_DENIED;
               } else if (filenum) {
                  atommc->response = ATOMMC_STATUS_COMPLETE | FILENUM_OFFSET | filenum;
               } else {
                  atommc->response = ATOMMC_STATUS_COMPLETE;
               }
            } else {
               // File exists, give an error
               fclose(*fdp);
               *fdp = NULL;
               atommc->response = ATOMMC_STATUS_COMPLETE | FR_EXIST;
            }
         }
         break;

      case ATOMMC_CMD_FILE_DELETE:
         if (remove(getfilename(atommc))) {
            atommc->response = ATOMMC_STATUS_COMPLETE | FR_NO_PATH;
         } else {
            atommc->response = ATOMMC_STATUS_COMPLETE;
         }
         break;

      case ATOMMC_CMD_FILE_GETINFO:
         atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
         if (*fdp) {
            struct stat statbuf;
            if (fstat(fileno(*fdp), &statbuf) == 0) {
               // File size
               *(uint32_t *)(atommc->global_data) = statbuf.st_size;
               // Start Sector (TODO)
               *(uint32_t *)(atommc->global_data + 4) = 0;
               // Current random access file pointer
               *(uint32_t *)(atommc->global_data + 8) = ftell(*fdp);
               // File attributes (TODO)
               atommc->global_data[12] = 0;
               atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
            }
         }
         break;

      case ATOMMC_CMD_FILE_SEEK:
         atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
         if (*fdp) {
            int offset = *(uint32_t *)(&atommc->global_data[0]);
            fseek(*fdp, offset, SEEK_SET);
            atommc->response = ATOMMC_STATUS_COMPLETE;
         }
         break;

      case ATOMMC_CMD_INIT_READ:
         atommc->response = atommc->global_data[0];
         atommc->global_index = 1;
         atommc->address = ATOMMC_READ_DATA_REG;
         break;

      case ATOMMC_CMD_INIT_WRITE:
         atommc->global_index = 0;
         break;

      case ATOMMC_CMD_READ_BYTES:
         atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
         if (*fdp) {
            int len = atommc->latch;
            if (len == 0) {
               len = 256;
            }
            if (fread(atommc->global_data, len, 1, *fdp) == 1) {
               atommc->response = ATOMMC_STATUS_COMPLETE;
            }
         }
         break;

      case ATOMMC_CMD_WRITE_BYTES:
         atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
         if (*fdp) {
            int len = atommc->latch;
            if (len == 0) {
               len = 256;
            }
            if (fwrite(atommc->global_data, len, 1, *fdp) == 1) {
               atommc->response = ATOMMC_STATUS_COMPLETE;
            }
         }
         break;

      // This is never used by the AtoMMC filesystem ROM
      case ATOMMC_CMD_EXEC_PACKET:
         atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
         break;

      // SDDOS image commands
      // This is never used by the AtoMMC filesystem ROM

      case ATOMMC_CMD_LOAD_PARAM:
      case ATOMMC_CMD_FILE_OPEN_IMG:
      case ATOMMC_CMD_GET_IMG_STATUS:
      case ATOMMC_CMD_GET_IMG_NAME:
      case ATOMMC_CMD_READ_IMG_SEC:
      case ATOMMC_CMD_WRITE_IMG_SEC:
      case ATOMMC_CMD_SER_IMG_INFO:
      case ATOMMC_CMD_VALID_IMG_NAMES:
      case ATOMMC_CMD_IMG_UNMOUNT:
         atommc->response = ATOMMC_STATUS_COMPLETE | FR_INT_ERR;
         break;

      // Utility commands

      case ATOMMC_CMD_GET_CARD_TYPE:
         // TODO: Initialize the disk at the point, should be a slow cmd
         atommc->response = ATOMMC_CT_SD1;
         break;

      case ATOMMC_CMD_GET_PORT_DDR:
         atommc->response = atommc->port_tris;
         break;

      case ATOMMC_CMD_SET_PORT_DDR:
         atommc->port_tris = atommc->latch;
         atommc->response = ATOMMC_STATUS_OK;
         break;

      case ATOMMC_CMD_READ_PORT:
         atommc->response = atommc->port_data;
         break;

      case ATOMMC_CMD_WRITE_PORT:
         atommc->port_data = atommc->latch;
         atommc->response = ATOMMC_STATUS_OK;
         break;

      case ATOMMC_CMD_GET_FW_VER:
         atommc->response = 0x2D;
         break;

      case ATOMMC_CMD_GET_BL_VER:
         atommc->response = 0x29;
         break;

      case ATOMMC_CMD_GET_CFG_BYTE:
         atommc->response = atommc->cfg_byte;
         break;

      case ATOMMC_CMD_SET_CFG_BYTE:
         atommc->cfg_byte = atommc->latch;
         atommc->response = ATOMMC_STATUS_OK;
         break;

      case ATOMMC_CMD_READ_AUX:
         atommc->response = atommc->address;
         atommc->response = ATOMMC_STATUS_OK;
         break;

      case ATOMMC_CMD_GET_HEARTBEAT:
         atommc->heartbeat ^= 0xff;
         atommc->response = atommc->heartbeat;
         break;

      }
      break;

   case ATOMMC_LATCH_REG:
#ifdef ATOMMC_DEBUG
      printf("atommc: latch=%02x\n", data);
#endif
      atommc->latch = data;
      atommc->response = data;
      break;

   case ATOMMC_WRITE_DATA_REG:
#ifdef ATOMMC_DEBUG
      printf("atommc: global=%02x\n", data);
#endif
      atommc->global_data[atommc->global_index++] = data;
      break;
   }
}

static uint8_t _atommc_read(atommc_t* atommc, uint8_t addr) {
   uint8_t data = atommc->response;
   if (atommc->address == ATOMMC_READ_DATA_REG) {
      atommc->response = atommc->global_data[atommc->global_index++];
   } else {
#ifdef ATOMMC_DEBUG
      printf("atommc: resp=%02x\n", data);
#endif
   }
   return data;
}

uint64_t atommc_iorq(atommc_t* atommc, uint64_t pins) {
    if ((pins & (ATOMMC_CS)) == ATOMMC_CS) {
        uint8_t addr = pins & ATOMMC_A;
        if (pins & ATOMMC_RW) {
            /* a read operation */
            uint8_t data = _atommc_read(atommc, addr);
            ATOMMC_SET_DATA(pins, data);
        }
        else {
            /* a write operation */
            uint8_t data = ATOMMC_GET_DATA(pins);
            _atommc_write(atommc, addr, data);
        }
        atommc->pins = pins;
    }
    return pins;
}

void atommc_tick(atommc_t* atommc) {
}

#endif /* CHIPS_IMPL */
