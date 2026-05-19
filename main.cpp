#include "VM_Manager.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    // Usage: vm_manager <init_file> <va_file> <output_file> [--demand]
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <init_file> <va_file> <output_file> [--demand]\n";
        return 1;
    }

    bool demand_paging = (argc >= 5 && std::string(argv[4]) == "--demand");

    try {
        VMManager vm(demand_paging);
        vm.load_init_file(argv[1]);
        vm.run(argv[2], argv[3]);
        std::cout << "Done. Output written to " << argv[3] << "\n";
    } catch (const std::exception &e) {
        std::cerr << "-1";
        return 1;
    }

    return 0;
}