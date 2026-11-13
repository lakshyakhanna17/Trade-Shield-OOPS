#include "SimulationManager.hpp"
#include "utils/TimeUtils.hpp"
#include <iostream>
#include <chrono>

namespace tradeshield {

SimulationManager::SimulationManager(TradingEngine& engine, TraderRepository& trader_repo)
    : engine_(engine),
      trader_repo_(trader_repo) {
    strategies_["NORMAL"] = std::make_shared<NormalBotStrategy>();
    strategies_["HIGH_VOLUME"] = std::make_shared<SuspiciousBotStrategy>();
    strategies_["RAPID_CANCEL"] = std::make_shared<RapidCancelBotStrategy>();
    strategies_["RANDOM"] = std::make_shared<RandomBotStrategy>();

    discoverBotTraders();
}

SimulationManager::~SimulationManager() {
    stop();
}

void SimulationManager::discoverBotTraders() {
    active_bot_ids_.clear();
    auto traders = trader_repo_.findAll();
    for (const auto& t : traders) {
        if (t && t->isBot()) {
            active_bot_ids_.push_back(t->getId());
        }
    }
}

void SimulationManager::registerStrategy(const std::string& name, std::shared_ptr<IBotStrategy> strategy) {
    if (!strategy) return;
    std::lock_guard<std::mutex> lock(sim_mutex_);
    strategies_[name] = std::move(strategy);
}

int SimulationManager::getTickIntervalMs() const {
    std::lock_guard<std::mutex> lock(sim_mutex_);
    return tick_interval_ms_;
}

bool SimulationManager::isRunning() const noexcept {
    return is_running_.load();
}

bool SimulationManager::start(int tick_interval_ms, const std::vector<int64_t>& selected_bot_ids) {
    if (is_running_.load()) {
        return false; // Prevent multiple concurrent worker threads
    }

    {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        tick_interval_ms_ = std::max(50, tick_interval_ms);

        if (!selected_bot_ids.empty()) {
            active_bot_ids_ = selected_bot_ids;
        } else {
            discoverBotTraders();
        }

        stop_requested_.store(false);
        is_running_.store(true);
    }

    worker_thread_ = std::make_unique<std::thread>(&SimulationManager::workerLoop, this);
    return true;
}

bool SimulationManager::stop() {
    if (!is_running_.load()) {
        return true; // Idempotent: already stopped
    }

    stop_requested_.store(true);
    cv_.notify_all();

    if (worker_thread_ && worker_thread_->joinable()) {
        worker_thread_->join();
    }
    worker_thread_.reset();

    is_running_.store(false);
    return true;
}

SimulationStatus SimulationManager::getStatus() const {
    std::lock_guard<std::mutex> lock(sim_mutex_);
    SimulationStatus status;
    status.is_running = is_running_.load();
    status.tick_interval_ms = tick_interval_ms_;
    status.total_ticks = total_ticks_;
    status.total_orders_generated = total_orders_generated_;
    status.total_executions_generated = total_executions_generated_;
    status.total_cancellations_generated = total_cancellations_generated_;
    status.total_rejections = total_rejections_;
    status.total_system_faults = total_system_faults_;
    status.total_errors = total_errors_;
    status.last_tick_time = last_tick_time_;

    for (int64_t bot_id : active_bot_ids_) {
        auto trader_opt = trader_repo_.findById(bot_id);
        if (trader_opt.has_value() && (*trader_opt)->isBot()) {
            status.active_bots.push_back({
                bot_id,
                (*trader_opt)->getName(),
                (*trader_opt)->getBotStrategy().value_or("NORMAL")
            });
        }
    }

    return status;
}

void SimulationManager::resetMetrics() {
    std::lock_guard<std::mutex> lock(sim_mutex_);
    total_ticks_ = 0;
    total_orders_generated_ = 0;
    total_executions_generated_ = 0;
    total_cancellations_generated_ = 0;
    total_rejections_ = 0;
    total_system_faults_ = 0;
    total_errors_ = 0;
    last_tick_time_.clear();
}

std::vector<BotActionResult> SimulationManager::stepTick() {
    std::vector<int64_t> bot_ids;
    {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        if (active_bot_ids_.empty()) {
            discoverBotTraders();
        }
        bot_ids = active_bot_ids_;
    }

    std::vector<BotActionResult> results;
    for (int64_t bot_id : bot_ids) {
        results.push_back(executeBotAction(bot_id));
    }

    {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        total_ticks_++;
        last_tick_time_ = TimeUtils::currentIsoTimestamp();
    }

    return results;
}

BotActionResult SimulationManager::executeBotAction(int64_t bot_id) {
    auto trader_opt = trader_repo_.findById(bot_id);
    if (!trader_opt.has_value() || !(*trader_opt)->isBot()) {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        total_system_faults_++;
        total_errors_++;
        return {BotActionType::NONE, false, "Trader #" + std::to_string(bot_id) + " not found or is not a bot"};
    }

    std::string strategy_name = (*trader_opt)->getBotStrategy().value_or("NORMAL");
    std::shared_ptr<IBotStrategy> strategy;

    {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        auto it = strategies_.find(strategy_name);
        if (it != strategies_.end()) {
            strategy = it->second;
        } else {
            strategy = strategies_["NORMAL"];
        }
    }

    if (!strategy) {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        total_system_faults_++;
        total_errors_++;
        return {BotActionType::NONE, false, "No valid strategy assigned for bot #" + std::to_string(bot_id)};
    }

    BotActionResult result;
    try {
        result = strategy->executeTick(engine_, bot_id);
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        total_system_faults_++;
        total_errors_++;
        return {BotActionType::NONE, false, "Bot execution exception: " + std::string(e.what())};
    } catch (...) {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        total_system_faults_++;
        total_errors_++;
        return {BotActionType::NONE, false, "Unknown bot execution exception"};
    }

    {
        std::lock_guard<std::mutex> lock(sim_mutex_);
        if (result.success) {
            if (result.action_type == BotActionType::BUY_ORDER || result.action_type == BotActionType::SELL_ORDER) {
                total_orders_generated_++;
                if (result.trade.has_value()) {
                    total_executions_generated_++;
                }
            } else if (result.action_type == BotActionType::CANCEL_ORDER) {
                total_cancellations_generated_++;
            }
        } else if (result.action_type != BotActionType::NONE) {
            // Pre-trade validation rejections from TradingEngine / OrderValidator
            total_rejections_++;
            total_errors_++;
        }
    }

    return result;
}

void SimulationManager::workerLoop() {
    while (!stop_requested_.load()) {
        std::vector<int64_t> bot_ids;
        int interval_ms = 2000;

        {
            std::lock_guard<std::mutex> lock(sim_mutex_);
            bot_ids = active_bot_ids_;
            interval_ms = tick_interval_ms_;
        }

        for (int64_t bot_id : bot_ids) {
            if (stop_requested_.load()) {
                break;
            }
            executeBotAction(bot_id);
        }

        if (stop_requested_.load()) {
            break;
        }

        {
            std::lock_guard<std::mutex> lock(sim_mutex_);
            total_ticks_++;
            last_tick_time_ = TimeUtils::currentIsoTimestamp();
        }

        std::unique_lock<std::mutex> lock(sim_mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(interval_ms), [this] {
            return stop_requested_.load();
        });
    }
}

} // namespace tradeshield
