#include "pmm.h"
#include "../serial.h"

#define MULTIBOOT2_TAG_TYPE_END 0
#define MULTIBOOT2_TAG_TYPE_MMAP 6
#define MULTIBOOT2_MEMORY_AVAILABLE 1

typedef struct{
    uint32_t total_size;
    uint32_t reserved;
} multiboot_info_t;

typedef struct{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} multiboot_mmap_tag_t;

typedef struct{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} multiboot_mmap_entry_t;

static uint8_t *frame_bitmap;
static uint64_t total_pages;
static uint64_t free_pages;

uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t address);

static void bitmap_set(uint64_t page){
    frame_bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(uint64_t page){
    frame_bitmap[page / 8] &= ~(1 << (page % 8));
}

static int bitmap_test(uint64_t page){
    return frame_bitmap[page / 8] & (1 << (page % 8));
}

static uint64_t align_down(uint64_t address){
    return address & ~(PAGE_SIZE - 1);
}

static uint64_t align_up(uint64_t address){
    return (address + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

void pmm_init(uint32_t multiboot_info_address){
    multiboot_info_t *info = (multiboot_info_t *)(uint64_t)multiboot_info_address;
    uint8_t *tag_address = (uint8_t *)info + 8;
    uint8_t *end = (uint8_t *)info + info->total_size;
    uint64_t highest_address = 0;
    while (tag_address < end){
        uint32_t type = *(uint32_t *)tag_address;
        uint32_t size = *(uint32_t *)(tag_address + 4);
        if (type == MULTIBOOT2_TAG_TYPE_MMAP){
            multiboot_mmap_tag_t *tag = (multiboot_mmap_tag_t *)tag_address;
            uint8_t *entry_address = tag_address + 16;
            uint8_t *entry_end = tag_address + tag->size;

            while (entry_address < entry_end){
                multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)entry_address;

                if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE){
                    uint64_t end_address =entry->base + entry->length;
                    if (end_address > highest_address)
                        highest_address = end_address;
                }
                entry_address += tag->entry_size;
            }
        }

        if (type == MULTIBOOT2_TAG_TYPE_END)
            break;

        tag_address += (size + 7) & ~7;
    }

    total_pages = align_up(highest_address) / PAGE_SIZE;

    static uint8_t bitmap[1024 * 1024];

    frame_bitmap = bitmap;

    for (uint64_t page = 0; page < total_pages; page++)
        bitmap_set(page);

    free_pages = 0;
    tag_address = (uint8_t *)info + 8;

    while (tag_address < end){
        uint32_t type = *(uint32_t *)tag_address;
        uint32_t size = *(uint32_t *)(tag_address + 4);

        if (type == MULTIBOOT2_TAG_TYPE_MMAP){
            multiboot_mmap_tag_t *tag = (multiboot_mmap_tag_t *)tag_address;
            uint8_t *entry_address = tag_address + 16;
            uint8_t *entry_end = tag_address + tag->size;
            while (entry_address < entry_end){
                multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)entry_address;

                if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE){
                    uint64_t start = align_up(entry->base);
                    uint64_t finish = align_down(entry->base + entry->length);

                    for (uint64_t address = start;address < finish;address += PAGE_SIZE){
                        uint64_t page = address / PAGE_SIZE;

                        if (page < total_pages &&bitmap_test(page)){
                            bitmap_clear(page);
                            free_pages++;
                        }
                    }
                }

                entry_address += tag->entry_size;
            }
        }

        if (type == MULTIBOOT2_TAG_TYPE_END)
            break;

        tag_address += (size + 7) & ~7;
    }

    serial_write_str("PMM initialized.\n");
    serial_write_str("Total pages: ");
    serial_write_hex(total_pages);
    serial_write_str("\n");
    serial_write_str("Free pages: ");
    serial_write_hex(free_pages);
    serial_write_str("\n");
}

uint64_t pmm_alloc_page(void){
    for (uint64_t page = 0; page < total_pages; page++){
        if (!bitmap_test(page)){
            bitmap_set(page);
            free_pages--;

            return page * PAGE_SIZE;
        }
    }

    return 0;
}

void pmm_free_page(uint64_t address)
{
    uint64_t page = address / PAGE_SIZE;

    if (page >= total_pages)
        return;

    if (!bitmap_test(page)){
        return;
    }

    bitmap_clear(page);
    free_pages++;
}

uint64_t pmm_get_total_pages(void){
    return total_pages;
}

uint64_t pmm_get_free_pages(void){
    return free_pages;
}