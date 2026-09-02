#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>

#include "emmus/algorithms/replacement/IPageReplacementPolicy.hpp"
#include "emmus/algorithms/replacement/PageReplacementPolicyType.hpp"

namespace emmus::algorithms::replacement
{

/**
 * @brief Centralized factory for page-replacement policy creation.
 *
 * PageReplacementPolicyFactory provides a single construction boundary
 * for page-replacement policies.
 *
 * Concrete policies register creator functions with a factory instance.
 * Callers request policies using PageReplacementPolicyType and receive
 * ownership through std::unique_ptr<IPageReplacementPolicy>.
 *
 * The factory does not depend on any specific replacement algorithm.
 * FIFO, LRU, Clock, Optimal, and future policies can be registered
 * independently when their implementations become available.
 *
 * The factory owns only creator functions. It does not own policy
 * instances after they have been returned to the caller.
 */
class PageReplacementPolicyFactory final
{
public:

    using PolicyPtr =
        std::unique_ptr<IPageReplacementPolicy>;

    using PolicyCreator =
        std::function<PolicyPtr()>;


    /**
     * @brief Constructs an empty policy factory.
     *
     * No policies are registered by default. Concrete policy
     * implementations may register themselves with the factory
     * configuration used by the application.
     */
    PageReplacementPolicyFactory() = default;


    PageReplacementPolicyFactory(
        const PageReplacementPolicyFactory&
    ) = delete;


    PageReplacementPolicyFactory& operator=(
        const PageReplacementPolicyFactory&
    ) = delete;


    PageReplacementPolicyFactory(
        PageReplacementPolicyFactory&&
    ) noexcept = default;


    PageReplacementPolicyFactory& operator=(
        PageReplacementPolicyFactory&&
    ) noexcept = default;


    ~PageReplacementPolicyFactory() = default;


    /**
     * @brief Registers a page-replacement policy creator.
     *
     * Registering an already registered policy replaces its existing
     * creator.
     *
     * @param type Policy type to register.
     * @param creator Function that creates a new policy instance.
     *
     * @return true when the registration is valid and succeeds.
     * @return false when the policy type is invalid or the creator
     *         function is empty.
     */
    [[nodiscard]]
    bool registerPolicy(
        PageReplacementPolicyType type,
        PolicyCreator creator
    );


    /**
     * @brief Removes a registered policy creator.
     *
     * Existing policy objects returned by create() are unaffected.
     *
     * @param type Policy type to unregister.
     *
     * @return true when a registered policy was removed.
     * @return false when the policy was not registered or the type
     *         is invalid.
     */
    [[nodiscard]]
    bool unregisterPolicy(
        PageReplacementPolicyType type
    );


    /**
     * @brief Creates a new policy instance.
     *
     * The requested policy must have been registered with this factory.
     *
     * @param type Policy type to create.
     *
     * @return A newly created policy when registered.
     * @return nullptr when the policy is invalid or unsupported.
     */
    [[nodiscard]]
    PolicyPtr create(
        PageReplacementPolicyType type
    ) const;


    /**
     * @brief Determines whether a policy is registered.
     *
     * @param type Policy type to query.
     *
     * @return true when a creator is registered.
     * @return false otherwise.
     */
    [[nodiscard]]
    bool isRegistered(
        PageReplacementPolicyType type
    ) const noexcept;


    /**
     * @brief Removes all registered policy creators.
     */
    void clear() noexcept;


private:

    using PolicyRegistry =
        std::unordered_map<
            PageReplacementPolicyType,
            PolicyCreator
        >;


    [[nodiscard]]
    static bool isValidPolicyType(
        PageReplacementPolicyType type
    ) noexcept;


    PolicyRegistry policyRegistry_;
};

} // namespace emmus::algorithms::replacement