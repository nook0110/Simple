# SimpleChessEngine

| **CI Service** | Build Status |
|:---------------|-------------:|
| GitHub Actions | [![Build Status (GitHub Actions)](https://github.com/nook0110/Simple/workflows/CMake%20Tests/badge.svg)](https://github.com/nook0110/Simple/actions?query=workflow%3A"CMake%20Tests") |

## Syzygy tablebases

Set the UCI option `SyzygyPath` to a directory containing matching `.rtbw`
and `.rtbz` files. WDL probes are used during search and DTZ is used at the
root. The engine uses the MIT-licensed Fathom probe library pinned in CMake;
its license is retained in `ThirdParty/Fathom-LICENSE`.
