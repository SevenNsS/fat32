#include <fcntl.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "fat.h"
typedef struct {
    uint8_t LDIR_Ord;
    uint16_t LDIR_Name1[5];
    uint8_t LDIR_Attr;
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    uint16_t LDIR_Name2[6];
    uint16_t LDIR_FstClusLO;
    uint16_t LDIR_Name3[2];
} LongDirEntry;

/*
 * Check if the given disk image is a FAT32 disk image.
 * Return true if it is, false otherwise.
 *
 * WARNING: THIS FUNCTION IS FOR DEMONSTRATION PURPOSES ONLY!
 * This function uses an external command called "file" to check the file
 * system, but your program shouldn't rely on outside programs. You need to
 * create your own function to check the file system type and delete this
 * function before submitting. If you use this function in your submission,
 * you'll get a penalty.
 *
 * RTFM: Section 3.5
 */
// bool check_fat32(const char *disk) {
// #warning "You must remove this function before submitting."
//     char buf[PATH_MAX];
//     FILE *proc;

//     snprintf(buf, PATH_MAX, "file %s", disk);
//     proc = popen(buf, "r");
//     if (proc == NULL) {
//         perror("popen");
//         exit(1);
//     }
//     fread(buf, 1, PATH_MAX, proc);
//     return strstr(buf, "FAT (32 bit)") != NULL;
// }

unsigned int get_fat_type(const struct BPB *bpb) {
    if (bpb->BPB_FATSz16 != 0) {
        if (bpb->BPB_RootEntCnt == 0) {
            return 32;
        }
        if (bpb->BPB_TotSec16 < 4085) {
            return 12;
        } else {
            return 16;
        }
    } else {
        return 32;
    }
}
void list_files(const uint8_t *image, const struct BPB *hdr, uint32_t cluster) {
    uint32_t fatsz;
    uint32_t first_data_sector;
	fatsz = hdr->fat32.BPB_FATSz32;

    uint32_t root_dir_sectors = ((hdr->BPB_RootEntCnt * 32) + (hdr->BPB_BytsPerSec - 1)) / hdr->BPB_BytsPerSec;
    first_data_sector = hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * fatsz) + root_dir_sectors;

    wchar_t long_name[256] = {0};
    int long_name_index = 0;


    while (1) {
        uint32_t first_sector = first_data_sector + (hdr->BPB_SecPerClus * (cluster - 2));
        union DirEntry *entry = (union DirEntry *)(image + (hdr->BPB_BytsPerSec * first_sector));
		union DirEntry *long_entry = &entry.ldir;
        
            if (entry.dir.DIR_Name[0] == 0x00) {
                return;
            }
            if ((entry->dir.DIR_Attr & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
                int long_entry_index = (long_entry->ldir.LDIR_Ord & ~0x40) - 1;

                for (int j = 0; j < 5; j++) {
                    long_name[long_entry_index * 13 + j] = long_entry->ldir.LDIR_Name1[j];
                }
                for (int j = 0; j < 6; j++) {
                    long_name[long_entry_index * 13 + 5 + j] = long_entry->ldir.LDIR_Name2[j];
                }
                for (int j = 0; j < 2; j++) {
                    long_name[long_entry_index * 13 + 11 + j] = long_entry->ldir.LDIR_Name3[j];
                }

           	}

			  if (entry.dir.DIR_Name[0] == 0xE5 || entry.dir.DIR_Name[0] == 0x2E) {
				  memset(long_name, 0, sizeof(long_name));
					 continue;
				}

            char name[256];
            if (long_name[0]) {
                wcstombs(name, long_name, sizeof(name));
                memset(long_name, 0, sizeof(long_name));
            } else {
               strncpy(name, entry.dir.DIR_Name, 8);
                int end = 7;
                while (name[end] == ' ') {
                    end--;
                }
                name[end + 1] = '.';
                strncpy(name + end + 2, entry.dir.DIR_Name + 8, 3);
                end += 4;
                while (name[end] == ' ') {
                    end--;
                }
                name[end + 1] = '\0';
            }

            if (entry.dir.DIR_Attr == ATTR_DIRECTORY) {
                printf("Directory: %s\n", name);
                uint32_t subcluster = entry.dir.DIR_FstClusLO | (entry.dir.DIR_FstClusHI << 16);
                if (subcluster >= 2 && subcluster != cluster) {
                    list_files(image, hdr, subcluster);
                }
            } else {
				// wprintf(L"File: %ls\n", long_name);
                printf("File: %s\n", name);
            }


    }
}

/*
 * Hexdump the given data.
 *
 * WARNING: THIS FUNCTION IS ONLY FOR DEBUGGING PURPOSES!
 * This function prints the data using an external command called "hexdump", but
 * your program should not depend on external programs. Before submitting, you
 * must remove this function. If you include this function in your submission,
 * you may face a penalty.
 */
// void hexdump(const void *data, size_t size) {
// #warning "You must remove this function before submitting."
//     FILE *proc;

//     proc = popen("hexdump -C", "w");
//     if (proc == NULL) {
//         perror("popen");
//         exit(1);
//     }
//     fwrite(data, 1, size, proc);
// }

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    if (argc < 3) {
        fprintf(stderr, "Usage: %s disk.img ck\n", argv[0]);
        exit(1);
    }
    const char *diskimg = argv[1];

    /*
     * This demo program only works on FAT32 disk images.
     */
//     if (!check_fat32(argv[1])) {
//         fprintf(stderr, "%s is not a FAT32 disk image\n", diskimg);
//         exit(1);
//     }

    /*
     * Open the disk image and map it to memory.
     *
     * This demonstration program opens the image in read-only mode, which means
     * you won't be able to modify the disk image. However, if you need to make
     * changes to the image in later tasks, you should open and map it in
     * read-write mode.
     */
    int fd = open(diskimg, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    // get file length
    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) {
        perror("lseek");
        exit(1);
    }
    uint8_t *image = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (image == (void *)-1) {
        perror("mmap");
        exit(1);
    }
    close(fd);

    /*
     * Print some information about the disk image.
     */
    const struct BPB *hdr = (const struct BPB *)image;
    unsigned int fat_type = get_fat_type(hdr);
    if(strcmp(argv[2], "ck") == 0){
        const char *fat_name = (fat_type == 12) ? "FAT12" : (fat_type == 16) ? "FAT16" : "FAT32";
        printf("%s filesystem\n", fat_name);
        printf("BytsPerSec = %u\n", hdr->BPB_BytsPerSec);
        printf("SecPerClus = %u\n", hdr->BPB_SecPerClus);
        printf("RsvdSecCnt = %u\n", hdr->BPB_RsvdSecCnt);

        uint32_t fatsz = (fat_type != 32) ? hdr->BPB_FATSz16 : hdr->fat32.BPB_FATSz32;
        printf("FATsSecCnt = %u\n", fatsz * hdr->BPB_NumFATs);

        uint32_t root_sec_cnt = 0;
        if (fat_type != 32) {
            root_sec_cnt = ((hdr->BPB_RootEntCnt * 32) + (hdr->BPB_BytsPerSec - 1)) / hdr->BPB_BytsPerSec;
        }
        printf("RootSecCnt = %u\n", root_sec_cnt);

        uint32_t data_sec_cnt = hdr->BPB_TotSec32 - (hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * fatsz) + root_sec_cnt);
        printf("DataSecCnt = %u\n", data_sec_cnt);
    }else if (strcmp(argv[2], "ls") == 0) {
        if (fat_type != 32) {
            fprintf(stderr, "Only FAT32 is supported for ls command\n");
            exit(1);
        }
        list_files(image, hdr, hdr->fat32.BPB_RootClus);
    }
    
    
    
//     const struct BPB *hdr = (const struct BPB *)image;
//     printf("FAT%d filesystem\n", 32);
//     printf("BytsPerSec = %u\n", hdr->BPB_BytsPerSec);
//     printf("SecPerClus = %u\n", hdr->BPB_SecPerClus);
//     printf("RsvdSecCnt = %u\n", hdr->BPB_RsvdSecCnt);
//     printf("FATsSecCnt = ?\n");
//     printf("RootSecCnt = ?\n");
//     printf("DataSecCnt = ?\n");

    /*
     * Print the contents of the first cluster.
     */
    //hexdump(image, sizeof(*hdr));

    munmap(image, size);
}
