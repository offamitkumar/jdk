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

    assert(_cont.tail()->is_in_chunk(sp), "");

    frame hf(sp, sp, fp, f.pc(), nullptr, nullptr, true /* on_heap */);
    // frame_top() and frame_bottom() read these before relativize_interpreted_frame_metadata() is called
    *hf.addr_at(_z_ijava_idx(locals)) = locals_offset;
    *hf.addr_at(_z_ijava_idx(esp))    = f.interpreter_frame_esp() - f.fp();
    return hf;
  } else {
    // TODO: needs to step through it at some point of time.
    int fsize = FKind::size(f);
    sp = caller.unextended_sp() - fsize;
    if (caller.is_interpreted_frame()) {
      // If the caller is interpreted, our stackargs are not supposed to overlap with it
      // so we make more room by moving sp down by argsize
      int argsize = FKind::stack_argsize(f);
      sp -= argsize + frame::metadata_words_at_top;
    }
    fp = sp + fsize;
    caller.set_sp(fp);

    assert(_cont.tail()->is_in_chunk(sp), "");

    return frame(sp, sp, fp, f.pc(), nullptr, nullptr, true /* on_heap */);
  }
}

void FreezeBase::adjust_interpreted_frame_unextended_sp(frame& f) {
  // nothing to do (TODO: why ? will it be same for s390)
}

inline void FreezeBase::prepare_freeze_interpreted_top_frame(frame& f) {
  // nothing to do
//  DEBUG_ONLY( intptr_t* lspp = (intptr_t*) &(f.ijava_state()->top_frame_sp); )
  // TODO / FIXME below assert failed so needs to be revisited to fix it.
  // for now I found comments which mentioned not to use top_frame_sp.
//  assert(*lspp == f.unextended_sp() - f.fp(), "should be " INTPTR_FORMAT " usp:" INTPTR_FORMAT " fp:" INTPTR_FORMAT " diff = %ld", *lspp, p2i(f.unextended_sp()), p2i(f.fp()), f.unextended_sp() - f.fp());
}

inline void FreezeBase::relativize_interpreted_frame_metadata(const frame& f, const frame& hf) {
  intptr_t* vfp = f.fp();
  intptr_t* hfp = hf.fp();
  assert(f.fp() > (intptr_t*)f.interpreter_frame_esp(), "");

  // There is alignment padding between vfp and f's locals array in the original
  // frame, because we freeze the padding (see recurse_freeze_interpreted_frame)
  // in order to keep the same relativized locals pointer, we don't need to change it here.

  // Make sure that monitors is already relativized.
  assert(hf.at_absolute(_z_ijava_idx(monitors)) <= -(frame::z_ijava_state_size / wordSize), "");
  // Make sure that esp is already relativized.
  assert(hf.at_absolute(_z_ijava_idx(esp)) <= hf.at_absolute(_z_ijava_idx(monitors)), "");
  // top_frame_sp is already relativized

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
    int fsize = FKind::size(hf);
    int argsize = FKind::stack_argsize(hf);
    intptr_t* frame_sp = caller.sp() - fsize;

    if ((bottom && argsize > 0) || caller.is_interpreted_frame()) {
      frame_sp -= argsize + frame::metadata_words_at_top;
      frame_sp = align_down(frame_sp, frame::alignment_in_bytes);
      caller.set_sp(frame_sp + fsize);
    }

    assert(hf.cb() != nullptr, "");
    assert(hf.oop_map() != nullptr, "");
    intptr_t* fp = frame_sp + fsize;
    return frame(frame_sp, frame_sp, fp, hf.pc(), hf.cb(), hf.oop_map(), false);
  }
}

inline void ThawBase::derelativize_interpreted_frame_metadata(const frame& hf, const frame& f) {
  intptr_t* vfp = f.fp();
  // Make sure that monitors is still relativized.
  assert(f.at_absolute(_z_ijava_idx(monitors)) <= -(frame::z_ijava_state_size / wordSize), "");
  // Make sure that esp is still relativized.
  assert(f.at_absolute(_z_ijava_idx(esp)) <= f.at_absolute(_z_ijava_idx(monitors)), "");
  // Keep top_frame_sp relativized.
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
  for (intptr_t* callers_sp; sp < bottom; sp = callers_sp) {
    address pc = (address)((frame::z_java_abi*) sp)->return_pc;
    assert(pc != nullptr, "");
    // see ThawBase::patch_return() which gets called just before
    bool is_entry_frame = pc == StubRoutines::cont_returnBarrier() || pc == _cont.entryPC();
    if (is_entry_frame) {
      callers_sp = _cont.entryFP();
    } else {
      assert(!Interpreter::contains(pc), "sp:" PTR_FORMAT " pc:" PTR_FORMAT, p2i(sp), p2i(pc));
      // TODO: 8290965: PPC64: Implement post-call NOPs
      CodeBlob* cb = CodeCache::find_blob(pc);
      callers_sp = sp + cb->frame_size();
    }
    // set the back link
    ((frame::z_java_abi*) sp)->callers_sp = (intptr_t) callers_sp;
  }
}

inline void ThawBase::prefetch_chunk_pd(void* start, int size) {

}

#endif // CPU_S390_CONTINUATION_S390_INLINE_HPP
