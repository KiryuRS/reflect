// Copyright (c) 2026 KiryuRS
// SPDX-License-Identifier: MIT

#include "../include/inteoc/interface_ref.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ranges>
#include <string>

namespace mocks {

struct ifoo
{
    void on_message(const std::string&);
    std::size_t size() const noexcept;
    bool empty() const noexcept;
};

struct impl_foo
{
    std::string data;

    void on_message(const std::string& msg)
    {
        data = msg;
    }

    std::size_t size() const noexcept
    {
        return data.length();
    }

    bool empty() const noexcept
    {
        return data.empty();
    }
};

struct another_foo
{
    std::vector<std::string> all_messages;

    void on_message(const std::string& msg)
    {
        all_messages.push_back(msg);
    }

    std::size_t size() const noexcept
    {
        return all_messages.size();
    }

    bool empty() const noexcept
    {
        return all_messages.empty();
    }
};

class collector
{
    using interface_t = ifoo;
    using interface_ref = krrs::inteoc::interface_ref<ifoo>;

public:
    template <krrs::inteoc::concepts::compliant_with<ifoo> T>
    void push(T&& object)
    {
        objects_.push_back(object);
    }

    void on_message(const std::string& msg)
    {
        for (auto& elem : objects_)
        {
            elem.on_message(msg);
        }
    }

    std::size_t size() const noexcept
    {
        const auto size_view = objects_ | std::views::transform([] (const auto& object) {
            return object.size();
        });
        return std::ranges::fold_left(size_view, 0uz, std::plus<std::size_t>());
    }

    bool empty() const noexcept
    {
        return std::ranges::all_of(objects_, [](const auto& object) { return object.empty(); });
    }

private:
    std::vector<interface_ref> objects_;
};

} // namespace mocks

namespace tests {

using namespace ::testing;

TEST(test_inteoc, test_type_erasure)
{
    mocks::impl_foo f1, f2, f3;
    mocks::collector c;
    c.push(f1);
    c.push(f2);
    c.push(f3);

    const std::string message = "Hello WOrld";
    c.on_message(message);

    EXPECT_EQ(f1.data, message);
    EXPECT_EQ(f2.data, message);
    EXPECT_EQ(f3.data, message);

    EXPECT_EQ(c.size(), message.length() * 3);
    EXPECT_FALSE(c.empty());
}

TEST(test_inteoc, test_type_erasure_multiple_types)
{
    mocks::impl_foo f1;
    mocks::another_foo af1;

    mocks::collector c;
    c.push(f1);
    c.push(af1);

    const std::string msg1 = "Goodbye World";
    const std::string msg2 = "This is Sparta";
    const std::string msg3 = "Well, not really";
    c.on_message(msg1);
    c.on_message(msg2);
    c.on_message(msg3);

    EXPECT_EQ(f1.data, msg3);
    EXPECT_THAT(af1.all_messages, ElementsAre(msg1, msg2, msg3));

    // f1 current holds msg3 as a copy, af1 stores all of them into a vector
    EXPECT_EQ(c.size(), msg3.length() + 3);
    EXPECT_FALSE(c.empty());
}

} // namespace tests
