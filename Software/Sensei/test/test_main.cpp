#include "test_manager.hpp"
#include "unity.h"

void setUp() {}

void tearDown() {}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_manager_ownership);
  RUN_TEST(test_api);
  RUN_TEST(test_api2);
  return UNITY_END();
}