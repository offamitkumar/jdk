/*
 * Copyright (c) 2026, IBM Corp. and/or its affiliates. All rights reserved.
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

/*
 * @test
 * @summary On s390x, class space reservation in the range [4GB, 4TB) must use
 *          4GB alignment so that the klass decoder can always use the aih
 *          (Add Immediate High) fast path. The upper search bound must also be
 *          capped at 4TB to avoid an irreversible Linux kernel page table upgrade.
 * @requires os.arch == "s390x"
 * @requires vm.flagless
 * @library /test/lib
 * @modules java.base/jdk.internal.misc
 *          java.management
 * @run driver S390ClassSpaceAihAlignment
 */

import jdk.test.lib.process.OutputAnalyzer;
import jdk.test.lib.process.ProcessTools;

import java.io.IOException;

public class S390ClassSpaceAihAlignment {

    private static OutputAnalyzer runWithSimulatedFullAddressSpace() throws IOException {
        ProcessBuilder pb = ProcessTools.createLimitedTestJavaProcessBuilder(
                "-Xshare:off",
                "-Xmx128m",
                "-XX:CompressedClassSpaceSize=128m",
                "-XX:-UseCompactObjectHeaders",
                "-XX:+UnlockDiagnosticVMOptions",
                "-Xlog:os+map=debug",
                "-XX:+SimulateFullAddressSpace",
                "-version");
        OutputAnalyzer out = new OutputAnalyzer(pb.start());
        out.reportDiagnosticSummary();
        return out;
    }

    // Step 3 of reserve_address_space_for_compressed_classes searches [4GB, 4TB).
    // It must use 4GB (nth_bit(32)) alignment so every candidate is 4GB-aligned —
    // the condition for the aih fast path in decode_klass_not_null().
    //
    // Log line format: reserve_between (range [<from>-<to>), size 0x<sz>, alignment 0x<align>, ...)
    // PTR_FORMAT = 16 lowercase hex digits; alignment printed without leading zeros.
    private static void testAihAlignment() throws IOException {
        runWithSimulatedFullAddressSpace().shouldMatch(
            "reserve_between \\(range \\[0x0000000100000000-0x0000040000000000\\)," +
            " size 0x[0-9a-f]+, alignment 0x100000000,");
    }

    // The upper bound of Step 3 must be capped at 4TB = nth_bit(42).
    // Any mmap hint at or above 4TB triggers an irreversible Linux kernel
    // page table upgrade from 3-level to 4-level, raising TLB miss cost JVM-wide.
    // PTR_FORMAT = 16 hex digits; 7th digit is nonzero for any address >= 4TB.
    private static void testPageTableExpansionBound() throws IOException {
        runWithSimulatedFullAddressSpace().shouldNotMatch(
            "reserve_between \\(range \\[0x00000[4-9a-f][0-9a-f]{10}-");
    }

    public static void main(String[] args) throws Exception {
        testAihAlignment();
        testPageTableExpansionBound();
    }
}
