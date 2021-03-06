/*
 * The Qubes OS Project, http://www.qubes-os.org
 *
 * Copyright (c) Invisible Things Lab
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#pragma once

#define PAGE_SIZE 0x1000

// registry configuration key, user mode and kernel mode names
#define REG_CONFIG_USER_KEY     L"Software\\Invisible Things Lab\\Qubes Tools"
#define REG_CONFIG_KERNEL_KEY   L"\\Registry\\Machine\\Software\\Invisible Things Lab\\Qubes Tools"

// value names in registry config
#define REG_CONFIG_FPS_VALUE        L"MaxFps"
#define REG_CONFIG_DIRTY_VALUE      L"UseDirtyBits"
#define REG_CONFIG_CURSOR_VALUE     L"DisableCursor"
#define REG_CONFIG_SEAMLESS_VALUE   L"SeamlessMode"

// path to the executable to launch at system start (done by helper service)
#define REG_CONFIG_AUTOSTART_VALUE  L"Autostart"

// event created by the helper service, trigger to simulate SAS (ctrl-alt-delete)
#define QGA_SAS_EVENT_NAME L"Global\\QGA_SAS_TRIGGER"

// When signaled, causes agent to shutdown gracefully.
#define QGA_SHUTDOWN_EVENT_NAME L"Global\\QGA_SHUTDOWN"

// these are hardcoded
#define	MIN_RESOLUTION_WIDTH	320UL
#define	MIN_RESOLUTION_HEIGHT	200UL

#define ALIGN(x, a)	(((x) + (a) - 1) & ~((a) - 1))
#define	FRAMEBUFFER_PAGE_COUNT(width, height)	(ALIGN(((width)*(height)*4), PAGE_SIZE) / PAGE_SIZE)

#ifdef _X86_
typedef ULONG PFN_NUMBER;
#else
typedef ULONG64 PFN_NUMBER;
#endif

// size of PFN_ARRAY
#define PFN_ARRAY_SIZE(width, height) ((FRAMEBUFFER_PAGE_COUNT(width, height) * sizeof(PFN_NUMBER)) + sizeof(UINT32))

#pragma warning(push)
#pragma warning(disable: 4200) // zero-sized array

// NOTE: this struct is variable size, use FRAMEBUFFER_PAGE_COUNT
typedef struct _PFN_ARRAY
{
    ULONG NumberOf4kPages;
    PFN_NUMBER Pfn[0];
} PFN_ARRAY, *PPFN_ARRAY;

#pragma warning(pop)

// User mode -> display interface

#define	QVIDEO_MAGIC	0x49724515
#define	QVIDEO_ESC_BASE	0x11000

#define QVESC_SUPPORT_MODE          (QVIDEO_ESC_BASE + 0)
#define QVESC_GET_SURFACE_DATA      (QVIDEO_ESC_BASE + 1)
#define QVESC_WATCH_SURFACE         (QVIDEO_ESC_BASE + 2)
#define QVESC_STOP_WATCHING_SURFACE (QVIDEO_ESC_BASE + 3)
#define QVESC_GET_PFN_LIST          (QVIDEO_ESC_BASE + 4)
#define QVESC_SYNCHRONIZE           (QVIDEO_ESC_BASE + 5)
#define QVESC_RELEASE_SURFACE_DATA  (QVIDEO_ESC_BASE + 6)

// 0 is defined as "not supported" for DrvEscape
#define QV_NOT_SUPPORTED 0
#define QV_SUCCESS 1
#define QV_INVALID_PARAMETER 2
#define QV_SUPPORT_MODE_INVALID_RESOLUTION 3
#define QV_SUPPORT_MODE_INVALID_BPP 4
#define QV_INVALID_HANDLE 5
#define QV_MAP_ERROR 6

#define	IS_RESOLUTION_VALID(uWidth, uHeight)	((MIN_RESOLUTION_WIDTH <= (uWidth)) && (MIN_RESOLUTION_HEIGHT <= (uHeight)))

// gui agent-qvideo interface
typedef struct _QV_SUPPORT_MODE
{
    ULONG Magic;		// must be present at the top of every QV_ structure

    ULONG Height;
    ULONG Width;
    ULONG Bpp;
} QV_SUPPORT_MODE;

// maps surface's PFN array into the client process
typedef struct _QV_GET_SURFACE_DATA
{
    ULONG Magic;		// must be present at the top of every QV_ structure
    // A surface handle is converted automatically by GDI from HDC to SURFOBJ, so do not pass it here
} QV_GET_SURFACE_DATA;

typedef struct _QV_GET_SURFACE_DATA_RESPONSE
{
    ULONG Magic;		// must be present at the top of every QV_ structure

    ULONG Width;
    ULONG Height;
    ULONG Stride;
    ULONG Bpp;
    BOOLEAN IsScreen;
    PPFN_ARRAY PfnArray; // this is a user-mode mapped address of the surface's pfn array from the kernel, the array is read-only
} QV_GET_SURFACE_DATA_RESPONSE;

// unmap PFN array from the client process
typedef struct _QV_RELEASE_SURFACE_DATA
{
    ULONG Magic;		// must be present at the top of every QV_ structure
    // A surface handle is converted automatically by GDI from HDC to SURFOBJ, so do not pass it here
} QV_RELEASE_SURFACE_DATA;

typedef struct _QV_WATCH_SURFACE
{
    ULONG Magic;		// must be present at the top of every QV_ structure

    HANDLE DamageEvent;
} QV_WATCH_SURFACE;

typedef struct _QV_STOP_WATCHING_SURFACE
{
    ULONG Magic;		// must be present at the top of every QV_ structure

    // A surface handle is converted automatically by GDI from HDC to SURFOBJ, so do not pass it here
} QV_STOP_WATCHING_SURFACE;

// gui agent->display: confirmation that all dirty page data has been read
typedef struct _QV_SYNCHRONIZE
{
    ULONG Magic;		// must be present at the top of every QV_ structure

    // A surface handle is converted automatically by GDI from HDC to SURFOBJ, so do not pass it here
} QV_SYNCHRONIZE;

#pragma warning(push)
#pragma warning(disable: 4200) // zero-sized array
// Structure describing dirty pages of surface memory.
// Maintained by the display driver, mapped as QvideoDirtyPages_* section.
typedef struct _QV_DIRTY_PAGES
{
    // User mode client sets this to 1 after it reads the data (indirectly by QVESC_SYNCHRONIZE).
    // Driver overwrites the dirty bitfield with fresh data if this is 1 (and sets it to 0).
    // If this is 0, driver ORs the bitfield with new data to accumulate changes
    // until the client reads everything.
    LONG Ready;

    // Bitfield describing which surface memory pages changed since the last check.
    // number_of_pages = screen_width*screen_height*4 //32bpp
    // Size of DirtyBits array (in bytes) = (number_of_pages >> 3) + 1
    // Bit set means that the corresponding memory page has changed.
    UCHAR DirtyBits[0];
} QV_DIRTY_PAGES, *PQV_DIRTY_PAGES;
#pragma warning(pop)

#define BIT_GET(array, bit_number) (array[(bit_number)/8] & (1 << ((bit_number) % 8)))
#define BIT_SET(array, bit_number) (array[(bit_number)/8] |= (1 << ((bit_number) % 8)))
#define BIT_CLEAR(array, bit_number) (array[(bit_number)/8] &= ~(1 << ((bit_number) % 8)))

// Display -> Miniport interface

// TODO? all those functions can be moved to display since we link with kernel anyway now...
#define	QVMINI_DEVICE	0x0a000

#define IOCTL_QVMINI_ALLOCATE_MEMORY    (ULONG)(CTL_CODE(QVMINI_DEVICE, 1, METHOD_BUFFERED, FILE_ANY_ACCESS))
#define IOCTL_QVMINI_FREE_MEMORY        (ULONG)(CTL_CODE(QVMINI_DEVICE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS))
#define IOCTL_QVMINI_MAP_PFNS           (ULONG)(CTL_CODE(QVMINI_DEVICE, 3, METHOD_BUFFERED, FILE_ANY_ACCESS))
#define IOCTL_QVMINI_UNMAP_PFNS         (ULONG)(CTL_CODE(QVMINI_DEVICE, 4, METHOD_BUFFERED, FILE_ANY_ACCESS))

typedef struct _QVMINI_ALLOCATE_MEMORY
{
    ULONG Size;
    BOOLEAN UseDirtyBits;
} QVMINI_ALLOCATE_MEMORY, *PQVMINI_ALLOCATE_MEMORY;

typedef struct _QVMINI_ALLOCATE_MEMORY_RESPONSE
{
    PVOID KernelVa;
    PPFN_ARRAY PfnArray;
    PQV_DIRTY_PAGES DirtyPages;
} QVMINI_ALLOCATE_MEMORY_RESPONSE, *PQVMINI_ALLOCATE_MEMORY_RESPONSE;

typedef struct _QVMINI_FREE_MEMORY
{
    PVOID KernelVa;
} QVMINI_FREE_MEMORY, *PQVMINI_FREE_MEMORY;

typedef struct _QVMINI_MAP_PFNS
{
    PVOID KernelVa;
} QVMINI_MAP_PFNS, *PQVMINI_MAP_PFNS;

typedef struct _QVMINI_MAP_PFNS_RESPONSE
{
    PVOID UserVa;
} QVMINI_MAP_PFNS_RESPONSE, *PQVMINI_MAP_PFNS_RESPONSE;

typedef struct _QVMINI_UNMAP_PFNS
{
    PVOID KernelVa;
} QVMINI_UNMAP_PFNS, *PQVMINI_UNMAP_PFNS;
