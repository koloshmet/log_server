#pragma once

#include <coroutine>

#include <memory>
#include <utility>
#include <exception>

class TTask {
public:
    struct promise_type;

private:
    using TCoroHandle = std::coroutine_handle<promise_type>;

    struct TTaskValues {
        std::exception_ptr Err = nullptr;
        bool Done = false;
    };

public:
    struct promise_type {
        TTask get_return_object() {
            Values = std::make_shared<TTaskValues>();
            return TTask{TCoroHandle::from_promise(*this), Values};
        }
        std::suspend_never initial_suspend() {
            return {};
        }
        std::suspend_never final_suspend() noexcept {
            return {};
        }
        void unhandled_exception() {
            Values->Done = true;
            Values->Err = std::current_exception();
        }
        void return_void() {
            Values->Done = true;
        }

        std::shared_ptr<TTaskValues> Values;
    };

    TTask()
        : Coro(nullptr)
        , Values(nullptr)
    {}

    TTask(const TTask&) = delete;
    TTask(TTask&&) = delete;
    TTask& operator=(const TTask&) = delete;
    TTask& operator=(TTask&&) = delete;

    [[nodiscard]]
    bool Done() const {
        return Values->Done;
    }

    void Join() const {
        ThrowIfError();
    }

private:
    explicit TTask(std::coroutine_handle<promise_type> coro, std::shared_ptr<TTaskValues> vals)
        : Coro(coro)
        , Values(std::move(vals))
    {}

    void ThrowIfError() const {
        if (Values->Err) [[unlikely]] {
            std::rethrow_exception(Values->Err);
        }
    }

public:
    TCoroHandle Coro;
    std::shared_ptr<TTaskValues> Values;
};
