#include "core/framework/mem_pattern_planner.h"
#include "gtest/gtest.h"

namespace Lotus {
namespace Test {
TEST(MemPatternPlannerTest, TraceAllocaitonTest) {
  MemPatternPlanner planner;
  planner.TraceAllocation(0, 1024);
  planner.TraceAllocation(1, 256);
  planner.TraceAllocation(2, 512);
  planner.TraceAllocation(3, 1024);

  MemoryPattern pattern;
  auto status = planner.GenerateMemPattern(&pattern);
  EXPECT_TRUE(status.IsOK());

  EXPECT_EQ(pattern.peak_size(), 1024 + 256 + 512 + 1024);
  EXPECT_EQ(pattern.GetBlock(0)->offset_, 0);
  EXPECT_EQ(pattern.GetBlock(1)->offset_, 1024);
  EXPECT_EQ(pattern.GetBlock(2)->offset_, 1024 + 256);
  EXPECT_EQ(pattern.GetBlock(3)->offset_, 1024 + 256 + 512);

  planner.TraceFree(1);
  planner.TraceAllocation(4, 512);
  planner.TraceFree(3);
  planner.TraceAllocation(5, 600);
  planner.TraceAllocation(6, 200);

  pattern.clear();
  status = planner.GenerateMemPattern(&pattern);
  EXPECT_TRUE(status.IsOK());

  EXPECT_EQ(pattern.peak_size(), 1024 + 256 + 512 + 1024 + 512);
  EXPECT_EQ(pattern.GetBlock(4)->offset_, 1024 + 256 + 512 + 1024);
  EXPECT_EQ(pattern.GetBlock(5)->offset_, 1024 + 256 + 512);
  EXPECT_EQ(pattern.GetBlock(6)->offset_, 1024);
}
}  // namespace Test
}  // namespace Lotus
