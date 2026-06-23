/* VioletOS - VioletFS simple persistent filesystem */
#include "violet.h"

#define FS_MAGIC       0x56494F4C  /* "VIOL" */
#define FS_VERSION     1
#define FS_MAX_ENTRIES 64
#define FS_MAX_NAME    32
#define FS_MAX_CONTENT 4096
#define FS_DISK_LBA    2048
#define FS_SECTOR_SIZE 512
#define FS_DISK_SECTORS 32

#define FS_TYPE_FREE  0
#define FS_TYPE_FILE  1
#define FS_TYPE_DIR   2

struct fs_entry {
    char     name[FS_MAX_NAME];
    uint8_t  type;
    uint32_t size;
    char     content[FS_MAX_CONTENT];
};

struct fs_superblock {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_count;
    char     cwd[FS_MAX_NAME];
    struct fs_entry entries[FS_MAX_ENTRIES];
};

static struct fs_superblock fs;

static struct fs_entry* find_entry(const char* name) {
    for (uint32_t i = 0; i < fs.entry_count; i++) {
        if (strcmp(fs.entries[i].name, name) == 0)
            return &fs.entries[i];
    }
    return NULL;
}

static struct fs_entry* alloc_entry(const char* name, uint8_t type) {
    if (fs.entry_count >= FS_MAX_ENTRIES) return NULL;
    struct fs_entry* e = &fs.entries[fs.entry_count++];
    memset(e, 0, sizeof(struct fs_entry));
    strncpy(e->name, name, FS_MAX_NAME - 1);
    e->type = type;
    return e;
}

static void fs_format(void) {
    memset(&fs, 0, sizeof(fs));
    fs.magic = FS_MAGIC;
    fs.version = FS_VERSION;
    fs.entry_count = 0;
    strcpy(fs.cwd, "/");
    alloc_entry("home", FS_TYPE_DIR);
    alloc_entry("readme.txt", FS_TYPE_FILE);
    struct fs_entry* readme = find_entry("readme.txt");
    if (readme) {
        strcpy(readme->content,
            "Welcome to VioletOS!\n"
            "Type 'help' in the terminal for available commands.\n");
        readme->size = strlen(readme->content);
    }
}

void fs_init(void) {
    if (!fs_load())
        fs_format();
}

bool fs_load(void) {
    uint8_t sector[FS_SECTOR_SIZE];
    uint8_t* disk = (uint8_t*)kmalloc(FS_SECTOR_SIZE * FS_DISK_SECTORS);
    if (!disk) { fs_format(); return false; }

    bool ok = true;
    for (uint32_t i = 0; i < FS_DISK_SECTORS; i++) {
        if (!ata_read_sector(FS_DISK_LBA + i, sector)) {
            ok = false;
            break;
        }
        memcpy(disk + i * FS_SECTOR_SIZE, sector, FS_SECTOR_SIZE);
    }

    if (!ok) {
        kfree(disk);
        return false;
    }

    struct fs_superblock* loaded = (struct fs_superblock*)disk;
    if (loaded->magic != FS_MAGIC) {
        kfree(disk);
        return false;
    }

    memcpy(&fs, loaded, sizeof(fs));
    kfree(disk);
    return true;
}

bool fs_save(void) {
    uint8_t sector[FS_SECTOR_SIZE];
    uint8_t* disk = (uint8_t*)&fs;
    size_t total = sizeof(fs);
    if (total > FS_SECTOR_SIZE * FS_DISK_SECTORS)
        total = FS_SECTOR_SIZE * FS_DISK_SECTORS;

    for (uint32_t i = 0; i < FS_DISK_SECTORS; i++) {
        memset(sector, 0, FS_SECTOR_SIZE);
        size_t offset = i * FS_SECTOR_SIZE;
        if (offset < total) {
            size_t chunk = total - offset;
            if (chunk > FS_SECTOR_SIZE) chunk = FS_SECTOR_SIZE;
            memcpy(sector, disk + offset, chunk);
        }
        if (!ata_write_sector(FS_DISK_LBA + i, sector))
            return false;
    }
    return true;
}

int fs_mkdir(const char* path) {
    if (!path || !path[0]) return -1;
    if (find_entry(path)) return -2;
    if (!alloc_entry(path, FS_TYPE_DIR)) return -3;
    fs_save();
    return 0;
}

int fs_touch(const char* path) {
    if (!path || !path[0]) return -1;
    struct fs_entry* e = find_entry(path);
    if (e) return 0;
    e = alloc_entry(path, FS_TYPE_FILE);
    if (!e) return -3;
    fs_save();
    return 0;
}

int fs_rm(const char* path) {
    if (!path || !path[0]) return -1;
    for (uint32_t i = 0; i < fs.entry_count; i++) {
        if (strcmp(fs.entries[i].name, path) == 0) {
            for (uint32_t j = i; j < fs.entry_count - 1; j++)
                fs.entries[j] = fs.entries[j + 1];
            fs.entry_count--;
            fs_save();
            return 0;
        }
    }
    return -2;
}

int fs_exists(const char* path) {
    return find_entry(path) != NULL;
}

int fs_read(const char* path, char* buf, size_t buf_size) {
    struct fs_entry* e = find_entry(path);
    if (!e || e->type != FS_TYPE_FILE) return -1;
    size_t len = e->size;
    if (len >= buf_size) len = buf_size - 1;
    memcpy(buf, e->content, len);
    buf[len] = '\0';
    return (int)len;
}

int fs_cat(const char* path, char* buf, size_t buf_size) {
    return fs_read(path, buf, buf_size);
}

int fs_write(const char* path, const char* data, size_t len) {
    struct fs_entry* e = find_entry(path);
    if (!e) {
        e = alloc_entry(path, FS_TYPE_FILE);
        if (!e) return -1;
    }
    if (e->type != FS_TYPE_FILE) return -2;
    if (len >= FS_MAX_CONTENT) len = FS_MAX_CONTENT - 1;
    memcpy(e->content, data, len);
    e->content[len] = '\0';
    e->size = (uint32_t)len;
    fs_save();
    return (int)len;
}

int fs_ls(const char* path, char* buf, size_t buf_size) {
    (void)path;
    buf[0] = '\0';
    for (uint32_t i = 0; i < fs.entry_count; i++) {
        if (strlen(buf) + strlen(fs.entries[i].name) + 4 >= buf_size) break;
        if (fs.entries[i].type == FS_TYPE_DIR)
            strcat(buf, "[DIR]  ");
        else
            strcat(buf, "[FILE] ");
        strcat(buf, fs.entries[i].name);
        strcat(buf, "\n");
    }
    return 0;
}

size_t fs_get_used_storage(void) {
    size_t used = 0;
    for (uint32_t i = 0; i < fs.entry_count; i++)
        used += fs.entries[i].size + FS_MAX_NAME;
    return used;
}

size_t fs_get_total_storage(void) {
    return (size_t)FS_MAX_ENTRIES * FS_MAX_CONTENT;
}

const char* fs_get_cwd(void) {
    return fs.cwd;
}

int fs_set_cwd(const char* path) {
    if (!path) return -1;
    strncpy(fs.cwd, path, FS_MAX_NAME - 1);
    fs.cwd[FS_MAX_NAME - 1] = '\0';
    return 0;
}
