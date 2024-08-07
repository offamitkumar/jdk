/*
 * Copyright (c) 1999, 2024, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
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

/**
 * Provides the means for dynamically plugging in support for accessing
 * naming and directory services through the {@code javax.naming}
 * and related packages.
 *
 * <p>
 * This package defines the service provider interface (SPI) of the Java Naming
 * and Directory Interface (JNDI). &nbsp;
 * JNDI provides naming and directory functionality to applications
 * written in the Java programming language. It is designed to be
 * independent of any specific naming or directory service
 * implementation. Thus a variety of services--new, emerging, and
 * already deployed ones--can be accessed in a common way.
 *
 * <p>
 * The JNDI SPI provides the means for creating JNDI service providers,
 * through which JNDI applications access different naming and
 * directory services.
 *
 *
 * <h2>Plug-in Architecture</h2>
 *
 * The service provider package allows different implementations to be plugged in
 * dynamically.
 * These different implementations include those for the
 * <em>initial context</em>,
 * and implementations for contexts that can be reached
 * from the initial context.
 *
 * <h2>Java Object Support</h2>
 *
 * The service provider package provides support
 * for implementors of the
 * {@code javax.naming.Context.lookup()}
 * method and related methods to return Java objects that are natural
 * and intuitive for the Java programmer.
 * For example, when looking up a printer name from the directory,
 * it is natural for you to expect to get
 * back a printer object on which to operate.
 *
 *
 * <h2>Multiple Naming Systems (Federation)</h2>
 *
 * JNDI operations allow applications to supply names that span multiple
 * naming systems. So in the process of completing
 * an operation, one service provider might need to interact
 * with another service provider, for example, to pass on
 * the operation to be continued in the next naming system.
 * The service provider package provides support for
 * different providers to cooperate to complete JNDI operations.
 *
 *
 * <h2>Package Specification</h2>
 *
 * The JNDI SPI Specification and related documents can be found in the
 * {@extLink jndi_overview JNDI documentation}.
 *
 * @since 1.3
 */
package javax.naming.spi;
