#include "fixtures/PageReplacementAlgorithmFixture.hpp"

#include "emmus/algorithms/replacement/PageReplacementPolicyFactory.hpp"
#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <optional>


namespace
{

using emmus::algorithms::replacement::IPageReplacementPolicy;
using emmus::algorithms::replacement::PageReplacementPolicyFactory;
using emmus::algorithms::replacement::PageReplacementPolicyType;


/**
 * @brief Test-only implementation of IPageReplacementPolicy.
 *
 * This class exists only to verify the factory infrastructure.
 *
 * It is intentionally not a FIFO, LRU, Clock, or Optimal implementation.
 * Those concrete policies belong to their respective implementation
 * tasks that follow US-303.
 */
class TestPageReplacementPolicy final
    : public IPageReplacementPolicy
{
public:

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
};

} // namespace


namespace emmus::test
{

class PageReplacementPolicyFactoryTest
    : public PageReplacementAlgorithmFixture
{
protected:

    PageReplacementPolicyFactory factory_;


    static PageReplacementPolicyFactory::PolicyCreator
    testCreator()
    {
        return []()
        {
            return std::make_unique<TestPageReplacementPolicy>();
        };
    }
};

} // namespace emmus::test


using emmus::algorithms::replacement::PageReplacementPolicyFactory;
using emmus::algorithms::replacement::PageReplacementPolicyType;
using emmus::test::PageReplacementPolicyFactoryTest;


TEST_F(
    PageReplacementPolicyFactoryTest,
    FactoryStartsWithNoRegisteredPolicies
)
{
    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::LRU
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::CLOCK
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::OPTIMAL
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    RegistersFifoPolicy
)
{
    EXPECT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    EXPECT_TRUE(
        factory_.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    RegistersLruPolicy
)
{
    EXPECT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::LRU,
            testCreator()
        )
    );

    EXPECT_TRUE(
        factory_.isRegistered(
            PageReplacementPolicyType::LRU
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    RegistersClockPolicy
)
{
    EXPECT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::CLOCK,
            testCreator()
        )
    );

    EXPECT_TRUE(
        factory_.isRegistered(
            PageReplacementPolicyType::CLOCK
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    RegistersOptimalPolicy
)
{
    EXPECT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::OPTIMAL,
            testCreator()
        )
    );

    EXPECT_TRUE(
        factory_.isRegistered(
            PageReplacementPolicyType::OPTIMAL
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    CreatesPolicyThroughCommonInterface
)
{
    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    std::unique_ptr<IPageReplacementPolicy> policy =
        factory_.create(
            PageReplacementPolicyType::FIFO
        );

    ASSERT_NE(
        policy,
        nullptr
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    CreatedPolicySupportsCommonInterface
)
{
    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    auto policy =
        factory_.create(
            PageReplacementPolicyType::FIFO
        );

    ASSERT_NE(
        policy,
        nullptr
    );

    policy->pageLoaded(
        kPage0,
        kFrame0
    );

    policy->pageAccessed(
        kPage0,
        kFrame0
    );

    policy->pageRemoved(
        kPage0,
        kFrame0
    );

    EXPECT_FALSE(
        policy->chooseVictim().has_value()
    );

    policy->reset();
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    ReturnsNullptrForUnregisteredPolicy
)
{
    EXPECT_EQ(
        factory_.create(
            PageReplacementPolicyType::FIFO
        ),
        nullptr
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    RejectsEmptyCreator
)
{
    EXPECT_FALSE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            {}
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    RejectsInvalidPolicyType
)
{
    constexpr auto invalidType =
        static_cast<PageReplacementPolicyType>(255U);

    EXPECT_FALSE(
        factory_.registerPolicy(
            invalidType,
            testCreator()
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            invalidType
        )
    );

    EXPECT_EQ(
        factory_.create(
            invalidType
        ),
        nullptr
    );

    EXPECT_FALSE(
        factory_.unregisterPolicy(
            invalidType
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    UnregistersRegisteredPolicy
)
{
    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    ASSERT_TRUE(
        factory_.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );

    EXPECT_TRUE(
        factory_.unregisterPolicy(
            PageReplacementPolicyType::FIFO
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );

    EXPECT_EQ(
        factory_.create(
            PageReplacementPolicyType::FIFO
        ),
        nullptr
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    UnregisteringUnregisteredPolicyReturnsFalse
)
{
    EXPECT_FALSE(
        factory_.unregisterPolicy(
            PageReplacementPolicyType::FIFO
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    ReRegistrationReplacesExistingCreator
)
{
    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    EXPECT_TRUE(
        factory_.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );

    EXPECT_NE(
        factory_.create(
            PageReplacementPolicyType::FIFO
        ),
        nullptr
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    CreatesIndependentPolicyInstances
)
{
    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    auto first =
        factory_.create(
            PageReplacementPolicyType::FIFO
        );

    auto second =
        factory_.create(
            PageReplacementPolicyType::FIFO
        );

    ASSERT_NE(
        first,
        nullptr
    );

    ASSERT_NE(
        second,
        nullptr
    );

    EXPECT_NE(
        first.get(),
        second.get()
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    ClearRemovesAllRegisteredPolicies
)
{
    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::LRU,
            testCreator()
        )
    );

    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::CLOCK,
            testCreator()
        )
    );

    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::OPTIMAL,
            testCreator()
        )
    );

    factory_.clear();

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::LRU
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::CLOCK
        )
    );

    EXPECT_FALSE(
        factory_.isRegistered(
            PageReplacementPolicyType::OPTIMAL
        )
    );
}


TEST_F(
    PageReplacementPolicyFactoryTest,
    FactoryInstancesHaveIndependentRegistries
)
{
    PageReplacementPolicyFactory secondFactory;

    ASSERT_TRUE(
        factory_.registerPolicy(
            PageReplacementPolicyType::FIFO,
            testCreator()
        )
    );

    EXPECT_TRUE(
        factory_.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );

    EXPECT_FALSE(
        secondFactory.isRegistered(
            PageReplacementPolicyType::FIFO
        )
    );
}