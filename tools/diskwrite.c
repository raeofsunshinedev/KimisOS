#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FILE_END    0xFFFFFFFF
#define FILE_FREE   0x00000000

#define FAT_ATTR_READONLY   0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLID      0x08
#define FAT_ATTR_DIR        0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_LFN             FAT_ATTR_READONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLID

typedef uint8_t *buffer_t;

typedef struct{
    uint8_t jmp[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_dir_entries;
    uint16_t sectors_in_volume_small;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat_small;
    uint16_t sectors_per_track;
    uint16_t heads_per_volume;
    uint32_t partition_start;
    uint32_t large_sector_count;
    
    uint32_t sectors_per_fat;
    uint16_t flags;
    uint16_t fat_verno;
    uint32_t root_dir_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_no;
    uint8_t nt_flags;
    uint8_t sig;
    uint32_t vol_uuid;
    char volid[11];
    char sysid[8];
}__attribute__((packed))fat_bpb_t;

typedef struct{
    uint32_t lead_sig;
    uint8_t reserved[480];
    uint32_t sig0;
    uint32_t last_free_cluster;
    uint32_t cluster_search_start;
    uint8_t reserved0[12];
    uint32_t trail_sig;
}fsinfo_t;

typedef struct{
    char filename[8];
    char ext[3];
    uint8_t attributes;
    uint8_t ntflags;
    uint8_t creation_time;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access;
    uint16_t start_cluster_high;
    uint16_t last_modification_time;
    uint16_t last_modification_date;
    uint16_t start_cluster_low;
    uint32_t size_bytes;
}__attribute__((packed)) file_t;

typedef struct{
    fat_bpb_t bpb;
    fsinfo_t fsinfo;
    uint32_t *file_table;
    uint32_t file_table_size_sectors;
    file_t *root_dir;
}fat_info;

int verbose = 0;

int read_sector(FILE *file, uint32_t sector_start, uint32_t count, buffer_t buf){
    verbose && printf("Attempting to read bytes 0x%x - 0x%x\n", sector_start * 512, sector_start * 512  + count * 512);
    // rewind(file);
    if(!file){
        return 0;
    }
    int result = fseek(file, sector_start * 512, SEEK_SET);
    if(result){
        return result;
    }
    fflush(stdout);
    memset(buf, 0, count * 512);
    result = fread(buf, 512, count, file);
    return result;
}

int write_sector(FILE *file, uint32_t sector_start, uint32_t count, buffer_t buf){
    // printf("pre seek\n");
    // rewind(file);
    if(!file){
        return 0;
    }
    int result = fseek(file, sector_start * 512, SEEK_SET);
    if(result){
        printf("Error during seek\n");
        return result;
    }
    result = fwrite(buf, 512, count, file);
    // verbose && printf("Writing sector %d. res: %d\n", sector_start, result);
    return result;
}

uint32_t find_free_cluster(fat_info *info){
    // verbose && printf("finding free cluster\n");
    uint32_t start = info->fsinfo.cluster_search_start;
    if(start < 2){
        start = 2;
    }
    for(int i = start; i < (info->bpb.large_sector_count/info->bpb.sectors_per_cluster); i++){
        // printf("%8x, %8x\n", i, info->file_table[i]);
        if(info->file_table[i]) continue;
        // verbose && printf("Free cluster found: %d | %d\n", i, info->file_table[i]);
        return i;
    }
    return -1;
}

void write_cluster_value(fat_info *info, uint32_t value, uint32_t cluster){
    // printf("writing cluster :3\n");
    for(int i = 0; i < info->bpb.fat_count; i++){
        info->file_table[cluster + i * info->bpb.sectors_per_fat * (info->bpb.bytes_per_sector/4)] = value;
    }
}
//read cluster - returns next cluster in chain
uint32_t read_cluster(fat_info *info, uint32_t cluster, buffer_t buffer, size_t size, FILE *disk){
    // uint8_t *new_buffer = calloc(4096, 1 * (info->bpb.bytes_per_sector * info->bpb.sectors_per_cluster));
    read_sector(disk, info->bpb.reserved_sectors + info->file_table_size_sectors + info->bpb.partition_start + ((cluster - 2) * info->bpb.sectors_per_cluster), info->bpb.sectors_per_cluster, buffer);
    // memcpy(buffer, new_buffer, size);
    return info->file_table[cluster];
}

//write size bytes to file from buffer. no value may be zero or null. If file is null, will write to root directory
int write_file(fat_info *info, file_t *file, buffer_t buffer, size_t size, FILE *disk){
    uint32_t was_root = 0;
    if(!file){
        was_root = 1;
        file_t *root_file = malloc(sizeof(file_t));
        root_file->start_cluster_low = info->bpb.root_dir_cluster&0xffff;
        root_file->start_cluster_high = info->bpb.root_dir_cluster>>16;
        // printf("Rootdirwrite: %d", root_file.start_cluster_high << 16 | root_file.start_cluster_low);
        file = root_file;
    }
    uint32_t first_cluster = file->start_cluster_low | (file->start_cluster_high << 16);
    uint32_t clusters  = (size / (info->bpb.bytes_per_sector * info->bpb.sectors_per_cluster )) + 1;
    uint8_t *new_buffer = calloc(1, clusters * (info->bpb.bytes_per_sector * info->bpb.sectors_per_cluster ));
    memcpy(new_buffer, buffer, size);
    uint32_t cluster = first_cluster;
    // printf("start: %d count: %d\n", cluster, clusters);
    // return -1;
    for(uint32_t i = 0; i < clusters; i++){
        write_sector(disk, info->bpb.reserved_sectors + info->file_table_size_sectors + info->bpb.partition_start + ((cluster - 2) * info->bpb.sectors_per_cluster), info->bpb.sectors_per_cluster, new_buffer + i * info->bpb.sectors_per_cluster * info->bpb.bytes_per_sector);
        uint32_t next_cluster = info->file_table[cluster];
        if(next_cluster == -1){
            next_cluster = find_free_cluster(info);
            if(next_cluster){
                if(was_root){
                    free(file);
                }
                return 0;
            }
            write_cluster_value(info, next_cluster, cluster);
            write_cluster_value(info, -1, next_cluster);
        }
        // verbose && printf("writing at cluster %-d. Next: %-d", cluster, next_cluster);
        cluster = next_cluster;
    }
    write_cluster_value(info, -1, cluster);
    if(was_root){
        free(file);
    }
    free(new_buffer);
    return size * info->bpb.bytes_per_sector * info->bpb.sectors_per_cluster;
}

int create_file(char *path, fat_info *info, FILE *file, FILE *disk){
    file_t *root_files = calloc(info->bpb.sectors_per_cluster * 512, sizeof(file_t));
    // read_file(info, 0, (buffer_t)root_files, (uint32_t)4096, disk);
    read_cluster(info, info->bpb.root_dir_cluster, (buffer_t)root_files, info->bpb.sectors_per_cluster * 512, disk);
    uint32_t index = 0;
    while(root_files[index].filename[0]){
        index++;
    }
    // printf("%s\n", root_files[index].filename);
    verbose && printf("Creating file at index: %d\n", index);
    char *ext = strchr(path, '.');
    char *tmpptr = path;
    int nidx = 0;
    while(tmpptr < ext){
        root_files[index].filename[nidx++] = *(tmpptr++);
    }
    for(int i = 1; i < 4 && ext[i]; i++){
        root_files[index].ext[i-1] = ext[i];
    }
    rewind(file);
    fseek(file, 0, SEEK_END);
    uint32_t size = ftell(file);
    root_files[index].size_bytes = size;
    uint32_t first_cluster = find_free_cluster(info);
    if(first_cluster == -1){
        return -1;
    }
    info->file_table[first_cluster] = -1;
    root_files[index].start_cluster_low = first_cluster & 0xffff;
    root_files[index].start_cluster_high = first_cluster >> 16;
    buffer_t buffer = calloc(size, 1);
    rewind(file);
    fread(buffer, size, 1, file);
    info->bpb.root_dir_entries++;
    write_file(info, &root_files[index], buffer, size, disk);
    write_file(info, 0, (buffer_t)root_files, (info->bpb.root_dir_entries * sizeof(file_t)), disk);
    free(buffer);
    return 0;
}

int fat_write_back(FILE *file, fat_info *info){
    int res = 0;
    if(res = write_sector(file, info->bpb.reserved_sectors, info->bpb.sectors_per_fat * info->bpb.fat_count, (buffer_t)info->file_table) == 0){
        verbose && printf("Error during write: %d\n", res);
    }
    return res;
}

int detect_repair_fs(FILE *file, fat_info *info){
    //perform sanity checks and set data if wrong
    buffer_t boot_sector = malloc(512);
    read_sector(file, 0, 1, boot_sector);
    info->bpb = *(fat_bpb_t *)boot_sector;
    verbose && printf("\033[0;33mBytes per sector: %d\n", info->bpb.bytes_per_sector);
    verbose && printf("Sectors per cluster: %x\n", info->bpb.sectors_per_cluster);
    info->bpb.bytes_per_sector += 512 * info->bpb.bytes_per_sector == 0;
    info->bpb.sectors_per_cluster += 8 * info->bpb.sectors_per_cluster == 0;
    verbose && printf("Reserved sectors %d\n", info->bpb.reserved_sectors);
    info->bpb.reserved_sectors += 32 * info->bpb.reserved_sectors == 0;
    verbose && printf("Root directory cluster: %d\n", info->bpb.root_dir_cluster);
    info->bpb.root_dir_cluster += 2 * info->bpb.root_dir_cluster == 0;
    memcpy(boot_sector, &info->bpb, sizeof(fat_bpb_t));
    write_sector(file, 0, 1, boot_sector);
    
    info->file_table_size_sectors = info->bpb.fat_count * info->bpb.sectors_per_fat;
    verbose && printf("\033[1;31mAllocating File Table: 0x%x bytes\033[0m\n",  info->bpb.sectors_per_fat * info->bpb.fat_count * info->bpb.bytes_per_sector);
    info->file_table = calloc(info->bpb.sectors_per_fat * info->bpb.fat_count * info->bpb.bytes_per_sector, 1);
    read_sector(file, info->bpb.reserved_sectors, info->bpb.sectors_per_fat * info->bpb.fat_count, (buffer_t)info->file_table);
    if(!info->file_table[info->bpb.root_dir_cluster]){
        info->file_table[0] = -1;
        info->file_table[1] = -1;
        info->file_table[info->bpb.root_dir_cluster] = -1;
    }
    free(boot_sector);
    return 0;
}

int main(int argc, char **argv){
    char list_root_dir = 0;
    char **files = malloc(512*sizeof(char*));
    int filec = 0;
    FILE *output_file = 0;
    
    fat_info info = {};
    
    if(argc == 1 || !strcmp(argv[1], "-h")){
        printf("write files to virtual disk\n");
        printf("Usage:\ndiskwrite <input files> -o <output file>");
        printf("\n-h: Help");
        printf("\n-o: Specify output file\n");
        printf("-v: Verbose\n");
    }
    for(int i = 1; i < argc; i++){
        if(!strcmp(argv[i], "-l")){
            list_root_dir = 1;
            continue;
        }
        if(!strcmp(argv[i], "-o")){
            i++;
            if(i >= argc){
                printf("Error: No output file specified!\n");
            }
            verbose && printf("OF: %s\n", argv[i]);
            output_file = fopen(argv[i], "r+");
            if(!output_file){
                printf("Error opening file");
            }
            continue;
        }
        if(!strcmp(argv[i], "-v")){
            verbose = 1;
            continue;
        }
        files[filec++] = argv[i];
        verbose && printf("IF: %s\n", argv[i]);
    }
    detect_repair_fs(output_file, &info);
    // if(filec == 0){
    //     printf("Nothing to do, no files to write\n");
    //     return 0;
    // }
    for(int i = 0; i < filec; i++){
        // printf("\033[1;34mWriting file %d in list: %s\033[0m\n", i, files[i]);
        FILE *f = fopen(files[i], "r");
        if(!f){
            printf("Error Opening file\n");
            exit(-1);
        }
        create_file(files[i], &info, f, output_file);
        fclose(f);
    }
    if(list_root_dir){
        buffer_t buf = calloc(info.bpb.sectors_per_cluster * 512, 1);
        read_cluster(&info, 2, buf, info.bpb.sectors_per_cluster * 512, output_file);
        file_t *root = (void *)buf;
        uint32_t index = 0;
        while(root[index].filename[0]){
            printf("%s", root[index].filename);
            printf(".%s\n", root[index].ext);
            index++;
        }
        free(root);
    }
    fat_write_back(output_file, &info);
    verbose && printf("Finished. Exiting...\n");
    fclose(output_file);
    free(info.file_table);
    free(files);
    return 0;
}