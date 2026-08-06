#include "burstline/transport.hpp"

namespace burstline {

bool RequestResult::has_transport_error() const noexcept {
    return !error.empty();
}

}
