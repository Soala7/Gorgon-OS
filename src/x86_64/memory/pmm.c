#include "pmm.h"
#include "../../kernel/serial.h"

#define MULTIBOOT2_TAG_TYPE_END 0
#define MULTIBOOT2_TAG_TYPE_MMAP 6
#define MULTIBOOT2_MEMORY_AVAILABLE 1
#define BITMAP_SIZE (1024 * 1024)
#define BITMAP_MAX_PAGES (BITMAP_SIZE * 8ULL)

typedef struct
{
    uint32_t total_size;
    uint32_t reserved;
} multiboot_info_t;

typedef struct
{
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
} multiboot_mmap_tag_t;

typedef struct
{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} multiboot_mmap_entry_t;

// FIX: linker-provided symbols marking the physical range the
// kernel image itself occupies. These are just addresses - the
// & is required to get the symbol's address, not its "value".
extern char kernel_start[];
extern char kernel_end[];

static uint8_t *frame_bitmap;
static uint64_t total_pages;
static uint64_t free_pages;

uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t address);

static void bitmap_set(uint64_t page)
{
    frame_bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(uint64_t page)
{
    frame_bitmap[page / 8] &= ~(1 << (page % 8));
}

static int bitmap_test(uint64_t page)
{
    return frame_bitmap[page / 8] & (1 << (page % 8));
}

static uint64_t align_down(uint64_t address)
{
    return address & ~(PAGE_SIZE - 1);
}

static uint64_t align_up(uint64_t address)
{
    return (address + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

// FIX: mark every page in [start, end) as used, decrementing
// free_pages for any page that was previously marked free.
// Used to reserve the kernel image and the Multiboot info block
// after the memory-map pass has already marked them free.
static void reserve_range(uint64_t start, uint64_t end)
{
    uint64_t first_page = align_down(start) / PAGE_SIZE;
    uint64_t last_page = align_up(end) / PAGE_SIZE;

    for (uint64_t page = first_page; page < last_page; page++)
    {
        if (page >= total_pages)
            break;

        if (!bitmap_test(page))
        {
            bitmap_set(page);
            free_pages--;
        }
    }
}

void pmm_init(uint32_t multiboot_info_address)
{
    multiboot_info_t *info = (multiboot_info_t *)(uint64_t)multiboot_info_address;
    uint8_t *tag_address = (uint8_t *)info + 8;
    uint8_t *end = (uint8_t *)info + info->total_size;
    uint64_t highest_address = 0;

    while (tag_address < end)
    {
        uint32_t type = *(uint32_t *)tag_address;
        uint32_t size = *(uint32_t *)(tag_address + 4);

        if (type == MULTIBOOT2_TAG_TYPE_MMAP)
        {
            multiboot_mmap_tag_t *tag = (multiboot_mmap_tag_t *)tag_address;
            uint8_t *entry_address = tag_address + 16;
            uint8_t *entry_end = tag_address + tag->size;

            while (entry_address < entry_end)
            {
                multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)entry_address;

                if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE)
                {
                    uint64_t end_address = entry->base + entry->length;
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

    /* TODO: this bitmap is a fixed 1 MiB, capping addressable
     physical memory at 32 GB with no bounds check. Fine for
     QEMU-scale development; revisit if total_pages can exceed
     this before allocating the bitmap dynamically.*/
    total_pages = align_up(highest_address) / PAGE_SIZE;

    if (total_pages > BITMAP_MAX_PAGES)
        total_pages = BITMAP_MAX_PAGES;

    static uint8_t bitmap[BITMAP_SIZE];
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

            while (entry_address < entry_end)
            {
                multiboot_mmap_entry_t *entry = (multiboot_mmap_entry_t *)entry_address;

                if (entry->type == MULTIBOOT2_MEMORY_AVAILABLE)
                {
                    uint64_t start = align_up(entry->base);
                    uint64_t finish = align_down(entry->base + entry->length);

                    for (uint64_t address = start; address < finish; address += PAGE_SIZE){
                        uint64_t page = address / PAGE_SIZE;

                        if (page < total_pages && bitmap_test(page)){
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
    serial_write_str("Free before reservations: ");
    serial_write_hex(free_pages);
    serial_write_str("\n");

    serial_write_str("kernel_start: ");
    serial_write_hex((uint64_t)kernel_start);
    serial_write_str("\n");

    serial_write_str("kernel_end: ");
    serial_write_hex((uint64_t)kernel_end);
    serial_write_str("\n");

    serial_write_str("multiboot_info: ");
    serial_write_hex((uint64_t)multiboot_info_address);
    serial_write_str("\n");
    // FIX: the memory map above only reports which physical RAM
    // is present, not which of it is currently occupied. Without
    // this, pmm_alloc_page() would happily hand out pages the
    // running kernel and the Multiboot info block themselves
    // live in.
    reserve_range(0, (uint64_t)kernel_start);

    serial_write_str("Free after low reservation: ");
    serial_write_hex(free_pages);
    serial_write_str("\n");

    reserve_range((uint64_t)kernel_start, (uint64_t)kernel_end);

    serial_write_str("Free after kernel reservation: ");
    serial_write_hex(free_pages);
    serial_write_str("\n");

    reserve_range((uint64_t)multiboot_info_address,(uint64_t)multiboot_info_address + info->total_size);

    serial_write_str("Free after Multiboot reservation: ");
    serial_write_hex(free_pages);
    serial_write_str("\n");
}

uint64_t pmm_alloc_page(void)
{
    serial_write_str("PMM alloc: searching...\n");

    for (uint64_t page = 1; page < total_pages; page++)
    {
        if (!bitmap_test(page))
        {
            serial_write_str("PMM alloc: found page ");
            serial_write_hex(page);
            serial_write_str("\n");

            bitmap_set(page);

            if (free_pages > 0)
                free_pages--;

            return page * PAGE_SIZE;
        }
    }

    serial_write_str("PMM alloc: no free page found.\n");
    return 0;
}

void pmm_free_page(uint64_t address){
    if (address == 0 || address % PAGE_SIZE != 0)
        return;

    uint64_t page = address / PAGE_SIZE;

    if (page >= total_pages)
        return;

    if (!bitmap_test(page))
        return;

    bitmap_clear(page);
    free_pages++;
}

uint64_t pmm_get_total_pages(void)
{
    return total_pages;
}

uint64_t pmm_get_free_pages(void)
{
    return free_pages;
}
