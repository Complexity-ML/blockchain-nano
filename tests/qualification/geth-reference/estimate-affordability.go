// Offline reference helper. Run from the pinned go-ethereum v1.16.7 module.
// Calls Geth's estimator directly; it does not test the HTTP RPC adapter.
package main

import (
	"context"
	"encoding/json"
	"fmt"
	"github.com/ethereum/go-ethereum/common"
	"github.com/ethereum/go-ethereum/consensus"
	"github.com/ethereum/go-ethereum/consensus/ethash"
	"github.com/ethereum/go-ethereum/core"
	"github.com/ethereum/go-ethereum/core/state"
	"github.com/ethereum/go-ethereum/core/tracing"
	"github.com/ethereum/go-ethereum/core/types"
	"github.com/ethereum/go-ethereum/eth/gasestimator"
	"github.com/ethereum/go-ethereum/params"
	"github.com/holiman/uint256"
	"math/big"
	"os"
)

type chain struct{ consensus.ChainHeaderReader }

func (chain) Engine() consensus.Engine { return ethash.NewFaker() }
func number(s string) *big.Int {
	if s == "" {
		return new(big.Int)
	}
	n, ok := new(big.Int).SetString(s, 0)
	if !ok {
		panic(s)
	}
	return n
}

type entry struct {
	Fork    string
	Type    int
	Delta   int
	Name    string
	Request map[string]string
	Nano    struct {
		Result string
		Error  json.RawMessage
	}
}

func main() {
	var input struct{ Results []entry }
	data, e := os.ReadFile(os.Args[1])
	if e != nil {
		panic(e)
	}
	if e = json.Unmarshal(data, &input); e != nil {
		panic(e)
	}
	output := []map[string]any{}
	matched := 0
	for _, r := range input.Results {
		cfg := *params.AllDevChainProtocolChanges
		cfg.CancunTime = nil
		cfg.PragueTime = nil
		cfg.OsakaTime = nil
		if r.Fork == "Paris" {
			cfg.ShanghaiTime = nil
		}
		st, e := state.New(types.EmptyRootHash, state.NewDatabaseForTesting())
		if e != nil {
			panic(e)
		}
		from := common.HexToAddress(r.Request["from"])
		to := common.HexToAddress(r.Request["to"])
		cap := big.NewInt(2000000000)
		if r.Type == 2 {
			cap = big.NewInt(3000000000)
		}
		balance := new(big.Int).Add(new(big.Int).Mul(big.NewInt(21000), cap), big.NewInt(int64(5+r.Delta)))
		st.SetBalance(from, uint256.MustFromBig(balance), tracing.BalanceChangeUnspecified)
		header := &types.Header{Number: big.NewInt(1), Time: 1, Difficulty: new(big.Int), GasLimit: 30000000, BaseFee: big.NewInt(1000000000)}
		price := number(r.Request["gasPrice"])
		fee := new(big.Int).Set(price)
		tip := new(big.Int).Set(price)
		if v, ok := r.Request["maxFeePerGas"]; ok {
			fee = number(v)
			tip = number(r.Request["maxPriorityFeePerGas"])
			price = new(big.Int).Add(header.BaseFee, tip)
			if price.Cmp(fee) > 0 {
				price.Set(fee)
			}
		}
		msg := &core.Message{From: from, To: &to, Value: number(r.Request["value"]), GasLimit: number(r.Request["gas"]).Uint64(), GasPrice: price, GasFeeCap: fee, GasTipCap: tip, SkipNonceChecks: true, SkipTransactionChecks: true}
		gas, _, err := gasestimator.Estimate(context.Background(), msg, &gasestimator.Options{Config: &cfg, Chain: chain{}, Header: header, State: st}, 30000000)
		nanoError := len(r.Nano.Error) > 0 && string(r.Nano.Error) != "null"
		same := nanoError == (err != nil)
		if err == nil {
			same = same && number(r.Nano.Result).Uint64() == gas
		}
		if same {
			matched++
		}
		if st.GetBalance(from).ToBig().Cmp(balance) != 0 || st.GetNonce(from) != 0 {
			panic("estimator mutated input account")
		}
		row := map[string]any{"fork": r.Fork, "type": r.Type, "delta": r.Delta, "name": r.Name, "request": r.Request, "gas": gas, "matchesNano": same, "inputAccountUnchanged": true}
		if err != nil {
			row["error"] = err.Error()
		}
		output = append(output, row)
	}
	json.NewEncoder(os.Stdout).Encode(map[string]any{"reference": "go-ethereum v1.16.7 gasestimator.Estimate", "scope": "Offline estimator with same requests and sender balances; not HTTP RPC, consensus or signed admission", "total": len(output), "matched": matched, "results": output})
	if matched != len(output) {
		fmt.Fprintln(os.Stderr, "estimator divergences retained")
		os.Exit(1)
	}
}
