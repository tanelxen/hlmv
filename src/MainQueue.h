//
//  MainQueue.h
//  hlmv
//
//  Created by Fedor Artemenkov on 22.04.25.
//

#pragma once
#include <functional>
#include <queue>
#include <mutex>

class MainQueue
{
public:
    static MainQueue& instance()
    {
        static MainQueue q;
        return q;
    }
    
    // Add async task to main loop
    void async(std::function<void()> task)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(task));
    }
    
    // Poll tasks in main loop
    void poll()
    {
        std::queue<std::function<void()>> local;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(local, queue_);
        }
        
        while (!local.empty())
        {
            local.front()();
            local.pop();
        }
    }
    
private:
    MainQueue() = default;
    
    std::mutex mutex_;
    std::queue<std::function<void()>> queue_;
};

