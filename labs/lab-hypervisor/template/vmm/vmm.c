// KVM API reference: https://www.kernel.org/doc/html/latest/virt/kvm/api.html
// Code initially based on example from https://lwn.net/Articles/658511/

#include <err.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/eventfd.h>
#include <unistd.h>

typedef struct {
    int kvmfd;
    int vmfd;
    int vcpufd;

    struct kvm_run *run;
    int vcpu_mmap_size;

    uint8_t *guest_mem;
    u_int guest_mem_size;
} vm_t;

static void handle_pmio(vm_t *vm) {
    struct kvm_run *run = vm->run;
    // Guest wrote to an I/O port
    if (run->io.direction == KVM_EXIT_IO_OUT) {
        uint8_t *addr = (uint8_t *)run + run->io.data_offset;
        uint32_t value;
        switch (run->io.size) {
            case 1:  // retrieve the 8-bit value written by the guest
                value = *(uint8_t *)addr;
                break;
            case 2:  // retrieve the 16-bit value written by the guest
                value = *(uint16_t *)addr;
                break;
            default:
                fprintf(stderr, "VMM: Unsupported size in KVM_EXIT_IO\n");
                value = 0;
        }
        printf("VMM: PMIO guest write: size=%d port=%d value=%d\n", run->io.size, run->io.port, value);
    }
    // Guest read from an I/O port
    else if (run->io.direction == KVM_EXIT_IO_IN) {
        int val;
        switch (run->io.size) {
            case 1:  // the guest is reading 8-bit from the port
                {
                uint8_t *addr = (uint8_t *)run + run->io.data_offset;
                val = 42;    // dummy 8-bit value injected into the guest
                *addr = val;
                }
                break;
            case 2:  // the guest is reading 16-bit from the port
                {
                uint16_t *addr = (uint16_t *)((uint8_t *)run + run->io.data_offset);
                val = 42;    // dummy 16-bit value injected into the guest
                *addr = val;
                }
                break;
            default:
                fprintf(stderr, "VMM: Unsupported size in KVM_EXIT_IO\n");
        }
        printf("VMM: PMIO guest read: size=%d port=%d [value injected by VMM=%d]\n", run->io.size, run->io.port, val);
    }
    else
        fprintf(stderr, "VMM: unhandled KVM_EXIT_IO\n");
}

static void handle_mmio(vm_t *vm) {
    struct kvm_run *run = vm->run;
    // Guest wrote to an MMIO address (i.e. a memory slot marked as read-only)
    if (run->mmio.is_write) {
        uint32_t value;
        switch (run->mmio.len) {
            case 1:
                value = *((uint8_t *)run->mmio.data);
                break;
            case 2:
                value = *((uint16_t *)run->mmio.data);
                break;
            case 4:
                value = *((uint32_t *)run->mmio.data);
                break;
            default:
                fprintf(stderr, "VMM: Unsupported size in KVM_EXIT_MMIO\n");
                value = 0;
        }
        printf("VMM: MMIO guest write: len=%d addr=%ld value=%d\n", run->mmio.len, (long int)run->mmio.phys_addr, value);
    }
}

static void check_capability(int kvm, int cap, char *cap_string) {
    if (!ioctl(kvm, KVM_CHECK_EXTENSION, cap))
        errx(1, "VMM: Required extension %s not available", cap_string);
}

static vm_t* vm_create(const char *guest_binary) {
    vm_t *vm = malloc(sizeof(vm_t));
    if (!vm) err(1, NULL);
    memset(vm, 0, sizeof(vm_t));

    char kvm_dev[] = "/dev/kvm";
    vm->kvmfd = open(kvm_dev, O_RDWR | O_CLOEXEC);
    if (vm->kvmfd < 0) err(1, "%s", kvm_dev);

    // Make sure we have the right version of the API
    int version = ioctl(vm->kvmfd, KVM_GET_API_VERSION, NULL);
    if (version < 0) err(1, "VMM: KVM_GET_API_VERSION");
    if (version != KVM_API_VERSION) err(1, "VMM: KVM_GET_API_VERSION %d, expected %d", version, KVM_API_VERSION);

    vm->vmfd = ioctl(vm->kvmfd, KVM_CREATE_VM, 0);
    if (vm->vmfd < 0) err(1, "VMM: KVM_CREATE_VM");

    // Make sure we can manage guest physical memory slots
    check_capability(vm->kvmfd, KVM_CAP_USER_MEMORY, "KVM_CAP_USER_MEMORY");

    // mmap syscall:
    // 1st arg: specifies at which virtual address to start the mapping; if NULL, kernel chooses the address
    // 2nd arg: size to allocate (in bytes)
    // 3rd arg: access type (read, write, etc.)
    // 4th arg: flags; MAP_ANONYMOUS = mapping not backed by any file and contents initialized to zero
    // 5th arg: file descriptor if mmap a file (otherwise, set to -1)
    // 6th arg: offset when mmap a file
    // IMPORTANT: size must be a multiple of a page (4KB) and address must be on a page boundary!

    // Allocate 256KB of RAM for the guest
    vm->guest_mem_size = 4096*64;
    vm->guest_mem = mmap(NULL, vm->guest_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (!vm->guest_mem) err(1, "VMM: allocating guest memory");

    // Map guest_mem to physical address 0 in the guest address space
    struct kvm_userspace_memory_region mem_region = {
        .slot = 0,
        .guest_phys_addr = 0,
        .memory_size = vm->guest_mem_size,
        .userspace_addr = (uint64_t)vm->guest_mem,
        .flags = 0
    };
    if (ioctl(vm->vmfd, KVM_SET_USER_MEMORY_REGION, &mem_region) < 0) err(1, "VMM: KVM_SET_USER_MEMORY_REGION");

    // Create the vCPU
    vm->vcpufd = ioctl(vm->vmfd, KVM_CREATE_VCPU, 0);
    if (vm->vcpufd < 0) err(1, "VMM: KVM_CREATE_VCPU");

    // Setup memory for the vCPU
    vm->vcpu_mmap_size = ioctl(vm->kvmfd, KVM_GET_VCPU_MMAP_SIZE, NULL);
    if (vm->vcpu_mmap_size < 0) err(1, "VMM: KVM_GET_VCPU_MMAP_SIZE");

    if (vm->vcpu_mmap_size < (int)sizeof(struct kvm_run)) err(1, "VMM: KVM_GET_VCPU_MMAP_SIZE unexpectedly small");
    vm->run = mmap(NULL, (size_t)vm->vcpu_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vm->vcpufd, 0);
    if (!vm->run) err(1, "VMM: mmap vcpu");

    // Initialize CS to point to 0
    struct kvm_sregs sregs;
    if (ioctl(vm->vcpufd, KVM_GET_SREGS, &sregs) < 0) err(1, "VMM: KVM_GET_SREGS");
    sregs.cs.base = 0;
    sregs.cs.selector = 0;
    if (ioctl(vm->vcpufd, KVM_SET_SREGS, &sregs) < 0) err(1, "VMM: KVM_SET_SREGS");

    // Initialize instruction pointer and flags register
    struct kvm_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.rsp = vm->guest_mem_size;  // set stack pointer at the top of the guest's RAM
    regs.rip = 0;
    regs.rflags = 0x2;  // bit 1 is reserved and should always bet set to 1
    if (ioctl(vm->vcpufd, KVM_SET_REGS, &regs) < 0) err(1, "VMM: KVM_SET_REGS");

    return vm;
}

static void vm_run(vm_t *vm) {
    // Runs the VM (guest code) and handles VM exits
    while (1) {
        // Runs the vCPU until encoutering a VM_EXIT
        if (ioctl(vm->vcpufd, KVM_RUN, NULL) < 0) err(1, "VMM: KVM_RUN");

        switch (vm->run->exit_reason) {
            // NOTE: KVM_EXIT_IO is significantly faster than KVM_EXIT_MMIO

            case KVM_EXIT_IO:    // encountered an I/O instruction
                handle_pmio(vm);
                break;
            case KVM_EXIT_MMIO:  // encountered a MMIO instruction which could not be satisfied
                handle_mmio(vm);
                break;
            case KVM_EXIT_HLT:   // encountered "hlt" instruction
                fprintf(stderr, "VMM: KVM_EXIT_HLT\n");
                return;
            case KVM_EXIT_FAIL_ENTRY:
                fprintf(stderr, "VMM: KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx\n",
                    (unsigned long long)vm->run->fail_entry.hardware_entry_failure_reason);
                break;
            case KVM_EXIT_INTERNAL_ERROR:
                fprintf(stderr, "VMM: KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x\n", vm->run->internal.suberror);
                return;
            case KVM_EXIT_SHUTDOWN:
                fprintf(stderr, "VMM: KVM_EXIT_SHUTDOWN\n");
                return;
            default:
                fprintf(stderr, "VMM: unhandled exit reason (0x%x)\n", vm->run->exit_reason);
                return;
        }
    }
}

static void vm_destroy(vm_t *vm) {
    if (vm->guest_mem)
        munmap(vm->guest_mem, vm->guest_mem_size);
    if (vm->run)
        munmap(vm->run, vm->vcpu_mmap_size);
    close(vm->kvmfd);
    memset(vm, 0, sizeof(vm_t));
    free(vm);
}

int main(int argc, char **argv) {
    vm_t *vm = vm_create("my_awesome_guest_os");
    vm_run(vm);
    vm_destroy(vm);
    return EXIT_SUCCESS;
}
