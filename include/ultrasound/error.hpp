#pragma once

#include <cstdint>
#include <string>

namespace ultrasound {

enum class ErrorCode {
    Ok = 0,
    OutOfOrderTimestamp,
    MissingVehicleState,
    InvalidInput,
    InternalError
};

struct Status {
    ErrorCode code{ErrorCode::Ok};
    std::string message{};

    static Status ok() {
        return Status{};
    }

    static Status fail(ErrorCode c, std::string m) {
        Status s;
        s.code = c;
        s.message = std::move(m);
        return s;
    }

    bool is_ok() const {
        return code == ErrorCode::Ok;
    }
};

}  // namespace ultrasound
