# Virtual-Memory-Manager# Virtual Memory Manager (C++)

## Overview

A two-level virtual memory manager implementing segmented paging with optional demand paging.

### Address Format (27-bit Virtual Address)

```
[ s : 9 bits ][ p : 9 bits ][ w : 9 bits ]
   segment       page         offset
```

### Memory Layout

| Structure | Location |
|-----------|----------|
| Physical Memory | `PM[524288]` integers |
| Disk (demand paging) | `D[1024][512]` integers |
| Segment Table (ST) | `PM[0..2*s+1]` (frame 0) |
| Page Tables (PT) | In PM frames, indexed by ST |

### Segment Table Entry (per segment `s`)

```
PM[2*s]   = PT frame number  (-1 if not in memory)
PM[2*s+1] = PT size (number of pages in this segment)
```

### Page Table Entry (per segment `s`, page `p`)

```
PM[pt_frame * 512 + p] = physical frame number  (-1 if not in memory)
```

---

## Building

```bash
g++ -std=c++17 -Wall -O2 -o VM_Manager VM_Manager.cpp
```

---

## Usage

### Basic Mode (60% credit — no demand paging)

```bash
./vm_manager <init_file> <va_file> <output_file>
```

### Extended Mode (100% credit — with demand paging)

```bash
./vm_manager <init_file> <va_file> <output_file> --demand
```

---

## Input File Formats

### Init File (2 lines)

**Line 1 — Segment Table entries** (triples: `s  pt_frame  pt_size`):
```
0 1 4   1 2 3
```
This sets:
- Segment 0: PT in frame 1, size 4 pages
- Segment 1: PT in frame 2, size 3 pages

**Line 2 — Page Table entries** (triples: `s  p  frame`):
```
0 0 5   0 1 7   0 2 9   1 0 13
```
This sets:
- Segment 0, Page 0 → frame 5
- Segment 0, Page 1 → frame 7
- Segment 0, Page 2 → frame 9
- Segment 1, Page 0 → frame 13

Use `-1` for "not in memory" (demand paging only):
```
0 -1 4   1 -1 3
```

### VA File

One integer virtual address per line:
```
0
512
1024
262144
```

### Output File

One result per VA line:
- Physical address (integer) on success
- `segmentation error` if segment invalid or page out of bounds
- `page error` if page not present (basic mode only)

---

## Translation Algorithm

```
decode VA => (s, p, w)
lookup ST: pt_frame = PM[2*s], pt_size = PM[2*s+1]
if pt_frame == -1 and pt_size == -1  → segmentation error
if p >= pt_size                       → segmentation error
if pt_frame == -1 (demand paging)    → load PT from disk, update ST
page_frame = PM[pt_frame * 512 + p]
if page_frame == -1 (demand paging)  → load page from disk, update PT
PA = page_frame * 512 + w
```

---

## Demand Paging — Disk Layout

When demand paging is enabled:

- **PT miss** (segment `s`): PT data loaded from disk row `s`
- **Page miss** (segment `s`, page `p`): page data loaded from disk row `(s * 512 + p) % 1024`

No page replacement algorithm is needed (PM always has free frames per spec).