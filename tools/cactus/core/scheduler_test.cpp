#include <catch2/catch_test_macros.hpp>

#include <cactus/cactus.h>

using namespace cactus;

std::function<void()> ExpectCancel(bool* flag) {
    return [flag] {
        try {
            while (true) {
                Yield();
            }
        } catch (const FiberCanceledException&) {
            *flag = true;
            throw;
        }
    };
}

TEST_CASE("Scheduler simple operation") {
    Scheduler scheduler;

    SECTION("Normal exit") {
        bool ok = false;
        scheduler.Run([&] { ok = true; });
        REQUIRE(ok);
    }

    SECTION("Error exit") {
        REQUIRE_THROWS_AS(scheduler.Run([&] { throw std::logic_error("tests"); }),
                          std::logic_error);
    }

    SECTION("Run single fiber") {
        bool ok = false;
        scheduler.Run([&] {
            Fiber fiber([&] { ok = true; });

            Yield();
        });

        REQUIRE(ok);
    }

    SECTION("Fiber cancelation at exit") {
        bool first_canceled = false;
        bool second_run = false;

        scheduler.Run([&] {
            SetFiberName("root");

            Fiber fiber0([&] {
                SetFiberName("fiber0");

                try {
                    Yield();
                } catch (const FiberCanceledException&) {
                    Fiber fiber1([&] {
                        SetFiberName("fiber1");

                        Yield();
                        second_run = true;
                    });

                    first_canceled = true;
                }
            });

            Yield();
        });

        REQUIRE(first_canceled);
        REQUIRE(!second_run);
    }

    SECTION("Nursery cancelation") {
        bool first_canceled = false;

        scheduler.Run([&] {
            { Fiber fiber(ExpectCancel(&first_canceled)); }

            REQUIRE(first_canceled);
        });
    }

    SECTION("Exception propagation from scheduler") {
        REQUIRE_THROWS_AS(scheduler.Run([&] { throw std::logic_error{"tests"}; }),
                          std::logic_error);
    }

    SECTION("Sleep ordering") {
        int i = 0;
        scheduler.FreezeTime();
        scheduler.Run([&] {
            Fiber fiber0([&] {
                SleepFor(std::chrono::microseconds(2));
                REQUIRE(i++ == 1);
            });
            Fiber fiber1([&] {
                scheduler.UnfreezeTime();
                SleepFor(std::chrono::microseconds(1));
                REQUIRE(i++ == 0);
            });

            SleepFor(std::chrono::microseconds(3));
            REQUIRE(i++ == 2);
        });
    }

    SECTION("Sleep cancelation") {
        bool canceled = true;
        scheduler.Run([&] {
            Fiber fiber([&] {
                try {
                    SleepFor(std::chrono::seconds(1));
                } catch (const FiberCanceledException&) {
                    canceled = true;
                    throw;
                }
            });

            Yield();
        });
        REQUIRE(canceled);
    }

    SECTION("With timeout") {
        bool timedout = true;
        scheduler.FreezeTime();
        scheduler.Run([&] {
            TimeoutGuard guard(std::chrono::seconds(5));

            scheduler.AdvanceTime(std::chrono::seconds(10));
            try {
                SleepFor(std::chrono::seconds(1));
            } catch (const TimeoutException&) {
                timedout = true;
            }
        });

        REQUIRE(timedout);
    }
}

TEST_CASE("Debug facilities") {
    REQUIRE(nullptr == GetCurrentFiber());
    REQUIRE(nullptr == GetCurrentScheduler());
}
