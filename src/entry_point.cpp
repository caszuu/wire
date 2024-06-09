#include "server.hpp"
#include "node.hpp"
#include "wios.hpp"

#include <iostream>

int main() {
    std::cout << "==== Starting W.I.R.E v0 ====" << '\n';

    wire::KernSourceStorage storage{};

    wire::WireNode node{5465465};
    node.tick();

    return 0;
}