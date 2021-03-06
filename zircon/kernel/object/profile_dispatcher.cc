// Copyright 2018 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include "object/profile_dispatcher.h"

#include <bits.h>
#include <err.h>
#include <lib/counters.h>
#include <zircon/errors.h>
#include <zircon/rights.h>

#include <fbl/alloc_checker.h>
#include <fbl/ref_ptr.h>
#include <object/thread_dispatcher.h>

KCOUNTER(dispatcher_profile_create_count, "dispatcher.profile.create")
KCOUNTER(dispatcher_profile_destroy_count, "dispatcher.profile.destroy")

static zx_status_t parse_cpu_mask(const zx_cpu_set_t& set, cpu_mask_t* result) {
  // Only support reading up to 1 word in the mask for the moment.
  static_assert(SMP_MAX_CPUS <= sizeof(set.mask[0]) * CHAR_BIT,
                "Zircon only supports reading from a single word in the cpu mask.");
  static_assert(SMP_MAX_CPUS <= ZX_CPU_SET_MAX_CPUS);

  // Ensure that the user is not setting more CPUs than are available.
  uint32_t num_cpus = arch_max_num_cpus();
  cpu_mask_t mask = set.mask[0];
  if ((mask & ~BIT_MASK(num_cpus)) != 0) {
    return ZX_ERR_INVALID_ARGS;
  }

  // Ensure at least one CPU is active in the mask.
  if ((mask & BIT_MASK(num_cpus)) == 0) {
    return ZX_ERR_INVALID_ARGS;
  }

  // Ensure other words in the mask are 0.
  constexpr int num_bitmap_words = sizeof(set.mask) / sizeof(set.mask[0]);
  static_assert(num_bitmap_words * sizeof(set.mask[0]) * CHAR_BIT == ZX_CPU_SET_MAX_CPUS);
  for (size_t i = 1; i < num_bitmap_words; i++) {
    if (set.mask[i] != 0) {
      return ZX_ERR_INVALID_ARGS;
    }
  }

  *result = mask;
  return ZX_OK;
}

zx_status_t validate_profile(const zx_profile_info_t& info) {
  uint32_t flags = info.flags;

  // Ensure at least one flag has been set.
  if (flags == 0) {
    return ZX_ERR_INVALID_ARGS;
  }

  // Ensure priority is valid.
  if ((flags & ZX_PROFILE_INFO_FLAG_PRIORITY) != 0) {
    if ((info.priority < LOWEST_PRIORITY) || (info.priority > HIGHEST_PRIORITY)) {
      return ZX_ERR_INVALID_ARGS;
    }
    flags &= ~ZX_PROFILE_INFO_FLAG_PRIORITY;
  }

  // Ensure affinity mask is valid.
  if ((flags & ZX_PROFILE_INFO_FLAG_CPU_MASK) != 0) {
    cpu_mask_t unused_mask;
    zx_status_t result = parse_cpu_mask(info.cpu_affinity_mask, &unused_mask);
    if (result != ZX_OK) {
      return result;
    }
    flags &= ~ZX_PROFILE_INFO_FLAG_CPU_MASK;
  }

  // Ensure no other flags have been set.
  if (flags != 0) {
    return ZX_ERR_INVALID_ARGS;
  }

  return ZX_OK;
}

zx_status_t ProfileDispatcher::Create(const zx_profile_info_t& info,
                                      KernelHandle<ProfileDispatcher>* handle,
                                      zx_rights_t* rights) {
  auto status = validate_profile(info);
  if (status != ZX_OK)
    return status;

  fbl::AllocChecker ac;
  KernelHandle new_handle(fbl::AdoptRef(new (&ac) ProfileDispatcher(info)));
  if (!ac.check())
    return ZX_ERR_NO_MEMORY;

  *rights = default_rights();
  *handle = ktl::move(new_handle);
  return ZX_OK;
}

ProfileDispatcher::ProfileDispatcher(const zx_profile_info_t& info) : info_(info) {
  kcounter_add(dispatcher_profile_create_count, 1);
}

ProfileDispatcher::~ProfileDispatcher() { kcounter_add(dispatcher_profile_destroy_count, 1); }

zx_status_t ProfileDispatcher::ApplyProfile(fbl::RefPtr<ThreadDispatcher> thread) {
  // Set priority.
  if ((info_.flags & ZX_PROFILE_INFO_FLAG_PRIORITY) != 0) {
    zx_status_t result = thread->SetPriority(info_.priority);
    if (result != ZX_OK) {
      return result;
    }
  }

  // Set affinity.
  if ((info_.flags & ZX_PROFILE_INFO_FLAG_CPU_MASK) != 0) {
    cpu_mask_t mask;
    zx_status_t result = parse_cpu_mask(info_.cpu_affinity_mask, &mask);
    if (result != ZX_OK) {
      return result;
    }
    return thread->SetAffinity(mask);
  }

  return ZX_OK;
}
