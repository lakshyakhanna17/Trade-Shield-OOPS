#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <exception>

namespace tradeshield::test {

struct TestCase {
    std::string name;
    std::function<void()> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void addTest(const std::string& name, std::function<void()> func) {
        tests_.push_back({name, std::move(func)});
    }

    int runAll() {
        int passed = 0;
        int failed = 0;

        std::cout << "\n==================================================\n";
        std::cout << "        TradeShield Test Runner (C++17)           \n";
        std::cout << "==================================================\n\n";

        for (const auto& test : tests_) {
            current_test_failed_ = false;
            current_test_name_ = test.name;

            std::cout << "[ RUN      ] " << test.name << std::endl;
            try {
                test.func();
            } catch (const std::exception& e) {
                std::cout << "  [EXCEPTION] Unexpected exception caught: " << e.what() << std::endl;
                current_test_failed_ = true;
            } catch (...) {
                std::cout << "  [EXCEPTION] Unknown non-std exception caught!" << std::endl;
                current_test_failed_ = true;
            }

            if (current_test_failed_) {
                std::cout << "[     FAIL ] " << test.name << "\n\n";
                failed++;
            } else {
                std::cout << "[       OK ] " << test.name << "\n";
                passed++;
            }
        }

        std::cout << "\n==================================================\n";
        std::cout << "TEST SUMMARY: " << passed << " passed, " << failed << " failed, "
                  << tests_.size() << " total\n";
        std::cout << "==================================================\n";

        return failed == 0 ? 0 : 1;
    }

    void recordFailure(const std::string& file, int line, const std::string& expr, const std::string& msg = "") {
        current_test_failed_ = true;
        std::cout << "  [FAIL] " << file << ":" << line << " | Expression: " << expr;
        if (!msg.empty()) {
            std::cout << " | " << msg;
        }
        std::cout << std::endl;
    }

private:
    std::vector<TestCase> tests_;
    bool current_test_failed_{false};
    std::string current_test_name_;
};

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> func) {
        TestRegistry::instance().addTest(name, std::move(func));
    }
};

} // namespace tradeshield::test

#define TS_CONCAT_INNER(a, b) a ## b
#define TS_CONCAT(a, b) TS_CONCAT_INNER(a, b)

#define TEST_CASE(name) \
    static void TS_CONCAT(test_func_, __LINE__)(); \
    static ::tradeshield::test::TestRegistrar TS_CONCAT(registrar_, __LINE__)(name, TS_CONCAT(test_func_, __LINE__)); \
    static void TS_CONCAT(test_func_, __LINE__)()

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, #condition); \
        } \
    } while (0)

#define CHECK_TRUE(condition) CHECK(condition)

#define CHECK_FALSE(condition) \
    do { \
        if (condition) { \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, "!(" #condition ")"); \
        } \
    } while (0)

#define CHECK_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            std::stringstream ss; \
            ss << "Expected [" << (a) << "] == [" << (b) << "]"; \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, #a " == " #b, ss.str()); \
        } \
    } while (0)

#define CHECK_NE(a, b) \
    do { \
        if (!((a) != (b))) { \
            std::stringstream ss; \
            ss << "Expected [" << (a) << "] != [" << (b) << "]"; \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, #a " != " #b, ss.str()); \
        } \
    } while (0)

#define CHECK_THROWS_AS(expr, ExceptionType) \
    do { \
        bool caught = false; \
        try { \
            expr; \
        } catch (const ExceptionType&) { \
            caught = true; \
        } catch (const std::exception& e) { \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, #expr, \
                std::string("Caught unexpected exception type: ") + e.what()); \
            caught = true; \
        } catch (...) { \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, #expr, \
                "Caught non-std exception"); \
            caught = true; \
        } \
        if (!caught) { \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, #expr, \
                "Expected exception " #ExceptionType " was not thrown"); \
        } \
    } while (0)

#define CHECK_NOTHROW(expr) \
    do { \
        try { \
            expr; \
        } catch (const std::exception& e) { \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, #expr, \
                std::string("Expected no throw, but caught: ") + e.what()); \
        } catch (...) { \
            ::tradeshield::test::TestRegistry::instance().recordFailure(__FILE__, __LINE__, #expr, \
                "Expected no throw, but caught unknown exception"); \
        } \
    } while (0)
