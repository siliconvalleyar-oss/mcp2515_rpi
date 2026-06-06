---
name: ecu-emulator
description: |
  Use when working on the ECU Multi-Emulator project (mcp2515_rpi2w) — a C++17 multi-brand vehicle diagnostic emulator for Raspberry Pi + MCP2515 CAN controller. Covers build with arm-linux-gnueabihf-g++ cross-compiler, all source under src/, Makefile in project root, manufacturer-specific protocols (Chevrolet/GM, Ford, Toyota, BMW, VW/Audi, Mercedes, Honda, Nissan, Hyundai), OBD2/UDS/GMLAN/KWP2000/CAN-TP, seed/key security, simulation engine, REST API, WebSocket, SQLite3, and documentation. Use ONLY when the task targets this specific emulator project.
compatibility:
  - C++17
  - arm-linux-gnueabihf-g++
  - Linux (SocketCAN)
  - Raspberry Pi
---

# ECU Multi-Emulator Skill

## Project Structure

```
mcp2515_rpi2w/
├── Makefile                    # Cross-compile: arm-linux-gnueabihf-g++ -std=c++17
├── config.json                 # Emulator configuration (CAN, manufacturers, API, logging)
├── install_multi_emulator.sh   # Full installation script (systemd, logrotate, CAN)
├── README.md                   # Full documentation
├── docker/                     # Dockerfile + entrypoint
├── docs/                       # OpenAPI 3.0 spec, connection diagram
├── examples/                   # Python client, bash test commands
├── scripts/                    # Utility scripts (install, build, deploy, fuzz, diag)
└── src/
    ├── main.cpp                # Entry point (CAN, simulation, API threads)
    ├── core/                   # can_manager, protocol_router, session_manager, security
    ├── manufacturers/          # base.hpp/cpp + 8 brands (chevrolet, ford, toyota, bmw, ...)
    ├── protocols/              # obd2_standard, uds, gmlan, kwp2000, can_tp
    ├── database/               # db_manager (SQLite3), migrations, seed_data, schema.sql
    ├── diagnostics/            # dtc_manager, freeze_frame, readiness, vin_decoder, calibration
    ├── simulation/             # driving_cycle, sensor_simulator, fault_injector, environment
    ├── api/                    # rest_api, websocket, grpc_server
    ├── security/               # access_control, secure_comm, seed_key
    ├── logging/                # can_logger, metrics_exporter, replay_analyzer
    └── tests/                  # test_all_modes, test_manufacturers, fuzzing_suite (Catch2)
```

## Build Commands

```bash
make                    # Build with arm-linux-gnueabihf-g++ (default)
make CROSS_COMPILE=     # Build with native g++
make -j4                # Parallel build
make clean              # Remove build/
make info               # Show compiler, flags, source count
make test               # Build test binary (requires Catch2 for ARM)
```

## Scripts

```bash
scripts/install_deps.sh    # Install toolchain + dependencies + vcan
scripts/setup_can.sh       # Configure CAN interface (vcan/can0/status)
scripts/build.sh           # Build with flags: -n (native), -d (debug), -t (tests)
scripts/deploy.sh          # Deploy to remote Raspberry Pi via SSH
scripts/run_tests.sh       # Compile & run Catch2 tests
scripts/fuzz.sh            # CAN fuzzing (random OBD2/UDS frames)
scripts/diag.sh            # Interactive CAN diagnostic client
```

## Key Architecture Rules

- **All source is under `src/`**. Includes use relative paths (e.g., `"../core/can_manager.hpp"` from `manufacturers/`).
- **CAN frames**: `cf.frame.data` is `__u8 data[8]` (C array). Convert to vector:
  ```cpp
  std::vector<uint8_t> data(cf.frame.data, cf.frame.data + cf.frame.can_dlc);
  ```
- **Manufacturers** inherit from `BaseManufacturer` (src/manufacturers/base.hpp). Each implements `handleCanFrame()` and `getManufacturerId()`.
- **ProtocolRouter** routes CAN frames to the active manufacturer. Switch at runtime via API or `router->selectManufacturer()`.
- **OBD2 Modes 01-0A** are handled in `BaseManufacturer`. Manufacturer-specific modes go in each subclass.
- **UDS Services** (ISO 14229) are virtual methods on `BaseManufacturer` — override for brand-specific behavior.
- **SessionManager** tracks UDS diagnostic sessions with timeouts and security levels.
- **SecurityManager** holds seed/key algorithms per manufacturer. Register with `sec.registerAlgorithm("chevrolet", 1, SecurityManager::gmAlgorithm28bit())`.
- **DTCs** use 16-bit codes encoded as `P0101 → 0x0101`. The `DTCManager` helper handles encode/decode.
- **config.json** is loaded at startup. Changes require restart.

## Cross-Compilation Notes

- `nlohmann/json.hpp` is auto-downloaded to `build/include/` if not in sysroot.
- `sqlite3.h` / `libsqlite3` must be available in the cross-compiler sysroot, or link against a static build.
- Test with virtual CAN: `sudo modprobe vcan && sudo ip link add vcan0 type vcan && sudo ip link set vcan0 up`
- Real CAN: `sudo ip link set can0 type can bitrate 500000 && sudo ip link set can0 up`

## When to Use This Skill

- Adding a new manufacturer (create `src/manufacturers/newbrand.hpp/.cpp`, register in `main.cpp` and `protocol_router.cpp`)
- Adding/modifying OBD2 PIDs in `BaseManufacturer::setupDefaultPIDs()` or `manufacturers/base.cpp`
- Implementing seed/key algorithms in `core/security.cpp`
- Adding UDS DIDs in manufacturer `handleReadDataByIdentifier()`
- Modifying the simulation engine (driving cycles, sensor correlation)
- Extending the REST API endpoints in `api/rest_api.hpp`
- Changing the build system (Makefile targets, compiler flags, linker options)
- Debugging compilation errors specific to `arm-linux-gnueabihf-g++` cross-compiler
- Creating or modifying database schema in `database/schema.sql`
- Writing tests in `src/tests/test_all_modes.cpp`
- Generating OpenAPI docs in `docs/api.yaml`
