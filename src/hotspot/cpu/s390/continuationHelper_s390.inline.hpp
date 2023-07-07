/*
 * Copyright (c) 2022, 2023, Oracle and/or its affiliates. All rights reserved.
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

inline int ContinuationHelper::frame_align_words(int size) {
  return size & 1;
}

inline intptr_t* ContinuationHelper::frame_align_pointer(intptr_t* sp) {
  return align_down(sp, frame::frame_alignment);
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
  // We don't have a frame pointer. (See javaFrameAnchor_s390.hpp)
  // No need to do anything
}

#ifdef ASSERT
inline void ContinuationHelper::set_anchor_pd(JavaFrameAnchor* anchor, intptr_t* sp) {
  // We don't have a frame pointer. (See javaFrameAnchor_s390.hpp)
  // No need to do anything
}

inline bool ContinuationHelper::Frame::assert_frame_laid_out(frame f) {
  intptr_t* sp = f.sp();
  address pc = *(address*)(sp - frame::sender_sp_ret_address_offset());
  intptr_t* fp = (intptr_t*)f.own_abi()->callers_sp;
  assert(f.raw_pc() == pc, "f.raw_pc: " INTPTR_FORMAT " actual: " INTPTR_FORMAT, p2i(f.raw_pc()), p2i(pc));
  assert(f.fp() == fp, "f.fp: " INTPTR_FORMAT " actual: " INTPTR_FORMAT, p2i(f.fp()), p2i(fp));
  return f.raw_pc() == pc && f.fp() == fp;
}
#endif

inline intptr_t** ContinuationHelper::Frame::callee_link_address(const frame& f) {
  return (intptr_t**)&f.own_abi()->callers_sp;
}

inline address* ContinuationHelper::InterpretedFrame::return_pc_address(const frame& f) {
  return (address*)&f.callers_abi()->return_pc;
}

inline void ContinuationHelper::InterpretedFrame::patch_sender_sp(frame& f, const frame& caller) {
  Unimplemented();
}

inline address* ContinuationHelper::Frame::return_pc_address(const frame& f) {
  // FIXME: sender_pc_addr() is private method, can we make it public ?? does the same thing
  return (address*)&f.callers_abi()->return_pc;
}

inline address ContinuationHelper::Frame::real_pc(const frame& f) {
  Unimplemented();
  return nullptr;
}

inline void ContinuationHelper::Frame::patch_pc(const frame& f, address pc) {
  Unimplemented();
}

inline intptr_t* ContinuationHelper::InterpretedFrame::frame_top(const frame& f, InterpreterOopMap* mask) { // inclusive; this will be copied with the frame
  Unimplemented();
  return nullptr;
}

inline intptr_t* ContinuationHelper::InterpretedFrame::frame_bottom(const frame& f) { // exclusive; this will not be copied with the frame
  Unimplemented();
  return nullptr;
}

inline intptr_t* ContinuationHelper::InterpretedFrame::frame_top(const frame& f, int callee_argsize, bool callee_interpreted) {
  Unimplemented();
  return nullptr;
}

inline intptr_t* ContinuationHelper::InterpretedFrame::callers_sp(const frame& f) {
  Unimplemented();
  return nullptr;
}

#endif // CPU_S390_CONTINUATIONHELPER_S390_INLINE_HPP
