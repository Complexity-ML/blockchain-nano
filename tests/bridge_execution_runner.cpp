#include "nano/engine.hpp"

#include <array>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace nano;

namespace {
constexpr std::size_t max_line_bytes = 2 * 1024 * 1024;
constexpr std::size_t max_requests = 5000;
const std::string coinbase = "0x0000000000000000000000000000000000009999";

void require(bool value, const char* message) {
    if (!value) throw std::invalid_argument(message);
}

std::string bounded_error(const std::exception& error) {
    std::string out = error.what();
    if (out.size() > 240) out.resize(240);
    for (char& c : out) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 32 || u > 126) c = '?';
    }
    return out;
}

void fields(const Json& value, const std::set<std::string>& required,
            const std::set<std::string>& optional = {}) {
    require(value.is_object(), "request must be an object");
    for (const auto& key : required) require(value.contains(key), "required field absent");
    for (auto it = value.begin(); it != value.end(); ++it)
        require(required.count(it.key()) || optional.count(it.key()), "unknown request field");
}

bool valid_id(const Json& value) {
    if (value.is_string()) {
        const auto s = value.get<std::string>();
        if (s.empty() || s.size() > 64) return false;
        for (unsigned char c : s) if (c < 32 || c > 126) return false;
        return true;
    }
    if (!value.is_number_integer()) return false;
    if (value.is_number_unsigned()) return value.get<uint64_t>() <= 9007199254740991ULL;
    return value.get<int64_t>() >= 0 && value.get<int64_t>() <= 9007199254740991LL;
}

Int uint256(const Json& value) {
    std::string digits;
    int base = 10;
    if (value.is_string()) {
        digits = value.get<std::string>();
        if (digits.size() >= 2 && digits[0] == '0' && digits[1] == 'x') {
            base = 16;
            digits.erase(0, 2);
        }
    } else if (value.is_number_unsigned()) {
        digits = std::to_string(value.get<uint64_t>());
    } else if (value.is_number_integer() && value.get<int64_t>() >= 0) {
        digits = std::to_string(value.get<int64_t>());
    } else {
        throw std::invalid_argument("unsigned integer or integer string required");
    }
    require(!digits.empty() && digits.size() <= (base == 16 ? 64U : 78U),
            "integer encoding outside uint256 bound");
    for (unsigned char c : digits)
        require(base == 16 ? std::isxdigit(c) != 0 : c >= '0' && c <= '9',
                "invalid unsigned integer encoding");
    Int out;
    require(mpz_set_str(out.get_mpz_t(), digits.c_str(), base) == 0 &&
            out >= 0 && out < (Int(1) << 256), "integer outside uint256");
    return out;
}

std::string input_address(const Json& value) {
    require(value.is_string(), "address string required");
    auto text = value.get<std::string>();
    require(text.size() == 42 && text.substr(0, 2) == "0x", "20-byte address required");
    for (std::size_t i = 2; i < text.size(); ++i)
        require(std::isxdigit(static_cast<unsigned char>(text[i])) != 0, "invalid address hex");
    return address(text);
}

Bytes input_data(const Json& value) {
    require(value.is_string(), "data string required");
    const auto text = value.get<std::string>();
    require(text.size() >= 2 && text.substr(0, 2) == "0x" && text.size() % 2 == 0,
            "even 0x-prefixed data required");
    for (std::size_t i = 2; i < text.size(); ++i)
        require(std::isxdigit(static_cast<unsigned char>(text[i])) != 0, "invalid data hex");
    return unhex(text);
}

uint64_t input_gas(const Json& request) {
    const Int value = request.contains("gas") ? uint256(request.at("gas")) : Int(30000000);
    require(value > 0 && value <= Int(30000000), "gas outside 1..30000000");
    return u64(value);
}

enum class Line { end, ready, oversized };
Line read_line(std::string& out) {
    out.clear();
    for (;;) {
        const int c = std::cin.get();
        if (c == std::char_traits<char>::eof()) {
            if (std::cin.bad()) throw std::runtime_error("stdin read failed");
            return out.empty() ? Line::end : Line::ready;
        }
        if (c == '\n') return Line::ready;
        if (out.size() == max_line_bytes) return Line::oversized;
        out.push_back(static_cast<char>(c));
    }
}

Json logs_json(const std::vector<Log>& logs) {
    Json out = Json::array();
    for (const auto& log : logs) out.push_back(log.to_json());
    return out;
}

struct ChainSlot {
    Int chain_id;
    State state;
    uint64_t number = 0;
    std::vector<Receipt> receipts;
    explicit ChainSlot(unsigned id) : chain_id(id) {}
};

class Runner {
    // Public unit-test keys 1, 2, 3. Never generated or loaded from the host.
    std::array<Bytes, 3> keys_;
    std::array<std::string, 3> senders_;
    std::array<ChainSlot, 2> chains_{ChainSlot(31337), ChainSlot(31338)};

    std::size_t signer(const Json& from) const {
        const auto sender = input_address(from);
        for (std::size_t i = 0; i < senders_.size(); ++i)
            if (senders_[i] == sender) return i;
        throw std::invalid_argument("sender is not one of the three public fixtures");
    }

    Header header(const ChainSlot& slot, uint64_t number) const {
        Header h;
        h.coinbase = coinbase;
        h.base_fee = Int(0);
        h.difficulty = 0;
        h.gas_limit = Int(30000000);
        h.number = Int(std::to_string(number));
        h.timestamp = Int(1700000000) + Int(12) * h.number;
        return h;
    }

    Json send(ChainSlot& slot, const Json& request) {
        fields(request, {"id", "chain", "action", "from", "data"}, {"to", "value", "gas"});
        const auto i = signer(request.at("from"));
        Transaction tx;
        tx.type = 0;
        tx.chain_id = slot.chain_id;
        tx.nonce = static_cast<const State&>(slot.state).get(senders_[i]).nonce;
        tx.gas_price = 1;
        tx.gas = Int(std::to_string(input_gas(request)));
        tx.value = request.contains("value") ? uint256(request.at("value")) : Int(0);
        tx.data = input_data(request.at("data"));
        if (request.contains("to") && !request.at("to").is_null())
            tx.to = input_address(request.at("to"));
        const auto signed_tx = SignedTransaction::sign(tx, keys_[i]);
        const auto before_root = slot.state.root();
        const auto h = header(slot, slot.number + 1);
        ExecutionEngine engine(slot.chain_id, {}, EvmRevision::Paris);
        const auto receipt = engine.apply_transaction(slot.state, signed_tx, h, 0, 0);
        if (!receipt.success && !receipt.logs.empty())
            throw std::logic_error("failed transaction retained observable logs");
        slot.receipts.push_back(receipt);
        ++slot.number; // Reverted EVM transactions still consume nonce, gas and this synthetic step.
        return {
            {"success", receipt.success}, {"gasUsed", quantity(receipt.gas_used)},
            {"logs", logs_json(receipt.logs)},
            {"contractAddress", receipt.contract_address ? Json(*receipt.contract_address) : Json(nullptr)},
            {"transactionHash", hex(receipt.transaction_hash)}, {"from", senders_[i]},
            {"to", tx.to ? Json(*tx.to) : Json(nullptr)}, {"nonce", quantity(tx.nonce)},
            {"senderNonceAfter", quantity(static_cast<const State&>(slot.state).get(senders_[i]).nonce)},
            {"chainId", quantity(slot.chain_id)}, {"number", quantity(h.number)},
            {"timestamp", quantity(h.timestamp)}, {"stateRootBefore", hex(before_root)},
            {"stateRoot", hex(slot.state.root())}, {"syntheticHeader", true}
        };
    }

    Json call(const ChainSlot& slot, const Json& request) {
        fields(request, {"id", "chain", "action", "from", "to", "data"}, {"value", "gas"});
        const auto i = signer(request.at("from"));
        const auto target = input_address(request.at("to"));
        const auto value = request.contains("value") ? uint256(request.at("value")) : Int(0);
        const auto before_root = slot.state.root();
        auto copy = slot.state;
        require(copy.transfer(senders_[i], target, value), "insufficient simulated value balance");
        const auto h = header(slot, slot.number);
        ExecutionContext ctx;
        ctx.revision = EvmRevision::Paris;
        ctx.origin = senders_[i];
        ctx.caller = senders_[i];
        ctx.address = target;
        ctx.coinbase = coinbase;
        ctx.chain_id = slot.chain_id;
        ctx.gas_price = 1;
        ctx.base_fee = 0;
        ctx.gas_limit = Int(30000000);
        ctx.gas = input_gas(request);
        ctx.number = h.number;
        ctx.timestamp = h.timestamp;
        ctx.value = value;
        ctx.code = static_cast<const State&>(copy).get(target).code;
        ctx.data = input_data(request.at("data"));
        ctx.is_static = false; // Mutable simulation on an isolated copy, never an admitted state write.
        const auto result = EVM(copy).execute(ctx);
        if (slot.state.root() != before_root) throw std::logic_error("simulation changed retained state");
        return {
            {"success", result.success}, {"gasUsed", quantity(Int(std::to_string(result.gas_used)))},
            {"returnData", hex(result.return_data)}, {"error", result.error.substr(0, 240)},
            {"logs", result.success ? logs_json(result.logs) : Json::array()},
            {"stateDiscarded", true}, {"stateRoot", hex(before_root)}
        };
    }

    Json reload(ChainSlot& slot, const Json& request) {
        fields(request, {"id", "chain", "action"});
        const auto before_root = slot.state.root();
        const auto before = slot.state.to_json();
        const auto encoded = before.dump();
        const auto old_number = slot.number;
        auto restored = State::from_json(Json::parse(encoded));
        if (restored.root() != before_root || restored.to_json() != before)
            throw std::logic_error("JSON reload changed account, code, nonce or storage");
        slot.state = std::move(restored);
        if (slot.number != old_number) throw std::logic_error("reload changed synthetic step counter");
        return {
            {"unchanged", true}, {"stateRootBefore", hex(before_root)},
            {"stateRoot", hex(slot.state.root())}, {"number", quantity(Int(std::to_string(slot.number)))},
            {"serializedBytes", encoded.size()}, {"accounts", slot.state.accounts_view().size()},
            {"persistence", "in-memory JSON roundtrip only"}
        };
    }

    Json events(const ChainSlot& slot, const Json& request) const {
        fields(request, {"id", "chain", "action"}, {"address"});
        std::optional<std::string> filter;
        if (request.contains("address")) filter = input_address(request.at("address"));
        Json receipts = Json::array(), logs = Json::array();
        for (const auto& receipt : slot.receipts) {
            Json selected = Json::array();
            for (std::size_t i = 0; i < receipt.logs.size(); ++i) {
                const auto& log = receipt.logs[i];
                if (filter && address(log.address) != *filter) continue;
                auto item = log.to_json();
                item["transactionHash"] = hex(receipt.transaction_hash);
                item["number"] = quantity(receipt.block_number);
                item["logIndex"] = quantity(Int(std::to_string(i)));
                selected.push_back(std::move(item));
            }
            if (filter && selected.empty()) continue;
            receipts.push_back(receipt.to_json()); // Full actual receipt, never a filtered substitute.
            for (const auto& log : selected) logs.push_back(log);
        }
        return {{"receipts", receipts}, {"logs", logs}, {"stateRoot", hex(slot.state.root())},
                {"inMemoryIndexOnly", true}, {"syntheticHeaders", true}};
    }

public:
    Runner() {
        for (std::size_t i = 0; i < keys_.size(); ++i) {
            keys_[i] = bytes(Int(std::to_string(i + 1)), 32);
            senders_[i] = address(public_address(public_key(keys_[i])));
        }
        for (auto& slot : chains_)
            for (const auto& sender : senders_)
                slot.state.get(sender).balance = Int("1000000000000000000000000");
    }

    Json handle(const Json& request) {
        require(request.is_object() && request.contains("id") && valid_id(request.at("id")),
                "bounded scalar id required");
        require(request.contains("chain") && request.at("chain").is_number_integer() &&
                (request.at("chain") == 0 || request.at("chain") == 1), "chain must be zero or one");
        require(request.contains("action") && request.at("action").is_string(), "action string required");
        auto& slot = chains_[request.at("chain").get<unsigned>()];
        const auto action = request.at("action").get<std::string>();
        if (action == "send") return send(slot, request);
        if (action == "call") return call(slot, request);
        if (action == "reload") return reload(slot, request);
        if (action == "events") return events(slot, request);
        require(action == "root", "unknown action");
        fields(request, {"id", "chain", "action"});
        return {{"stateRoot", hex(slot.state.root())}, {"chainId", quantity(slot.chain_id)},
                {"number", quantity(Int(std::to_string(slot.number)))}};
    }
};
} // namespace

int main(int argc, char**) {
    if (argc != 1) {
        std::cerr << "bridge_execution_runner accepts no arguments\n";
        return 2;
    }
    try {
        Runner runner;
        bool malformed = false;
        std::size_t requests = 0;
        std::string line;
        for (;;) {
            const auto status = read_line(line);
            if (status == Line::end) break;
            if (status == Line::oversized || requests == max_requests) {
                std::cout << Json{{"id", nullptr}, {"ok", false},
                    {"error", status == Line::oversized ? "input line exceeds 2 MiB" : "request count exceeds 5000"}}.dump()
                    << '\n' << std::flush;
                return 1;
            }
            ++requests;
            Json id = nullptr;
            try {
                const auto request = Json::parse(line);
                if (request.is_object() && request.contains("id") && valid_id(request.at("id")))
                    id = request.at("id");
                const auto result = runner.handle(request);
                std::cout << Json{{"id", id}, {"ok", true}, {"result", result}}.dump() << '\n' << std::flush;
            } catch (const std::exception& error) {
                malformed = true;
                std::cout << Json{{"id", id}, {"ok", false}, {"error", bounded_error(error)}}.dump()
                          << '\n' << std::flush;
            }
        }
        return malformed ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << bounded_error(error) << '\n';
        return 1;
    }
}
