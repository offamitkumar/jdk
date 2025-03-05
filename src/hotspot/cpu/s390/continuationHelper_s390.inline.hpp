/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates. All rights reserved.
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

#ifndef CPU_S390_CONTINUATIONHELPER_S390_INLINE_HPP
#define CPU_S390_CONTINUATIONHELPER_S390_INLINE_HPP

#include "runtime/continuationHelper.hpp"

// TODO: Implement

template<typename FKind>
static inline intptr_t** link_address(const frame& f) {
  Unimplemented();
  return nullptr;
}

static inline void patch_return_pc_with_preempt_stub(frame& f) {
  Unimplemented();
}

inline int ContinuationHelper::frame_align_words(int size) {
  return size & 1;
}

inline intptr_t* ContinuationHelper::frame_align_pointer(intptr_t* sp) {
  Unimplemented();
  return nullptr;
}

template<typename FKind>
inline void ContinuationHelper::update_register_map(const frame& f, RegisterMap* map) {
  Unimplemented();
}

inline void ContinuationHelper::update_register_map_with_callee(const frame& f, RegisterMap* map) {
  Unimplemented();
}

inline void ContinuationHelper::push_pd(const frame& f) {
  f.own_abi()->callers_sp = (uint64_t)f.fp();
}

inline void ContinuationHelper::set_anchor_to_entry_pd(JavaFrameAnchor* anchor, ContinuationEntry* cont) {
  // TODO: are we sure about this ? Why ?
  // nothing to do
}

inline void ContinuationHelper::set_anchor_pd(JavaFrameAnchor* anchor, intptr_t* sp) {
  // TODO: are we sure about this ? Why ?
  // nothing to do
}

#ifdef ASSERT
inline bool ContinuationHelper::Frame::assert_frame_laid_out(frame f) {
  intptr_t* sp = f.sp();
  address pc = *(address*)(sp - frame::sender_sp_ret_address_offset());
  intptr_t* fp = (intptr_t*)f.own_abi()->callers_sp;
  assert(f.raw_pc() == pc, "f.ra_pc: " INTPTR_FORMAT " actual: " INTPTR_FORMAT, p2i(f.raw_pc()), p2i(pc));
  assert(f.fp() == fp, "f.fp: " INTPTR_FORMAT " actual: " INTPTR_FORMAT, p2i(f.fp()), p2i(fp));
  return f.raw_pc() == pc && f.fp() == fp;
}
#endif

inline intptr_t** ContinuationHelper::Frame::callee_link_address(const frame& f) {
  return (intptr_t**)&f.own_abi()->callers_sp;
}

template<typename FKind>
static inline intptr_t* real_fp(const frame& f) {
  Unimplemented();
  return nullptr;
}

inline address* ContinuationHelper::InterpretedFrame::return_pc_address(const frame& f) {
  return (address*)&f.callers_abi()->return_pc;
}

inline void ContinuationHelper::InterpretedFrame::patch_sender_sp(frame& f, const frame& caller) {
  intptr_t* sp = caller.unextended_sp();
  if (!f.is_heap_frame() && caller.is_interpreted_frame()) {
    // TODO: how about we make a diagram ?
    // See diagram "Interpreter Calling Procedure on PPC" at the end of continuationFreezeThaw_ppc.inline.hpp
    // 1. https://bugs.openjdk.org/browse/JDK-8308984
    sp = (intptr_t*)caller.at_relative(_z_ijava_idx(top_frame_sp));
  }
  assert(f.is_interpreted_frame(), "");
  assert(f.is_heap_frame() || is_aligned(sp, frame::alignment_in_bytes), "");
  intptr_t* la = f.addr_at(_z_ijava_idx(sender_sp));
  *la = f.is_heap_frame() ? (intptr_t)(sp - f.fp()) : (intptr_t)sp;
}

inline address* ContinuationHelper::Frame::return_pc_address(const frame& f) {
  return (address*)&f.callers_abi()->return_pc;
}

inline address ContinuationHelper::Frame::real_pc(const frame& f) {
  return (address)f.own_abi()->return_pc;
}

inline void ContinuationHelper::Frame::patch_pc(const frame& f, address pc) {
  f.own_abi()->return_pc = (uint64_t)pc;
}


// TODO: finish it.
//                   |                              |
//                   |                              |
//                   |                              |
//                   |                              |
//                   |==============================|
//                   |    L0                        |
//                   |    .                         |
//                   |    .                         |
//                   |    .                         |
//                   |    Ln                        | <- ijava_state.esp (1 slot below Pn)
//                   |                              |
//                   |------------------------------|
//                   |     SP alignment (opt.)      |
//                   |------------------------------|
//                   |      Minimal ABI             |
//                   |   (frame:z_java_abi)         |
//                   | which is derived from        |
//                   | z_common_abi and only holds  |
//                   | return_pc and callers_sp     |
//                   |                              |
//                   | 2 Words                      |
//                   | Caller's SP                  | <- SP of f / FP of f's callee
//                   |==============================|
//                   |      z_ijava_state           |
//                   |        (metadata)            |          Frame of f's callee
//                   |                              |
//                   |                              |
//                          |  Growth  |
//                          v          v

inline intptr_t* ContinuationHelper::InterpretedFrame::frame_top(const frame& f, InterpreterOopMap* mask) { // inclusive; this will be copied with the frame
  int expression_stack_sz = expression_stack_size(f, mask);
  intptr_t* res = (intptr_t*)f.interpreter_frame_monitor_end() - expression_stack_sz;
  assert(res <= (intptr_t*)f.ijava_state() - expression_stack_sz,
      "res=" PTR_FORMAT " f.ijava_state()=" PTR_FORMAT " expression_stack_sz=%d",
      p2i(res), p2i(f.ijava_state()), expression_stack_sz);
  assert(res >= f.unextended_sp(),
      "res: " INTPTR_FORMAT " ijava_state: " INTPTR_FORMAT " esp: " INTPTR_FORMAT " unextended_sp: " INTPTR_FORMAT " expression_stack_size: %d",
      p2i(res), p2i(f.ijava_state()), f.ijava_state()->esp, p2i(f.unextended_sp()), expression_stack_sz);
  return res;
}

inline intptr_t* ContinuationHelper::InterpretedFrame::frame_bottom(const frame& f) { // exclusive; this will not be copied with the frame
  return (intptr_t*)f.at_relative(_z_ijava_idx(locals)) + 1; // exclusive; this will not be copied with the frame
}

inline intptr_t* ContinuationHelper::InterpretedFrame::frame_top(const frame& f, int callee_argsize, bool callee_interpreted) {
  intptr_t* pseudo_unextended_sp = f.interpreter_frame_esp() + 1 - frame::metadata_words_at_top;
  // TODO: callee_argsize is inclusion of metadata ?
  return pseudo_unextended_sp + (callee_interpreted ? callee_argsize : 0);
}

inline intptr_t* ContinuationHelper::InterpretedFrame::callers_sp(const frame& f) {
  Unimplemented();
  return nullptr;
}

#endif // CPU_S390_CONTINUATIONHELPER_S390_INLINE_HPP
