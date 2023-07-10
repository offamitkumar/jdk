/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef CPU_S390_CONTINUATION_S390_INLINE_HPP
#define CPU_S390_CONTINUATION_S390_INLINE_HPP

#include "oops/stackChunkOop.inline.hpp"
#include "runtime/frame.hpp"
#include "runtime/frame.inline.hpp"

inline void patch_callee_link(const frame& f, intptr_t* fp) {
  *ContinuationHelper::Frame::callee_link_address(f) = fp;
}

inline void patch_callee_link_relative(const frame& f, intptr_t* fp) {
  intptr_t* la = (intptr_t*)ContinuationHelper::Frame::callee_link_address(f);
  intptr_t new_value = fp - la;
  *la = new_value;
}

inline void FreezeBase::set_top_frame_metadata_pd(const frame& hf) {
  stackChunkOop chunk = _cont.tail();
  assert(chunk->is_in_chunk(hf.sp()), "hf.sp()=" PTR_FORMAT, p2i(hf.sp()));

  hf.own_abi()->return_pc = (uint64_t)hf.pc();
  if (hf.is_interpreted_frame()) {
    patch_callee_link_relative(hf, hf.fp());
  }
#ifdef ASSERT
  else {
    // See also FreezeBase::patch_pd()
    patch_callee_link(hf, (intptr_t*)badAddress);
  }
#endif
}

template<typename FKind>
inline frame FreezeBase::sender(const frame& f) {
  assert(FKind::is_instance(f), "");
  if (FKind::interpreted) {
    return frame(f.sender_sp(), f.sender_pc(), f.interpreter_frame_sender_sp());
  }


  intptr_t* sender_sp = f.sender_sp();
  address sender_pc = f.sender_pc();
  assert(sender_sp != f.sp(), "must have changed");

  int slot = 0;
  CodeBlob* sender_cb = CodeCache::find_blob_and_oopmap(sender_pc, slot);
  return sender_cb != nullptr
         ? frame(sender_sp, sender_sp, nullptr, sender_pc, sender_cb, slot == -1 ? nullptr : sender_cb->oop_map_for_slot(slot, sender_pc))
         : frame(sender_sp, sender_pc, sender_sp);
}

template<typename FKind> frame FreezeBase::new_heap_frame(frame& f, frame& caller) {
  Unimplemented();
  return frame();
}

void FreezeBase::adjust_interpreted_frame_unextended_sp(frame& f) {
  // nothing to do;
}

inline void FreezeBase::relativize_interpreted_frame_metadata(const frame& f, const frame& hf) {
  Unimplemented();
}

inline void FreezeBase::patch_pd(frame& hf, const frame& caller) {
  Unimplemented();
}

inline void FreezeBase::patch_stack_pd(intptr_t* frame_sp, intptr_t* heap_sp) {
  // FIXME, please
  // Nothing to do. The backchain is reconstructed when thawing (see Thaw<ConfigT>::patch_caller_links())
}

// Slow path

inline frame ThawBase::new_entry_frame() {
  intptr_t* sp = _cont.entrySP();
  return frame(sp, _cont.entryPC(), sp, _cont.entryFP());
}

template<typename FKind> frame ThawBase::new_stack_frame(const frame& hf, frame& caller, bool bottom) {
  Unimplemented();
  return frame();
}

inline void ThawBase::derelativize_interpreted_frame_metadata(const frame& hf, const frame& f) {
  Unimplemented();
}

inline intptr_t* ThawBase::align(const frame& hf, intptr_t* frame_sp, frame& caller, bool bottom) {
  //FIXME: need to visit this one again.
  return nullptr;
}

inline void ThawBase::patch_pd(frame& f, const frame& caller) {
  patch_callee_link(caller, caller.fp());
}

template <typename ConfigT>
inline void Thaw<ConfigT>::patch_caller_links(intptr_t* sp, intptr_t* bottom) {
  for (intptr_t* callers_sp; sp < bottom; sp = callers_sp) {
    adderss pc = (address) ((frame::z_java_abi*) sp) -> return_pc;
    assert(pc != nullptr, "");
    bool is_entry_frame = pc == StubRoutines::cont_returnBarrier() || pc == _cont.entryPC();
    if (is_entry_frame) {
      callers_sp = _cont.entryFP();
    } else {
      CodeBlob* cb = CodeCache::find_blob(pc);
      callers_sp = sp + cb->frame_size();
    }
    // set the back link
    ((frame::java_abi*) sp)->callers_sp = (intptr_t) callers_sp;
  }
}

inline void ThawBase::prefetch_chunk_pd(void* start, int size) {
  size <<= LogBytesPerWord;
  Prefetch::read(start, size);
  Prefetch::read(start, size - 64);}

#endif // CPU_S390_CONTINUATION_S390_INLINE_HPP
