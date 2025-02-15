#pragma once
/*#
    # atommc

    Header-only AtoMMC File System emulator written in C.

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

    - This is a functional emulation only, i.e. commands execute instantaneously
    - The SDDOS disk images commands are currently not implemented

    ## zlib/libpng license

    Copyright (c) 2019 David Banks (hoglet)
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

// #define ATOMMC_DEBUG

/* Control pins */
#define ATOMMC_RW    (1ULL<<24)      /* RW pin is on same location as M6502 RW pin */
#define ATOMMC_CS    (1ULL<<40)      /* chip-select, active high */

/* Register select same as lower 4 shared address bus bits */
#define ATOMMC_A0   (1ULL<<0)
#define ATOMMC_A1   (1ULL<<1)
#define ATOMMC_A2   (1ULL<<2)
#define ATOMMC_A3   (1ULL<<3)
#define ATOMMC_A    (ATOMMC_A0|ATOMMC_A1|ATOMMC_A2|ATOMMC_A3)

/* Data bus pins shared with CPU */
#define ATOMMC_D0    (1ULL<<16)
#define ATOMMC_D1    (1ULL<<17)
#define ATOMMC_D2    (1ULL<<18)
#define ATOMMC_D3    (1ULL<<19)
#define ATOMMC_D4    (1ULL<<20)
#define ATOMMC_D5    (1ULL<<21)
#define ATOMMC_D6    (1ULL<<22)
#define ATOMMC_D7    (1ULL<<23)

/* AtoMMC Registers */
#define ATOMMC_CMD_REG                     (0)
#define ATOMMC_LATCH_REG                   (1)
#define ATOMMC_READ_DATA_REG               (2)
#define ATOMMC_WRITE_DATA_REG              (3)

/* AtoMMC Directory Commands */
#define ATOMMC_CMD_DIR_OPEN                (0x00)
#define ATOMMC_CMD_DIR_READ                (0x01)
#define ATOMMC_CMD_DIR_CWD                 (0x02)
#define ATOMMC_CMD_DIR_GETCWD              (0x03)
#define ATOMMC_CMD_DIR_MKDIR               (0x04)
#define ATOMMC_CMD_DIR_RMDIR               (0x05)

/* AtoMMC File Commands */
#define ATOMMC_CMD_FILE_CLOSE              (0x10)
#define ATOMMC_CMD_FILE_OPEN_READ          (0x11)
#define ATOMMC_CMD_FILE_OPEN_IMG           (0x12)
#define ATOMMC_CMD_FILE_OPEN_WRITE         (0x13)
#define ATOMMC_CMD_FILE_DELETE             (0x14)
#define ATOMMC_CMD_FILE_GETINFO            (0x15)
#define ATOMMC_CMD_FILE_SEEK               (0x16)
#define ATOMMC_CMD_FILE_OPEN_RAF           (0x17)

/* AtoMMC Data Transfer Commands */
#define ATOMMC_CMD_INIT_READ               (0x20)
#define ATOMMC_CMD_INIT_WRITE              (0x21)
#define ATOMMC_CMD_READ_BYTES              (0x22)
#define ATOMMC_CMD_WRITE_BYTES             (0x23)

/* AtoMMC Execute Arbitrary Command */
#define ATOMMC_CMD_EXEC_PACKET             (0x3F)

/* SDDOS disk image Commands */
#define ATOMMC_CMD_LOAD_PARAM              (0x40)
#define ATOMMC_CMD_GET_IMG_STATUS          (0x41)
#define ATOMMC_CMD_GET_IMG_NAME            (0x42)
#define ATOMMC_CMD_READ_IMG_SEC            (0x43)
#define ATOMMC_CMD_WRITE_IMG_SEC           (0x44)
#define ATOMMC_CMD_SER_IMG_INFO            (0x45)
#define ATOMMC_CMD_VALID_IMG_NAMES         (0x46)
#define ATOMMC_CMD_IMG_UNMOUNT             (0x47)

/* Utility Commands */
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

/* Status Codes */
#define ATOMMC_STATUS_OK                   (0x3F)
#define ATOMMC_STATUS_COMPLETE             (0x40)
#define ATOMMC_STATUS_EOF                  (0x60)
#define ATOMMC_STATUS_BUSY                 (0x80)

/* Error Codes */
#define ATOMMC_ERROR_INT_ERR               (0x42)
#define ATOMMC_ERROR_NO_FILE               (0x44)
#define ATOMMC_ERROR_NO_PATH               (0x45)
#define ATOMMC_ERROR_DENIED                (0x47)
#define ATOMMC_ERROR_EXIST                 (0x48)
#define ATOMMC_ERROR_TOO_MANY_OPEN         (0x52)

/* Offset returned file numbers by 0x20, to disambiguate from errors */
#define FILENUM_OFFSET                     (0x20)

/* SD Card Types */
#define ATOMMC_CT_MMC                      (0x01) /* MMC ver 3 */
#define ATOMMC_CT_SD1                      (0x02) /* SD ver 1 */
#define ATOMMC_CT_SD2                      (0x04) /* SD ver 2 */
#define ATOMMC_CT_SDC             (CT_SD1|CT_SD2) /* SD */
#define ATOMMC_CT_BLOCK                    (0x08) /* Block addressing */

#define ATOMMC_CT_DEFAULT          ATOMMC_CT_SD1

   typedef uint8_t (*atommc_in_t)(int port_id, void* user_data);

typedef void (*atommc_out_t)(int port_id, uint8_t data, void* user_data);

/* AtoMMC initialization parameters */
typedef struct {
    atommc_in_t in_cb;
    atommc_out_t out_cb;
    void* user_data;
    bool autoboot;
} atommc_desc_t;

/* Limits on file/directory lengths */
#define MAX_FILENAME                         (20)
#define MAX_FILEPATH                        (200)
#define MAX_DIRSIZE                         (100)
#define MAX_FD                                (4)
#define MAX_GLOBAL                        (0x100)
#define WILD_LEN                             (16)


/* AtoMMC File Attributes */
#define ATOMMC_ATTR_HIDDEN                 (0x02)
#define ATOMMC_ATTR_DIR                    (0x10)

// AtoMMC supports three types of file open
enum {
   ATOMMC_MODE_READ,
   ATOMMC_MODE_WRITE,
   ATOMMC_MODE_RAF
};

/* AtoMMC Directory Entry */
typedef struct {
   char name[MAX_FILENAME];
   uint8_t attr;
   uint32_t len;
} atommc_dirent_t;

/* AtoMMC state */
typedef struct {
   /* Command parameter latch */
   uint8_t latch;
   /* Last write address */
   uint8_t address;
   /* Heartbeat state */
   uint8_t heartbeat;
   /* Command response */
   uint8_t response;
   /* Config Byte (Shift-break behaviour) */
   uint8_t cfg_byte;
   /* 8-bit I/O Port */
   uint8_t port_tris;
   uint8_t port_data;
   /* Global Data */
   uint8_t global_data[MAX_GLOBAL];
   uint8_t global_index;
   /* Pool of file descriptors */
   FILE *fd[MAX_FD];
   /* Relative pah of current working directory */
   char cwd[MAX_FILEPATH];
   /* Currently loaded directory */
   int dir_size;
   int dir_index;
   atommc_dirent_t *dirsorted[MAX_DIRSIZE];
   atommc_dirent_t dirlist[MAX_DIRSIZE];
   /* Current wildcard */
   char wildPattern[WILD_LEN + 1];
   /* Standard emulator callbacks, etc */
   atommc_in_t in_cb;
   atommc_out_t out_cb;
   void* user_data;
   uint64_t pins;          /* last pin value for debug inspection */
} atommc_t;

/* extract 8-bit data bus from 64-bit pins */
#define ATOMMC_GET_DATA(p) ((uint8_t)(p>>16))

/* merge 8-bit data bus value into 64-bit pins */
#define ATOMMC_SET_DATA(p,d) {p=((p&~0xFF0000)|(((d)&0xFF)<<16));}

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
   atommc->cfg_byte = desc->autoboot ? 0xA0 : 0xE0;
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
   // Close any open files
   for (int i = 0; i < MAX_FD; i++) {
      if (atommc->fd[i]) {
         fclose(atommc->fd[i]);
         atommc->fd[i] = NULL;
      }
   }
   // Reset CWD to the root
   strcpy(atommc->cwd, ".");
}

// Construct a complete file path from the string in the global data area
static char *getFilename(atommc_t* atommc) {
   static char buffer[MAX_FILEPATH];
   // Strip any leading / characters
   int index = 0;
   while (atommc->global_data[index] == '/') {
      index++;
   }
   if (index) {
      // Path is absolute
      sprintf(buffer, "./%s", (char *)atommc->global_data + index);
   } else {
      // Path is relative to cwd
      sprintf(buffer, "%s/%s", atommc->cwd, (char *)atommc->global_data);
   }
#ifdef ATOMMC_DEBUG
   printf("%s\n", buffer);
#endif
   return buffer;
}

// Compare two directory entries, based on their names
static int cmpstringp(const void *p1, const void *p2) {
   atommc_dirent_t *d1 = (atommc_dirent_t *)p1;
   atommc_dirent_t *d2 = (atommc_dirent_t *)p2;
   return strcmp(* (char * const *) d1->name, * (char * const *) d2->name);
}

// Case 1: Open Read
// if exists and regular, mode = "r"
// if exists and not regular, error = "DENIED"
// if not exists, error "NOT FOUND"

// Case 2: Open Write
// if exists and regular, error = "EXISTS"
// if exists and not regular, error = "DENIED"
// if not exists, mode ="w"

// Case 3: Open RAF
// if exists and regular, mode = r+
// if exists and not regulat=r, error = "DENIED"
// if not exists, mode = w+

static void openFile(atommc_t* atommc, int filenum, int open_mode) {
   // Response defaults to internal error, should never occur
   atommc->response = ATOMMC_ERROR_INT_ERR;
   // Construct the filename
   char *filename = getFilename(atommc);
   // Stat the file
   struct stat statbuf;
   bool exists = !stat(filename, &statbuf);
   bool regular = exists && (statbuf.st_mode & S_IFREG);

   // This error is common to all three modes
   // and will typical mean trying read a directory
   if (exists && !regular) {
      atommc->response = ATOMMC_ERROR_DENIED;
      return;
   }
   // Initial checks, and pick the right mode to match AtoMMC semantics
   char *mode;
   switch (open_mode) {
   case ATOMMC_MODE_READ:
      if (exists) {
         mode = "r";
      } else {
         atommc->response = ATOMMC_ERROR_NO_FILE;
         return;
      }
      break;
   case ATOMMC_MODE_WRITE:
      if (exists) {
         atommc->response = ATOMMC_ERROR_EXIST;
         return;
      } else {
         mode = "w";
      }
      break;
   case ATOMMC_MODE_RAF:
      if (exists) {
         mode = "r+";
      } else {
         mode = "w+";
      }
      break;
   default:
      return;
   }
   // Find a free file number
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
      atommc->response = ATOMMC_ERROR_TOO_MANY_OPEN;
      return;
   }
   // Try to open the file
   FILE **fdp = &atommc->fd[filenum];
   *fdp = fopen(getFilename(atommc), mode);
   if (*fdp == 0) {
      atommc->response = ATOMMC_ERROR_DENIED;
      return;
   }
   // And return the appropriate response
   if (*fdp == 0) {
      atommc->response = ATOMMC_ERROR_DENIED;
   } else if (filenum) {
      atommc->response = ATOMMC_STATUS_COMPLETE | FILENUM_OFFSET | filenum;
   } else {
      atommc->response = ATOMMC_STATUS_COMPLETE;
   }
}

// Parse a file spec into a file path and an optional wildcard
//
// Used by *CAT, *INFO and *DELETE, e.g.
//
// *INFO  ([directory path]/)... ([file path])
// or
// *INFO  ([directory path]/)... ([wildcard pattern])
//
// This code is taken from the actual AtoMMC implementation

void parseWildcard(atommc_t *atommc) {
   unsigned long idx = 0;
   int wildPos = -1;
   int lastSlash = -1;

   while ((idx < strlen((const char*)atommc->global_data)) && (wildPos < 0)) {
      // Check for wildcard character
      if((atommc->global_data[idx]=='?') || (atommc->global_data[idx]=='*'))
         wildPos=idx;

      // Check for path seperator
      if((atommc->global_data[idx]=='\\') || (atommc->global_data[idx]=='/'))
         lastSlash=idx;

      idx++;
   }

   if (wildPos > -1) {
      if (lastSlash > -1) {
         // Path followed by wildcard
         // Terminate dir filename at last slash and copy wildcard
         atommc->global_data[lastSlash]=0x00;
         strncpy(atommc->wildPattern,(const char*)&atommc->global_data[lastSlash+1],WILD_LEN);
      } else {
         // Wildcard on it's own
         // Copy wildcard, then set path to null
         strncpy(atommc->wildPattern,(const char*)atommc->global_data,WILD_LEN);
         atommc->global_data[0]=0x00;
      }
   } else {
      // No wildcard, show all files
      strcpy(atommc->wildPattern, "*");
   }
}

// Test whether a string matches a wildcard pattern
// This code is taken from the actual AtoMMC implementation

static int wildcmp(const char *wild, const char *string)  {
   // Written by Jack Handy - jakkhandy@hotmail.com
   // http://www.codeproject.com/KB/string/wildcmp.aspx

   const char *cp = NULL;
   const char *mp = NULL;

   while ((*string) && (*wild != '*')) {
      if ((*wild != *string) && (*wild != '?')) {
         return 0;
      }
      wild++;
      string++;
   }

   while (*string) {
      if (*wild == '*') {
         if (!*++wild) {
            return 1;
         }
         mp = wild;
         cp = string+1;
      } else if ((*wild == *string) || (*wild == '?')) {
         wild++;
         string++;
      } else {
         wild = mp;
         string = cp++;
      }
   }

   while (*wild == '*') {
      wild++;
   }
   return !*wild;
}

// Handle writes to the following registers:
//   ATOMMC_CMD_REG
//   ATOMMC_LATCH_REG
//   ATOMMC_WRITE_DATA_REG
//
// Note, ATOMMC_READ_DATA_REG is read only, so is ignored here

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

      uint8_t cmd = data;

      // Assume all commands are slow commands
      atommc->response = ATOMMC_STATUS_BUSY;

#ifdef ATOMMC_DEBUG
      printf("atommc: cmd=%02x\n", cmd);
#endif

      switch (cmd) {

      case ATOMMC_CMD_DIR_OPEN:
         {
            // Separate wildcard and path
            parseWildcard(atommc);

#ifdef ATOMMC_DEBUG
            printf("wildcard = %s\n", atommc->wildPattern);
            printf("path = %s\n", getFilename(atommc));
#endif

            // Cache the directory entries, in sorted order
            DIR *dir = opendir(getFilename(atommc));
            if (dir) {
               struct dirent *entry;
               int i = 0;
               while (i < MAX_DIRSIZE && (entry = readdir(dir)) != NULL) {

                  uint8_t attr = 0;

                  if (!wildcmp(atommc->wildPattern, entry->d_name)) {
                     continue;
                  }

                  char *ptr = atommc->dirlist[i].name;
                  if (entry->d_type == DT_DIR) {
                     attr |= ATOMMC_ATTR_DIR;
                     *ptr++ = '<';
                  }

                  strcpy(ptr, entry->d_name);
                  ptr = ptr + strlen(entry->d_name);
                  if (entry->d_type == DT_DIR) {
                     *ptr++ = '>';
                     *ptr++ = 0;
                  }

                  // TODO: populate the length field
                  atommc->dirlist[i].len = 0;

                  atommc->dirlist[i].attr = attr;
#ifdef ATOMMC_DEBUG
                  printf("dirent name:%s\n",   atommc->dirlist[i].name);
                  printf("dirent  len:%d\n",   atommc->dirlist[i].len);
                  printf("dirent attr:%02x\n", atommc->dirlist[i].attr);
#endif
                  atommc->dirsorted[i] = &atommc->dirlist[i];
                  i++;
               }
               closedir(dir);
               qsort(atommc->dirsorted, i, sizeof(char *), cmpstringp);
               atommc->dir_size = i;
               atommc->dir_index = 0;
               atommc->response = ATOMMC_STATUS_OK;
            } else {
               atommc->response = ATOMMC_ERROR_NO_PATH;
            }
         }
         break;

      case ATOMMC_CMD_DIR_READ:
         {
            // Return the next name from the caches directory
            if (atommc->dir_index < atommc->dir_size) {
               memset(atommc->global_data, 0, sizeof(atommc->global_data));
               char *name = atommc->dirsorted[atommc->dir_index]->name;
               strcpy((char *)atommc->global_data, name);
               // Additional metadata folloes the name
               uint8_t *ptr = (uint8_t *)(atommc->global_data) + strlen(name) + 1;
               // Copy the attribute byte
               *ptr++ = atommc->dirsorted[atommc->dir_index]->attr;
               // Copy the length
               uint32_t len = atommc->dirsorted[atommc->dir_index]->len;
               for (int i = 0; i < 4; i++) {
                  *ptr++ = len & 0xff;
                  len >>= 8;
               }
               // Move on to the entry
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
            char *dirname = getFilename(atommc);

            // Test if it's a directory
            DIR *dir = opendir(dirname);
            if (dir) {
               strcpy(atommc->cwd, dirname);
               closedir(dir);
               atommc->response = ATOMMC_STATUS_COMPLETE;
            } else {
               atommc->response = ATOMMC_ERROR_NO_PATH;
            }
         }
         break;

      case ATOMMC_CMD_DIR_GETCWD:
         // This is never used by the AtoMMC filesystem ROM
         atommc->response = ATOMMC_ERROR_INT_ERR;
         break;

      case ATOMMC_CMD_DIR_MKDIR:
         if (mkdir(getFilename(atommc), 0x755)) {
            atommc->response = ATOMMC_ERROR_DENIED;
         } else {
            atommc->response = ATOMMC_STATUS_COMPLETE;
         }
         break;

      case ATOMMC_CMD_DIR_RMDIR:
         if (rmdir(getFilename(atommc))) {
            atommc->response = ATOMMC_ERROR_DENIED;
         } else {
            atommc->response = ATOMMC_STATUS_COMPLETE;
         }
         break;

      case ATOMMC_CMD_FILE_CLOSE:
         atommc->response = ATOMMC_ERROR_INT_ERR;
         if (*fdp) {
            if (fclose(*fdp) == 0) {
               atommc->response = ATOMMC_STATUS_COMPLETE;
            }
         }
         *fdp = NULL;
         break;

      case ATOMMC_CMD_FILE_OPEN_READ:
         openFile(atommc, filenum, ATOMMC_MODE_READ);
         break;

      case ATOMMC_CMD_FILE_OPEN_RAF:
         openFile(atommc, filenum, ATOMMC_MODE_RAF);
         break;

      case ATOMMC_CMD_FILE_OPEN_WRITE:
         openFile(atommc, filenum, ATOMMC_MODE_WRITE);
         break;

      case ATOMMC_CMD_FILE_DELETE:
         if (remove(getFilename(atommc))) {
            atommc->response = ATOMMC_ERROR_NO_PATH;
         } else {
            atommc->response = ATOMMC_STATUS_COMPLETE;
         }
         break;

      case ATOMMC_CMD_FILE_GETINFO:
         atommc->response = ATOMMC_ERROR_INT_ERR;
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
               atommc->response = ATOMMC_ERROR_INT_ERR;
            }
         }
         break;

      case ATOMMC_CMD_FILE_SEEK:
         atommc->response = ATOMMC_ERROR_INT_ERR;
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
         atommc->response = ATOMMC_ERROR_INT_ERR;
         if (*fdp) {
            int len = atommc->latch;
            if (len == 0) {
               len = 256;
            }
            if (fread(atommc->global_data, len, 1, *fdp) == 1) {
               atommc->response = ATOMMC_STATUS_COMPLETE;
            } else if (feof(*fdp)) {
               atommc->response = ATOMMC_STATUS_EOF;
            } else {
               atommc->response = ATOMMC_ERROR_DENIED;
            }
         }
         break;

      case ATOMMC_CMD_WRITE_BYTES:
         atommc->response = ATOMMC_ERROR_INT_ERR;
         if (*fdp) {
            int len = atommc->latch;
            if (len == 0) {
               len = 256;
            }
            if (fwrite(atommc->global_data, len, 1, *fdp) == 1) {
               atommc->response = ATOMMC_STATUS_COMPLETE;
            } else {
               atommc->response = ATOMMC_ERROR_DENIED;
            }
         }
         break;

      // This is never used by the AtoMMC filesystem ROM
      case ATOMMC_CMD_EXEC_PACKET:
         atommc->response = ATOMMC_ERROR_INT_ERR;
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
         atommc->response = ATOMMC_ERROR_INT_ERR;
         break;

      // Utility commands

      case ATOMMC_CMD_GET_CARD_TYPE:
         atommc->response = ATOMMC_CT_DEFAULT;
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

// Handle raads
//
// The read address is actually ignored (the hardware actually
// works like this).
//
// If the last command was INIT_READ then, the next byte from the
// global data area is returned.
//
// Otherwise the last command response is returned.

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

// Handle reads or writes

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

// Tick is currently empty, as this is a purely functional emulation

void atommc_tick(atommc_t* atommc) {
}

#endif /* CHIPS_IMPL */
