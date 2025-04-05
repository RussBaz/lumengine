#ifndef LE_WORKLOAD_HPP
#define LE_WORKLOAD_HPP

#include "swift_function_wrapper.hpp"
#include "variant_wrapper.hpp"
#include <optional>

#include "server.hpp"

using PointInTime = std::chrono::time_point<std::chrono::steady_clock>;

struct ExecuteNow {};
struct ExecuteAt {
    PointInTime start_time;
};
struct ExecuteAfter {
    std::chrono::nanoseconds delay;
};

using ExecuteSchedule = std::variant<ExecuteNow, ExecuteAt, ExecuteAfter>;
using ExecuteScheduleVariant = VariantWrapper<ExecuteSchedule>;

using FunctionWorkload = SwiftFunctionWrapper<void, void>;
struct StartServerWorkload {
    ServerConfigPtr config;
};
struct StopServerWorkload {
    int port { 8080 };
};

using WorkloadType = std::variant<FunctionWorkload, StartServerWorkload, StopServerWorkload>;
using WorkloadTypeVariant = VariantWrapper<WorkloadType>;

struct Workload {
    WorkloadTypeVariant workload;
    std::optional<SwiftFunctionWrapper<void, std::error_code>> callback { std::nullopt };

    // Correct swift closures must be provided. Their types are not verified at compile time.
    // Incorrect function signatures will result in a runtime failure and terminate the library.
    // Provided closures must not throw unhandled exceptions.
    static Workload create_function(void* swift_function, void* callback = nullptr) {
        Workload w { WorkloadTypeVariant(FunctionWorkload(swift_function)) };
        if (callback) {
            w.callback = SwiftFunctionWrapper<void, std::error_code>(callback);
        }

        return w;
    }
    static Workload create_start_server(ServerConfigPtr config, void* callback = nullptr) {
        Workload w { WorkloadTypeVariant(StartServerWorkload { std::move(config) }) };
        if (callback) {
            w.callback = SwiftFunctionWrapper<void, std::error_code>(callback);
        }

        return w;
    }
    static Workload create_stop_server(const int port, void* callback = nullptr) {
        Workload w { WorkloadTypeVariant(StopServerWorkload { port }) };
        if (callback) {
            w.callback = SwiftFunctionWrapper<void, std::error_code>(callback);
        }

        return w;
    }
};

using WorkloadPtr = std::shared_ptr<Workload>;

#endif //LE_WORKLOAD_HPP
