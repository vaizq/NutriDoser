#ifndef DOSER_MANAGER2_HPP
#define DOSER_MANAGER2_HPP

#include "Clock.hpp"
#include <deque>
#include <semaphore>
#include <unordered_set>

class DoserManager {
  constexpr static char tag[] = "DoserManager";

public:
  class Doser {
    friend DoserManager;

  public:
    Doser(Doser &&other)
        : manager{other.manager}, id{other.id}, isOn{other.isOn} {
      other.manager = nullptr;
    }

    Doser &operator=(Doser &&other) {
      if (this != &other) {
        if (manager) {
          off();
          manager->returnDoser(id);
        }
        manager = other.manager;
        id = other.id;
        isOn = other.isOn;
        other.manager = nullptr;
      }
      return *this;
    }

    ~Doser() {
      if (manager) {
        off();
        manager->returnDoser(id);
      }
    }

    void on(float flowRate_mL_per_min) {
      if (manager) {
        manager->doserOn(id, flowRate_mL_per_min, isOn);
        isOn = true;
      }
    }

    bool tryOn(float flowRate_mL_per_min) {
      if (manager && manager->tryDoserOn(id, flowRate_mL_per_min, isOn)) {
        isOn = true;
        return true;
      }
      return false;
    }

    void off() {
      if (manager && isOn) {
        manager->doserOff(id);
        isOn = false;
      }
    }

    int getId() const { return id; }
    bool getIsOn() const { return isOn; }

    Doser(const Doser &) = delete;
    Doser &operator=(const Doser &) = delete;

  private:
    Doser(DoserManager *manager, int id) : manager{manager}, id{id} { off(); }

    DoserManager *manager;
    int id;
    bool isOn{false};
  };

  DoserManager(int parallelMax) : sem{parallelMax} {}
  virtual ~DoserManager() = default;

  void connectDosers() {
    std::lock_guard guard{mtx};
    flowRates = implConnectDosers();
    for (int i = 0; i < flowRates.size(); ++i) {
      available.insert(i);
    }
  }

  std::optional<Doser> lendDoser(int id) {
    std::lock_guard guard{mtx};
    if (available.erase(id) == 1) {
      return Doser{this, id};
    } else {
      return std::nullopt;
    }
  }

  std::unordered_set<int> availableDosers() {
    std::lock_guard guard{mtx};
    return available;
  }

  const std::vector<float> &getFlowRates() const { return flowRates; }

private:
  virtual std::vector<float> implConnectDosers() = 0;
  virtual void implDoserOn(int id, float flowRate) = 0;
  virtual void implDoserOff(int id) = 0;

  void doserOn(int id, float flowRate, bool isOn) {
    if (!isOn)
      sem.acquire();
    implDoserOn(id, flowRate);
  }

  bool tryDoserOn(int id, float flowRate, bool isOn) {
    if (isOn) {
      implDoserOn(id, flowRate);
      return true;
    }

    if (sem.try_acquire()) {
      implDoserOn(id, flowRate);
      return true;
    }

    return false;
  }

  void doserOff(int id) {
    implDoserOff(id);
    sem.release();
  }

  void returnDoser(int id) {
    std::lock_guard guard{mtx};
    available.insert(id);
  }

  std::vector<float> flowRates;
  std::unordered_set<int> available;
  std::counting_semaphore<> sem;
  std::mutex mtx;
};

extern std::unique_ptr<DoserManager> gDoserManager;

static void dose(DoserManager::Doser &doser, float amount_mL,
                 float flowRate_mL_per_min) {
  doser.on(flowRate_mL_per_min);
  std::this_thread::sleep_for(
      std::chrono::duration<float, std::chrono::minutes::period>(
          amount_mL / flowRate_mL_per_min));
  doser.off();
}

#endif