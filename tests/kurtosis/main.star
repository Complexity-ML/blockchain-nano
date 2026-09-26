# Two real Nano processes. RPC remains loopback-only: inspect via container exec.
def rpc_command(method, params = []):
    body = json.encode({"jsonrpc": "2.0", "id": 1, "method": method, "params": params})
    return ["curl", "-fsS", "--max-time", "5", "-H", "Content-Type: application/json", "--data", body, "http://127.0.0.1:8545"]

def scalar(method, field, params = []):
    # These responses have one matching scalar field; no arbitrary JSON traversal.
    # Shell positional arguments preserve the JSON request without interpolation.
    command = rpc_command(method, params)
    pattern = 's/.*"' + field + '":"([^"]+)".*/\\1/p'
    return ExecRecipe(command = ["sh", "-c", 'response=$("$@") || exit; printf "%s" "$response" | sed -nE \'{}\' | tr -d \'\\n\''.format(pattern), "rpc"] + command)

def run(plan, args):
    image = args.get("image", "blockchain-nano:local")
    assets = plan.upload_files(src = "./assets", name = "beacon-fixtures")
    first = None
    for index in range(2):
        name = "nano-{}".format(index)
        node = json.decode(read_file("./assets/node-{}.json".format(index)))
        node["rpc_host"] = "127.0.0.1"
        if args.get("witness_recovery", False):
            node["bellatrix"]["recoveryWitness"] = "/var/lib/nano/core-witness"
            for key in node["bellatrix"]["validator"]["keys"]:
                key["recoveryWitness"] = key["journal"] + "-witness"
        if index == 1:
            node["bootnodes"] = ["/ip4/" + first.ip_address + "/tcp/30303/p2p/" + peer]
        templates = {"node.json": struct(template = "{{.config}}", data = {"config": json.encode(node)})}
        if args.get("witness_recovery", False):
            restart = json.decode(json.encode(node))
            restart["bellatrix"]["initial"] = None
            templates["restart.json"] = struct(template = "{{.config}}", data = {"config": json.encode(restart)})
        configuration = plan.render_templates(config = templates)
        service = plan.add_service(name = name, config = ServiceConfig(
            image = image, ports = {"p2p": PortSpec(number = 30303, wait = "60s")},
            files = {"/config": assets, "/node": configuration},
            env_vars = json.decode(read_file("./assets/env-{}.json".format(index))),
            entrypoint = ["/bin/sh", "-c"] if args.get("witness_recovery", False) else ["/usr/local/bin/nano"],
            cmd = ["config=/node/node.json; if [ -d /var/lib/nano/chain ]; then config=/node/restart.json; fi; exec /usr/local/bin/nano run /config/genesis.json /var/lib/nano/chain \"$config\""] if args.get("witness_recovery", False) else ["run", "/config/genesis.json", "/var/lib/nano/chain", "/node/node.json"],
        ))
        if index == 0:
            first = service
            response = plan.wait(service_name = name, recipe = scalar("nno_getNetworkInfo", "peerId"), field = "output", assertion = "!=", target_value = "", timeout = "30s")
            peer = response["output"]
    for name in ["nano-0", "nano-1"]:
        plan.wait(service_name = name, recipe = scalar("net_peerCount", "result"), field = "output", assertion = "==", target_value = "0x1", timeout = "60s", description = "Require one connected peer")
        plan.print(plan.exec(service_name = name, recipe = ExecRecipe(command = rpc_command("nno_getNetworkInfo")))["output"])
    check_finality(plan)

def check_finality(plan):
    # Require a nonzero finalized block on each node. RPC errors/null never pass.
    finalized_recipe = ExecRecipe(command = ["sh", "-c", 'response=$("$@") || exit; printf "%s" "$response" | grep -Eq \'"number":"0x([2-9a-f]|[1-9a-f][0-9a-f]+)"\'', "rpc"] + rpc_command("eth_getBlockByNumber", ["finalized", False]))
    for name in ["nano-0", "nano-1"]:
        plan.wait(service_name = name, recipe = finalized_recipe, field = "code", assertion = "==", target_value = 0, timeout = "180s", description = "Require Beacon finality beyond the imported block-1 anchor")
    finalized = plan.exec(service_name = "nano-0", recipe = scalar("eth_getBlockByNumber", "hash", ["finalized", False]))
    plan.verify(finalized["output"], "!=", "")
    other = plan.exec(service_name = "nano-1", recipe = scalar("eth_getBlockByNumber", "hash", ["finalized", False]))
    plan.verify(other["output"], "==", finalized["output"], description = "Require identical finalized execution block hashes")
    for name in ["nano-0", "nano-1"]:
        plan.print(plan.exec(service_name = name, recipe = ExecRecipe(command = rpc_command("eth_getBlockByNumber", ["finalized", False])))["output"])
    plan.print(plan.exec(service_name = "nano-0", recipe = ExecRecipe(command = rpc_command("engine_exchangeCapabilities", [[]])))["output"])
