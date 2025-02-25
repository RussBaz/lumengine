#include "cxxLumengine.hpp"

namespace {
    struct CxxLumengineInitializer {
        CxxLumengineInitializer() {
            initialise_terminate_handler();
        }
    };
    CxxLumengineInitializer initializer;
}
