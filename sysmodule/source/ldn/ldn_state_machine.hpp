/**
 * @file ldn_state_machine.hpp
 * @brief LDN Communication State Machine
 *
 * Thread-safe state machine for managing LDN communication states
 * with proper transition validation.
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#pragma once

#include <stratosphere.hpp>
#include "ldn_types.hpp"

namespace ams::mitm::ldn {

/**
 * @brief Result codes for state transitions
 */
enum class StateTransitionResult {
    Success,            ///< Transition successful
    InvalidTransition,  ///< Transition not allowed from current state
    AlreadyInState,     ///< Already in the target state
};

/**
 * @brief LDN State Machine
 *
 * Manages the LDN communication state with thread-safe transitions
 * and automatic event signaling on state changes.
 *
 * ## State Diagram
 *
 * ```
 *                    +------+
 *                    | None |
 *                    +------+
 *                       |
 *                  Initialize
 *                       v
 *                +-------------+
 *                | Initialized |
 *                +-------------+
 *               /               \
 *       OpenAccessPoint      OpenStation
 *             /                   \
 *            v                     v
 *     +-------------+         +---------+
 *     | AccessPoint |         | Station |
 *     +-------------+         +---------+
 *            |                     |
 *      CreateNetwork            Connect
 *            v                     v
 * +--------------------+   +------------------+
 * | AccessPointCreated |   | StationConnected |
 * +--------------------+   +------------------+
 * ```
 *
 * Any state can transition to Error on fatal errors.
 * Finalize from any state returns to None.
 */
class LdnStateMachine {
public:
    using StateCallback = void (*)(CommState old_state, CommState new_state, void* user_data);

    /**
     * @brief Constructor
     */
    /// @gdb{tag="LDN:STATE", msg="LdnStateMachine: constructor"}
    LdnStateMachine();

    /**
     * @brief Destructor
     */
    /// @gdb{tag="LDN:STATE", msg="LdnStateMachine: destructor"}
    ~LdnStateMachine();

    // Non-copyable
    LdnStateMachine(const LdnStateMachine&) = delete;
    LdnStateMachine& operator=(const LdnStateMachine&) = delete;

    // ========================================================================
    // State Queries
    // ========================================================================

    /**
     * @brief Get current state
     * @return Current CommState
     */
    /// @gdb{tag="LDN:STATE", msg="GetState"}
    CommState GetState() const;

    /**
     * @brief Check if in a specific state
     * @param state State to check
     * @return true if in the specified state
     */
    /// @gdb{tag="LDN:STATE", msg="IsInState"}
    bool IsInState(CommState state) const;

    /**
     * @brief Check if initialized (not None or Error)
     * @return true if service is initialized
     */
    /// @gdb{tag="LDN:STATE", msg="IsInitialized"}
    bool IsInitialized() const;

    /**
     * @brief Check if in a connected state (AccessPointCreated or StationConnected)
     * @return true if connected to a network
     */
    /// @gdb{tag="LDN:STATE", msg="IsNetworkActive"}
    bool IsNetworkActive() const;

    // ========================================================================
    // State Transitions
    // ========================================================================

    /**
     * @brief Initialize the service (None -> Initialized)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="Initialize → Initialized"}
    StateTransitionResult Initialize();

    /**
     * @brief Finalize the service (Any -> None)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="Finalize → None"}
    StateTransitionResult Finalize();

    /**
     * @brief Open access point mode (Initialized -> AccessPoint)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="OpenAccessPoint → AccessPoint"}
    StateTransitionResult OpenAccessPoint();

    /**
     * @brief Close access point mode (AccessPoint/AccessPointCreated -> Initialized)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="CloseAccessPoint → Initialized"}
    StateTransitionResult CloseAccessPoint();

    /**
     * @brief Create network (AccessPoint -> AccessPointCreated)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="CreateNetwork → AccessPointCreated"}
    StateTransitionResult CreateNetwork();

    /**
     * @brief Destroy network (AccessPointCreated -> AccessPoint)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="DestroyNetwork → AccessPoint"}
    StateTransitionResult DestroyNetwork();

    /**
     * @brief Open station mode (Initialized -> Station)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="OpenStation → Station"}
    StateTransitionResult OpenStation();

    /**
     * @brief Close station mode (Station/StationConnected -> Initialized)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="CloseStation → Initialized"}
    StateTransitionResult CloseStation();

    /**
     * @brief Connect to network (Station -> StationConnected)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="Connect → StationConnected"}
    StateTransitionResult Connect();

    /**
     * @brief Disconnect from network (StationConnected -> Station)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="Disconnect → Station"}
    StateTransitionResult Disconnect();

    /**
     * @brief Set error state (Any -> Error)
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="SetError → error state"}
    StateTransitionResult SetError();

    // ========================================================================
    // Event Management
    // ========================================================================

    /**
     * @brief Get the state change event handle
     * @return Event handle for state changes
     */
    /// @gdb{tag="LDN:STATE", msg="GetStateChangeEventHandle"}
    os::NativeHandle GetStateChangeEventHandle() const;

    /**
     * @brief Set callback for state changes
     * @param callback Callback function (can be nullptr)
     * @param user_data User data passed to callback
     */
    /// @gdb{tag="LDN:STATE", msg="SetStateCallback: callback=%p", args="$x0"}
    void SetStateCallback(StateCallback callback, void* user_data);

    // ========================================================================
    // Utilities
    // ========================================================================

    /**
     * @brief Convert state to string for logging
     * @param state State to convert
     * @return Human-readable state name
     */
    /// @gdb{tag="LDN:STATE", msg="StateToString"}
    static const char* StateToString(CommState state);

    /**
     * @brief Convert transition result to string
     * @param result Result to convert
     * @return Human-readable result name
     */
    /// @gdb{tag="LDN:STATE", msg="ResultToString"}
    static const char* ResultToString(StateTransitionResult result);

    /**
     * @brief Manually signal state change event
     *
     * Call this when external events (like server packets) indicate
     * a state-relevant change that games should be notified about.
     */
    /// @gdb{tag="LDN:STATE", msg="SignalStateChange: signaling event"}
    void SignalStateChange();

private:
    /**
     * @brief Perform state transition if valid
     * @param new_state Target state
     * @return Transition result
     */
    /// @gdb{tag="LDN:STATE", msg="TransitionTo: new_state=%d", args="$x1"}
    StateTransitionResult TransitionTo(CommState new_state);

    /**
     * @brief Check if transition is valid
     * @param from Source state
     * @param to Target state
     * @return true if transition is allowed
     */
    static bool IsValidTransition(CommState from, CommState to);

private:
    mutable os::SdkMutex m_mutex;           ///< Mutex for thread safety
    CommState m_state;                       ///< Current state
    os::SystemEvent m_state_event;           ///< State change event

    StateCallback m_callback;                ///< Optional state change callback
    void* m_callback_user_data;              ///< User data for callback
};

} // namespace ams::mitm::ldn
