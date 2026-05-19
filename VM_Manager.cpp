#include "VM_Manager.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cmath>

VMManager::VMManager(bool demand_paging)
    : demand_paging_(demand_paging), free_frame_count_(0)
{
    for (int i = 0; i < PM_SIZE; i++) { PM[i] = INVALID; }

    if (demand_paging_) {
        for (int i = 0; i < DISK_ROWS; i++)
            for (int j = 0; j < DISK_COLS; j++)
                D[i][j] = 0;
    }
}

// va layout (32 bits): [ unused: 5 ][ s: 9 ][ p: 9 ][ w: 9 ]
void VMManager::decode_va(int va, int &s, int &p, int &w, int &pw) const {
    w  =  va        & MASK_W;   // bottom 9 bits
    p  = (va >>  9) & MASK_P;   // next 9 bits
    s  = (va >> 18) & MASK_W;   // next 9 bits
    pw =  va        & MASK_PW;  // bottom 18 bits (offset into segment)
}

int VMManager::alloc_frame() {
    if (free_frame_count_ == 0)
        throw std::runtime_error("No free frames available");
    return free_frames_[--free_frame_count_];
}

void VMManager::read_block(int block, int frame) {
    // copy disk block into PM frame
    int pm_start = frame * PAGE_SIZE;
    for (int i = 0; i < PAGE_SIZE; i++)
        PM[pm_start + i] = D[block][i];
}

void VMManager::load_init_file(const std::string &filename) {
    std::ifstream fin(filename);
    if (!fin.is_open())
        throw std::runtime_error("Cannot open init file: " + filename);

    std::string line1, line2;
    std::getline(fin, line1);
    std::getline(fin, line2);

    bool used[PM_SIZE / PAGE_SIZE] = {};
    used[0] = true; // st occupies frames 0 and 1
    used[1] = true;

    // line 1: ST entries => triples (segNum, segSize, ptFrame)
    {
        std::istringstream ss(line1);
        int segNum, segSize, ptFrame;
        while (ss >> segNum >> segSize >> ptFrame) {
            PM[2 * segNum]     = segSize;  // segment size
            PM[2 * segNum + 1] = ptFrame;  // PT frame (positive) or -block (negative)
            if (ptFrame > 0) { used[ptFrame] = true; }
        }
    }

    // line 2: PT entries => triples (s, p, f)
    // f positive -> page is in frame f
    // f negative -> page is on disk block f
    {
        std::istringstream ss(line2);
        int s, p, f;
        while (ss >> s >> p >> f) {
            int pt_frame = get_pt_frame(s);

            if (pt_frame > 0) {
                // PT is resident in PM — store directly
                set_page_frame(pt_frame, p, f);
                if (f > 0) used[f] = true;
            } else {
                // PT is on disk — store into D[block][p]
                int block = -pt_frame;
                D[block][p] = f;
            }
        }
    }

    // build free frame list (demand paging only)
    if (demand_paging_) {
        int total_frames = PM_SIZE / PAGE_SIZE;  // should be: 1024
        for (int i = total_frames - 1; i >= 0; --i) {
            if (!used[i])
                free_frames_[free_frame_count_++] = i;
        }
    }
}

int VMManager::translate(int va, std::ostream &log) {
    int s, p, w, pw;
    decode_va(va, s, p, w, pw);

    // bounds check
    if (pw >= get_seg_size(s)) {
        return -1;
    }

    // pt check
    int pt_frame = get_pt_frame(s);

    if (pt_frame < 0) {
        if (!demand_paging_) { return -1; }
        int f1 = alloc_frame();
        int block = -pt_frame;
        read_block(block, f1);
        set_pt_frame(s, f1);
        pt_frame = f1;
    }

    // page check
    int page_frame = get_page_frame(pt_frame, p);

    if (page_frame < 0) {
        if (!demand_paging_) { return -1; }
        int f2 = alloc_frame();
        int block = -page_frame;
        read_block(block, f2);
        set_page_frame(pt_frame, p, f2);
        page_frame = f2;
    }

    // form pa
    return page_frame * PAGE_SIZE + w;
}

void VMManager::run(const std::string &va_file, const std::string &output_file) {
    std::ifstream va_in(va_file);
    if (!va_in.is_open())
        throw std::runtime_error("Cannot open VA file: " + va_file);

    std::ofstream out(output_file);
    if (!out.is_open())
        throw std::runtime_error("Cannot open output file: " + output_file);

    int va;
    bool first = true;
    while (va_in >> va) {
        if (!first) out << " ";
        int pa = translate(va, out);
        if (pa >= 0)
            out << pa;
        else
            out << -1;
        first = false;
    }
    out << "\n";
}