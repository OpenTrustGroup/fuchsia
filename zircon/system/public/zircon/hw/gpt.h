// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SYSROOT_ZIRCON_HW_GPT_H_
#define SYSROOT_ZIRCON_HW_GPT_H_

#include <stdbool.h>
#include <stdint.h>

#include <zircon/compiler.h>

#define GPT_MAGIC (0x5452415020494645ull)  // 'EFI PART'
#define GPT_HEADER_SIZE 0x5c
#define GPT_ENTRY_SIZE 0x80
#define GPT_GUID_LEN 16
#define GPT_GUID_STRLEN 37
#define GPT_NAME_LEN 72

typedef struct gpt_header {
  uint64_t magic;
  uint32_t revision;
  uint32_t size;
  uint32_t crc32;
  uint32_t reserved0;
  uint64_t current;
  uint64_t backup;
  uint64_t first;
  uint64_t last;
  uint8_t guid[GPT_GUID_LEN];
  uint64_t entries;
  uint32_t entries_count;
  uint32_t entries_size;
  uint32_t entries_crc;
} __PACKED gpt_header_t;

typedef struct gpt_entry {
  uint8_t type[GPT_GUID_LEN];
  uint8_t guid[GPT_GUID_LEN];
  uint64_t first;
  uint64_t last;
  uint64_t flags;
  uint8_t name[GPT_NAME_LEN];  // UTF-16 on disk
} __PACKED gpt_entry_t;

// clang-format off
#define GUID_EMPTY_STRING "00000000-0000-0000-0000-000000000000"
#define GUID_EMPTY_VALUE {                         \
    0x00, 0x00, 0x00, 0x00,                        \
    0x00, 0x00,                                    \
    0x00, 0x00,                                    \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 \
}
#define GUID_EMPTY_NAME "empty"

#define GUID_EFI_STRING "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"
#define GUID_EFI_VALUE {                           \
    0x28, 0x73, 0x2a, 0xc1,                        \
    0x1f, 0xf8,                                    \
    0xd2, 0x11,                                    \
    0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b \
}
#define GUID_EFI_NAME "efi-system"

// GUID for a system partition
#define GUID_SYSTEM_STRING "606B000B-B7C7-4653-A7D5-B737332C899D"
#define GUID_SYSTEM_VALUE {                        \
    0x0b, 0x00, 0x6b, 0x60,                        \
    0xc7, 0xb7,                                    \
    0x53, 0x46,                                    \
    0xa7, 0xd5, 0xb7, 0x37, 0x33, 0x2c, 0x89, 0x9d \
}
#define GUID_SYSTEM_NAME "fuchsia-system"

// GUID for a data partition
#define GUID_DATA_STRING "08185F0C-892D-428A-A789-DBEEC8F55E6A"
#define GUID_DATA_VALUE {                          \
    0x0c, 0x5f, 0x18, 0x08,                        \
    0x2d, 0x89,                                    \
    0x8a, 0x42,                                    \
    0xa7, 0x89, 0xdb, 0xee, 0xc8, 0xf5, 0x5e, 0x6a \
}
#define GUID_DATA_NAME "fuchsia-data"

// GUID for a installer partition
#define GUID_INSTALL_STRING "48435546-4953-2041-494E-5354414C4C52"
#define GUID_INSTALL_VALUE {                       \
    0x46, 0x55, 0x43, 0x48,                        \
    0x53, 0x49,                                    \
    0x41, 0x20,                                    \
    0x49, 0x4E, 0x53, 0x54, 0x41, 0x4C, 0x4C, 0x52 \
}
#define GUID_INSTALL_NAME "fuchsia-install"

#define GUID_BLOB_STRING "2967380E-134C-4CBB-B6DA-17E7CE1CA45D"
#define GUID_BLOB_VALUE {                          \
    0x0e, 0x38, 0x67, 0x29,                        \
    0x4c, 0x13,                                    \
    0xbb, 0x4c,                                    \
    0xb6, 0xda, 0x17, 0xe7, 0xce, 0x1c, 0xa4, 0x5d \
}
#define GUID_BLOB_NAME "fuchsia-blob"

#define GUID_FVM_STRING "41D0E340-57E3-954E-8C1E-17ECAC44CFF5"
#define GUID_FVM_VALUE {                           \
    0x40, 0xe3, 0xd0, 0x41,                        \
    0xe3, 0x57,                                    \
    0x4e, 0x95,                                    \
    0x8c, 0x1e, 0x17, 0xec, 0xac, 0x44, 0xcf, 0xf5 \
}
#define GUID_FVM_NAME "fuchsia-fvm"

#define GUID_ZIRCON_A_STRING "DE30CC86-1F4A-4A31-93C4-66F147D33E05"
#define GUID_ZIRCON_A_VALUE {                       \
    0x86, 0xcc, 0x30, 0xde,                         \
    0x4a, 0x1f,                                     \
    0x31, 0x4a,                                     \
    0x93, 0xc4, 0x66, 0xf1, 0x47, 0xd3, 0x3e, 0x05, \
}
#define GUID_ZIRCON_A_NAME "zircon-a"

#define GUID_ZIRCON_B_STRING "23CC04DF-C278-4CE7-8471-897D1A4BCDF7"
#define GUID_ZIRCON_B_VALUE {                      \
    0xdf, 0x04, 0xcc, 0x23,                        \
    0x78, 0xc2,                                    \
    0xe7, 0x4c,                                    \
    0x84, 0x71, 0x89, 0x7d, 0x1a, 0x4b, 0xcd, 0xf7 \
}
#define GUID_ZIRCON_B_NAME "zircon-b"

#define GUID_ZIRCON_R_STRING "A0E5CF57-2DEF-46BE-A80C-A2067C37CD49"
#define GUID_ZIRCON_R_VALUE {                      \
    0x57, 0xcf, 0xe5, 0xa0,                        \
    0xef, 0x2d,                                    \
    0xbe, 0x46,                                    \
    0xa8, 0x0c, 0xa2, 0x06, 0x7c, 0x37, 0xcd, 0x49 \
}
#define GUID_ZIRCON_R_NAME "zircon-r"

#define GUID_SYS_CONFIG_STRING "4E5E989E-4C86-11E8-A15B-480FCF35F8E6"
#define GUID_SYS_CONFIG_VALUE {                    \
    0x9e, 0x98, 0x5e, 0x4e,                        \
    0x86, 0x4c,                                    \
    0xe8, 0x11,                                    \
    0xa1, 0x5b, 0x48, 0x0f, 0xcf, 0x35, 0xf8, 0xe6 \
}
#define GUID_SYS_CONFIG_NAME "sys-config"

#define GUID_FACTORY_CONFIG_STRING "5A3A90BE-4C86-11E8-A15B-480FCF35F8E6"
#define GUID_FACTORY_CONFIG_VALUE {                \
    0xbe, 0x90, 0x3a, 0x5a,                        \
    0x86, 0x4c,                                    \
    0xe8, 0x11,                                    \
    0xa1, 0x5b, 0x48, 0x0f, 0xcf, 0x35, 0xf8, 0xe6 \
}
#define GUID_FACTORY_CONFIG_NAME "factory"

#define GUID_BOOTLOADER_STRING "5ECE94FE-4C86-11E8-A15B-480FCF35F8E6"
#define GUID_BOOTLOADER_VALUE {                    \
    0xfe, 0x94, 0xce, 0x5e,                        \
    0x86, 0x4c,                                    \
    0xe8, 0x11,                                    \
    0xa1, 0x5b, 0x48, 0x0f, 0xcf, 0x35, 0xf8, 0xe6 \
}
#define GUID_BOOTLOADER_NAME "bootloader"

#define GUID_TEST_STRING "8B94D043-30BE-4871-9DFA-D69556E8C1F3"
#define GUID_TEST_VALUE {                          \
    0x43, 0xD0, 0x94, 0x8b,                        \
    0xbe, 0x30,                                    \
    0x71, 0x48,                                    \
    0x9d, 0xfa, 0xd6, 0x95, 0x56, 0xe8, 0xc1, 0xf3 \
}
#define GUID_TEST_NAME "guid-test"

#define GUID_VBMETA_A_STRING "A13B4D9A-EC5F-11E8-97D8-6C3BE52705BF"
#define GUID_VBMETA_A_VALUE {                      \
    0x9a, 0x4d, 0x3b, 0xa1,                        \
    0x5f, 0xec,                                    \
    0xe8, 0x11,                                    \
    0x97, 0xd8, 0x6c, 0x3b, 0xe5, 0x27, 0x05, 0xbf \
}
#define GUID_VBMETA_A_NAME "vbmeta_a"

#define GUID_VBMETA_B_STRING "A288ABF2-EC5F-11E8-97D8-6C3BE52705BF"
#define GUID_VBMETA_B_VALUE {                      \
    0xf2, 0xab, 0x88, 0xa2,                        \
    0x5f, 0xec,                                    \
    0xe8, 0x11,                                    \
    0x97, 0xd8, 0x6c, 0x3b, 0xe5, 0x27, 0x05, 0xbf \
}
#define GUID_VBMETA_B_NAME "vbmeta_b"

#define GUID_VBMETA_R_STRING "6A2460C3-CD11-4E8B-A880-12CCE268ED0A"
#define GUID_VBMETA_R_VALUE {                      \
    0xc3, 0x60, 0x24, 0x6a,                        \
    0x11, 0xcd,                                    \
    0x8b, 0x4e,                                    \
    0x80, 0xa8, 0x12, 0xcc, 0xe2, 0x68, 0xed, 0x0a \
}
#define GUID_VBMETA_R_NAME "vbmeta_r"

#define GUID_CROS_KERNEL_STRING "FE3A2A5D-4F32-41A7-B725-ACCC3285A309"
#define GUID_CROS_KERNEL_VALUE {                   \
    0x5d, 0x2a, 0x3a, 0xfe,                        \
    0x32, 0x4f,                                    \
    0xa7, 0x41,                                    \
    0xb7, 0x25, 0xac, 0xcc, 0x32, 0x85, 0xa3, 0x09 \
}
#define GUID_CROS_KERNEL_NAME "cros-kernel"

#define GUID_CROS_ROOTFS_STRING "3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC"
#define GUID_CROS_ROOTFS_VALUE {                   \
    0x02, 0xe2, 0xb8, 0x3C,                        \
    0x7e, 0x3b,                                    \
    0xdd, 0x47,                                    \
    0x8a, 0x3c, 0x7f, 0xf2, 0xa1, 0x3c, 0xfc, 0xec \
}
#define GUID_CROS_ROOTFS_NAME "cros-rootfs"

#define GUID_CROS_RESERVED_STRING "2E0A753D-9E48-43B0-8337-B15192CB1B5E"
#define GUID_CROS_RESERVED_VALUE {                 \
    0x3d, 0x75, 0x0a, 0x2e,                        \
    0x48, 0x9e,                                    \
    0xb0, 0x43,                                    \
    0x83, 0x37, 0xb1, 0x51, 0x92, 0xcb, 0x1b, 0x5e \
}
#define GUID_CROS_RESERVED_NAME "cros-reserved"

#define GUID_CROS_FIRMWARE_STRING "CAB6E88E-ABF3-4102-A07A-D4BB9BE3C1D3"
#define GUID_CROS_FIRMWARE_VALUE {                 \
    0x8e, 0xe8, 0xb6, 0xca,                        \
    0xf3, 0xab,                                    \
    0x02, 0x41,                                    \
    0xa0, 0x7a, 0xd4, 0xbb, 0x9b, 0xe3, 0xc1, 0xd3 \
}
#define GUID_CROS_FIRMWARE_NAME "cros-firmware"

#define GUID_CROS_DATA_STRING "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"
#define GUID_CROS_DATA_VALUE {                     \
    0xa2, 0xa0, 0xd0, 0xeb,                        \
    0xe5, 0xb9,                                    \
    0x33, 0x44,                                    \
    0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 \
}
#define GUID_CROS_DATA_NAME "cros-data"

#define GUID_BIOS_STRING "21686148-6449-6E6F-744E-656564454649"
#define GUID_BIOS_VALUE {                          \
    0x48, 0x61, 0x68, 0x21,                        \
    0x49, 0x64,                                    \
    0x6f, 0x6e,                                    \
    0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 \
}
#define GUID_BIOS_NAME "bios"

// clang-format on

#endif  // SYSROOT_ZIRCON_HW_GPT_H_
