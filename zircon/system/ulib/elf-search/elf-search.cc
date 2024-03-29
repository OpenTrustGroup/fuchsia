// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <elf-search.h>

#include <assert.h>
#include <elf.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zircon/status.h>
#include <zircon/syscalls.h>
#include <zircon/syscalls/object.h>

namespace {

// This is a reasonable upper limit on the number of program headers that are
// expected. 7 or 8 is more typical.
constexpr size_t kMaxProgramHeaders = 16;
// kWindowSize is a tuning parameter. It specifies how much memory should be
// read in by ProcessMemReader when a new read is needed. The goal is to
// optimize the trade-off between making too many system calls and reading in
// too much memory. The larger kWindowSize is the fewer system calls are made
// but the more bytes are copied over that don't need to be. The smaller it is
// the more system calls need to be made but the fewer superfluous bytes are
// copied.
// TODO(jakehehrlich): Tune kWindowSize rather than just guessing.
constexpr size_t kWindowSize = 0x400;
// This is an upper bound on the number of bytes that can be used in a build ID.
// md5 and sha1 are the most common hashes used for build ids and they use 20
// and 16 bytes respectively. This makes 32 a generous upper bound.
constexpr size_t kMaxBuildIDSize = 32;

bool IsPossibleLoadedEhdr(const Elf64_Ehdr& ehdr) {
  // Do some basic sanity checks including checking the ELF identifier
  return ehdr.e_ident[EI_MAG0] == ELFMAG0 && ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
         ehdr.e_ident[EI_MAG2] == ELFMAG2 && ehdr.e_ident[EI_MAG3] == ELFMAG3 &&
         ehdr.e_ident[EI_CLASS] == ELFCLASS64 && ehdr.e_ident[EI_DATA] == ELFDATA2LSB &&
         ehdr.e_ident[EI_VERSION] == EV_CURRENT && ehdr.e_type == ET_DYN &&
         ehdr.e_machine == kNativeElfMachine && ehdr.e_version == EV_CURRENT &&
         ehdr.e_ehsize == sizeof(Elf64_Ehdr) && ehdr.e_phentsize == sizeof(Elf64_Phdr) &&
         ehdr.e_phnum > 0 && (ehdr.e_phoff % alignof(Elf64_Phdr) == 0);
}

// TODO(jakehehrlich): Switch uses of uint8_t to std::byte where appropriate.

class ProcessMemReader {
 public:
  ProcessMemReader(const zx::process& proc) : process_(proc) {}

  // TODO (jakehehrlich): Make this interface zero-copy (by returning
  // a pointer rather than copying for instance). It's important that
  // the lifetime of the underlying storage is correctly managed.
  template <typename T>
  [[nodiscard]] zx_status_t Read(uintptr_t vaddr, T* x) {
    return ReadBytes(vaddr, reinterpret_cast<uint8_t*>(x), sizeof(T));
  }
  template <typename T>
  [[nodiscard]] zx_status_t ReadArray(uintptr_t vaddr, fbl::Array<T>* arr) {
    return ReadBytes(vaddr, arr->get(), arr->size());
  }
  template <typename T>
  [[nodiscard]] zx_status_t ReadArray(uintptr_t vaddr, T* arr, size_t sz) {
    return ReadBytes(vaddr, reinterpret_cast<uint8_t*>(arr), sz * sizeof(T));
  }

 private:
  const zx::process& process_;
  uint8_t window_[kWindowSize];
  uintptr_t window_start_ = 0;
  size_t window_size_ = 0;

  zx_status_t ReadBytes(uintptr_t vaddr, uint8_t* mem, size_t size) {
    if (vaddr >= window_start_ && vaddr - window_start_ < window_size_) {
      size_t from_window_size = fbl::min(size, window_size_ - (vaddr - window_start_));
      memcpy(mem, window_ + (vaddr - window_start_), from_window_size);
      vaddr += from_window_size;
      mem += from_window_size;
      size -= from_window_size;
    }
    while (size > 0) {
      // TODO(jakehehrlich): Only read into window on the last iteration of this loop.
      size_t actual;
      zx_status_t status = process_.read_memory(vaddr, window_, kWindowSize, &actual);
      if (status != ZX_OK) {
        return status;
      }
      window_start_ = vaddr;
      window_size_ = actual;
      size_t bytes_read = fbl::min(actual, size);
      memcpy(mem, window_, bytes_read);
      vaddr += bytes_read;
      mem += bytes_read;
      size -= bytes_read;
    }
    return ZX_OK;
  }
};

[[nodiscard]] zx_status_t GetBuildID(ProcessMemReader* reader, uintptr_t base,
                                     const Elf64_Phdr& notes, uint8_t* buildID,
                                     size_t* buildIDSize) {
  auto NoteAlign = [](uint32_t x) { return (x + 3) & -4; };
  // TODO(jakehehrlich): Sanity check here that notes.p_vaddr falls in the
  // [p_vaddr,p_vaddr+p_filesz) range of some RO PT_LOAD.
  // TODO(jakehehrlich): Check that base is actually the bias and do something to alert the user to
  // base not being the bias.
  uintptr_t vaddr = base + notes.p_vaddr;
  uintptr_t end = vaddr + notes.p_filesz;
  // If the virtual address we found is not aligned or the ending overflowed return early.
  if ((vaddr & 3) || end < vaddr) {
    return ZX_ERR_NOT_FOUND;
  }
  while (end - vaddr > sizeof(Elf64_Nhdr)) {
    Elf64_Nhdr nhdr;
    zx_status_t status = reader->Read(vaddr, &nhdr);
    if (status != ZX_OK) {
      return status;
    }
    vaddr += sizeof(Elf64_Nhdr);
    if (end - vaddr < NoteAlign(nhdr.n_namesz)) {
      break;
    }
    uintptr_t nameAddr = vaddr;
    vaddr += NoteAlign(nhdr.n_namesz);
    if (end - vaddr < NoteAlign(nhdr.n_descsz)) {
      break;
    }
    uintptr_t descAddr = vaddr;
    vaddr += NoteAlign(nhdr.n_descsz);
    // TODO(jakehehrlich): If descsz is larger than kMaxBuildIDSize but
    // the type and name indicate that this note entry is a build ID we
    // should log a warning to the user.
    if (nhdr.n_type == NT_GNU_BUILD_ID && nhdr.n_namesz == sizeof(ELF_NOTE_GNU) &&
        nhdr.n_descsz <= kMaxBuildIDSize) {
      char name[sizeof(ELF_NOTE_GNU)];
      status = reader->ReadArray(nameAddr, name, nhdr.n_namesz);
      if (status != ZX_OK) {
        return status;
      }
      if (memcmp(name, ELF_NOTE_GNU, nhdr.n_namesz) == 0) {
        status = reader->ReadArray(descAddr, buildID, nhdr.n_descsz);
        if (status != ZX_OK) {
          return status;
        }
        *buildIDSize = nhdr.n_descsz;
        return ZX_OK;
      }
    }
  }
  return ZX_ERR_NOT_FOUND;
}

}  // anonymous namespace

zx_status_t ForEachModule(const zx::process& process, ModuleAction action) {
  ProcessMemReader reader(process);

  // Read in the process maps.
  size_t actual, avail;
  zx_status_t status = process.get_info(ZX_INFO_PROCESS_MAPS, nullptr, 0, &actual, &avail);
  if (status != ZX_OK) {
    return status;
  }
  fbl::AllocChecker ac;
  fbl::Array<zx_info_maps> maps(new (&ac) zx_info_maps[avail], avail);
  if (!ac.check()) {
    return ZX_ERR_NO_MEMORY;
  }
  status = process.get_info(ZX_INFO_PROCESS_MAPS, maps.get(), avail * sizeof(zx_info_maps), &actual,
                            &avail);
  if (status != ZX_OK) {
    return status;
  }
  // TODO(jakehehrlich): Check permissions of program headers to make sure they agree with mappings.
  // 'maps' should be sorted in ascending order of base address so we should be able to use that to
  // quickly find the mapping associated with any given PT_LOAD.

  for (const auto& map : maps) {
    // Skip any writable maps since the RODATA segment containing the
    // headers will not be writable.
    if (map.type != ZX_INFO_MAPS_TYPE_MAPPING) {
      continue;
    }
    if ((map.u.mapping.mmu_flags & ZX_VM_PERM_WRITE) != 0) {
      continue;
    }
    // Skip any mapping that doesn't start at the beginning of a VMO.
    // We assume that the VMO represents the ELF file. ELF headers
    // always start at the beginning of the file so if our assumption
    // holds then we can't be looking at the start of an ELF header if
    // the offset into the VMO isn't also zero.
    if (map.u.mapping.vmo_offset != 0) {
      continue;
    }

    // Read in what might be an ELF header.
    Elf64_Ehdr ehdr;
    status = reader.Read(map.base, &ehdr);
    if (status != ZX_OK) {
      continue;
    }
    // Do some basic checks to see if this could ever be an ELF file.
    if (!IsPossibleLoadedEhdr(ehdr)) {
      continue;
    }

    // We only support ELF files with <= 16 program headers.
    // TODO(jakehehrlich): Log this because with the exception of core dumps
    // almost nothing should get here *and* have such a large number of phdrs.
    // This might indicate a larger issue.
    if (ehdr.e_phnum > kMaxProgramHeaders) {
      continue;
    }
    Elf64_Phdr phdrs_buf[kMaxProgramHeaders];
    auto phdrs = MakeArrayRef(phdrs_buf, ehdr.e_phnum);
    status = reader.ReadArray(map.base + ehdr.e_phoff, phdrs_buf, ehdr.e_phnum);
    if (status != ZX_OK) {
      continue;
    }

    // Loop though program headers looking for a build ID.
    uint8_t build_id_buf[kMaxBuildIDSize];
    ArrayRef<uint8_t> build_id;
    for (const auto& phdr : phdrs) {
      if (phdr.p_type == PT_NOTE) {
        size_t size;
        status = GetBuildID(&reader, map.base, phdr, build_id_buf, &size);
        if (status == ZX_OK && size != 0) {
          build_id = MakeArrayRef(build_id_buf, size);
          break;
        }
      }
    }
    // We're not considering otherwise valid files with no build id here.
    // TODO(jakehehrlich): Consider reporting loaded modules with no build ID.
    if (build_id.empty()) {
      continue;
    }

    // TODO(jakehehrlich): Synthesize a better name via PT_DYNAMIC->DT_SONAME.
    char name[ZX_MAX_NAME_LEN];
    if (map.name[0] == '\0') {
      snprintf(name, sizeof(name), "<VMO#%" PRIu64 ">", map.u.mapping.vmo_koid);
    } else {
      snprintf(name, sizeof(name), "<VMO#%" PRIu64 "=%s>", map.u.mapping.vmo_koid, map.name);
    }

    // All checks have passed so we can give the user a module.
    action(ModuleInfo{
        .name = fbl::StringPiece(name),
        .vaddr = map.base,
        .build_id = build_id,
        .ehdr = ehdr,
        .phdrs = phdrs,
    });
  }

  return ZX_OK;
}
