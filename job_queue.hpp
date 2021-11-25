//
// Created by X-ray on 11/23/2021.
//

#pragma once

#ifndef CBSODATA_DUMPER_JOB_QUEUE_HPP
#define CBSODATA_DUMPER_JOB_QUEUE_HPP
//
// Created by X-ray on 8/10/2021.
//
#pragma once
#include <queue>
#include <vector>
#include <thread>
#include <functional>
#include <mutex>

class job_queue {
public:
  using job_t = std::function<void()>;

  explicit job_queue(std::uint32_t thread_count) {
    std::lock_guard lock(mutex_);
    for (int i = 0; i < thread_count; i++) {
      threads_.emplace_back([this]{
        while(this->running_) {
          // keeps cpu usage down
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
          this->run_tick();
        }
      });
    }
    threads_.shrink_to_fit();
  }

  ~job_queue() {
    running_ = false;
    for (auto&& thread : threads_) {
      if (thread.joinable())
        thread.join();
    }
  }

  void add_job(job_t job) {
    try {
      if (job) {
        std::lock_guard lock(mutex_);
        jobs_.emplace(std::move(job));
      }
    } catch(...) {
    }
  }

  [[nodiscard]] std::uint32_t count() {
    std::lock_guard lock(mutex_);
    return jobs_.size();
  }

private:
  std::vector<std::thread> threads_;
  std::queue<job_t> jobs_;
  std::mutex mutex_;
  std::atomic<bool> running_ = true;
private:
  void run_tick() {
    try {
      job_t job = nullptr;
      {
        if (!jobs_.empty()) {
          std::lock_guard lock(mutex_);
          job = jobs_.front();
          jobs_.pop();
        }
      }
      if (job) {
        std::invoke(job);
      } else {
        std::this_thread::yield();
      }
    } catch(...) {}
  }
};
#endif //CBSODATA_DUMPER_JOB_QUEUE_HPP
