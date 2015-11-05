/**
* Copyright (C) Mellanox Technologies Ltd. 2015.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

#include <iostream>

#include "common.h"
#include "gtest/gtest.h"
#include "gtest/gtest-all.cc"


GTEST_API_ int main(int argc, char **argv) {
  std::cout << "Running ibverbs tests\n";

  sys_getenv();
  testing::GTEST_FLAG(print_time) = true;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
