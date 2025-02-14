//
// Created by xabdomo on 2/14/25.
//

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <string>
#include <utility>
#include <functional>
#include <iostream>
#include <ostream>
#include <stdexcept>

#define DEFAULT_PROGRESS_BAR "%prog%% |%bar%| %curr%/%max% [%eta%, %spd%it/s]"
#define DEFAULT_PROGRESS_BAR_UNLIMITED "[%spd%it/s]"
namespace progress_bar {


    void set(const std::string &bar, long long max_progress = 100, int bar_size = 25, const std::string& bar_char = "\u25A0");
    void setUnlimited(const std::string &bar = DEFAULT_PROGRESS_BAR_UNLIMITED);
    void step();
    void setMax(long long max_progress);
    void setSize(int bar_size);
    void setBarChar(const std::string &bar_char);
    void setProgress(long long prog);
    void reset();
    void hide();

    std::string defaultBarWithTitle(const std::string& title);


    template <typename Iterator>
    class ProgressIterator {
    public:
        ProgressIterator(Iterator iter, Iterator end, std::function<void(int)> progressCallback)
            : iter_(iter), end_(end), progressCallback_(std::move(progressCallback)), index_(0) {}

        auto& operator*() { return *iter_; }

        ProgressIterator& operator++() {
            ++iter_;
            ++index_;
            if (progressCallback_) {
                progressCallback_(index_);
            }
            return *this;
        }

        ProgressIterator operator++(int) {
            ProgressIterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator!=(const ProgressIterator& other) const {
            return iter_ != other.iter_;
        }

    private:
        Iterator iter_;
        Iterator end_;
        std::function<void(int)> progressCallback_;
        int index_;
    };

    template <typename T>
    class IterableWrapper {
    public:
        explicit IterableWrapper(T& iterable) : iterable_(iterable) {}

        static void progressCallback(int index) {
            setProgress(index);
        }

        auto begin() {
            return ProgressIterator(iterable_.begin(), iterable_.end(),[](const int index) { progressCallback(index); });
        }

        auto end() {
            return ProgressIterator(iterable_.end(), iterable_.end(),[](const int index) { progressCallback(index); });
        }

    private:
        T& iterable_;
    };

    template <typename T>
    IterableWrapper<T> from(T& iterator, long size, const std::string& name = "") {
        if (name.empty()) {
            set(DEFAULT_PROGRESS_BAR, size);
        } else {
            set(defaultBarWithTitle(name), size);
        }
        return IterableWrapper<T>(iterator);
    }


    template <typename T>
    IterableWrapper<T> from(T& iterator, const std::string& name = "") {
        const auto size = std::distance(iterator.begin(), iterator.end());
        if (name.empty()) {
            set(DEFAULT_PROGRESS_BAR, size);
        } else {
            set(defaultBarWithTitle(name), size);
        }
        return IterableWrapper<T>(iterator);
    }


    template <typename T>
    class RangedProgressIterator {
    public:
        RangedProgressIterator(T current, T end, T step, std::function<void(int)> progressCallback)
            : current_(current), end_(end), step_(step), progressCallback_(std::move(progressCallback)), step_prog(1), current_progress(0) {}

        T operator*() const { return current_; }

        RangedProgressIterator& operator++() {
            current_ += step_;

            if (step_ > 0 && current_ > end_) {
                current_ = end_ + step_;
            }

            if (step_ < 0 && current_ < end_) {
                current_ = end_ + step_;
            }

            current_progress += step_prog;
            if (progressCallback_) {
                progressCallback_(static_cast<int>(current_progress));
            }

            return *this;
        }

        RangedProgressIterator operator++(int) {
            ProgressIterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator!=(const RangedProgressIterator& other) const {
            return !(
                (
                    (current_ == other.current_) ||
                    (step_ > 0 && current_ > end_ && other.current_ > other.end_) || // two special conditions for floating numbers
                    (step_ < 0 && current_ < end_ && other.current_ < other.end_)    // translates to : both out of range.
                ) &&
                end_ == other.end_ &&
                step_ == other.step_
            );
        }

    private:
        T current_;
        T end_;
        T step_;
        long double step_prog;
        long double current_progress;
        std::function<void(int)> progressCallback_;
    };

    template <typename T>
    class IterableRange {
    public:
        IterableRange(T start, T end, T step) : start_(start), end_(end), step_(step) {}

        static void progressCallback(const int index) {
            setProgress(index);
        }

        auto begin() {
            return RangedProgressIterator<T>(start_, end_, step_, [](const int index) { progressCallback(index); });
        }

        auto end() {
            return RangedProgressIterator<T>(end_ + step_, end_, step_, [](const int index) { progressCallback(index); });
        }

    private:
        T start_;
        T end_;
        T step_;
    };

    template <typename T>
    IterableRange<T> ranged(T start = 0, T end = 100, T step = 1, const std::string& name = "") {

        if (step == 0) {
            throw std::invalid_argument("step = 0");
        }

        if (step > 0) {
            if (start > end) {
                throw std::invalid_argument("start > end");
            }
        } else {
            if (start < end) {
                throw std::invalid_argument("start < end");
            }
        }

        if (name.empty()) {
            set(DEFAULT_PROGRESS_BAR, static_cast<int>(std::abs((start - end) / step)));
        } else {
            set(defaultBarWithTitle(name), static_cast<int>(std::abs((start - end) / step)));
        }
        return IterableRange<T>(start, end, step);
    }
};

#endif //PROGRESS_BAR_H
