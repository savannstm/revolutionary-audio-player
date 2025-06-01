#pragma once

#include "aliases.hpp"
#include "rapidhasher.hpp"

#include <ranges>

class IndexSet {
   public:
    auto insert(const u32 element) -> bool {
        if (map.contains(element)) {
            return false;
        }

        map[element] = elements.size();
        elements.push_back(element);
        return true;
    }

    auto remove(const u32 element) -> bool {
        if (!map.contains(element)) {
            return false;
        }

        const usize indexToRemove = map[element];
        const usize lastIndex = elements.size() - 1;

        if (indexToRemove != lastIndex) {
            elements[indexToRemove] = elements[lastIndex];
            map[elements[indexToRemove]] = indexToRemove;
        }

        elements.pop_back();
        map.erase(element);
        return true;
    }

    [[nodiscard]] auto contains(const u32 element) const -> bool {
        return map.contains(element);
    }

    [[nodiscard]] auto at(const usize index) const -> u32 {
        if (index >= elements.size()) {
            return UINT32_MAX;
        }

        return elements[index];
    }

    [[nodiscard]] auto indexOf(const u32 element) -> usize {
        if (!map.contains(element)) {
            return SIZE_MAX;
        }

        return map.at(element);
    }

    [[nodiscard]] auto size() const -> usize { return elements.size(); }

    [[nodiscard]] auto empty() const -> bool { return elements.empty(); }

    [[nodiscard]] auto last() const -> u32 { return elements.back(); }

    auto pop() -> u32 {
        const u32 element = this->last();
        elements.pop_back();

        return element;
    }

    [[nodiscard]] auto first() const -> u32 { return elements.front(); }

    void clear() {
        elements.clear();
        map.clear();
    }

    [[nodiscard]] auto begin() const { return elements.begin(); }

    [[nodiscard]] auto end() const { return elements.end(); }

    [[nodiscard]] auto view() const { return views::all(elements); }

   private:
    vector<u32> elements;
    rapidhashmap<u32, usize> map;
};