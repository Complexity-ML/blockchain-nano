"""Negative controls: the EEST bridge must never turn missing work into a pass."""
import json
from pathlib import Path
import pytest
from ethereum_test_fixtures import StateFixture
from nano_consumer import NanoFixtureConsumer

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "tests/fixtures/ethereum-official/cancun-ported_static--stPreCompiledContracts2--call_ecrecover_invalid_signature--call_ecrecover_invalid_signature.json"

@pytest.mark.parametrize("mutation", ["none", "wrong-root", "missing-case", "unsupported-fork"])
def test_consumer_failure_controls(tmp_path, mutation):
    fixture = json.loads(SOURCE.read_text())
    name = next(iter(fixture))
    if mutation == "wrong-root":
        for variants in fixture[name]["post"].values():
            for case in variants:
                case["hash"] = "0x" + "00" * 32
    if mutation == "unsupported-fork":
        fixture[name]["post"] = {"UnimplementedFork": next(iter(fixture[name]["post"].values()))}
    target = tmp_path / "fixture.json"
    target.write_text(json.dumps(fixture))
    consumer = NanoFixtureConsumer(binary=ROOT / "build-release/nano-ethereum-state-runner")
    def run():
        consumer.consume_fixture(StateFixture, target, "nonexistent" if mutation == "missing-case" else name)
    if mutation == "none":
        run()
    else:
        with pytest.raises(AssertionError):
            run()
