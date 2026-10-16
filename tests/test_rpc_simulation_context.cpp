#include "nano/rpc.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>
#include <unistd.h>

using namespace nano;
namespace {
unsigned checks = 0, refusals = 0, groups = 0;
std::string stage = "arguments";
Json case_records = Json::array(), fixtures = Json::object();
void check(bool ok, const std::string& why) {
    ++checks;
    if (!ok) throw std::runtime_error(why);
}
std::string addr(unsigned n) { return hex(bytes(Int(n), 20)); }
const Int balance("1000000000000000000"), gas_limit(100000), value(5);
// GASPRICE, BASEFEE, BALANCE(CALLER), CALLVALUE and SELFBALANCE.
const std::string context_code = "0x3a600052486020523331604052346060524760805260a06000f3";
// Only the exact balance supplied in calldata passes. A successful probe then
// writes slot zero on its disposable copy. Lower gas changes the upfront debit.
const std::string balance_guard = "0x333160003514600e5760006000fd5b600160005500";
const std::string zero_fee_guard = "0x48153a1516600d5760006000fd5b600160005500";
const std::string revert_code = "0x600160005560006000fd";
// COPY nine init bytes at byte 33; CREATE must succeed or the parent REVERTs.
// Returning zero after child OOG would make a successful but weaker parent a
// legitimate estimate. Keep the address/quote oracle tied to successful CREATE.
// The child touches slot zero for an independent access-list observation.
const std::string create_code = "0x60096021600039600960006000f08015601b5760005260206000f35b60006000fd6000545060006000f3";
struct Temp {
    std::filesystem::path path;
    Temp() {
        auto text = (std::filesystem::temp_directory_path() / "nano-rpc-context-XXXXXX").string();
        if (!mkdtemp(text.data())) throw std::runtime_error("mkdtemp");
        path = text;
    }
    ~Temp() { std::error_code ec; std::filesystem::remove_all(path, ec); }
};
struct Fixture {
    Temp tmp;
    const std::string sender = addr(0xa1), probe = addr(0xa2), guard = addr(0xa3),
        reverter = addr(0xa4), creator = addr(0xa5), empty = addr(0xa6), zero_guard = addr(0xa7);
    Genesis genesis;
    const std::string fork;
    std::unique_ptr<Blockchain> chain;
    std::unique_ptr<Rpc> rpc;
    explicit Fixture(const char* selected_fork) : fork(selected_fork) {
        Json alloc = {{sender, {{"balance", quantity(balance)}, {"nonce", "0x3"}}},
            {probe, {{"balance", "0xa"}, {"code", context_code}}},
            {guard, {{"code", balance_guard}}}, {reverter, {{"code", revert_code}}},
            {zero_guard, {{"code", zero_fee_guard}}},
            {creator, {{"balance", quantity(balance)}, {"nonce", "0x7"}, {"code", create_code}}}};
        const Json input = {{"config", {{"chainId", 1337}, {"evmFork", fork}}}, {"alloc", alloc}};
        genesis = Genesis::from_json(input);
        fixtures[fork] = {{"genesisInput", input}, {"header", genesis.block.header.to_json()}};
        chain = std::make_unique<Blockchain>(tmp.path / "db", genesis);
        rpc = std::make_unique<Rpc>(*chain);
    }
    Json call(const std::string& target) const {
        return {{"from", sender}, {"to", target}, {"gas", quantity(gas_limit)}, {"chainId", "0x539"}};
    }
    Json before() const {
        return {{"state", chain->state().to_json()}, {"head", hex(chain->tip().header.hash())},
            {"pool", chain->pool_info(true)}, {"events", chain->event_cursor()}};
    }
    Json response(const char* method, const Json& args) {
        const Json params = Json::array({args, "latest"});
        const auto reply = rpc->handle({{"jsonrpc", "2.0"}, {"id", 17}, {"method", method}, {"params", params}});
        if (case_records.size() >= 256) throw std::runtime_error("public case record bound");
        case_records.push_back({{"fork", fork}, {"method", method}, {"params", params}, {"response", reply}});
        return reply;
    }
    Json result(const char* method, const Json& args) {
        const auto r = response(method, args);
        check(r.contains("result") && !r.contains("error"), std::string(method) + " result: " + r.dump());
        return r.at("result");
    }
    void rejected(const char* method, const Json& args, int code) {
        const auto r = response(method, args);
        check(r.contains("error") && !r.contains("result") && r.at("error").at("code") == code,
            std::string(method) + " expected refusal " + std::to_string(code) + ": " + r.dump());
        ++refusals;
    }
    void unchanged(const Json& expected) const {
        check(before() == expected, "simulation changed state, head, pool or event cursor");
        const auto stored = chain->store().state_at(0);
        check(stored && stored->to_json() == expected.at("state"), "simulation changed durable genesis state");
    }
    void reopen(const Json& expected) {
        rpc.reset(); chain.reset();
        chain = std::make_unique<Blockchain>(tmp.path / "db", genesis);
        rpc = std::make_unique<Rpc>(*chain);
        unchanged(expected);
    }
};
std::string words(std::initializer_list<Int> values) {
    Bytes encoded;
    for (const auto& n : values) { auto b = bytes(n, 32); encoded.insert(encoded.end(), b.begin(), b.end()); }
    return hex(encoded);
}
void emit(const char* name) {
    ++groups;
    std::cout << Json{{"scenario", name}, {"passed", true}, {"checks", checks},
        {"expectedRefusals", refusals}, {"fixtures", fixtures}, {"caseRecords", case_records},
        {"privateInputsRead", false}, {"networkStarted", false}, {"productionQualified", false}}.dump() << std::endl;
    case_records = Json::array(); fixtures = Json::object();
}
void fees() {
    stage = "fees";
    for (const auto* fork : {"Paris", "Shanghai"}) {
        Fixture f(fork); const auto before = f.before();
        check(f.genesis.block.header.base_fee.value_or(Int(0)) > 0, "fixture needs nonzero canonical base fee");
        for (const auto& fields : std::vector<Json>{Json::object(),
            {{"gasPrice", nullptr}, {"maxFeePerGas", nullptr}, {"maxPriorityFeePerGas", nullptr}},
            {{"gasPrice", "0x0"}}, {{"maxFeePerGas", "0x0"}, {"maxPriorityFeePerGas", "0x0"}},
            {{"gas", nullptr}, {"nonce", nullptr}, {"type", nullptr}, {"chainId", nullptr},
             {"data", nullptr}, {"input", nullptr}, {"accessList", nullptr},
             {"gasPrice", nullptr}, {"maxFeePerGas", nullptr}, {"maxPriorityFeePerGas", nullptr}}}) {
            auto args = f.call(f.probe); args["value"] = quantity(value); args.update(fields);
            const auto out = f.result("eth_call", args);
            check(out == words({Int(0), Int(0), Int(balance - value), value, Int(15)}),
                "zero-price simulation must zero BASEFEE only in its copy and transfer value");
            args["to"] = f.zero_guard;
            const auto quoted = f.result("eth_estimateGas", args);
            check(quantity(quoted) > 21000, "zero-fee estimator must execute BASEFEE/GASPRICE guard and SSTORE");
            args["gas"] = quoted;
            check(f.result("eth_call", args) == "0x", "zero-fee quoted guard must replay successfully");
            f.unchanged(before);
        }
        for (bool null_from : {false, true}) {
            Json args = {{"to", f.probe}, {"gas", quantity(gas_limit)}};
            if (null_from) { args["from"] = nullptr; args["value"] = nullptr; }
            check(f.result("eth_call", args) == words({Int(0), Int(0), Int(0), Int(0), Int(10)}),
                "absent/null sender with zero fees/value must use the zero-address copy");
            f.unchanged(before);
        }
        check(f.chain->tip().header.base_fee == f.genesis.block.header.base_fee,
            "zero-price simulation changed canonical header base fee");
        f.reopen(before);
    }
    emit("rpc-simulation-context-fees");
}
void balances() {
    stage = "balances";
    for (const auto* fork : {"Paris", "Shanghai"}) {
        Fixture f(fork); const auto before = f.before();
        const Int base = f.genesis.block.header.base_fee.value_or(Int(0));
        for (unsigned mode = 0; mode < 6; ++mode) {
            Json fields;
            Int price;
            if (mode == 0 || mode == 5) { fields = {{"gasPrice", "0x77359400"}}; price = 2000000000; }
            else {
                const Int cap = mode == 1 ? Int(3000000000) : Int(1100000000), tip(200000000);
                fields = {{"maxFeePerGas", quantity(cap)}, {"maxPriorityFeePerGas", quantity(tip)}};
                price = std::min(cap, Int(base + tip));
            }
            // Fee style, not a legacy/dynamic type hint, selects GASPRICE and
            // the upfront debit in message simulations (not signed admission).
            if (mode == 3 || mode == 4) fields["type"] = mode == 3 ? "0x0" : "0x1";
            if (mode == 5) fields["type"] = "0x2";
            auto args = f.call(f.probe); args["value"] = quantity(value); args.update(fields);
            const Int after = balance - gas_limit * price - value;
            const auto out = f.result("eth_call", args);
            check(out == words({price, base, after, value, Int(15)}), "upfront effective gas purchase/context differs");
            args["to"] = f.guard; args["data"] = hex(bytes(after, 32));
            check(f.result("eth_call", args) == "0x", "balance guard rejects correct upfront purchase");
            const auto quote = f.result("eth_estimateGas", args);
            check(quantity(quote) == gas_limit, "balance-sensitive estimate must retain the only successful gas budget");
            args["gas"] = quote;
            check(f.result("eth_call", args) == "0x", "quoted budget does not reproduce successful purchase context");
            const auto list = f.result("eth_createAccessList", args);
            check(list.is_object() && !list.contains("error") && list.at("accessList").is_array() &&
                quantity(list.at("gasUsed")) > 0 && quantity(list.at("gasUsed")) <= gas_limit,
                "access-list simulation must share effective debit and preserve its own gas accounting");
            f.unchanged(before);
        }
        auto reverting = f.call(f.reverter); reverting["gasPrice"] = "0x77359400"; reverting["value"] = "0x5";
        for (const auto* method : {"eth_call", "eth_estimateGas"}) f.rejected(method, reverting, 3);
        const auto failed_list = f.result("eth_createAccessList", reverting);
        check(failed_list.contains("error") && failed_list.at("error").is_string() &&
            failed_list.at("accessList").is_array() && quantity(failed_list.at("gasUsed")) > 0,
            "access-list VM revert remains a result error, not success or parameter rejection");
        ++refusals;
        f.unchanged(before); f.reopen(before);
    }
    emit("rpc-simulation-context-balances");
}
void nonces() {
    stage = "nonces";
    for (const auto* fork : {"Paris", "Shanghai"}) {
        Fixture f(fork); const auto before = f.before();
        auto args = f.call(f.creator); args["from"] = f.creator; args["gas"] = "0x30d40";
        args["gasPrice"] = "0x77359400";
        const auto child = create_address(f.creator, Int(8));
        for (bool explicit_nonce : {false, true}) {
            if (explicit_nonce) args["nonce"] = "0x9";
            const auto out = f.result("eth_call", args);
            check(out == hex(bytes(integer(unhex(child)), 32)), "sender nonce must increment before nested CREATE");
            auto quoted = args; quoted["gas"] = f.result("eth_estimateGas", args);
            check(f.result("eth_call", quoted) == out, "CREATE quote changes state-derived address");
            const auto list = f.result("eth_createAccessList", args);
            bool found = false;
            for (const auto& row : list.at("accessList")) if (address(row.at("address")) == child) {
                found = row.at("storageKeys") == Json::array({hex(bytes(Int(0), 32))});
            }
            check(!list.contains("error") && found, "access-list child slot must use incremented captured State nonce");
            f.unchanged(before);
        }
        auto creation = f.call(f.empty); creation.erase("to"); creation["gas"] = "0x30d40";
        creation["gasPrice"] = "0x77359400"; creation["nonce"] = "0x9";
        creation["data"] = "0x3060005260206000f3";
        const auto expected = hex(bytes(integer(unhex(create_address(f.sender, Int(3)))), 32));
        check(f.result("eth_call", creation) == expected, "top-level CREATE must use pre-increment captured nonce");
        creation["gas"] = f.result("eth_estimateGas", creation);
        check(f.result("eth_call", creation) == expected, "top-level creation quote changes address");
        // BALANCE(ADDRESS) traces an address, unlike SLOAD. The VM derives the
        // creation address from State nonce 3; caller hint 9 must not change the
        // tracer's exclusion, either. The child's transferred value is visible.
        creation["gas"] = "0x30d40"; creation["value"] = "0x5";
        creation["data"] = "0x303160005260206000f3";
        check(f.result("eth_call", creation) == words({Int(5)}), "creation value/self-balance is not visible");
        const auto self_list = f.result("eth_createAccessList", creation);
        check(!self_list.contains("error") && self_list.at("accessList") == Json::array(),
            "top-level creation address exclusion must use captured State nonce, not nonce hint");
        Json null_creation = {{"from", nullptr}, {"to", nullptr}, {"type", nullptr},
            {"gas", "0x30d40"}, {"data", "0x3060005260206000f3"}};
        const auto zero_created = hex(bytes(integer(unhex(create_address(addr(0), Int(0)))), 32));
        check(f.result("eth_call", null_creation) == zero_created, "null to means creation; null from means zero-address caller");
        null_creation["gas"] = f.result("eth_estimateGas", null_creation);
        check(f.result("eth_call", null_creation) == zero_created, "nullable creation quote must reproduce the actual address");
        f.unchanged(before); f.reopen(before);
    }
    emit("rpc-simulation-context-nonces");
}
void bounds() {
    stage = "bounds";
    const Int two64 = Int(1) << 64, two256 = Int(1) << 256;
    for (const auto* fork : {"Paris", "Shanghai"}) {
        Fixture f(fork); const auto before = f.before();
        auto args = f.call(f.empty);
        // First case is accepted by the old call path: maxFee is unbounded and
        // gas purchase is absent. It is independently causal from context fixes.
        const std::vector<Json> invalid = {
            {{"maxFeePerGas", quantity(two256)}, {"maxPriorityFeePerGas", "0x0"}},
            {{"gasPrice", quantity(two256)}}, {{"value", quantity(two256)}},
            {{"gas", quantity(two64)}}, {{"nonce", quantity(two64)}},
            {{"maxFeePerGas", quantity(two256 + 1)}, {"maxPriorityFeePerGas", quantity(two256)}},
            {{"gas", "0x00"}}, {{"nonce", "0x01"}}, {{"value", 0}}, {{"gasPrice", "1"}},
            {{"blobVersionedHashes", Json::array()}}, {{"maxFeePerBlobGas", "0x1"}},
            {{"blobs", Json::array()}}, {{"commitments", Json::array()}}, {{"proofs", Json::array()}},
            {{"authorizationList", Json::array()}}, {{"type", "0x3"}}, {{"type", "0x4"}}};
        for (const auto& fields : invalid) {
            auto bad = args; bad.update(fields);
            for (const auto* method : {"eth_call", "eth_estimateGas", "eth_createAccessList"})
                f.rejected(method, bad, -32602);
        }
        auto largest_gas_nonce = args; largest_gas_nonce["gas"] = quantity(Int(two64 - 1));
        largest_gas_nonce["nonce"] = quantity(Int(two64 - 1)); largest_gas_nonce["gasPrice"] = "0x77359400";
        check(f.result("eth_call", largest_gas_nonce) == "0x", "valid uint64 argument boundaries rejected");
        check(quantity(f.result("eth_estimateGas", largest_gas_nonce)) == 21000, "valid uint64 gas boundary must clamp to header/funds");
        check(!f.result("eth_createAccessList", largest_gas_nonce).contains("error"), "access-list uint64 argument boundaries rejected");
        auto largest_fee = args; largest_fee["maxFeePerGas"] = quantity(Int(two256 - 1));
        largest_fee["maxPriorityFeePerGas"] = "0x0";
        for (const auto* method : {"eth_call", "eth_estimateGas", "eth_createAccessList"})
            f.rejected(method, largest_fee, -32000); // In range, but intentionally unaffordable.
        auto largest_value = args; largest_value["value"] = quantity(Int(two256 - 1));
        largest_value["gasPrice"] = "0x77359400";
        for (const auto* method : {"eth_call", "eth_estimateGas", "eth_createAccessList"})
            f.rejected(method, largest_value, -32000);
        f.unchanged(before); f.reopen(before);
    }
    emit("rpc-simulation-context-bounds");
}
} // namespace

int main(int argc, char** argv) {
    std::string selected = "all";
    if ((argc != 2 && argc != 4) || std::string(argv[1]) != "--simulation-context-self-test" ||
        (argc == 4 && std::string(argv[2]) != "--group")) {
        std::cerr << "usage: --simulation-context-self-test [--group fees|balances|nonces|bounds]\n"; return 2;
    }
    if (argc == 4) selected = argv[3];
    if (selected != "all" && selected != "fees" && selected != "balances" && selected != "nonces" && selected != "bounds") {
        std::cerr << "unknown simulation context group\n"; return 2;
    }
    try {
        if (selected == "all" || selected == "fees") fees();
        if (selected == "all" || selected == "balances") balances();
        if (selected == "all" || selected == "nonces") nonces();
        if (selected == "all" || selected == "bounds") bounds();
        std::cout << Json{{"format", "nano-rpc-simulation-context-targeted-v1"}, {"passed", true},
            {"selectedGroup", selected}, {"groups", groups}, {"checks", checks}, {"expectedRefusals", refusals},
            {"oldSuiteExecuted", false}, {"networkStarted", false}, {"signaturesCreated", 0},
            {"productionQualified", false}}.dump() << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        std::cout << Json{{"format", "nano-rpc-simulation-context-targeted-v1"}, {"passed", false},
            {"selectedGroup", selected}, {"stage", stage}, {"groups", groups}, {"checks", checks},
            {"expectedRefusals", refusals}, {"fixtures", fixtures}, {"caseRecords", case_records},
            {"productionQualified", false}}.dump() << std::endl;
        return 1;
    }
}
