#include "../src/equipment_selection.h"
namespace selection_test {
using namespace lantern::equipment;
void run() {
    const Catalog catalog{{100, 200}}; // Two types; two separate copies of type 100.
    Snapshot state{{1}, 7, 1, {900, {}},
        {{10, {100, 1, 501}}, {11, {100, 1, 502}}, {12, {200, 1, 503}}, {13, {300, 1, 504}}}};
    const auto choices = available(state, catalog);
    assert(choices.size() == 3 && choices[0].cell.stack.persistentId == 501 &&
        choices[1].cell.stack.persistentId == 502 && choices[2].cell.stack.item == 200);
    const auto first = prepare(state, catalog, choices[1]);
    assert(first.result == Result::ready && first.writes[0].before.address == 11 &&
        first.writes[0].after.empty() && first.writes[1].after.persistentId == 502);
    assert(stillCurrent(first, state));
    // Planning neither equips nor changes the bag. Only the native commit may do so.
    assert(state.equipped.stack.empty() && state.bag[1].stack.persistentId == 502);
    state.equipped.stack = {100, 1, 600};
    assert(!stillCurrent(first, state));
    ++state.revision;
    assert(prepare(state, catalog, choices[1]).result == Result::stale);
    auto replacement = prepare(state, catalog, select(state, state.bag[2]));
    assert(replacement.result == Result::ready && replacement.writes[0].after.persistentId == 600 &&
        replacement.writes[1].after.persistentId == 503);
    assert(prepare(state, catalog, select(state, state.equipped)).result == Result::full);
    state.bag[1].stack = {};
    const auto removal = prepare(state, catalog, select(state, state.equipped));
    assert(removal.result == Result::ready && removal.writes[1].before.address == 11 &&
        removal.writes[1].after.persistentId == 600 && removal.writes[0].after.empty());
    assert(stillCurrent(removal, state));
    state.bag[1].stack = {400, 1, 800}; // Destination filled after the click.
    assert(!stillCurrent(removal, state));
    state.bag[1].stack = {};
    auto stale = select(state, state.bag[0]);
    state.bag[0].stack.persistentId = 999; // Same type, different instance in same cell.
    assert(prepare(state, catalog, stale).result == Result::stale);
    stale = select(state, state.bag[0]);
    ++state.session;
    assert(prepare(state, catalog, stale).result == Result::stale);
    stale = select(state, state.bag[0]);
    state.character[0] = 2;
    assert(prepare(state, catalog, stale).result == Result::stale);
    assert(prepare(state, catalog, select(state, state.bag[3])).result == Result::unsupported);
    state.bag[0].stack.count = 2;
    assert(prepare(state, catalog, select(state, state.bag[0])).result == Result::unsupported);
    state.bag[0].stack.count = 1;
    state.bag[1].address = state.bag[0].address;
    assert(prepare(state, catalog, select(state, state.bag[0])).result == Result::invalidInventory);
    state.bag[1].address = state.equipped.address;
    assert(prepare(state, catalog, select(state, state.bag[0])).result == Result::invalidInventory);
    std::cout << "PASS: lantern instance selection, multiple types, full-bag swap, unequip capacity, stale UI/session/character/destination rejection\n";
}
}
