// Bounded read-only compatibility probe using Assertoor's exact Beacon SDK.
package main

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	client "github.com/ethpandaops/go-eth2-client"
	"github.com/ethpandaops/go-eth2-client/api"
	v1 "github.com/ethpandaops/go-eth2-client/api/v1"
	beaconhttp "github.com/ethpandaops/go-eth2-client/http"
	"github.com/ethpandaops/go-eth2-client/spec/bellatrix"
	dynssz "github.com/pk910/dynamic-ssz"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strconv"
	"time"
)

func fixtures(directory string) error {
	data, err := os.ReadFile(filepath.Join(directory, "spec.json"))
	if err != nil {
		return err
	}
	var rawSpec map[string]string
	if err = json.Unmarshal(data, &rawSpec); err != nil {
		return err
	}
	spec := make(map[string]any)
	for key, value := range rawSpec {
		n, e := strconv.ParseUint(value, 10, 64)
		if e == nil {
			spec[key] = n
		} else {
			spec[key] = value
		}
	}
	codec := dynssz.NewDynSsz(spec)
	for i := 0; i < 8; i++ {
		path := filepath.Join(directory, strconv.Itoa(i))
		data, err := os.ReadFile(path + ".json")
		if err != nil {
			return err
		}
		var block bellatrix.SignedBeaconBlock
		if err = json.Unmarshal(data, &block); err != nil {
			return fmt.Errorf("fixture %d: %w", i, err)
		}
		encoded, err := codec.MarshalSSZ(&block)
		if err != nil {
			return err
		}
		raw, err := os.ReadFile(path + ".ssz")
		if err != nil {
			return err
		}
		if !bytes.Equal(raw, encoded) {
			return fmt.Errorf("fixture %d JSON->SSZ mismatch", i)
		}
	}
	fmt.Println("{\"officialBodyFixtures\":8,\"jsonToSSZExact\":true,\"consensusAdmissionTest\":false}")
	return nil
}
func run() error {
	if len(os.Args) == 3 && os.Args[1] == "--fixtures" {
		return fixtures(os.Args[2])
	}
	if len(os.Args) != 2 {
		return fmt.Errorf("expected loopback URL")
	}
	base := os.Args[1]
	ctx, cancel := context.WithTimeout(context.Background(), 15*time.Second)
	defer cancel()
	svc, err := beaconhttp.New(ctx, beaconhttp.WithAddress(base), beaconhttp.WithCustomSpecSupport(true))
	if err != nil {
		return err
	}
	if _, err = svc.(client.GenesisProvider).Genesis(ctx, &api.GenesisOpts{}); err != nil {
		return err
	}
	spec, err := svc.(client.SpecProvider).Spec(ctx, &api.SpecOpts{})
	if err != nil {
		return err
	}
	header, err := svc.(client.BeaconBlockHeadersProvider).BeaconBlockHeader(ctx, &api.BeaconBlockHeaderOpts{Block: "head"})
	if err != nil {
		return err
	}
	root := fmt.Sprintf("0x%x", header.Data.Root)
	block, err := svc.(client.SignedBeaconBlockProvider).SignedBeaconBlock(ctx, &api.SignedBeaconBlockOpts{Block: root})
	if err != nil {
		return err
	}
	if block.Data.Bellatrix == nil {
		return fmt.Errorf("expected Bellatrix")
	}
	fetch := func(accept string) ([]byte, error) {
		req, e := http.NewRequestWithContext(ctx, "GET", base+"/eth/v2/beacon/blocks/"+root, nil)
		if e != nil {
			return nil, e
		}
		req.Header.Set("Accept", accept)
		response, e := http.DefaultClient.Do(req)
		if e != nil {
			return nil, e
		}
		defer response.Body.Close()
		if response.StatusCode != 200 || response.Header.Get("Eth-Consensus-Version") != "bellatrix" {
			return nil, fmt.Errorf("response %d version %q", response.StatusCode, response.Header.Get("Eth-Consensus-Version"))
		}
		return io.ReadAll(io.LimitReader(response.Body, 16*1024*1024))
	}
	raw, err := fetch("application/octet-stream")
	if err != nil {
		return err
	}
	data, err := fetch("application/json")
	if err != nil {
		return err
	}
	var envelope struct {
		Data *bellatrix.SignedBeaconBlock `json:"data"`
	}
	if err = json.Unmarshal(data, &envelope); err != nil {
		return err
	}
	if envelope.Data == nil {
		return fmt.Errorf("missing JSON block")
	}
	codec := dynssz.NewDynSsz(spec.Data)
	encoded, err := codec.MarshalSSZ(envelope.Data)
	if err != nil {
		return err
	}
	if !bytes.Equal(raw, encoded) {
		return fmt.Errorf("JSON->SSZ differs from archived block")
	}
	encoded, err = codec.MarshalSSZ(block.Data.Bellatrix)
	if err != nil {
		return err
	}
	if !bytes.Equal(raw, encoded) {
		return fmt.Errorf("SDK block differs from archived bytes")
	}
	events := make(chan *v1.BlockEvent, 1)
	if err = svc.(client.EventsProvider).Events(ctx, &api.EventsOpts{Topics: []string{"block", "finalized_checkpoint"}, BlockHandler: func(_ context.Context, event *v1.BlockEvent) {
		select {
		case events <- event:
		default:
		}
	}, FinalizedCheckpointHandler: func(_ context.Context, _ *v1.FinalizedCheckpointEvent) {}}); err != nil {
		return err
	}
	select {
	case event := <-events:
		if event.ExecutionOptimistic {
			return fmt.Errorf("native block event marked optimistic")
		}
		observed, e := svc.(client.BeaconBlockHeadersProvider).BeaconBlockHeader(ctx, &api.BeaconBlockHeaderOpts{Block: fmt.Sprintf("0x%x", event.Block)})
		if e != nil {
			return e
		}
		if observed.Data.Header.Message.Slot != event.Slot {
			return fmt.Errorf("SSE event differs from durable header")
		}
		fmt.Printf("{\"nativeSSEBlockReceived\":true,\"durableHeaderMatched\":true,\"slot\":%d}\n", event.Slot)
	case <-ctx.Done():
		return fmt.Errorf("no native SSE block received: %w", ctx.Err())
	}
	fmt.Printf("{\"beaconSDK\":\"v0.1.6\",\"blockRoot\":%q,\"sszBytes\":%d,\"jsonRoundTripExact\":true,\"assertoorSuiteExecuted\":false}\n", root, len(raw))
	return nil
}
func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
