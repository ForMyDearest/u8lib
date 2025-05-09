# Introduction

[![License](https://img.shields.io/github/license/ForMyDearest/u8lib?label=license&style=flat-square)](./LICENSE)

Unicode library for C++

# Components

| Module     |                    Description                    | Reference                                                                                         |
|------------|:-------------------------------------------------:|---------------------------------------------------------------------------------------------------|
| `u8string` | A string compatible with all base character types | [OpenString](https://github.com/1762757171/OpenString) (MIT)                                      |
| `format`   |           Modern formatter with char8_t           | [fmt](https://github.com/fmtlib/fmt) (MIT),  [STL](https://github.com/microsoft/STL) (Apache-2.0) |
| `log`      |            High-performance log system            | [fmtlog](https://github.com/MengRao/fmtlog) (MIT)                                                 |
| `json`     |           JsonSerde for basic data type           | [SakuraEngine](https://github.com/SakuraEngine/SakuraEngine) (MIT)                                |
| `guid`     |      Cross-platform implementation for GUID       | [stduuid](https://github.com/mariusbancila/stduuid) (MIT)                                         |

# Dependencies

- [mimalloc](https://github.com/microsoft/mimalloc) (MIT)
- [yyjson](https://github.com/ibireme/yyjson) (MIT)

# Performance

U8Log just pushes static info along with formatted msg body onto the queue. which causes smaller program size but higher front-end latency.
In my test, it has a front-end latency of approximately 75ns, while static fmtlog has a front-end latency of approximately 10ns

# TODO
* [ ] Fix the bug that log pattern can't support "{{}}"
* [ ] Support std::ranges operations for u8string and u8string_view