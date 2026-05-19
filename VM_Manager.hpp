#pragma once

#include <string>
#include <ostream>

// ─── Constants ────────────────────────────────────────────────────────────────
static const int PM_SIZE   = 524288;  
static const int DISK_ROWS = 1024;
static const int DISK_COLS = 512;
static const int PAGE_SIZE = 512;     // 2^9 words per frame/page
static const int INVALID = -1;

// Bit widths
static const int BITS_W  = 9;
static const int BITS_P  = 9;
static const int BITS_S  = 9;

// Masks
static const int MASK_W  = 0x1FF;    // 9 bits
static const int MASK_P  = 0x1FF;    // 9 bits
static const int MASK_PW = 0x3FFFF;  // 18 bits (p + w)

// ─── VMManager ───────────────────────────────────────────────────────────────
class VMManager {
public:
    explicit VMManager(bool demand_paging);
    void load_init_file(const std::string &filename);
    int translate(int va, std::ostream &log);
    void run(const std::string &va_file, const std::string &output_file);

private:
    int  PM[PM_SIZE];
    int  D[DISK_ROWS][DISK_COLS];
    bool demand_paging_;

    // free frame list (demand paging)
    int free_frames_[PM_SIZE / PAGE_SIZE]; 
    int free_frame_count_;

    void decode_va(int va, int &s, int &p, int &w, int &pw) const;
    int  alloc_frame();
    void read_block(int b, int f);

    // ST accessors: PM[2s] = size, PM[2s+1] = PT frame
    int  get_seg_size    (int s) const { return PM[2 * s];     }
    int  get_pt_frame    (int s) const { return PM[2 * s + 1]; }
    void set_pt_frame    (int s, int f) { PM[2 * s + 1] = f;  }

    // PT accessor: PM[pt_frame*512 + p] = page frame
    int  get_page_frame  (int pt_frame, int p) const { return PM[pt_frame * PAGE_SIZE + p]; }
    void set_page_frame  (int pt_frame, int p, int f) { PM[pt_frame * PAGE_SIZE + p] = f;  }
};