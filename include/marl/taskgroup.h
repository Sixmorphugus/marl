// Copyright 2020 The Marl Authors.
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

#ifndef marl_taskgroup_h
#define marl_taskgroup_h

#include "waitgroup.h"

namespace marl {

// Task groups are used to monitor the state of their tasks from the scheduler.
// They are entirely optional but if in use allow you to do things like wait for
// all the tasks in the group to be completed or suspended.
class TaskGroup {
 public:
  void waitForAllComplete() const;

  void waitForAllCompleteOrSuspended() const;

  void taskStarted();
  void taskAboutToBeCompleted();
  void taskAboutToBeSuspended();
  void taskAboutToBeResumed();

 private:
  WaitGroup completed;
  WaitGroup completed_or_suspended;
};

inline void TaskGroup::waitForAllComplete() const {
  completed.wait();
}

inline void TaskGroup::waitForAllCompleteOrSuspended() const {
  completed_or_suspended.wait();
}

inline void TaskGroup::taskStarted() {
  completed.add();
  completed_or_suspended.add();
}

inline void TaskGroup::taskAboutToBeCompleted() {
  completed.done();
}

inline void TaskGroup::taskAboutToBeSuspended() {
  completed_or_suspended.done();
}

inline void TaskGroup::taskAboutToBeResumed() {
  completed_or_suspended.add();
}

}  // namespace marl

#endif  // marl_taskgroup_h
