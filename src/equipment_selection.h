#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

// Equipment-domain code. The native adapter must publish a fresh snapshot and
// apply both writes in ONE native transaction before publishing the next one.
// No game pointer, UI index or item-name lookup identifies an owned instance.
namespace lantern::equipment {
using Character = std::array<std::uint8_t, 16>;
struct Stack {
    std::uint32_t item = 0, count = 0, persistentId = 0;
    bool operator==(const Stack& rhs) const noexcept {
        return item == rhs.item && count == rhs.count && persistentId == rhs.persistentId;
    }
    bool empty() const noexcept { return *this == Stack{}; }
};
struct Cell {
    std::uint64_t address = 0; // Native InventorySlotId, not a memory address.
    Stack stack{};
};
struct Snapshot {
    Character character{};
    std::uint64_t session = 0, revision = 0;
    Cell equipped{};
    std::vector<Cell> bag; // Only actual, usable bag/actionbar cells.
};
struct Catalog {
    std::vector<std::uint32_t> items;
    bool accepts(const Stack& stack) const noexcept {
        return stack.item && stack.count == 1 &&
            std::find(items.begin(), items.end(), stack.item) != items.end();
    }
};
struct Selection {
    Character character{};
    std::uint64_t session = 0, revision = 0;
    Cell cell{};
};
inline Selection select(const Snapshot& snapshot, const Cell& cell) {
    return {snapshot.character, snapshot.session, snapshot.revision, cell};
}
inline std::vector<Selection> available(const Snapshot& snapshot, const Catalog& catalog) {
    std::vector<Selection> result;
    for (const auto& cell : snapshot.bag)
        if (cell.address && cell.address != snapshot.equipped.address && catalog.accepts(cell.stack))
            result.push_back(select(snapshot, cell));
    return result;
}
enum class Result { ready, stale, invalidInventory, unsupported, full, empty };
struct Write { Cell before{}; Stack after{}; };
struct Plan {
    Result result = Result::stale;
    Character character{};
    std::uint64_t session = 0, revision = 0;
    std::array<Write, 2> writes{};
};
inline bool valid(const Snapshot& snapshot) {
    if (snapshot.character == Character{} || !snapshot.session || !snapshot.equipped.address)
        return false;
    std::vector<std::uint64_t> addresses{snapshot.equipped.address};
    for (const auto& cell : snapshot.bag) {
        if (!cell.address || std::find(addresses.begin(), addresses.end(), cell.address) != addresses.end())
            return false;
        if ((!cell.stack.item || !cell.stack.count) && !cell.stack.empty()) return false;
        addresses.push_back(cell.address);
    }
    return true;
}
inline Plan prepare(const Snapshot& snapshot, const Catalog& catalog,
                    const Selection& selection) {
    Plan plan{};
    if (selection.character != snapshot.character || selection.session != snapshot.session ||
        selection.revision != snapshot.revision) return plan;
    if (!valid(snapshot)) { plan.result = Result::invalidInventory; return plan; }
    plan.character = snapshot.character;
    plan.session = snapshot.session;
    plan.revision = snapshot.revision;
    const auto& equipped = snapshot.equipped;
    if (!equipped.stack.empty() && !catalog.accepts(equipped.stack)) {
        plan.result = Result::unsupported; return plan;
    }
    if (selection.cell.address == equipped.address) {
        if (!(selection.cell.stack == equipped.stack)) return plan;
        if (equipped.stack.empty()) { plan.result = Result::empty; return plan; }
        const auto destination = std::find_if(snapshot.bag.begin(), snapshot.bag.end(),
            [](const Cell& cell) { return cell.stack.empty(); });
        if (destination == snapshot.bag.end()) { plan.result = Result::full; return plan; }
        plan.writes = {{{equipped, {}}, {*destination, equipped.stack}}};
    } else {
        const auto source = std::find_if(snapshot.bag.begin(), snapshot.bag.end(),
            [&](const Cell& cell) { return cell.address == selection.cell.address; });
        if (source == snapshot.bag.end() || !(source->stack == selection.cell.stack)) return plan;
        if (!catalog.accepts(source->stack)) { plan.result = Result::unsupported; return plan; }
        // Replacement uses the vacated source cell even when the bag is full.
        plan.writes = {{{*source, equipped.stack}, {equipped, source->stack}}};
    }
    plan.result = Result::ready;
    return plan;
}
// Revalidate on the inventory owner thread, immediately before beginning the
// transaction. A queued UI action must not overwrite a changed slot.
inline bool stillCurrent(const Plan& plan, const Snapshot& snapshot) {
    if (plan.result != Result::ready || plan.character != snapshot.character ||
        plan.session != snapshot.session || plan.revision != snapshot.revision || !valid(snapshot))
        return false;
    for (const auto& write : plan.writes) {
        if (write.before.address == snapshot.equipped.address) {
            if (!(write.before.stack == snapshot.equipped.stack)) return false;
        } else {
            const auto cell = std::find_if(snapshot.bag.begin(), snapshot.bag.end(),
                [&](const Cell& candidate) { return candidate.address == write.before.address; });
            if (cell == snapshot.bag.end() || !(write.before.stack == cell->stack)) return false;
        }
    }
    return true;
}
}
