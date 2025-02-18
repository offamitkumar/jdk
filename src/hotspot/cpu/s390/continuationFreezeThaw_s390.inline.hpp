/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates. All rights reserved.
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
  assert(FKind::is_instance(f), "");
  intptr_t *sp, *fp;
  if (FKind::interpreted) {
    intptr_t locals_offset = *f.addr_at(_z_ijava_idx(locals));

    // TODO: update this comment ?
    // If the caller.is_empty(), i.e. we're freezing into an empty chunk, then we set
    // the chunk's argsize in finalize_freeze and make room for it above the unextended_sp
    // See also comment on StackChunkFrameStream<frame_kind>::interpreter_frame_size()

    int overlap =
      (caller.is_interpreted_frame() || caller.is_empty())
      ? ContinuationHelper::InterpretedFrame::stack_argsize(f) + frame::metadata_words_at_top
      : 0;

    // TODO: this works but I don't know why, I don't know what does it even do ?
    fp = caller.unextended_sp() - 1 - locals_offset + overlap;

    // esp points one slot below the last argument
    intptr_t* x86_64_like_unextended_sp = f.interpreter_frame_esp() + 1 - frame::metadata_words_at_top;

    sp = fp - (f.fp() - x86_64_like_unextended_sp);
    assert (sp <= fp && (fp <= caller.unextended_sp() || caller.is_interpreted_frame()),
        "sp=" PTR_FORMAT " fp=" PTR_FORMAT " caller.unextended_sp()=" PTR_FORMAT " caller.is_interpreted_frame()=%d",
        p2i(sp), p2i(fp), p2i(caller.unextended_sp()), caller.is_interpreted_frame());
    caller.set_sp(fp);
    caller.set_sp(fp);

    assert(_cont.tail()->is_in_chunk(sp), "");

    frame hf(sp, sp, fp, f.pc(), nullptr, nullptr, true /* on_heap */);
    // frame_top() and frame_bottom() read these before relativize_interpreted_frame_metadata() is called
    *hf.addr_at(_z_ijava_idx(locals)) = locals_offset;
    *hf.addr_at(_z_ijava_idx(esp))    = f.interpreter_frame_esp() - f.fp();
    return hf;
  } else {
    assert(false, "else part: continuationFreezeThaw_s390.inline.hpp");
  }
  return frame();
}

void FreezeBase::adjust_interpreted_frame_unextended_sp(frame& f) {
  // nothing to do (TODO: why ? will it be same for s390)
}

inline void FreezeBase::prepare_freeze_interpreted_top_frame(frame& f) {
  Unimplemented();
}

static inline void relativize_one(intptr_t* const vfp, intptr_t* const hfp, int offset) {
  assert(*(hfp + offset) == *(vfp + offset), "");
  intptr_t* addr = hfp + offset;
  intptr_t value = *(intptr_t**)addr - vfp;
  *addr = value;
}

inline void FreezeBase::relativize_interpreted_frame_metadata(const frame& f, const frame& hf) {
  // TODO:
  // 1. https://bugs.openjdk.org/browse/JDK-8300197
  // 2. https://bugs.openjdk.org/browse/JDK-8308984
  // 3. https://bugs.openjdk.org/browse/JDK-8315966
  // 4. https://bugs.openjdk.org/browse/JDK-8316523
  intptr_t* vfp = f.fp();
  intptr_t* hfp = hf.fp();
  assert(f.fp() > (intptr_t*)f.interpreter_frame_esp(), "");

  // There is alignment padding between vfp and f's locals array in the original
  // frame, therefore we cannot use it to relativize the locals pointer.
  // This line can be changed into an assert when we have fixed the "frame padding problem", see JDK-8300197
  *hf.addr_at(_z_ijava_idx(locals)) = frame::metadata_words + f.interpreter_frame_method()->max_locals() - 1;
  relativize_one(vfp, hfp, _z_ijava_idx(monitors));
  relativize_one(vfp, hfp, _z_ijava_idx(esp));
  relativize_one(vfp, hfp, _z_ijava_idx(top_frame_sp));

  // hfp == hf.sp() + (f.fp() - f.sp()) is not true on ppc because the stack frame has room for
  // the maximal expression stack and the expression stack in the heap frame is trimmed.
  assert(hf.fp() == hf.interpreter_frame_esp() + (f.fp() - f.interpreter_frame_esp()), "");
  assert(hf.fp()                 <= (intptr_t*)hf.at(_z_ijava_idx(locals)), "");
}

inline void FreezeBase::patch_pd(frame& hf, const frame& caller) {
  if (caller.is_interpreted_frame()) {
    assert(!caller.is_empty(), "");
    patch_callee_link_relative(caller, caller.fp());
  }
#ifdef ASSERT
  else {
    // TODO: is below comment valid ?

    // For compiled frames the back link is actually redundant. It gets computed
    // as unextended_sp + frame_size.

    // Note the difference on x86_64: the link is not made relative if the caller
    // is a compiled frame because there rbp is used as a non-volatile register by
    // c1/c2 so it could be a computed value local to the caller.

    // See also:
    // - FreezeBase::set_top_frame_metadata_pd
    // - StackChunkFrameStream<frame_kind>::fp()
    // - UseContinuationFastPath: compiled frames are copied in a batch w/o patching the back link.
    //   The backlinks are restored when thawing (see Thaw<ConfigT>::patch_caller_links())
    patch_callee_link(hf, (intptr_t*)badAddress);
  }
#endif
}

inline void FreezeBase::patch_stack_pd(intptr_t* frame_sp, intptr_t* heap_sp) {
  Unimplemented();
}

inline frame ThawBase::new_entry_frame() {
  intptr_t* sp = _cont.entrySP();
  return frame(sp, _cont.entryPC(), sp, _cont.entryFP());
}

template<typename FKind> frame ThawBase::new_stack_frame(const frame& hf, frame& caller, bool bottom) {
  assert(FKind::is_instance(hf), "");

  assert(is_aligned(caller.fp(), frame::frame_alignment), PTR_FORMAT, p2i(caller.fp()));
  // caller.sp() can be unaligned. This is fixed below.
  if (FKind::interpreted) {
    // TODO: needs to be checked below comment validity.
    // Note: we have to overlap with the caller, at least if it is interpreted, to match the
    // max_thawing_size calculation during freeze. See also comment above.
    intptr_t* heap_sp = hf.unextended_sp();
    const int fsize = ContinuationHelper::InterpretedFrame::frame_bottom(hf) - hf.unextended_sp();
    const int overlap = !caller.is_interpreted_frame() ? 0
                        : ContinuationHelper::InterpretedFrame::stack_argsize(hf) + frame::metadata_words_at_top;
    intptr_t* frame_sp = caller.unextended_sp() + overlap - fsize;
    intptr_t* fp = frame_sp + (hf.fp() - heap_sp);
    // align fp
    int padding = fp - align_down(fp, frame::frame_alignment);
    fp -= padding;
    // alignment of sp is done by callee or in finish_thaw()
    frame_sp -= padding;

    // TODO: really ??
    // On s390x esp points to the next free slot on the expression stack and sp + metadata points to the last parameter
    DEBUG_ONLY(intptr_t* esp = fp + *hf.addr_at(_z_ijava_idx(esp));)
    assert(frame_sp + frame::metadata_words_at_top == esp+1, " frame_sp=" PTR_FORMAT " esp=" PTR_FORMAT, p2i(frame_sp), p2i(esp));
    caller.set_sp(fp);
    frame f(frame_sp, hf.pc(), frame_sp, fp);
    // we need to set the locals so that the caller of new_stack_frame() can call
    // ContinuationHelper::InterpretedFrame::frame_bottom
    // copy relativized locals from the heap frame
    *f.addr_at(_z_ijava_idx(locals)) = *hf.addr_at(_z_ijava_idx(locals));

    return f;
  } else { 
    assert(false, "else part" FILE_AND_LINE);
  }
  return frame();
}

static inline void derelativize_one(intptr_t* const fp, int offset) {
  intptr_t* addr = fp + offset;
  *addr = (intptr_t)(fp + *addr);
}

inline void ThawBase::derelativize_interpreted_frame_metadata(const frame& hf, const frame& f) {
  intptr_t* vfp = f.fp();
  // TODO: 1. https://bugs.openjdk.org/browse/JDK-8308984
  // TODO: 2. https://bugs.openjdk.org/browse/JDK-8315966
  // TODO: 3. https://bugs.openjdk.org/browse/JDK-8316523
  derelativize_one(vfp, _z_ijava_idx(monitors));
  derelativize_one(vfp, _z_ijava_idx(esp));
  derelativize_one(vfp, _z_ijava_idx(top_frame_sp));
}

inline intptr_t* ThawBase::align(const frame& hf, intptr_t* frame_sp, frame& caller, bool bottom) {
  Unimplemented();
  return nullptr;
}

inline void ThawBase::patch_pd(frame& f, const frame& caller) {
  // TODO: 1. https://bugs.openjdk.org/browse/JDK-8299375
  patch_callee_link(caller, caller.fp());
}

inline void ThawBase::patch_pd(frame& f, intptr_t* caller_sp) {
  Unimplemented();
}

inline intptr_t* ThawBase::push_cleanup_continuation() {
  Unimplemented();
  return nullptr;
}

template <typename ConfigT>
inline void Thaw<ConfigT>::patch_caller_links(intptr_t* sp, intptr_t* bottom) {
  Unimplemented();
}

inline void ThawBase::prefetch_chunk_pd(void* start, int size) {
  Unimplemented();
}

#endif // CPU_S390_CONTINUATION_S390_INLINE_HPP
