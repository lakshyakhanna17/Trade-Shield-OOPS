#pragma once

#include "IBotStrategy.hpp"
#include "NormalBotStrategy.hpp"
#include "SuspiciousBotStrategy.hpp"
#include "RapidCancelBotStrategy.hpp"
#include "RandomBotStrategy.hpp"
#include "engine/TradingEngine.hpp"
#include "database/repositories/TraderRepository.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace tradeshield {

/**
 * @brief Summary metadata for an active bot participating in the simulation.
 */
struct ActiveBotInfo {
    int64_t trader_id{0};
    std::string name;
    std::string strategy;
};

/**
 * @brief Real-time status report of the bot simulation engine.
 */
struct SimulationStatus {
    bool is_running{false};
    int tick_interval_ms{2000};
    uint64_t total_ticks{0};
    uint64_t total_orders_generated{0};
    uint64_t total_executions_generated{0};
    uint64_t total_cancellations_generated{0};
    uint64_t total_rejections{0};
    uint64_t total_system_faults{0};
    uint64_t total_errors{0};
    std::string last_tick_time;
    std::vector<ActiveBotInfo> active_bots;
};

/**
 * @brief Central controller and thread manager for automated simulation bots.
 * Governs lifecycle (start/stop), thread synchronization, strategy dispatch,
 * and simulation performance metrics.
 */
class SimulationManager {
public:
    SimulationManager(TradingEngine& engine, TraderRepository& trader_repo);
    ~SimulationManager();

    // Non-copyable, non-movable
    SimulationManager(const SimulationManager&) = delete;
    SimulationManager& operator=(const SimulationManager&) = delete;

    // Start simulation loop with configurable interval (ms) and optional bot ID filter
    bool start(int tick_interval_ms = 2000, const std::vector<int64_t>& selected_bot_ids = {});

    // Stop simulation loop gracefully and join background worker thread
    bool stop();

    // Query running state
    bool isRunning() const noexcept;

    // Query comprehensive simulation metrics
    SimulationStatus getStatus() const;

    // Reset in-memory simulation performance counters without affecting database records
    void resetMetrics();

    // Execute a single synchronous simulation tick across all active bots (test harness)
    std::vector<BotActionResult> stepTick();

    // Register or override a strategy instance
    void registerStrategy(const std::string& name, std::shared_ptr<IBotStrategy> strategy);

    // Get current interval
    int getTickIntervalMs() const;

private:
    TradingEngine& engine_;
    TraderRepository& trader_repo_;

    std::atomic<bool> is_running_{false};
    std::atomic<bool> stop_requested_{false};
    std::unique_ptr<std::thread> worker_thread_;

    mutable std::mutex sim_mutex_;
    std::condition_variable cv_;

    int tick_interval_ms_{2000};
    std::vector<int64_t> active_bot_ids_;

    uint64_t total_ticks_{0};
    uint64_t total_orders_generated_{0};
    uint64_t total_executions_generated_{0};
    uint64_t total_cancellations_generated_{0};
    uint64_t total_rejections_{0};
    uint64_t total_system_faults_{0};
    uint64_t total_errors_{0};
    std::string last_tick_time_;

    std::unordered_map<std::string, std::shared_ptr<IBotStrategy>> strategies_;

    void workerLoop();
    BotActionResult executeBotAction(int64_t bot_id);
    void discoverBotTraders();
};

} // namespace tradeshield
