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

#ifndef CPU_S390_STACKCHUNKFRAMESTREAM_S390_INLINE_HPP
#define CPU_S390_STACKCHUNKFRAMESTREAM_S390_INLINE_HPP

#include "interpreter/oopMapCache.hpp"
#include "runtime/frame.inline.hpp"
#include "runtime/registerMap.hpp"

#ifdef ASSERT
template <ChunkFrames frame_kind>
inline bool StackChunkFrameStream<frame_kind>::is_in_frame(void* p0) const {
  Unimplemented();
  return true;
}
#endif

template <ChunkFrames frame_kind>
inline frame StackChunkFrameStream<frame_kind>::to_frame() const {
  if (is_done()) {
    return frame(_sp, _sp, nullptr, nullptr, nullptr, nullptr, true);
  } else {
    // TODO: need to verify this. I don't know whether it's true for s390x or not.
    // Compiled frames on heap don't have back links. See FreezeBase::patch_pd() and frame::setup().
    return frame(sp(), unextended_sp(), Interpreter::contains(pc()) ? fp() : nullptr, pc(), cb(), _oopmap, true);
  }
}

template <ChunkFrames frame_kind>
inline address StackChunkFrameStream<frame_kind>::get_pc() const {
  assert(!is_done(), "");
  return (address)((frame::z_common_abi*) _sp)->return_pc;
}

template <ChunkFrames frame_kind>
inline intptr_t* StackChunkFrameStream<frame_kind>::fp() const {
  // See FreezeBase::patch_pd() and frame::setup()
  assert((frame_kind == ChunkFrames::Mixed && is_interpreted()), "");
  intptr_t* fp_addr = (intptr_t*)&((frame::z_common_abi*)_sp)->callers_sp;
  assert(*(intptr_t**)fp_addr != nullptr, "");
  // derelativize
  return fp_addr + *fp_addr;
}

template <ChunkFrames frame_kind>
inline intptr_t* StackChunkFrameStream<frame_kind>::derelativize(int offset) const {
  intptr_t* fp = this->fp();
  assert(fp != nullptr, "");
  return fp + fp[offset];
}

template <ChunkFrames frame_kind>
inline intptr_t* StackChunkFrameStream<frame_kind>::unextended_sp_for_interpreter_frame() const {
  // TODO: comment on return is valid ?
  assert_is_interpreted_and_frame_type_mixed();
  return derelativize(_z_ijava_idx(esp)) + 1 - frame::metadata_words; // On s390 esp points to the next free slot
}

template <ChunkFrames frame_kind>
inline void StackChunkFrameStream<frame_kind>::next_for_interpreter_frame() {
  assert_is_interpreted_and_frame_type_mixed();
  if (derelativize(_z_ijava_idx(locals)) + 1 >= _end) {
    _unextended_sp = _end;
    _sp = _end;
  } else {
    _unextended_sp = derelativize(_z_ijava_idx(sender_sp));
    _sp = this->fp();
  }
}

template <ChunkFrames frame_kind>
inline int StackChunkFrameStream<frame_kind>::interpreter_frame_size() const {
  assert_is_interpreted_and_frame_type_mixed();
  intptr_t* top = unextended_sp(); // later subtract argsize if callee is interpreted
  intptr_t* bottom = derelativize(_z_ijava_idx(locals)) + 1;
  return (int)(bottom - top);
}

// Size of stack args in words (P0..Pn above). Only valid if the caller is also
// interpreted. The function is also called if the caller is compiled but the
// result is not used in that case (same on x86).
// See also setting of sender_sp in ContinuationHelper::InterpretedFrame::patch_sender_sp()
template <ChunkFrames frame_kind>
inline int StackChunkFrameStream<frame_kind>::interpreter_frame_stack_argsize() const {
  assert_is_interpreted_and_frame_type_mixed();
  frame::z_ijava_state* state = (frame::z_ijava_state*)((uintptr_t)fp() - frame::z_ijava_state_size);
  int diff = (int)(state->locals - (state->sender_sp + frame::metadata_words_at_top) + 1);
  assert(diff == -frame::metadata_words_at_top || ((Method*)state->method)->size_of_parameters() == diff,
      "size_of_parameters(): %d diff: %d sp: " PTR_FORMAT " fp:" PTR_FORMAT,
      ((Method*)state->method)->size_of_parameters(), diff, p2i(sp()), p2i(fp()));
  return diff;
}

template <ChunkFrames frame_kind>
inline int StackChunkFrameStream<frame_kind>::interpreter_frame_num_oops() const {
  Unimplemented();
  assert_is_interpreted_and_frame_type_mixed();
  ResourceMark rm;
  InterpreterOopMap mask;
  frame f = to_frame();
  f.interpreted_frame_oop_map(&mask);
  return  mask.num_oops()
          + 1 // for the mirror oop
          + (f.interpreter_frame_method()->is_native() ? 1 : 0) // temp oop slot
          + pointer_delta_as_int((intptr_t*)f.interpreter_frame_monitor_begin(),
                                 (intptr_t*)f.interpreter_frame_monitor_end())/BasicObjectLock::size();
}

template<>
template<>
inline void StackChunkFrameStream<ChunkFrames::Mixed>::update_reg_map_pd(RegisterMap* map) {
  Unimplemented();
}

template<>
template<>
inline void StackChunkFrameStream<ChunkFrames::CompiledOnly>::update_reg_map_pd(RegisterMap* map) {
  Unimplemented();
}

template <ChunkFrames frame_kind>
template <typename RegisterMapT>
inline void StackChunkFrameStream<frame_kind>::update_reg_map_pd(RegisterMapT* map) {}

#endif // CPU_S390_STACKCHUNKFRAMESTREAM_S390_INLINE_HPP
