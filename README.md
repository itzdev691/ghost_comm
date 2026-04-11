# ESP-NOW Mesh-Like ESP32 Nodes

This project gives every ESP32 the same firmware so each board can:

- discover nearby nodes using ESP-NOW broadcast
- store peers dynamically at runtime
- send unicast or broadcast packets
- react to message types instead of fixed device roles

## Message model

Each packet uses a shared `MeshMessage` struct with:

- `type`: the behavior selector
- `senderMac`: sender identity
- `targetMac`: specific recipient or broadcast
- `nodeName`: friendly node label derived from MAC
- `text`: optional payload for text or status messages

Current message types:

- `DISCOVERY`: announces presence to the network
- `DISCOVERY_ACK`: confirms a node heard discovery
- `PING`: triggers a unicast `STATUS` reply
- `TEXT`: arbitrary application text payload
- `STATUS`: periodic liveness or response packet

## How it works

1. Every node starts in `WIFI_STA` mode on the same ESP-NOW channel.
2. Each node adds the ESP-NOW broadcast address as a peer.
3. Nodes periodically broadcast `DISCOVERY` packets.
4. When a node hears a valid packet, it stores the sender in a local peer table and adds that sender as an ESP-NOW peer.
5. Behavior is selected by `message.type`, so the same firmware can evolve into different logical behaviors without changing device roles.

## Serial commands

Open the serial monitor at `115200` baud and use:

- `help`
- `peers`
- `discover`
- `ping AA:BB:CC:DD:EE:FF`
- `send AA:BB:CC:DD:EE:FF hello there`
- `broadcast hello everyone`

## Build and flash

This project is set up for PlatformIO.

```bash
python3 -m platformio run
python3 -m platformio run -t upload
python3 -m platformio device monitor
```

## Notes

- All nodes must use the same ESP-NOW channel. This project defaults to channel `6`.
- The peer table is runtime-managed and prunes entries that have not been seen recently.
- The current code is a mesh-like discovery and messaging layer, not a full routed multi-hop mesh.
- If you want true relaying next, the clean extension is to add TTL, message IDs, and rebroadcast rules for selected packet types.
