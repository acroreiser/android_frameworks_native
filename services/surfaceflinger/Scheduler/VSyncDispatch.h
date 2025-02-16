/*
 * Copyright 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <functional>
#include <optional>
#include <string>

#include <ftl/mixins.h>
#include <scheduler/Time.h>
#include <utils/Timers.h>

namespace android::scheduler {

struct ScheduleResult {
    TimePoint callbackTime;
    TimePoint vsyncTime;
};

enum class CancelResult { Cancelled, TooLate, Error };

/*
 * VSyncDispatch is a class that will dispatch callbacks relative to system vsync events.
 */
class VSyncDispatch {
public:
    struct CallbackToken : ftl::DefaultConstructible<CallbackToken, size_t>,
                           ftl::Equatable<CallbackToken>,
                           ftl::Incrementable<CallbackToken> {
        using DefaultConstructible::DefaultConstructible;
    };

    virtual ~VSyncDispatch();

    /*
     * A callback that can be registered to be awoken at a given time relative to a vsync event.
     * \param [in] vsyncTime:        The timestamp of the vsync the callback is for.
     * \param [in] targetWakeupTime: The timestamp of intended wakeup time of the cb.
     * \param [in] readyTime:        The timestamp of intended time where client needs to finish
     *                               its work by.
     */
    using Callback =
            std::function<void(nsecs_t vsyncTime, nsecs_t targetWakeupTime, nsecs_t readyTime)>;

    /*
     * Registers a callback that will be called at designated points on the vsync timeline.
     * The callback can be scheduled, rescheduled targeting vsync times, or cancelled.
     * The token returned must be cleaned up via unregisterCallback.
     *
     * \param [in] callbackFn   A function to schedule for callback. The resources needed to invoke
     *                          callbackFn must have lifetimes encompassing the lifetime of the
     *                          CallbackToken returned.
     * \param [in] callbackName A human-readable, unique name to identify the callback.
     * \return                  A token that can be used to schedule, reschedule, or cancel the
     *                          invocation of callbackFn.
     *
     */
    virtual CallbackToken registerCallback(Callback, std::string callbackName) = 0;

    /*
     * Unregisters a callback.
     *
     * \param [in] token        The callback to unregister.
     *
     */
    virtual void unregisterCallback(CallbackToken token) = 0;

    /*
     * Timing information about a scheduled callback
     *
     * @workDuration:  The time needed for the client to perform its work
     * @readyDuration: The time needed for the client to be ready before a vsync event.
     *                 For external (non-SF) clients, not only do we need to account for their
     *                 workDuration, but we also need to account for the time SF will take to
     *                 process their buffer/transaction. In this case, readyDuration will be set
     *                 to the SF duration in order to provide enough end-to-end time, and to be
     *                 able to provide the ready-by time (deadline) on the callback.
     *                 For internal clients, we don't need to add additional padding, so
     *                 readyDuration will typically be 0.
     * @lastVsync: The targeted display time. This will be snapped to the closest
     *                 predicted vsync time after lastVsync.
     * @committedVsyncOpt: The display time that is committed to the callback as the
     *                 target vsync time.
     *
     * callback will be dispatched at 'workDuration + readyDuration' nanoseconds before a vsync
     * event.
     */
    struct ScheduleTiming {
        nsecs_t workDuration = 0;
        nsecs_t readyDuration = 0;
        nsecs_t lastVsync = 0;
        std::optional<nsecs_t> committedVsyncOpt;

        bool operator==(const ScheduleTiming& other) const {
            return workDuration == other.workDuration && readyDuration == other.readyDuration &&
                    lastVsync == other.lastVsync && committedVsyncOpt == other.committedVsyncOpt;
        }

        bool operator!=(const ScheduleTiming& other) const { return !(*this == other); }
    };

    /*
     * Schedules the registered callback to be dispatched.
     *
     * The callback will be dispatched at 'workDuration + readyDuration' nanoseconds before a vsync
     * event.
     *
     * The caller designates the earliest vsync event that should be targeted by the lastVsync
     * parameter.
     * The callback will be scheduled at (workDuration + readyDuration - predictedVsync), where
     * predictedVsync is the first vsync event time where ( predictedVsync >= lastVsync ).
     *
     * If (workDuration + readyDuration - lastVsync) is in the past, or if a callback has
     * already been dispatched for the predictedVsync, an error will be returned.
     *
     * It is valid to reschedule a callback to a different time.
     *
     * \param [in] token           The callback to schedule.
     * \param [in] scheduleTiming  The timing information for this schedule call
     * \return                     The expected callback time if a callback was scheduled,
     *                             along with VSYNC time for the callback scheduled.
     *                             std::nullopt if the callback is not registered.
     */
    virtual std::optional<ScheduleResult> schedule(CallbackToken token,
                                                   ScheduleTiming scheduleTiming) = 0;

    /*
     * Update the timing information for a scheduled callback.
     * If the callback is not scheduled, then this function does nothing.
     *
     * \param [in] token           The callback to schedule.
     * \param [in] scheduleTiming  The timing information for this schedule call
     * \return                     The expected callback time if a callback was scheduled,
     *                             along with VSYNC time for the callback scheduled.
     *                             std::nullopt if the callback is not registered.
     */
    virtual std::optional<ScheduleResult> update(CallbackToken token,
                                                 ScheduleTiming scheduleTiming) = 0;

    /* Cancels a scheduled callback, if possible.
     *
     * \param [in] token    The callback to cancel.
     * \return              A CancelResult::TooLate if the callback was already dispatched.
     *                      A CancelResult::Cancelled if the callback was successfully cancelled.
     *                      A CancelResult::Error if there was an pre-condition violation.
     */
    virtual CancelResult cancel(CallbackToken token) = 0;

    virtual void dump(std::string& result) const = 0;

protected:
    VSyncDispatch() = default;

    VSyncDispatch(const VSyncDispatch&) = delete;
    VSyncDispatch& operator=(const VSyncDispatch&) = delete;
};

class VSyncCallbackRegistration {
public:
    VSyncCallbackRegistration(std::shared_ptr<VSyncDispatch>, VSyncDispatch::Callback,
                              std::string callbackName);
    ~VSyncCallbackRegistration();

    VSyncCallbackRegistration(VSyncCallbackRegistration&&);
    VSyncCallbackRegistration& operator=(VSyncCallbackRegistration&&);

    // See documentation for VSyncDispatch::schedule.
    std::optional<ScheduleResult> schedule(VSyncDispatch::ScheduleTiming scheduleTiming);

    // See documentation for VSyncDispatch::update.
    std::optional<ScheduleResult> update(VSyncDispatch::ScheduleTiming scheduleTiming);

    // See documentation for VSyncDispatch::cancel.
    CancelResult cancel();

private:
    friend class VSyncCallbackRegistrationTest;

    std::shared_ptr<VSyncDispatch> mDispatch;
    std::optional<VSyncDispatch::CallbackToken> mToken;
};

} // namespace android::scheduler
