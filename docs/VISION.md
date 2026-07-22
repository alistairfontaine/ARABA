# The Tao of ARABA

## The Problem
The modern world is fragile. When a disaster strikes—earthquake, war, or grid failure—the digital lifeline snaps. People are left isolated, unable to call for help, share medical data, or coordinate rescue. Existing solutions are either too expensive (Starlink), too complex (HAM radio requires licensing), or non-existent for the average person.

## The Philosophy
**"Data must flow like water."**
Water finds the lowest path. It does not wait for permission. It does not care about borders. ARABA is the digital water. It flows through every available path, finding its way to the ocean (the internet) or to the nearest node that can help.

## The Promise
1.  **No Central Authority**: No server to hack, no company to shut it down.
2.  **No Cost**: Free software, running on hardware you already own.
3.  **Survival**: Built for the worst-case scenario. If the power grid fails, ARABA runs on a laptop battery for hours, passing messages to the next person.

## The Goal
To make ARABA the standard for disaster relief and civil liberty, ensuring that **no human is ever truly offline again**.

# ARABA Vision

## What

ARABA is an offline-first, encrypted mesh network protocol. It allows devices to communicate directly over Wi-Fi without internet, cell towers, or any central infrastructure.

## Why

The modern internet is fragile. Cables get cut. Towers lose power. Governments flip switches. When that happens, people need a way to talk that doesn't depend on anything except the radios already in their pockets.

ARABA turns your laptop, phone, or embedded device into a relay node. Messages hop from device to device until they reach their destination. If the destination is offline, messages wait on disk until it returns.

## How

- Raw Wi-Fi frames (AF_PACKET sockets on Linux)
- Custom binary protocol with AES-256 encryption
- AODV-lite routing with automatic route discovery
- Store-and-forward persistence (.rue format)
- Zero external dependencies

## Who

Built by one person on an HP 250 G7 in a room with too many Wi-Fi neighbors. Named after someone who left. The .rue files are for her.

## When

Now. Before you need it.
