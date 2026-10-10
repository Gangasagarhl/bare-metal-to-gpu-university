// lock_order_checker.hpp (F2-40): a debug mutex that records the order in which locks are
// taken (curriculum B10: "a debug mode that records lock acquisition order and detects
// inversions"). Every time a thread takes lock B while holding lock A, the edge A -> B is
// remembered. Taking A while holding B after A -> B was seen is reported as an inversion.
#pragma once
#include <algorithm>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

class LockOrderRegistry
{
public:
    static LockOrderRegistry& instance()
    {
        static LockOrderRegistry registry;
        return registry;
    }

    // Called before `wanted` is locked by a thread that already holds `held`.
    void check(const std::vector<const std::string*>& held, const std::string& wanted)
    {
        std::lock_guard<std::mutex> guard(m_);
        for (const std::string* h : held) {
            if (edges_.count({wanted, *h}) != 0) {
                ++inversions_;
                std::cout << "LOCK-ORDER INVERSION: taking \"" << wanted << "\" while holding \"" << *h
                          << "\", but \"" << wanted << "\" -> \"" << *h << "\" was recorded before\n";
            }
            edges_.insert({*h, wanted});
        }
    }

    int inversions()
    {
        std::lock_guard<std::mutex> guard(m_);
        return inversions_;
    }

private:
    std::mutex m_;  // protects the two members below
    std::set<std::pair<std::string, std::string>> edges_;
    int inversions_ = 0;
};

class CheckedMutex
{
public:
    explicit CheckedMutex(std::string name) : name_(std::move(name)) {}

    void lock()
    {
        LockOrderRegistry::instance().check(held(), name_);
        m_.lock();
        held().push_back(&name_);
    }

    void unlock()
    {
        auto& h = held();
        h.erase(std::find(h.begin(), h.end(), &name_));
        m_.unlock();
    }

private:
    static std::vector<const std::string*>& held()
    {
        thread_local std::vector<const std::string*> locksHeldByThisThread;
        return locksHeldByThisThread;
    }

    std::mutex m_;
    std::string name_;
};
