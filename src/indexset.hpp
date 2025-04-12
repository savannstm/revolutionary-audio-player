#pragma once

#include "aliases.hpp"

#include <ranges>

template <typename T>
class IndexSet {
   private:
    vector<T> elements;
    hashmap<T, usize> map;

   public:
    auto insert(const T& element) -> bool {
        if (map.contains(element)) {
            return false;
        }

        map[element] = elements.size();
        elements.push_back(element);
        return true;
    }

    auto remove(const T& element) -> bool {
        if (!map.contains(element)) {
            return false;
        }

        const usize indexToRemove = map[element];
        const usize lastIndex = elements.size() - 1;

        if (indexToRemove != lastIndex) {
            elements[indexToRemove] = std::move(elements[lastIndex]);
            map[elements[indexToRemove]] = indexToRemove;
        }

        elements.pop_back();
        map.erase(element);
        return true;
    }

    [[nodiscard]] auto contains(const T& element) const -> bool {
        return map.contains(element);
    }

    [[nodiscard]] auto at(const usize index) const -> const T& {
        if (index >= elements.size()) {
            throw panic(
                format("Index {} out of range {}", index, elements.size())
            );
        }

        return elements[index];
    }

    [[nodiscard]] auto indexOf(const T& element) const -> usize {
        if (!map.contains(element)) {
            throw panic("Element not found");
        }

        return map.at(element);
    }

    [[nodiscard]] auto size() const noexcept -> usize {
        return elements.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return elements.empty();
    }

    [[nodiscard]] auto last() const noexcept -> const T& {
        return elements.back();
    }

    auto pop() noexcept -> const T& {
        const T element = std::move(this->last());
        elements.pop_back();

        return element;
    }

    [[nodiscard]] auto first() const noexcept -> const T& {
        return elements.front();
    }

    void clear() noexcept {
        elements.clear();
        map.clear();
    }

    auto begin() const noexcept { return elements.begin(); }

    auto end() const noexcept { return elements.end(); }

    auto view() const noexcept { return views::all(elements); }
};