//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
//=================================================================================================

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <vector>

#include <gtest/gtest.h>

#include "hephaestus/containers/intrusive_circular_list.h"
#include "hephaestus/utils/exception.h"

namespace heph::containers::tests {

class Node {
public:
  Node() = default;
  Node(Node&) = delete;
  auto operator=(Node&) -> Node& = delete;
  Node(Node&&) = delete;
  auto operator=(Node&&) -> Node& = delete;
  ~Node() = default;

  auto circularListHook() -> CircularListHook& {
    return circular_list_hook_;
  }

private:
  CircularListHook circular_list_hook_;
};

TEST(IntrusiveCircularList, Empty) {
  IntrusiveCircularList<Node> list;
  EXPECT_TRUE(list.empty());
  EXPECT_EQ(list.size(), 0);
  EXPECT_THROW(list.front(), heph::Panic);
  EXPECT_THROW(list.back(), heph::Panic);
  EXPECT_EQ(list.begin(), list.end());
  EXPECT_TRUE(std::bidirectional_iterator<IntrusiveCircularList<Node>::IteratorT>);
  EXPECT_TRUE(std::bidirectional_iterator<IntrusiveCircularList<Node>::IteratorT>);
}

TEST(IntrusiveCircularList, Misuse) {
  IntrusiveCircularList<Node> list1;
  IntrusiveCircularList<Node> list2;
  Node n1;
  Node n2;

  list1.pushBack(n1);
  EXPECT_THROW(*list1.end(), heph::Panic);
  EXPECT_THROW(list1.pushBack(n1), heph::Panic);
  EXPECT_THROW(list2.erase(n1), heph::Panic);
  EXPECT_THROW(list1.erase(n2), heph::Panic);
  EXPECT_THROW(list1.insertAfter(list1.begin(), n1), heph::Panic);
  EXPECT_THROW(list1.insertBefore(list1.begin(), n1), heph::Panic);
  EXPECT_THROW(list1.insertAfter(list2.begin(), n2), heph::Panic);
  EXPECT_THROW(list1.insertBefore(list2.begin(), n2), heph::Panic);
  list1.erase(n1);
  EXPECT_THROW(list1.erase(n1), heph::Panic);
  EXPECT_THROW(list2.erase(n1), heph::Panic);
  EXPECT_THROW(*list2.begin(), heph::Panic);
  EXPECT_THROW(*list2.end(), heph::Panic);
  EXPECT_THROW((void)(list1.begin() == list2.begin()), heph::Panic);
}

TEST(IntrusiveCircularList, pushBackSingle) {
  IntrusiveCircularList<Node> list;
  Node n1;
  list.pushBack(n1);
  EXPECT_FALSE(list.empty());
  EXPECT_EQ(list.size(), 1);
  EXPECT_EQ(&list.front(), &n1);
  EXPECT_EQ(&list.back(), &n1);
  EXPECT_NE(list.begin(), list.end());
  EXPECT_NE(list.begin()++, list.end());
  EXPECT_EQ(++list.begin(), list.end());
  list.erase(n1);
  EXPECT_EQ(list.size(), 0);
  EXPECT_TRUE(list.empty());
  EXPECT_THROW(list.front(), heph::Panic);
  EXPECT_THROW(list.back(), heph::Panic);
  EXPECT_EQ(list.begin(), list.end());
}

TEST(IntrusiveCircularList, pushFrontSingle) {
  IntrusiveCircularList<Node> list;
  Node n1;
  list.pushFront(n1);
  EXPECT_FALSE(list.empty());
  EXPECT_EQ(list.size(), 1);
  EXPECT_EQ(&list.front(), &n1);
  EXPECT_EQ(&list.back(), &n1);
  EXPECT_NE(list.begin(), list.end());
  EXPECT_NE(list.begin()++, list.end());
  EXPECT_EQ(++list.begin(), list.end());
  list.erase(n1);
  EXPECT_EQ(list.size(), 0);
  EXPECT_TRUE(list.empty());
  EXPECT_THROW(list.front(), heph::Panic);
  EXPECT_THROW(list.back(), heph::Panic);
  EXPECT_EQ(list.begin(), list.end());
}

TEST(IntrusiveCircularList, pushBack) {
  static constexpr std::size_t NUMBER_OF_NODES = 100;
  std::vector<Node> nodes(NUMBER_OF_NODES);

  IntrusiveCircularList<Node> list;
  for (Node& node : nodes) {
    list.pushBack(node);
    EXPECT_EQ(&list.back(), &node);
  }
  EXPECT_EQ(list.size(), nodes.size());
  EXPECT_EQ(&list.front(), &nodes.front());
  EXPECT_EQ(&list.back(), &nodes.back());

  {
    auto it = nodes.begin();
    for (const Node& node : list) {
      EXPECT_EQ(&node, &(*it));
      ++it;
    }
    EXPECT_EQ(it, nodes.end());
  }
  {
    auto it = nodes.rbegin();
    auto range = std::views::reverse(nodes);
    for (const Node& node : range) {
      EXPECT_EQ(&node, &(*it));
      ++it;
    }
    EXPECT_EQ(it, nodes.rend());
  }
}

TEST(IntrusiveCircularList, pushFront) {
  static constexpr std::size_t NUMBER_OF_NODES = 100;
  std::vector<Node> nodes(NUMBER_OF_NODES);

  IntrusiveCircularList<Node> list;
  for (Node& node : nodes) {
    list.pushFront(node);
    EXPECT_EQ(&list.front(), &node);
  }
  EXPECT_EQ(list.size(), nodes.size());
  EXPECT_EQ(&list.front(), &nodes.back());
  EXPECT_EQ(&list.back(), &nodes.front());
  {
    auto it = nodes.rbegin();
    for (const Node& node : list) {
      EXPECT_EQ(&node, &(*it));
      ++it;
    }
    EXPECT_EQ(it, nodes.rend());
  }
  {
    auto it = nodes.begin();
    for (const Node& node : std::views::reverse(list)) {
      EXPECT_EQ(&node, &(*it));
      ++it;
    }
    EXPECT_EQ(it, nodes.end());
  }
}
TEST(IntrusiveCircularList, erase) {
  IntrusiveCircularList<Node> list;
  Node n0;
  list.pushBack(n0);
  Node n1;
  list.pushBack(n1);
  Node n2;
  list.pushBack(n2);
  Node n3;
  list.pushBack(n3);
  Node n4;
  list.pushBack(n4);
  Node n5;
  list.pushBack(n5);

  // baseline test..
  EXPECT_EQ(&list.front(), &n0);
  EXPECT_EQ(&list.back(), &n5);
  EXPECT_EQ(list.size(), 6);
  EXPECT_TRUE(std::ranges::equal(list | std::views::transform([](Node& n) { return &n; }),
                                 std::vector{ &n0, &n1, &n2, &n3, &n4, &n5 }));

  // erase first...
  list.erase(n0);
  EXPECT_EQ(&list.front(), &n1);
  EXPECT_EQ(&list.back(), &n5);
  EXPECT_EQ(list.size(), 5);
  EXPECT_TRUE(std::ranges::equal(list | std::views::transform([](Node& n) { return &n; }),
                                 std::vector{ &n1, &n2, &n3, &n4, &n5 }));

  // erase last...
  list.erase(n5);
  EXPECT_EQ(&list.front(), &n1);
  EXPECT_EQ(&list.back(), &n4);
  EXPECT_EQ(list.size(), 4);
  EXPECT_TRUE(std::ranges::equal(list | std::views::transform([](Node& n) { return &n; }),
                                 std::vector{ &n1, &n2, &n3, &n4 }));

  // erase middle...
  list.erase(n3);
  EXPECT_EQ(&list.front(), &n1);
  EXPECT_EQ(&list.back(), &n4);
  EXPECT_EQ(list.size(), 3);
  EXPECT_TRUE(std::ranges::equal(list | std::views::transform([](Node& n) { return &n; }),
                                 std::vector{ &n1, &n2, &n4 }));

  list.erase(n2);
  EXPECT_EQ(&list.front(), &n1);
  EXPECT_EQ(&list.back(), &n4);
  EXPECT_EQ(list.size(), 2);
  EXPECT_TRUE(
      std::ranges::equal(list | std::views::transform([](Node& n) { return &n; }), std::vector{ &n1, &n4 }));

  list.erase(n1);
  EXPECT_EQ(&list.front(), &n4);
  EXPECT_EQ(&list.back(), &n4);
  EXPECT_EQ(list.size(), 1);
  EXPECT_TRUE(
      std::ranges::equal(list | std::views::transform([](Node& n) { return &n; }), std::vector{ &n4 }));

  IntrusiveCircularList<Node> list2;
  list2.pushBack(n0);
  list2.pushBack(n1);
  list2.erase(n1);
  EXPECT_EQ(&list2.front(), &n0);
  EXPECT_EQ(&list2.front(), &n0);
  EXPECT_EQ(list.size(), 1);
  EXPECT_TRUE(
      std::ranges::equal(list2 | std::views::transform([](Node& n) { return &n; }), std::vector{ &n0 }));
}
}  // namespace heph::containers::tests
