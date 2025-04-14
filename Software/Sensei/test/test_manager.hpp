#ifndef TEST_MANAGER_HPP
#define TEST_MANAGER_HPP

#include "Clock.hpp"
#include "DoserManager.hpp"
#include "unity.h"
#include <deque>
#include <future>
#include <map>
#include <queue>
#include <thread>

using namespace std::chrono_literals;
using Status = std::map<int, float>;

Status status;

class TestManager : public DoserManager {
public:
  TestManager(int nDosers, int parallelMax)
      : n{nDosers}, DoserManager{parallelMax} {
    connectDosers();
  }

private:
  std::vector<float> implConnectDosers() override {
    return std::vector<float>(n);
  }
  void implDoserOn(int id, float flowRate) override { status[id] = flowRate; }
  void implDoserOff(int id) override { status[id] = 0; }
  const int n;
};

class TestManager2 : public DoserManager {
public:
  TestManager2(int nDosers, int parallelMax)
      : n{nDosers}, DoserManager{parallelMax} {
    connectDosers();
  }

private:
  std::vector<float> implConnectDosers() override {
    doserInfos.resize(n);
    return std::vector<float>(n);
  }

  void implDoserOn(int id, float flowRate) override {
    auto &di = doserInfos[id];
    const auto now = Clock::now();
    if (di.isOn) {
      updateStatus(id, now);
    }
    di.flowRate = flowRate;
    di.startedAt = now;
  }

  void implDoserOff(int id) override {
    auto &di = doserInfos[id];
    if (di.isOn) {
      updateStatus(id, Clock::now());
    }
    di.flowRate = 0;
  }

  void updateStatus(int id, Clock::time_point now) {
    auto &di = doserInfos[id];
    const float amount =
        di.flowRate *
        std::chrono::duration_cast<
            std::chrono::duration<float, std::chrono::minutes::period>>(
            now - di.startedAt)
            .count();
    status[id] += amount;
  }

  const int n;

  struct DoserInfo {
    bool isOn{false};
    float flowRate{0};
    Clock::time_point startedAt;
  };
  std::vector<DoserInfo> doserInfos;
};

void test_manager_ownership() {
  status.clear();
  constexpr int n = 8;
  TestManager man{n, 1};

  {
    auto doser = man.lendDoser(-1);
    TEST_ASSERT(!doser);
    doser = man.lendDoser(n);
    TEST_ASSERT(!doser);
  }

  for (int i = 0; i < n; ++i) {
    {
      auto doser = man.lendDoser(i);
      TEST_ASSERT(doser);
      TEST_ASSERT(!man.lendDoser(i));
      TEST_ASSERT_EQUAL(0, status[i]);
      doser->on(69);
      TEST_ASSERT_EQUAL(69, status[i]);
      doser->off();
      TEST_ASSERT_EQUAL(0, status[i]);
      doser->on(69);
      TEST_ASSERT_EQUAL(69, status[i]);
    }
    {
      auto doser = man.lendDoser(i);
      TEST_ASSERT(doser);
      TEST_ASSERT(!man.lendDoser(i));
      TEST_ASSERT_EQUAL(0, status[i]);
    }
  }

  for (int i = 0; i < n; ++i) {
    {
      auto doser = man.lendDoser(i);
      TEST_ASSERT(doser);
      TEST_ASSERT(!man.lendDoser(i));
      TEST_ASSERT_EQUAL(0, status[i]);
      doser->on(69);
      TEST_ASSERT_EQUAL(69, status[i]);
      doser->off();
      TEST_ASSERT_EQUAL(0, status[i]);
      doser->on(69);
      TEST_ASSERT_EQUAL(69, status[i]);
    }
    {
      auto doser = man.lendDoser(i);
      TEST_ASSERT(doser);
      TEST_ASSERT(!man.lendDoser(i));
      TEST_ASSERT_EQUAL(0, status[i]);
    }
  }

  {
    auto d0 = man.lendDoser(0);
    auto d1 = man.lendDoser(1);
    auto d2 = man.lendDoser(2);
    auto d3 = man.lendDoser(3);
    auto d4 = man.lendDoser(4);
    auto d5 = man.lendDoser(5);

    d0->on(69);
    TEST_ASSERT_EQUAL(69, status[0]);

    Clock::time_point readyAt;

    std::jthread th([&d0, &readyAt]() {
      std::this_thread::sleep_for(10ms);
      d0->off();
      readyAt = Clock::now();
    });

    d1->on(69);
    TEST_ASSERT_TRUE(Clock::now() > readyAt);
    TEST_ASSERT_EQUAL(69, status[1]);
  }
}

using Doser = DoserManager::Doser;

void test_api() {
  constexpr int numDosers = 8;
  constexpr int numParallel = 1;
  TestManager man{numDosers, numParallel};

  auto isRunning = [](const Doser &doser) { return status[doser.getId()]; };

  std::vector<Doser> dosers;
  for (int i = 0; i < numDosers; ++i) {
    auto doser = man.lendDoser(i);
    TEST_ASSERT_TRUE(doser);
    dosers.push_back(std::move(*doser));
  }

  for (auto &doser : dosers) {
    doser.on(69);
    TEST_ASSERT_EQUAL(69, status[doser.getId()]);
    doser.off();
  }
}

void test_api2() {
  status.clear();
  constexpr int n = 8;
  constexpr int p = 2;
  constexpr float flowrate = 1000;
  TestManager2 man{n, 1};
  man.connectDosers();

  std::map<int, float> schedule{{0, 1}, {1, 1}, {2, 1}, {3, 1}};

  std::vector<Doser> dosers;
  for (auto [id, amount] : schedule) {
    if (auto doser = man.lendDoser(id); doser) {
      dosers.push_back(std::move(*doser));
    }
  }

  std::vector<std::thread> jobs;
  for (auto [id, amount] : schedule) {
    auto &doser = dosers[id];
    jobs.emplace_back([amount, &doser]() { dose(doser, amount, flowrate); });
  }

  for (auto &job : jobs) {
    job.join();
  }
}

#endif