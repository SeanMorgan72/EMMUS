#include "emmus/algorithms/replacement/PageReplacementPolicyFactory.hpp"

namespace emmus::algorithms::replacement
{

bool PageReplacementPolicyFactory::registerPolicy(
    PageReplacementPolicyType type,
    PolicyCreator creator
)
{
    if (
        !isValidPolicyType(type)
        || !creator
    )
    {
        return false;
    }

    policyRegistry_[type] =
        std::move(creator);

    return true;
}


bool PageReplacementPolicyFactory::unregisterPolicy(
    PageReplacementPolicyType type
)
{
    if (!isValidPolicyType(type))
    {
        return false;
    }

    return policyRegistry_.erase(type) > 0U;
}


PageReplacementPolicyFactory::PolicyPtr
PageReplacementPolicyFactory::create(
    PageReplacementPolicyType type
) const
{
    if (!isValidPolicyType(type))
    {
        return nullptr;
    }

    const auto iterator =
        policyRegistry_.find(type);

    if (iterator == policyRegistry_.end())
    {
        return nullptr;
    }

    const PolicyCreator& creator =
        iterator->second;

    if (!creator)
    {
        return nullptr;
    }

    return creator();
}


bool PageReplacementPolicyFactory::isRegistered(
    PageReplacementPolicyType type
) const noexcept
{
    if (!isValidPolicyType(type))
    {
        return false;
    }

    return policyRegistry_.contains(type);
}


void PageReplacementPolicyFactory::clear() noexcept
{
    policyRegistry_.clear();
}


bool PageReplacementPolicyFactory::isValidPolicyType(
    PageReplacementPolicyType type
) noexcept
{
    switch (type)
    {
        case PageReplacementPolicyType::FIFO:
        case PageReplacementPolicyType::LRU:
        case PageReplacementPolicyType::CLOCK:
        case PageReplacementPolicyType::OPTIMAL:
            return true;

        default:
            return false;
    }
}

} // namespace emmus::algorithms::replacement