/*
 * Copyright (c) 2024, Oracle and/or its affiliates. All rights reserved.
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
 */

package compiler.gcbarriers;

import compiler.lib.ir_framework.*;
import java.lang.invoke.VarHandle;
import java.lang.invoke.MethodHandles;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.SoftReference;
import java.lang.ref.WeakReference;
import jdk.test.lib.Asserts;

/**
 * @test
 * @summary Test that G1 barriers are generated and optimized as expected.
 * @library /test/lib /
 * @requires vm.gc.G1
 * @run driver compiler.gcbarriers.TestG1BarrierGeneration
 */

public class TestG1BarrierGeneration {
    static final String PRE_ONLY = "pre";
    static final String POST_ONLY = "post";
    static final String PRE_AND_POST = "pre post";
    static final String PRE_AND_POST_NOT_NULL = "pre post notnull";

    static class Outer {
        Object f;
    }

    static class OuterWithFewFields implements Cloneable {
        Object f1;
        Object f2;
        public Object clone() throws CloneNotSupportedException {
            return super.clone();
        }
    }

    static class OuterWithManyFields implements Cloneable {
        Object f1;
        Object f2;
        Object f3;
        Object f4;
        Object f5;
        Object f6;
        Object f7;
        Object f8;
        Object f9;
        Object f10;
        public Object clone() throws CloneNotSupportedException {
            return super.clone();
        }
    }

    static final VarHandle fVarHandle;
    static {
        MethodHandles.Lookup l = MethodHandles.lookup();
        try {
            fVarHandle = l.findVarHandle(Outer.class, "f", Object.class);
        } catch (Exception e) {
            throw new Error(e);
        }
    }

    public static void main(String[] args) {
        TestFramework framework = new TestFramework();
        Scenario[] scenarios = new Scenario[2*2];
        int scenarioIndex = 0;
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                scenarios[scenarioIndex] =
                    new Scenario(scenarioIndex,
                                 "-XX:CompileCommand=inline,java.lang.ref.*::*",
                                 "-XX:" + (i == 0 ? "-" : "+") + "UseCompressedOops",
                                 "-XX:" + (j == 0 ? "-" : "+") + "ReduceInitialCardMarks");
                scenarioIndex++;
            }
        }
        framework.addScenarios(scenarios);
        framework.start();
    }

    @Test
    public static Object[] testCloneArrayOfObjects(Object[] a) {
        Object[] a1 = null;
        try {
            a1 = a.clone();
        } catch (Exception e) {}
        return a1;
    }

    @Test
    @IR(applyIf = {"ReduceInitialCardMarks", "true"},
        failOn = {IRNode.G1_STORE_P, IRNode.G1_STORE_N, IRNode.G1_ENCODE_P_AND_STORE_N},
        phase = CompilePhase.FINAL_CODE)
    @IR(applyIfAnd = {"ReduceInitialCardMarks", "false", "UseCompressedOops", "false"},
        counts = {IRNode.G1_STORE_P_WITH_BARRIER_FLAG, POST_ONLY, "2"},
        phase = CompilePhase.FINAL_CODE)
    @IR(applyIfAnd = {"ReduceInitialCardMarks", "false", "UseCompressedOops", "true"},
        counts = {IRNode.G1_STORE_N_WITH_BARRIER_FLAG, POST_ONLY, "2"},
        phase = CompilePhase.FINAL_CODE)
    public static OuterWithFewFields testCloneObjectWithFewFields(OuterWithFewFields o) {
        Object o1 = null;
        try {
            o1 = o.clone();
        } catch (Exception e) {}
        return (OuterWithFewFields)o1;
    }

    @Test
    @IR(applyIf = {"ReduceInitialCardMarks", "true"},
        counts = {IRNode.CALL_OF, "jlong_disjoint_arraycopy", "1"})
    @IR(applyIf = {"ReduceInitialCardMarks", "false"},
        counts = {IRNode.CALL_OF, "G1BarrierSetRuntime::clone", "1"})
    public static OuterWithManyFields testCloneObjectWithManyFields(OuterWithManyFields o) {
        Object o1 = null;
        try {
            o1 = o.clone();
        } catch (Exception e) {}
        return (OuterWithManyFields)o1;
    }

    @Run(test = {"testCloneArrayOfObjects",
                 "testCloneObjectWithFewFields",
                 "testCloneObjectWithManyFields"})
    public void runCloneTests() {
        {
            Object o1 = new Object();
            Object[] a = new Object[4];
            for (int i = 0; i < 4; i++) {
                a[i] = o1;
            }
            Object[] a1 = testCloneArrayOfObjects(a);
            for (int i = 0; i < 4; i++) {
                Asserts.assertEquals(o1, a1[i]);
            }
        }
        {
            Object a = new Object();
            Object b = new Object();
            OuterWithFewFields o = new OuterWithFewFields();
            o.f1 = a;
            o.f2 = b;
            OuterWithFewFields o1 = testCloneObjectWithFewFields(o);
            Asserts.assertEquals(a, o1.f1);
            Asserts.assertEquals(b, o1.f2);
        }
        {
            Object a = new Object();
            Object b = new Object();
            Object c = new Object();
            Object d = new Object();
            Object e = new Object();
            Object f = new Object();
            Object g = new Object();
            Object h = new Object();
            Object i = new Object();
            Object j = new Object();
            OuterWithManyFields o = new OuterWithManyFields();
            o.f1 = a;
            o.f2 = b;
            o.f3 = c;
            o.f4 = d;
            o.f5 = e;
            o.f6 = f;
            o.f7 = g;
            o.f8 = h;
            o.f9 = i;
            o.f10 = j;
            OuterWithManyFields o1 = testCloneObjectWithManyFields(o);
            Asserts.assertEquals(a, o1.f1);
            Asserts.assertEquals(b, o1.f2);
            Asserts.assertEquals(c, o1.f3);
            Asserts.assertEquals(d, o1.f4);
            Asserts.assertEquals(e, o1.f5);
            Asserts.assertEquals(f, o1.f6);
            Asserts.assertEquals(g, o1.f7);
            Asserts.assertEquals(h, o1.f8);
            Asserts.assertEquals(i, o1.f9);
            Asserts.assertEquals(j, o1.f10);
        }
    }
    }
//
//     @Test
//     @IR(applyIf = {"UseCompressedOops", "false"},
//         counts = {IRNode.G1_COMPARE_AND_EXCHANGE_P_WITH_BARRIER_FLAG, PRE_AND_POST, "1"},
//         phase = CompilePhase.FINAL_CODE)
//     @IR(applyIf = {"UseCompressedOops", "true"},
//         counts = {IRNode.G1_COMPARE_AND_EXCHANGE_N_WITH_BARRIER_FLAG, PRE_AND_POST, "1"},
//         phase = CompilePhase.FINAL_CODE)
//     static Object testCompareAndExchange(Outer o, Object oldVal, Object newVal) {
//         return fVarHandle.compareAndExchange(o, oldVal, newVal);
//     }

//     @Test
//     @IR(applyIf = {"UseCompressedOops", "false"},
//         counts = {IRNode.G1_COMPARE_AND_SWAP_P_WITH_BARRIER_FLAG, PRE_AND_POST, "1"},
//         phase = CompilePhase.FINAL_CODE)
//     @IR(applyIf = {"UseCompressedOops", "true"},
//         counts = {IRNode.G1_COMPARE_AND_SWAP_N_WITH_BARRIER_FLAG, PRE_AND_POST, "1"},
//         phase = CompilePhase.FINAL_CODE)
//     static boolean testCompareAndSwap(Outer o, Object oldVal, Object newVal) {
//         return fVarHandle.compareAndSet(o, oldVal, newVal);
//     }
//
//     @Test
//     @IR(applyIf = {"UseCompressedOops", "false"},
//         counts = {IRNode.G1_GET_AND_SET_P_WITH_BARRIER_FLAG, PRE_AND_POST, "1"},
//         phase = CompilePhase.FINAL_CODE)
//     @IR(applyIf = {"UseCompressedOops", "true"},
//         counts = {IRNode.G1_GET_AND_SET_N_WITH_BARRIER_FLAG, PRE_AND_POST, "1"},
//         phase = CompilePhase.FINAL_CODE)
//     static Object testGetAndSet(Outer o, Object newVal) {
//         return fVarHandle.getAndSet(o, newVal);
//     }

//     @Run(test = {"testCompareAndExchange"})//,
                 //"testCompareAndSwap",
//                  //"testGetAndSet"}*/)
//     public void runAtomicTests() {
//         {
//             Outer o = new Outer();
//             Object oldVal = new Object();
//             o.f = oldVal;
//             Object newVal = new Object();
//             Object oldVal2 = testCompareAndExchange(o, oldVal, newVal);
//             Asserts.assertEquals(oldVal, oldVal2);
//             Asserts.assertEquals(o.f, newVal);
//         }
//         {
//             Outer o = new Outer();
//             Object oldVal = new Object();
//             o.f = oldVal;
//             Object newVal = new Object();
//             boolean b = testCompareAndSwap(o, oldVal, newVal);
//             Asserts.assertTrue(b);
//             Asserts.assertEquals(o.f, newVal);
//         }
//         {
//             Outer o = new Outer();
//             Object oldVal = new Object();
//             o.f = oldVal;
//             Object newVal = new Object();
//             Object oldVal2 = testGetAndSet(o, newVal);
//             Asserts.assertEquals(oldVal, oldVal2);
//             Asserts.assertEquals(o.f, newVal);
//         }
