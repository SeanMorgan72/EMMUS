#include <algorithm>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"
#include "emmus/memory/identifiers/MemoryObjectIds.hpp"
#include "fixtures/PageReplacementAlgorithmFixture.hpp"

namespace emmus::test
{

namespace
{

using Policy =
    emmus::algorithms::replacement::IPageReplacementPolicy;

using PageId =
    emmus::memory::identifiers::PageId;

using FrameId =
    emmus::memory::identifiers::FrameId;


/**
 * @brief Minimal concrete implementation used to verify the
 *        IPageReplacementPolicy interface contract.
 *
 * This class deliberately does not implement FIFO, LRU, Clock,
 * Optimal, or any other replacement algorithm.
 *
 * It records interface notifications and allows tests to specify
 * a frame that should be returned by chooseVictim().
 */
class TestPageReplacementPolicy final
    : public Policy
{
public:

    struct LoadEvent
    {
        PageId pageId;
        FrameId frameId;
    };


    struct AccessEvent
    {
        PageId pageId;
        FrameId frameId;
    };


    struct RemoveEvent
    {
        PageId pageId;
        FrameId frameId;
    };


    void pageLoaded(
        PageId pageId,
        FrameId frameId
    ) override
    {
        loadedEvents_.push_back(
            LoadEvent{
                pageId,
                frameId
            }
        );
    }


    void pageAccessed(
        PageId pageId,
        FrameId frameId
    ) override
    {
        accessedEvents_.push_back(
            AccessEvent{
                pageId,
                frameId
            }
        );
    }


    void pageRemoved(
        PageId pageId,
        FrameId frameId
    ) override
    {
        removedEvents_.push_back(
            RemoveEvent{
                pageId,
                frameId
            }
        );
    }


    [[nodiscard]]
    std::optional<FrameId> chooseVictim() override
    {
        return victim_;
    }


    void reset() override
    {
        loadedEvents_.clear();
        accessedEvents_.clear();
        removedEvents_.clear();

        victim_.reset();
    }


    void setVictim(
        FrameId frameId
    )
    {
        victim_ = frameId;
    }


    void clearVictim()
    {
        victim_.reset();
    }


    [[nodiscard]]
    const std::vector<LoadEvent>& loadedEvents() const noexcept
    {
        return loadedEvents_;
    }


    [[nodiscard]]
    const std::vector<AccessEvent>& accessedEvents() const noexcept
    {
        return accessedEvents_;
    }


    [[nodiscard]]
    const std::vector<RemoveEvent>& removedEvents() const noexcept
    {
        return removedEvents_;
    }


private:

    std::vector<LoadEvent> loadedEvents_;

    std::vector<AccessEvent> accessedEvents_;

    std::vector<RemoveEvent> removedEvents_;

    std::optional<FrameId> victim_;
};

} // namespace


/**
 * @brief Verifies that a concrete policy can be used through the
 *        IPageReplacementPolicy interface.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    ConcretePolicyCanBeUsedThroughInterface
)
{
    TestPageReplacementPolicy concretePolicy;

    Policy* policy = &concretePolicy;

    ASSERT_NE(
        policy,
        nullptr
    );

    policy->pageLoaded(
        PageId{1},
        FrameId{10}
    );

    ASSERT_EQ(
        concretePolicy.loadedEvents().size(),
        1U
    );

    EXPECT_EQ(
        concretePolicy.loadedEvents().front().pageId,
        PageId{1}
    );

    EXPECT_EQ(
        concretePolicy.loadedEvents().front().frameId,
        FrameId{10}
    );
}


/**
 * @brief Verifies that pageLoaded() delivers both strongly typed
 *        identifiers to the policy.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    PageLoadedNotifiesPolicyOfPageAndFrame
)
{
    TestPageReplacementPolicy policy;

    constexpr PageId pageId{100};

    constexpr FrameId frameId{200};

    policy.pageLoaded(
        pageId,
        frameId
    );

    ASSERT_EQ(
        policy.loadedEvents().size(),
        1U
    );

    EXPECT_EQ(
        policy.loadedEvents()[0].pageId,
        pageId
    );

    EXPECT_EQ(
        policy.loadedEvents()[0].frameId,
        frameId
    );
}


/**
 * @brief Verifies that multiple page-loaded notifications are preserved
 *        in the order in which they are received.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    PageLoadedSupportsMultipleNotifications
)
{
    TestPageReplacementPolicy policy;

    constexpr PageId page1{1};
    constexpr PageId page2{2};
    constexpr PageId page3{3};

    constexpr FrameId frame1{10};
    constexpr FrameId frame2{20};
    constexpr FrameId frame3{30};

    policy.pageLoaded(
        page1,
        frame1
    );

    policy.pageLoaded(
        page2,
        frame2
    );

    policy.pageLoaded(
        page3,
        frame3
    );

    ASSERT_EQ(
        policy.loadedEvents().size(),
        3U
    );

    EXPECT_EQ(
        policy.loadedEvents()[0].pageId,
        page1
    );

    EXPECT_EQ(
        policy.loadedEvents()[0].frameId,
        frame1
    );

    EXPECT_EQ(
        policy.loadedEvents()[1].pageId,
        page2
    );

    EXPECT_EQ(
        policy.loadedEvents()[1].frameId,
        frame2
    );

    EXPECT_EQ(
        policy.loadedEvents()[2].pageId,
        page3
    );

    EXPECT_EQ(
        policy.loadedEvents()[2].frameId,
        frame3
    );
}


/**
 * @brief Verifies that pageAccessed() delivers both identifiers.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    PageAccessedNotifiesPolicyOfPageAndFrame
)
{
    TestPageReplacementPolicy policy;

    constexpr PageId pageId{42};

    constexpr FrameId frameId{7};

    policy.pageAccessed(
        pageId,
        frameId
    );

    ASSERT_EQ(
        policy.accessedEvents().size(),
        1U
    );

    EXPECT_EQ(
        policy.accessedEvents()[0].pageId,
        pageId
    );

    EXPECT_EQ(
        policy.accessedEvents()[0].frameId,
        frameId
    );
}


/**
 * @brief Verifies that multiple page-access notifications are preserved.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    PageAccessedSupportsMultipleNotifications
)
{
    TestPageReplacementPolicy policy;

    constexpr PageId page1{1};
    constexpr PageId page2{2};

    constexpr FrameId frame1{10};
    constexpr FrameId frame2{20};

    policy.pageAccessed(
        page1,
        frame1
    );

    policy.pageAccessed(
        page2,
        frame2
    );

    policy.pageAccessed(
        page1,
        frame1
    );

    ASSERT_EQ(
        policy.accessedEvents().size(),
        3U
    );

    EXPECT_EQ(
        policy.accessedEvents()[0].pageId,
        page1
    );

    EXPECT_EQ(
        policy.accessedEvents()[1].pageId,
        page2
    );

    EXPECT_EQ(
        policy.accessedEvents()[2].pageId,
        page1
    );
}


/**
 * @brief Verifies that pageRemoved() delivers both identifiers.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    PageRemovedNotifiesPolicyOfPageAndFrame
)
{
    TestPageReplacementPolicy policy;

    constexpr PageId pageId{55};

    constexpr FrameId frameId{12};

    policy.pageRemoved(
        pageId,
        frameId
    );

    ASSERT_EQ(
        policy.removedEvents().size(),
        1U
    );

    EXPECT_EQ(
        policy.removedEvents()[0].pageId,
        pageId
    );

    EXPECT_EQ(
        policy.removedEvents()[0].frameId,
        frameId
    );
}


/**
 * @brief Verifies that multiple page-removal notifications are preserved.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    PageRemovedSupportsMultipleNotifications
)
{
    TestPageReplacementPolicy policy;

    constexpr PageId page1{1};
    constexpr PageId page2{2};

    constexpr FrameId frame1{10};
    constexpr FrameId frame2{20};

    policy.pageRemoved(
        page1,
        frame1
    );

    policy.pageRemoved(
        page2,
        frame2
    );

    ASSERT_EQ(
        policy.removedEvents().size(),
        2U
    );

    EXPECT_EQ(
        policy.removedEvents()[0].pageId,
        page1
    );

    EXPECT_EQ(
        policy.removedEvents()[0].frameId,
        frame1
    );

    EXPECT_EQ(
        policy.removedEvents()[1].pageId,
        page2
    );

    EXPECT_EQ(
        policy.removedEvents()[1].frameId,
        frame2
    );
}


/**
 * @brief Verifies that chooseVictim() can report that no victim is
 *        currently available.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    ChooseVictimCanReportNoEligibleFrame
)
{
    TestPageReplacementPolicy policy;

    EXPECT_FALSE(
        policy.chooseVictim().has_value()
    );
}


/**
 * @brief Verifies that chooseVictim() returns the strongly typed
 *        FrameId selected by a concrete policy.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    ChooseVictimReturnsSelectedFrame
)
{
    TestPageReplacementPolicy policy;

    constexpr FrameId expectedVictim{25};

    policy.setVictim(
        expectedVictim
    );

    const auto victim =
        policy.chooseVictim();

    ASSERT_TRUE(
        victim.has_value()
    );

    EXPECT_EQ(
        victim.value(),
        expectedVictim
    );
}


/**
 * @brief Verifies that changing the selected victim is observable
 *        through the common interface.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    ChooseVictimReflectsCurrentPolicySelection
)
{
    TestPageReplacementPolicy policy;

    constexpr FrameId firstVictim{10};
    constexpr FrameId secondVictim{20};

    policy.setVictim(
        firstVictim
    );

    ASSERT_TRUE(
        policy.chooseVictim().has_value()
    );

    EXPECT_EQ(
        policy.chooseVictim().value(),
        firstVictim
    );

    policy.setVictim(
        secondVictim
    );

    ASSERT_TRUE(
        policy.chooseVictim().has_value()
    );

    EXPECT_EQ(
        policy.chooseVictim().value(),
        secondVictim
    );
}


/**
 * @brief Verifies that reset() clears policy-maintained notification
 *        state and victim-selection state.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    ResetClearsPolicyState
)
{
    TestPageReplacementPolicy policy;

    policy.pageLoaded(
        PageId{1},
        FrameId{10}
    );

    policy.pageAccessed(
        PageId{1},
        FrameId{10}
    );

    policy.pageRemoved(
        PageId{1},
        FrameId{10}
    );

    policy.setVictim(
        FrameId{10}
    );

    policy.reset();

    EXPECT_TRUE(
        policy.loadedEvents().empty()
    );

    EXPECT_TRUE(
        policy.accessedEvents().empty()
    );

    EXPECT_TRUE(
        policy.removedEvents().empty()
    );

    EXPECT_FALSE(
        policy.chooseVictim().has_value()
    );
}


/**
 * @brief Verifies that the interface supports runtime polymorphism
 *        with a concrete policy managed through the interface pointer.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    InterfaceSupportsRuntimePolymorphism
)
{
    auto concretePolicy =
        std::make_unique<TestPageReplacementPolicy>();

    Policy* policy =
        concretePolicy.get();

    constexpr PageId pageId{123};

    constexpr FrameId frameId{456};

    policy->pageLoaded(
        pageId,
        frameId
    );

    policy->pageAccessed(
        pageId,
        frameId
    );

    policy->pageRemoved(
        pageId,
        frameId
    );

    ASSERT_EQ(
        concretePolicy->loadedEvents().size(),
        1U
    );

    ASSERT_EQ(
        concretePolicy->accessedEvents().size(),
        1U
    );

    ASSERT_EQ(
        concretePolicy->removedEvents().size(),
        1U
    );
}


/**
 * @brief Verifies that a derived policy can be destroyed through the
 *        interface pointer, demonstrating the required virtual destructor.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    InterfaceHasPolymorphicDestruction
)
{
    class DestructionTrackingPolicy final
        : public Policy
    {
    public:

        explicit DestructionTrackingPolicy(
            bool& destroyed
        ) noexcept
            : destroyed_(destroyed)
        {
        }


        ~DestructionTrackingPolicy() override
        {
            destroyed_ = true;
        }


        void pageLoaded(
            PageId,
            FrameId
        ) override
        {
        }


        void pageAccessed(
            PageId,
            FrameId
        ) override
        {
        }


        void pageRemoved(
            PageId,
            FrameId
        ) override
        {
        }


        [[nodiscard]]
        std::optional<FrameId> chooseVictim() override
        {
            return std::nullopt;
        }


        void reset() override
        {
        }


    private:

        bool& destroyed_;
    };


    bool destroyed = false;

    Policy* policy =
        new DestructionTrackingPolicy(
            destroyed
        );

    delete policy;

    EXPECT_TRUE(
        destroyed
    );
}


/**
 * @brief Verifies that PageId and FrameId are distinct strong types
 *        at the interface boundary.
 */
TEST_F(
    PageReplacementAlgorithmFixture,
    InterfaceUsesStronglyTypedIdentifiers
)
{
    static_assert(
        !std::is_convertible_v<
            std::uint64_t,
            PageId
        >
    );

    static_assert(
        !std::is_convertible_v<
            std::uint64_t,
            FrameId
        >
    );

    static_assert(
        !std::is_same_v<
            PageId,
            FrameId
        >
    );

    SUCCEED();
}

} // namespace emmus::test