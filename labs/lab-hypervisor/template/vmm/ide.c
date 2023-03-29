#include "ide.h"

#define DIRECTION_IN KVM_EXIT_IO_IN
#define DIRECTION_OUT KVM_EXIT_IO_OUT

void write_file(ide_t *ide);

void state_1(struct ide *ide, struct kvm_run *run);
void state_2(struct ide *ide, struct kvm_run *run);
void state_3(struct ide *ide, struct kvm_run *run);
void state_4(struct ide *ide, struct kvm_run *run);
void state_5(struct ide *ide, struct kvm_run *run);
void state_6(struct ide *ide, struct kvm_run *run);
void state_7(struct ide *ide, struct kvm_run *run);
void state_8(struct ide *ide, struct kvm_run *run);
void state_9(struct ide *ide, struct kvm_run *run);

void reset_and_goto_1(struct ide *ide)
{
    printf("going back to 1\n");

    for (int i = 0; i < SECTOR_SIZE; i++)
    {
        ((uint8_t *)ide->data)[i] = 0;
    }

    ide->data_count = 0;
    ide->sector_idx = 0;
    ide->next = &state_1;
}

void state_1(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_IN && run->io.size == 1 && run->io.port == STATUS_PORT)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        *addr = 0x40;

        printf("starting process\n");
        ide->next = &state_2;
        return;
    }
}

void state_2(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_OUT && run->io.size == 1 && run->io.port == 0x1F2)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        uint32_t value = *addr;

        if (value == 1)
        {
            printf("writting 1 sector\n");
            ide->next = &state_3;
            return;
        }
    }

    reset_and_goto_1(ide);
}

void state_3(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_OUT && run->io.size == 1 && run->io.port == 0x1F3)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        uint32_t value = *addr;
        ide->sector_idx = value;

        printf("receive first byte for sector index %d\n", value);
        ide->next = &state_4;
        return;
    }

    reset_and_goto_1(ide);
}

void state_4(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_OUT && run->io.size == 1 && run->io.port == 0x1F4)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        uint32_t value = *addr;
        ide->sector_idx |= value << 8;

        printf("receive second byte for sector index %d\n", value);
        ide->next = &state_5;
        return;
    }

    reset_and_goto_1(ide);
}

void state_5(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_OUT && run->io.size == 1 && run->io.port == 0x1F5)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        uint32_t value = *addr;
        ide->sector_idx |= value << 16;

        printf("receive third byte for sector index %d\n", value);
        ide->next = &state_6;
        return;
    }

    reset_and_goto_1(ide);
}

void state_6(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_OUT && run->io.size == 1 && run->io.port == 0x1F6)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        uint32_t value = (*addr) & 0x0F;
        ide->sector_idx |= value << 24;

        printf("receive fourth byte for sector index %d\n", value);
        ide->next = &state_7;

        printf("sector index: 0x%x, %d\n", ide->sector_idx, ide->sector_idx);
        return;
    }

    reset_and_goto_1(ide);
}

void state_7(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_OUT && run->io.size == 1 && run->io.port == STATUS_PORT)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        uint32_t value = *addr;

        if (value == 0x30)
        {
            printf("now writing with retry\n");
            ide->next = &state_8;
            return;
        }
    }

    reset_and_goto_1(ide);
}

void state_8(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_IN && run->io.size == 1 && run->io.port == STATUS_PORT)
    {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        *addr = 0x40;

        printf("ready to receive data\n");
        ide->next = &state_9;
        return;
    }

    reset_and_goto_1(ide);
}

void state_9(struct ide *ide, struct kvm_run *run)
{
    if (run->io.direction == DIRECTION_OUT && run->io.port == DATA_PORT)
    {
        int size = run->io.size;

        if (size != 1 && size != 2 && size != 4)
        {
            printf("invalid size: %d\n", size);
            reset_and_goto_1(ide);
            return;
        }

        if (ide->data_count + size > SECTOR_SIZE)
        {
            printf("data overflow\n");
            reset_and_goto_1(ide);
            return;
        }

        uint8_t *addr = (uint8_t *)run + run->io.data_offset;

        switch (run->io.size)
        {
        case 1:
        {
            uint8_t *data = (uint8_t *)(ide->data + ide->data_count);
            uint8_t value = *(uint8_t *)addr;
            *data = value;
            break;
        }
        case 2:
        {
            uint16_t *data = (uint16_t *)(ide->data + ide->data_count);
            uint16_t value = *(uint16_t *)addr;
            *data = value;
            break;
        }
        case 4:
        {
            uint64_t *data = (uint64_t *)(ide->data + ide->data_count);
            uint64_t value = *(uint64_t *)addr;
            *data = value;
            break;
        }
        }

        ide->data_count += size;

        if (ide->data_count == SECTOR_SIZE)
        {
            printf("received all data\n");
            write_file(ide);
            reset_and_goto_1(ide);
            return;
        }

        printf("received %d bytes\n", run->io.size);
        return;
    }

    reset_and_goto_1(ide);
}

void write_file(ide_t *ide)
{
    printf("writing to file %s\n", ide->disk_path);
    FILE *fp = fopen(ide->disk_path, "r+b");
    if (fp == NULL)
    {
        printf("failed to open disk.img\n");
        return;
    }

    fseek(fp, ide->sector_idx * SECTOR_SIZE, SEEK_SET);
    fwrite(ide->data, 1, SECTOR_SIZE, fp);
    fclose(fp);
}

ide_t *create_ide_state_machine(char *disk_path)
{
    ide_t *ide = (ide_t *)malloc(sizeof(ide_t));
    ide->data = (void *)malloc(SECTOR_SIZE);
    ide->disk_path = disk_path;

    reset_and_goto_1(ide);

    return ide;
}

void destroy_ide_state_machine(ide_t *ide)
{
    free(ide->data);
    free(ide);
}