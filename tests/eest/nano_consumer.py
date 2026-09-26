"""EEST test-harness adapter. All execution and verdicts come from Nano C++.

This module is not a node dependency; load with consume direct -p nano_consumer.
Unsupported fixtures, timeouts and missing case results fail explicitly.
"""
import json
import hashlib
import re
import subprocess
from pathlib import Path
from ethereum_clis.ethereum_cli import EthereumCLI
from ethereum_clis.fixture_consumer_tool import FixtureConsumerTool
from ethereum_test_fixtures import StateFixture, BlockchainFixture

class NanoFixtureConsumer(FixtureConsumerTool, fixture_formats=[StateFixture, BlockchainFixture]):
    default_binary = Path("nano-ethereum-state-runner")
    detect_binary_pattern = re.compile(r"^nano-ethereum-state-runner 1$")
    version_flag = "--version"

    def __init__(self, *, binary=None, trace=False):
        if trace:
            raise ValueError("Nano direct consumer does not expose opcode traces yet")
        EthereumCLI.__init__(self, binary=binary)

    def consume_fixture(self, fixture_format, fixture_path, fixture_name=None, debug_output_path=None):
        if fixture_format is StateFixture:
            binary = self.binary
        elif fixture_format is BlockchainFixture:
            binary = self.binary.with_name("nano-ethereum-block-runner")
        else:
            raise ValueError(f"Unsupported fixture format: {fixture_format}")
        command = [str(binary), str(fixture_path)]
        if fixture_name is not None:
            command += ["--case", fixture_name]
        result = subprocess.run(command, capture_output=True, text=True, timeout=120)
        if debug_output_path:
            identity = str(Path(fixture_path).resolve()) + "\n" + (fixture_name or "<all>")
            target = Path(debug_output_path) / hashlib.sha256(identity.encode()).hexdigest()
            target.mkdir(parents=True, exist_ok=True)
            (target / "nano-command.json").write_text(json.dumps(command))
            (target / "nano-stdout.jsonl").write_text(result.stdout)
            (target / "nano-stderr.txt").write_text(result.stderr)
        lines = [json.loads(line) for line in result.stdout.splitlines() if line.strip()]
        summary = lines[-1].get("summary") if lines else None
        assert summary, f"Nano produced no summary: {result.stderr}"
        assert result.returncode == 0, result.stdout + result.stderr
        assert summary["matched"] > 0, "No matching Nano case was executed"
        assert all(summary[k] == 0 for k in ["mismatched", "unsupported", "errors"]), summary
        cases = [line for line in lines if "case" in line]
        assert cases, "Missing per-case execution evidence"
        if fixture_name is not None:
            assert all(line["case"] == fixture_name for line in cases), "Wrong fixture executed"
