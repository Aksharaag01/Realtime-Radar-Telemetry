# Real-Time Aircraft Simulation & SAM Engagement System

> **Integration of OpenEaagles Simulation Framework and Zenoh-C Middleware**  
> *Developed as part of an internship project at the Institute for Systems Studies & Analyses (ISSA), Defence Research & Development Organisation (DRDO).*

---

##  Overview

This project examines whether a legacy, slot-configured C++ constructive-simulation framework, **OpenEaagles**, can be extended with a modern, low-latency data-distribution middleware, **Zenoh-C**, to build a decoupled, real-time-observable simulation pipeline, using a Surface-to-Air Missile (SAM) intercept scenario as an end-to-end proof of concept. 

OpenEaagles supplies the object model, slot table configuration mechanism, and time-critical simulation loop. Zenoh-C supplies the publish/subscribe transport layer that lets external processes—such as visualizers, loggers, and command-and-control nodes—observe and influence the simulation without being compiled into it.

---

## Architecture & Core Components

```text
+-----------------------------------------------------------------+
|                   OpenEaagles Simulation Loop                   |
|  +---------------------+           +--------------------------+ |
|  |    MyJetAircraft    |           |       MySamMissile       | |
|  |  (Flight Dynamics)  |           | (PN Guidance Algorithm)  | |
|  +----------+----------+           +------------+-------------+ |
+-------------|-----------------------------------|---------------+
              |                                   |
              v                                   v
+-----------------------------------------------------------------+
|                          ZenohBridge                            |
|        (Lock-free SPSC Queue & Thread-Safe Telemetry Publisher) |
+-----------------------------------------------------------------+
                                  |
                   Zenoh-C Pub/Sub Transport Layer
                                  |
              +-------------------+-------------------+
              |                                       |
              v                                       v
    +-------------------+                   +-------------------+
    | External Consumer |                   | Real-Time Logger  |
    | / Radar Visualizer|                   |    & Evaluator    |
    +-------------------+                   +-------------------+
'''
### Component Breakdown

| Component | Role / Description |
| :--- | :--- |
| **OpenEaagles Core** | Provides object-oriented simulation management, scheduling, and EDL parsing. |
| **JSBSim Engine** | Handles realistic physics for the aircraft, calculating 3D motion, turns, acceleration, and aerodynamic forces. |
| **MyJetAircraft** | Custom point-mass extended aircraft entity generating real-time 3D motion flight parameters. |
| **MySamMissile** | Custom SAM interceptor implementing Proportional Navigation (PN) guidance tracking. |
| **ZenohBridge** | Custom bridge component using a lock-free Single-Producer Single-Consumer (SPSC) queue to safely transfer data across threads. |
| **Zenoh-C Layer** | High-performance C-API publish-subscribe communication middleware for real-time telemetry streaming. |

---

##  Key Features

* **Decoupled Telemetry:** Real-time streaming of aircraft state parameters (`position`, `velocity`, `altitude`, `heading`, `acceleration`) at 25 Hz / 40 ms cycles.
* **Proportional Navigation (PN):** Real-time SAM guidance tracking that continuously updates missile heading based on Line-of-Sight (LOS) angle rate.
* **Thread Safety:** Lock-free SPSC queue prevents Zenoh callback threads from directly modifying OpenEaagles' non-thread-safe object tree.
* **Independent Evaluation Methodology:** Deterministic evaluator computes miss distance at closest approach directly from published entity positions rather than internal status flags.
* **Declarative Scenario Setup:** Configured using the Eaagles Description Language (EDL) without modifying application source code.

---

##  Tech Stack & Dependencies

| Category | Technology / Library | Purpose |
| :--- | :--- | :--- |
| **Languages** | C++ (C++11/C++14), C | Core simulation logic, custom entities, and Zenoh bindings. |
| **Simulation** | OpenEaagles | Constructive-simulation framework, slot-table configuration, simulation loop. |
| **Physics** | JSBSim | Flight dynamics modeling background and flight parameter generation. |
| **Networking** | Zenoh-C | C API binding onto the Zenoh (Rust) core for real-time Pub/Sub transport. |
| **Build Tools** | CMake, Premake5, MSVC 2017 / MinGW | Project build environment, script generation, and library linkage[cite: 1]. |
| **Configuration** | EDL (Eaagles Description Language) | Declarative scenario definition parsed by OpenEaagles slot-table macros[cite: 1]. |
| **Debugging & Visualization** | Graphviz, GDB, AddressSanitizer | Thread safety verification, runtime debugging, and architecture design[cite: 1]. |

---

##  Technical Challenges & Solutions

* **Library Linking Order Issues:** Fixed unresolved reference errors by ensuring OpenEaagles simulation libraries were specified in proper dependency order relative to base libraries during build[cite: 1].
* **Slot Table Mismatch:** Resolved compilation errors in `MySamMissile` by ensuring exact correspondence between `BEGIN_SLOTTABLE` entries and `BEGIN_SLOT_MAP` handlers[cite: 1].
* **Thread Synchronization Crashes:** Replaced direct Zenoh callback updates to simulation entities with a thread-safe SPSC queue mechanism to bridge the communication and simulation loops safely[cite: 1].

---

##  Getting Started

### 1. Prerequisites
Ensure the following tools and compilers are available on your system:
* **CMake** (v3.15+)[cite: 1]
* **Microsoft Visual Studio 2017** (or GCC via **MinGW**)[cite: 1]
* **Rust Toolchain** (required to compile `zenoh-c` from source)[cite: 1]

### 2. Building Zenoh-C
```bash
git clone [https://github.com/eclipse-zenoh/zenoh-c.git](https://github.com/eclipse-zenoh/zenoh-c.git)
cd zenoh-c
mkdir build && cd build
cmake ..
cmake --build . --config Release
