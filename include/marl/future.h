// Copyright 2019 The Marl Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef marl_future_h
#define marl_future_h

#include "event.h"

namespace marl {

template <typename T>
struct PromiseShared {
  PromiseShared();

  T value;
  Event event;
};

// Future is a synchronization primitive that can be used to retrieve a value
// from somewhere, generally from another thread which made a Promise that
// relates to this future.
template <typename T>
class Future {
 public:
  Future(const std::shared_ptr<PromiseShared<T>>& shared);

  T& get() const {
    shared->event.wait();
    return shared->value;
  }

  T* poll() const {
    if (shared->event.isSignalled()) {
      return &shared->value;
    }
    return nullptr;
  }

 private:
  std::shared_ptr<PromiseShared<T>> shared;
};

// A Promise is a synchronization primitive that can be used to send a value
// somewhere, generally to another thread which holds a Future that relates to
// this promise.
template <typename T>
class Promise {
 public:
  Promise();
  Promise(const Promise<T>&) = delete;
  Promise(Promise<T>&&) = default;
  Promise<T>& operator=(const Promise<T>&) = delete;
  Promise<T>& operator=(Promise<T>&&) = default;
  ~Promise();

  void setValue(T&& value);

  Future<T> getFuture();

 private:
  std::shared_ptr<PromiseShared<T>> shared;
};

template <typename T>
PromiseShared<T>::PromiseShared() : event(Event::Mode::Manual) {}

template <typename T>
Future<T>::Future(const std::shared_ptr<PromiseShared<T>>& shared)
    : shared(shared) {}

template <typename T>
Promise<T>::Promise() : shared(std::make_shared<PromiseShared<T>>()) {}

template <typename T>
Promise<T>::~Promise() {
  MARL_ASSERT(shared->event.isSignalled(),
              "Promise destroyed without being signaled. A broken promise is "
              "considered a programming error.");
}

template <typename T>
void Promise<T>::setValue(T&& value) {
  MARL_ASSERT(!shared->event.isSignalled(), "Promise already signaled.");
  shared->value = std::move(value);
  shared->event.signal();
}

template <typename T>
Future<T> Promise<T>::getFuture() {
  return Future<T>(shared);
}

// This lets us extend the regular scheduler functions:

// schedule_returns() schedules the function f to be asynchronously called with
// the given arguments using the currently bound scheduler. Returns a future
// with the called function's result value.
template <typename Function, typename... Args>
inline Future<std::invoke_result_t<Function>>
schedule_returns(Function&& f, Task::Attributes&& attributes, Args&&... args) {
  MARL_ASSERT_HAS_BOUND_SCHEDULER("marl::schedule_returns");
  auto scheduler = Scheduler::get();

  Promise<std::invoke_result_t<Function>> promise;

  scheduler->enqueue(Task(
      [promise = std::move(promise), f = std::forward<Function>(f),
       ... args = std::forward<Args>(args)]() mutable {
        promise.setValue(std::move(std::invoke(f, std::move(args)...)));
      },
      std::move(attributes)));

  return promise.getFuture();
}

// schedule_returns() schedules the function f to be asynchronously called using
// the currently bound scheduler. Returns a future with the called function's
// result value.
template <typename Function>
inline Future<std::invoke_result_t<Function>> schedule_returns(
    Function&& f,
    Task::Attributes&& attributes) {
  MARL_ASSERT_HAS_BOUND_SCHEDULER("marl::schedule_returns");
  auto scheduler = Scheduler::get();

  Promise<std::invoke_result_t<Function>> promise;

  scheduler->enqueue(Task(
      [promise = std::move(promise), f = std::forward<Function>(f)]() mutable {
        promise.setValue(std::move(std::invoke(f)));
      },
      std::move(attributes)));

  return promise.getFuture();
}

}  // namespace marl

#endif  // marl_future_h
