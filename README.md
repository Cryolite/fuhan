# FuHan

<!-- Badges -->
<!-- TODO: Replace these placeholders with real badge URLs once CI, packaging, and release infrastructure are in place. -->

![Build status](https://img.shields.io/badge/build-TODO-lightgrey)
![Tests](https://img.shields.io/badge/tests-TODO-lightgrey)
![License](https://img.shields.io/badge/license-MIT-blue)
![C++ standard](https://img.shields.io/badge/C%2B%2B-23-blue)
![Version](https://img.shields.io/badge/version-TODO-lightgrey)

A header-only [C++23][cpp23] library that computes the **fu (符)**,
**han (翻)**, and **[yakuman multiplier](#yakuman-multiplier)** of a
winning hand in Riichi Mahjong, the Japanese variant of mahjong,
through a single public entry-point function: `FuHan::calculateFuHan`.

---

## Table of Contents

- [Overview](#overview)
- [Motivation](#motivation)
- [Features](#features)
- [Non-goals](#non-goals)
- [Supported Rule Variants](#supported-rule-variants)
- [Requirements](#requirements)
- [Installation](#installation)
- [Building from Source](#building-from-source)
- [Running Tests](#running-tests)
- [Quick Start](#quick-start)
- [API Overview](#api-overview)
- [Detailed API Reference](#detailed-api-reference)
- [Tile Encoding](#tile-encoding)
- [Yakuman Multiplier](#yakuman-multiplier)
- [Error Handling](#error-handling)
- [Thread Safety](#thread-safety)
- [Determinism and Reproducibility](#determinism-and-reproducibility)
- [Performance Notes](#performance-notes)
- [Examples](#examples)
- [Integration Guide](#integration-guide)
- [CMake Usage Example](#cmake-usage-example)
- [Notes for Library Consumers](#notes-for-library-consumers)
- [Notes for Contributors](#notes-for-contributors)
- [Versioning Policy](#versioning-policy)
- [Compatibility Policy](#compatibility-policy)
- [Limitations](#limitations)
- [Roadmap](#roadmap)
- [FAQ](#faq)
- [References](#references)
- [License](#license)
- [Acknowledgements](#acknowledgements)

---

## Overview

`FuHan` is a small, focused library whose entire public surface is a
single function:

```cpp
FuHan::Result FuHan::calculateFuHan(
    FuHan::Wind round_wind,
    FuHan::Wind seat_wind,
    std::array<std::uint_fast8_t, 34u> const &concealed_hand,
    std::array<std::uint_fast8_t, 21u> const &chii_list,
    std::array<std::uint_fast8_t, 34u> const &pon_list,
    std::array<std::uint_fast8_t, 34u> const &open_kan_list,
    std::array<std::uint_fast8_t, 34u> const &ankan_list,
    std::uint_fast8_t winning_tile,
    std::uint_fast8_t num_dora,
    FuHan::Context context,
    FuHan::Rule rule);
```

Given a fully described winning hand and its situational context, the
function evaluates the hand under every supported winning shape
(standard four-melds-plus-one-pair, Chiitoitsu, and Kokushi Musou) and
returns the highest-scoring interpretation as a `FuHan::Result`
containing `fu`, `han`, and `yakuman_multiplier`.

## Motivation

Determining the score of a Riichi Mahjong hand is deceptively hard:
a single hand may be decomposed into melds in several ways, each
producing different fu and han, and many yaku interact subtly with one
another and with the situational context (riichi, tsumo, haitei, etc.).

`FuHan` exists to encapsulate that complexity behind a single
well-defined function so that callers (game engines, AI players,
analytics tools, hand-replay viewers, training corpora, etc.) can score
hands without re-implementing the rules from scratch.

## Features

- **Single public entry point.** One function, three small custom
  types. No façade classes, no builders, no factories.
- **Header-only.** Just add the include path — or link the bundled
  `fuhan` [CMake][cmake] `INTERFACE` target; there is no compiled
  library to build.
- **C++23.** Uses modern features.
- **Multiple winning shapes.** Standard, Chiitoitsu (seven pairs), and
  Kokushi Musou (thirteen orphans) are all considered, and the
  highest-scoring interpretation is returned automatically.
- **Configurable rule variants, with ready-made presets.** Fu and han
  are computed under a caller-selected `FuHan::Rule` configuration that
  covers the scoring options which differ between rule sets (open
  tanyao, double yakuman, and double-wind pair fu). The `FuHan::Rules`
  namespace ships presets matching the rule sets of several major
  leagues, competitive organizations, and online services (e.g.
  [Tenhou][tenhou], [Mahjong Soul][mahjong-soul],
  [M.League][m-league]). See
  [Supported Rule Variants](#supported-rule-variants).
- **Strict input validation.** Structural invariants are checked on
  every call and violations are reported with descriptive
  `std::invalid_argument` messages.
- **Deterministic and pure.** The function depends only on its
  arguments, performs no I/O, and uses no global state.
- **Extensively validated against real-world data.** The library has
  been verified to produce correct fu, han, and yakuman multiplier on
  all **126,492,264** winning hands collected from actual
  [Mahjong Soul][mahjong-soul] (雀魂) game records.

## Non-goals

- The library does **not** compute final scores in points (it does not
  apply the mangan/haneman/baiman/etc. caps, nor distribute the score
  among players). It returns fu, han, and [yakuman multiplier](#yakuman-multiplier); mapping
  these to a points payout is the caller's responsibility.
- The library does **not** identify which specific yaku are present.
  Only the aggregate fu, han, and [yakuman multiplier](#yakuman-multiplier) are reported.
- The library does **not** decide whether a hand is in tenpai. It is,
  however, designed to accept arbitrary tile inputs that satisfy the
  structural invariants listed under
  [Preconditions / Invariants enforced](#preconditions--invariants-enforced):
  when the input does
  not form a winning shape (or forms one but establishes no yaku),
  this is reported through the returned `FuHan::Result` (see
  [Return value](#return-value)) rather than as an error. Callers can
  therefore use `FuHan::calculateFuHan` to *test* whether a given hand
  is a winning hand, in addition to scoring known winning hands.
- The library does **not** detect or interpret aka (red) dora from
  tile data. The caller must pass the total dora count (regular + aka + ura, when applicable) via `num_dora`.
- The library models only the three rule variations described under
  [Supported Rule Variants](#supported-rule-variants); other rule
  variants (local yaku, optional house rules, etc.) are out of scope
  and are enumerated under
  [Rules not supported](#rules-not-supported).

## Supported Rule Variants

Riichi Mahjong is played under many slightly different rule sets, and
several scoring decisions differ from one league, competitive
organization, or online service to another. `FuHan::calculateFuHan`
takes a [`FuHan::Rule`](#fuhanrule) argument that selects, independently,
each of the following three scoring options. Every option is a binary
choice that **must be specified explicitly** — there is no implicit
default, and an under-specified or contradictory configuration is
rejected (see [`FuHan::Rule`](#fuhanrule) and
[Error Handling](#error-handling)).

| Rule option | Alternatives | `FuHan::Rule` flags |
|-------------|--------------|---------------------|
| Open tanyao (kuitan, 喰いタン)            | awarded for an open hand / not awarded   | `kuitan_enabled` / `kuitan_disabled`               |
| Double yakuman (ダブル役満)              | recognised / every yakuman counts once   | `double_yakuman_enabled` / `double_yakuman_disabled` |
| Double-wind pair fu (連風牌の雀頭)        | 4 fu / 2 fu                              | `double_wind_pair_4fu` / `double_wind_pair_2fu`    |

The double-yakuman option governs the yaku that are conventionally
worth two yakuman: Suuankou tanki (四暗刻単騎), Daisuushii (大四喜),
Junsei Chuuren Poutou (純正九蓮宝燈), and Kokushi Musou juusan menmachi
(国士無双十三面待ち). Chiitoitsu (seven pairs) is unaffected by all three
options above: its fu is fixed at 25, it is always concealed, and none
of the three options can apply to it.

The fu for a rinshan kaihou self-draw (嶺上開花のツモ符) is **not**
configurable: such a win is always treated as an ordinary self-draw and
receives the standard 2 fu tsumo bonus, matching the rule sets of
essentially all major organizations, leagues, and services.

### Rule presets

For convenience, the [`FuHan::Rules`](#fuhanrules) namespace provides
ready-made `FuHan::Rule` configurations matching the rule sets adopted
by several major mahjong organizations, leagues, and online services.
Each preset is a complete, well-formed value and can be passed directly
as the `rule` argument:

| Preset                       | Rule set            |
|------------------------------|---------------------|
| `FuHan::Rules::tenhou`       | [Tenhou][tenhou] (天鳳)              |
| `FuHan::Rules::mahjong_soul` | [Mahjong Soul][mahjong-soul] (雀魂)  |
| `FuHan::Rules::m_league`     | [M.League][m-league] (Mリーグ)       |

See [`FuHan::Rules`](#fuhanrules) for the exact per-option values of
each preset.

### Rules not supported

The three options above are the **only** rule variations that affect the
fu and han computed by this library. The following rule variations,
several of which differ between real-world rule sets, are **not**
modeled and are out of scope:

- **Points / score rules.** Mapping fu and han to a points payout, the
  mangan / haneman / baiman / sanbaiman caps, **kiriage mangan**
  (切り上げ満貫), and any honba (本場) or riichi-stick accounting are
  the caller's responsibility (see [Non-goals](#non-goals)).
- **Counted yakuman (kazoe yakuman, 数え役満) policy.** The library
  reports a hand of `han >= 13` with `yakuman_multiplier == 0`; whether
  to treat that as a yakuman, and any cap on the number of stacked
  yakuman, is the caller's decision (see
  [Yakuman Multiplier](#yakuman-multiplier)).
- **Local / optional yaku (ローカル役).** For example Renhou (人和),
  Nagashi mangan (流し満貫), Daisharin (大車輪), Sanrenkou (三連刻),
  Suurenkou (四連刻), and Open riichi (オープン立直) are not
  recognised; correspondingly, there is no `FuHan::Context` flag for
  such situations.
- **Three-player mahjong (三人麻雀) rules.** Kita nukidora (北抜きドラ)
  and other sanma-specific scoring are not modeled.
- **Rinshan kaihou tsumo fu (嶺上開花のツモ符) variant.** A rinshan
  kaihou self-draw is always scored with the standard 2 fu tsumo bonus;
  the minority rule that awards 0 fu in this case is not supported.
- **Aka (red) dora detection.** The caller must count all dora
  (regular + aka + ura) and pass the total via `num_dora`; the library
  does not infer aka dora from tile data (see [Non-goals](#non-goals)).

## Requirements

- A [C++23][cpp23] compiler.
- [CMake][cmake] **3.25** or newer (for the bundled build and the header
  `install` rule).
- To build and run the bundled tests: a [Python 3][python3]
  interpreter, Python 3 development files, and the matching
  [Boost.Python][boost-python] library.
- To run `test/chiniisou`: the Python 3 interpreter selected by CMake
  must be able to import the [`mahjong`][python-mahjong] package.
- To run the scoring-corpus test driver: [POSIX shell][posix-shell],
  [`bzip2`][bzip2], [`gzip`][gzip], and [`xz`][xz].
- TODO: Confirm minimum supported versions of [GCC][gcc],
  [Clang][clang], and [MSVC][msvc].

## Installation

`FuHan` is header-only, so you can consume it in either of two ways.

**1. Add the headers to your include path directly.** Make the
repository root (or any copy of the `fuhan/` directory's parent)
available on your include path and include the top-level header:

```cpp
#include <fuhan/fuhan.hpp>
```

**2. Install the headers via [CMake][cmake].** The bundled CMake project
defines an `install` target — an `INTERFACE` library named `fuhan`
whose public headers are attached as a header `FILE_SET` — that
copies those headers into the include directory of your chosen
install prefix:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --install build --prefix /your/install/prefix
```

This installs the headers under `<prefix>/include/fuhan/` (so the
top-level header remains `#include <fuhan/fuhan.hpp>` once
`<prefix>/include` is on your include path). See
[CMake Usage Example](#cmake-usage-example) for how to consume the
`fuhan` target directly from another CMake project.

- TODO: Ship an exported package config (`FuHanConfig.cmake` via
  `install(EXPORT ...)`) so that installed copies can be located
  with `find_package(FuHan)`.

## Building from Source

Although the library itself is header-only, the repository ships a
[CMake][cmake] project for building the header target, installing the
headers, and, when explicitly enabled, building and running the test
suite.

```bash
git clone https://github.com/Cryolite/fuhan.git
cd fuhan
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Useful [CMake][cmake] options (see [`CMakeLists.txt`](CMakeLists.txt)):

| Option                | Default     | Effect                                              |
|-----------------------|-------------|-----------------------------------------------------|
| `CMAKE_BUILD_TYPE`    | `Release`   | Standard CMake build type.                          |
| `FUHAN_BUILD_TESTING` | Off         | Builds and registers the bundled tests.             |
| `FUHAN_WITH_COVERAGE` | Off         | Enables `-coverage` flags (GCC/Clang).              |
| `FUHAN_WITH_ASAN`     | On in Debug | Enables [AddressSanitizer][asan] (GCC/Clang).       |
| `FUHAN_WITH_UBSAN`    | On in Debug | Enables [UndefinedBehaviorSanitizer][ubsan] (GCC/Clang). |
| `FUHAN_WITH_TSAN`     | Off         | Enables [ThreadSanitizer][tsan] (GCC/Clang).        |

In `Release` builds, interprocedural optimization is enabled. In
`Debug` builds on GCC/Clang, `_GLIBCXX_DEBUG` and
`_GLIBCXX_DEBUG_PEDANTIC` are defined and ASan/UBSan are turned on by
default.

## Running Tests

Configure with `FUHAN_BUILD_TESTING=ON`, build the test targets, and
then run all registered tests with [CTest][ctest]:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFUHAN_BUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The test suite is split into the following test directories.

### `test/chiniisou`

The `test/chiniisou/` directory builds the `test-chiniisou`
executable and registers it with CTest as `test-chiniisou`.

This test enumerates closed single-suit hands and compares FuHan
results with the Python [`mahjong`][python-mahjong] hand calculator.
CTest runs it with the Python 3 interpreter discovered by
[CMake][cmake]. That interpreter environment must have the
`mahjong` package installed. It does not
require any external corpus files beyond the normal build and runtime
dependencies.

### `test/majsoul-scoring-corpus`

The `test/majsoul-scoring-corpus/` directory builds the
`majsoul-scoring-corpus-test-runner` executable and registers it with
CTest as `test-majsoul-scoring-corpus`.

The repository does **not** itself ship any Mahjong Soul-derived
scoring corpus. The test driver
[`test/majsoul-scoring-corpus/test-runner.sh`](test/majsoul-scoring-corpus/test-runner.sh)
instead assumes that one or more corpus files collected from
[Mahjong Soul][mahjong-soul] (雀魂) have been placed under the
`test/majsoul-scoring-corpus/` directory,
matching any of the following name patterns:

- `test/majsoul-scoring-corpus/*.tsv`
- `test/majsoul-scoring-corpus/*.tsv.gz`
- `test/majsoul-scoring-corpus/*.tsv.bz2`
- `test/majsoul-scoring-corpus/*.tsv.xz`

For each matching file, the driver streams its records (decompressing
on the fly when necessary) into the `majsoul-scoring-corpus-test-runner`
binary. If no such files are present, the corpus-based tests trivially
pass with no records exercised.

- TODO: Document how to obtain, prepare, and place Mahjong Soul
  scoring-corpus files under `test/majsoul-scoring-corpus/`, including the expected TSV
  schema and any preprocessing steps.
- TODO: Document how to interpret test-runner output and failures.

## Quick Start

The smallest possible call to the entry-point function. The hand below
is intentionally minimal: 1m–2m–3m–4m–5m–6m–7m–8m–9m + 1p–1p–1p + 2p–2p,
won by tsumo on 2p.

```cpp
#include <fuhan/fuhan.hpp>
#include <array>
#include <iostream>

int main()
{
    using namespace FuHan;

    std::array<std::uint_fast8_t, 34u> concealed_hand{};
    for (std::uint_fast8_t i = 0; i < 9; ++i) {
        // 1m..9m
        concealed_hand[i] = 1;
    }
    // 1p x3
    concealed_hand[9] = 3;
    // 2p x1 (the second 2p is the winning tile, supplied separately)
    concealed_hand[10] = 1;

    std::array<std::uint_fast8_t, 21u> chii_list{};
    std::array<std::uint_fast8_t, 34u> pon_list{};
    std::array<std::uint_fast8_t, 34u> open_kan_list{};
    std::array<std::uint_fast8_t, 34u> ankan_list{};

    Result const r = calculateFuHan(
        east, east,
        concealed_hand,
        chii_list, pon_list, open_kan_list, ankan_list,
        /*winning_tile=*/10u, // 2p
        /*num_dora=*/0u,
        tsumo,
        Rules::tenhou);

    std::cout << "fu="  << unsigned(r.fu)
              << " han=" << unsigned(r.han)
              << " yakuman_multiplier=" << unsigned(r.yakuman_multiplier)
              << '\n';
}
```

## API Overview

| Symbol                          | Kind        | Header                  | Purpose                                                                 |
|---------------------------------|-------------|-------------------------|-------------------------------------------------------------------------|
| `FuHan::calculateFuHan`         | function    | `fuhan/fuhan.hpp`       | **The** entry point. Returns the best-scoring `Result`.                 |
| `FuHan::Wind`                   | enum struct | `fuhan/types.hpp`       | The four wind values (`east`, `south`, `west`, `north`).                |
| `FuHan::Context`                | enum struct | `fuhan/types.hpp`       | Bitmask of situational flags (tsumo/ron/riichi/ippatsu/...).            |
| `FuHan::Rule`                   | enum struct | `fuhan/types.hpp`       | Bitmask selecting optional scoring rules (kuitan/double yakuman/...).   |
| `FuHan::Rules`                  | namespace   | `fuhan/types.hpp`       | Preset `Rule` configurations (`tenhou`, `mahjong_soul`, `m_league`).    |
| `FuHan::Result`                 | struct      | `fuhan/types.hpp`       | Output: `fu`, `han`, `yakuman_multiplier`.                              |

Parameter summary for `calculateFuHan`:

| Parameter        | Type                                            | Meaning                                                                 |
|------------------|-------------------------------------------------|-------------------------------------------------------------------------|
| `round_wind`     | `FuHan::Wind`                                   | Prevailing round wind.                                                  |
| `seat_wind`      | `FuHan::Wind`                                   | Seat wind of the winning player.                                        |
| `concealed_hand` | `std::array<std::uint_fast8_t, 34>`             | Per-tile counts of the concealed hand, excluding the winning tile.      |
| `chii_list`      | `std::array<std::uint_fast8_t, 21>`             | Per-base-tile counts of called chii melds.                              |
| `pon_list`       | `std::array<std::uint_fast8_t, 34>`             | Per-tile counts of pon melds (0 or 1 per tile).                         |
| `open_kan_list`  | `std::array<std::uint_fast8_t, 34>`             | Per-tile counts of open kans (daiminkan or kakan).                      |
| `ankan_list`     | `std::array<std::uint_fast8_t, 34>`             | Per-tile counts of concealed kans (ankan).                              |
| `winning_tile`   | `std::uint_fast8_t`                             | Internal tile index of the winning tile, in `[0, 34)`.                  |
| `num_dora`       | `std::uint_fast8_t`                             | Total dora (regular + aka + ura). Counted only if a yaku is established.|
| `context`        | `FuHan::Context`                                | Situational flag bitmask. Exactly one of `tsumo`/`ron` required.        |
| `rule`           | `FuHan::Rule`                                   | Optional-rule bitmask. Exactly one alternative per option required.     |

## Detailed API Reference

### `FuHan::calculateFuHan`

```cpp
inline FuHan::Result calculateFuHan(
    FuHan::Wind round_wind,
    FuHan::Wind seat_wind,
    std::array<std::uint_fast8_t, 34u> const &concealed_hand,
    std::array<std::uint_fast8_t, 21u> const &chii_list,
    std::array<std::uint_fast8_t, 34u> const &pon_list,
    std::array<std::uint_fast8_t, 34u> const &open_kan_list,
    std::array<std::uint_fast8_t, 34u> const &ankan_list,
    std::uint_fast8_t winning_tile,
    std::uint_fast8_t num_dora,
    FuHan::Context context,
    FuHan::Rule rule);
```

The single public entry point. The function:

1. Validates its inputs against the structural invariants listed
   under [Preconditions / Invariants enforced](#preconditions--invariants-enforced).
   Any violation throws `std::invalid_argument`.
2. Evaluates the hand under the three supported winning shapes:
   - **Standard** ("four melds plus one pair").
   - **Chiitoitsu** (seven pairs).
   - **Kokushi Musou** (thirteen orphans).
3. Returns the largest `Result` under the strict weak ordering defined
   by `operator<(Result, Result)` in [`fuhan/types.hpp`](fuhan/types.hpp).
   That ordering is used to choose the preferred scoring interpretation:
   [yakuman multiplier](#yakuman-multiplier) first, then `han`, then
   `roundUp10(fu) * 2^han` when `han` is tied, then `fu`. It is not a
   final point-payment comparison.

#### Parameters

- **`round_wind`** — The prevailing round wind. Used by the
  standard-shape evaluator to identify yakuhai.
- **`seat_wind`** — The seat wind of the winning player. Used by the
  standard-shape evaluator to identify yakuhai.
- **`concealed_hand`** — Counts of tiles in the concealed portion of
  the hand, indexed by the library's [internal tile index](#tile-encoding).
  **Excludes** any tile that is part of a called meld and **excludes**
  the winning tile itself (which is passed separately via
  `winning_tile`, regardless of tsumo vs. ron).
- **`chii_list`** — For each chii (sequence) meld shape, the number
  of called chii melds of that shape. Indexed in `[0, 21)` by the
  shape's **base** (smallest) tile, following the exact mapping
  defined in [Chii Index Encoding](#chii-index-encoding) (e.g.
  `chii_list[0]` is the count of 1m–2m–3m chii, `chii_list[7]` is the
  count of 1p–2p–3p chii, and so on).
- **`pon_list`** — For each tile in `[0, 34)`, the count of pon melds
  of that tile. Each entry is `0` or `1`.
- **`open_kan_list`** — For each tile in `[0, 34)`, the count of open
  kans (daiminkan or kakan). Each entry is `0` or `1`. The library
  does not distinguish daiminkan from kakan in this input.
- **`ankan_list`** — For each tile in `[0, 34)`, the count of
  concealed kans (ankan). Each entry is `0` or `1`.
- **`winning_tile`** — Internal tile index of the winning tile, in
  `[0, 34)`.
- **`num_dora`** — Total number of dora the hand counts, including
  regular dora, aka dora, and ura dora when applicable. Reflected
  in `Result::han` only in **case 3** of the
  [return-value case analysis](#return-value) below — i.e. when at
  least one non-yakuman yaku is established (possibly producing a
  counted yakuman). In **case 2** (winning shape but no yaku),
  `num_dora` is *not* reflected in `Result::han`, because dora alone
  does not constitute a yaku and such a hand is not a winning hand
  for scoring purposes.
- **`context`** — Bitmask of `FuHan::Context` flags describing the
  situational context. Multiple flags can be combined with
  `operator|` (e.g. `riichi | tsumo | ippatsu`). Exactly one of
  `tsumo` or `ron` must be set, and no pair of mutually exclusive
  flags may be combined; see [Error Handling](#error-handling).
- **`rule`** — Bitmask of `FuHan::Rule` flags selecting the optional
  scoring rules in effect (open tanyao, double yakuman, and
  double-wind pair fu). Each of the three options must select exactly
  one of its two mutually exclusive alternatives; an under-specified or
  contradictory configuration throws (see
  [Error Handling](#error-handling)). Preset configurations are
  provided in the `FuHan::Rules` namespace (e.g. `FuHan::Rules::tenhou`,
  `FuHan::Rules::mahjong_soul`, `FuHan::Rules::m_league`).

#### Return value

A `FuHan::Result` representing the **best-scoring** interpretation
across the standard, Chiitoitsu, and Kokushi Musou shapes. The
returned value falls into exactly one of the following five mutually
exclusive and exhaustive cases (the same case analysis used by
[`FuHan::Result`](#fuhanresult)):

1. **The input does not form a winning shape under any
   interpretation.** `fu`, `han`, and `yakuman_multiplier` are all
   `0`. This indicates that the input is **not a winning hand for
   scoring purposes**.
2. **The input forms a winning shape under some interpretation, but
   no yaku (excluding dora) is established under any
   interpretation.** `fu` is the non-zero fu value computed from the
   best-scoring winning shape; `han` and `yakuman_multiplier` are
   both `0`. Dora are not reflected in `han` here, because dora alone
   does not constitute a yaku.
3. **At least one non-yakuman yaku is established (which may,
   together with dora, escalate to a counted yakuman).** `fu` and
   `han` are the non-zero values of the best-scoring interpretation
   (with dora included in `han`), and `yakuman_multiplier` is `0`.
   A counted yakuman (kazoe yakuman) manifests as `han >= 13` with
   `yakuman_multiplier == 0`.
4. **Kokushi Musou (thirteen orphans) is established.** `fu` and
   `han` are both `0` (fu is not defined for this hand and yakuman
   wins do not use `han`), and `yakuman_multiplier` is the
   appropriate non-zero value for the recognised form of Kokushi
   Musou.
5. **A non-counted yakuman other than Kokushi Musou is
   established.** `fu` is the non-zero fu value computed from the
   winning shape, `han` is `0`, and `yakuman_multiplier` is the
   appropriate non-zero sum of per-yaku
   [yakuman multiplier](#yakuman-multiplier) contributions (each
   ordinary yakuman contributes `1` and each double yakuman
   contributes `2`).

See [`FuHan::Result`](#fuhanresult) for the field-by-field
restatement of the same cases.

#### Preconditions / Invariants enforced

The following invariants are enforced before scoring starts. A
violation throws `std::invalid_argument`; the message identifies the
offending quantity.

- `round_wind` and `seat_wind` are valid `Wind` enumerators
  (one of `east`, `south`, `west`, `north`).
- Each entry of `concealed_hand` is at most `4`, and the total
  concealed-tile count plus the winning tile is at most `14`.
- Each `chii_list` entry is at most `4`, and the total chii count is
  at most `4`.
- Each entry of `pon_list`, `open_kan_list`, and `ankan_list` is at
  most `1`, and each list's total count is at most `4`.
- The total number of called melds (chii + pon + open kan + ankan) is
  at most `4`.
- The structural tile count — taking each kan as contributing `3`
  tiles (so every meld counts as `3`) plus the winning tile — equals
  exactly `14` (= `4 * 3 + 2`).
- `winning_tile < 34`.
- For every tile kind, the total physical tile count across
  `concealed_hand`, `chii_list`, `pon_list`, `open_kan_list`,
  `ankan_list`, and `winning_tile` is at most `4`. For this check,
  each chii contributes one copy of each tile in its sequence, each
  pon contributes three copies, each kan contributes four copies, and
  the winning tile contributes one copy.
- `context` contains only defined `FuHan::Context` flags; no bit
  outside the supported flag mask is set.
- `context` is internally consistent and consistent with `seat_wind`
  and the open/closed status of the hand:
  - exactly one of `tsumo` and `ron` is set;
  - `ippatsu` is set only when either `riichi` or `double_riichi` is
    also set;
  - `tenhou` is set only for east seat wind, and `chiihou` is set only
    for non-east seat wind;
  - an open hand (one with at least one chii, pon, or open kan; ankan
    does not open the hand) does not combine with `riichi`,
    `double_riichi`, `ippatsu`, `tenhou`, or `chiihou`;
  - no pair of mutually exclusive flags is set. The mutually exclusive
    combinations rejected are:
    - `tsumo` with `chankan` or `houtei_raoyui`;
    - `ron` with `rinshan_kaihou`, `haitei_raoyue`, `tenhou`, or `chiihou`;
    - `riichi` with `double_riichi`, `tenhou`, or `chiihou`;
    - `chankan` with `rinshan_kaihou`, `haitei_raoyue`, `houtei_raoyui`,
      `tenhou`, or `chiihou`;
    - `rinshan_kaihou` with `haitei_raoyue`, `houtei_raoyui`, `ippatsu`,
      `tenhou`, or `chiihou`;
    - `haitei_raoyue` with `houtei_raoyui`, `tenhou`, or `chiihou`;
    - `houtei_raoyui` with `ippatsu`, `tenhou`, or `chiihou`;
    - `double_riichi` with `tenhou` or `chiihou`;
    - `ippatsu` with `tenhou` or `chiihou`;
    - `tenhou` with `chiihou`.
- `rule` contains only defined `FuHan::Rule` flags and is a
  well-formed configuration: for each of the three options (open
  tanyao, double yakuman, and double-wind pair fu) exactly one of its
  two mutually exclusive alternatives is set. An under-specified
  configuration (an option with neither alternative set) or a
  contradictory one (an option with both set) is rejected.

#### Postconditions

- The input arrays are not modified (all pass-by-`const`-reference).
- No global state is read or written.
- The returned `Result` satisfies the semantics described above.

#### Ownership and lifetime

All arguments are taken either by value (small trivially-copyable
types) or by `const` reference (the tile-count arrays). The function
returns a `Result` by value. The library does **not** retain pointers
or references to the caller's data beyond the duration of the call,
so there are no ownership or lifetime concerns for the caller.

### `FuHan::Wind`

Enum struct with underlying type `std::uint_fast8_t`. Enumerators:

| Name     | Underlying value | Meaning                       |
|----------|------------------|-------------------------------|
| `east_`  | `27`             | East wind  (tile index `27`)  |
| `south_` | `28`             | South wind (tile index `28`)  |
| `west_`  | `29`             | West wind  (tile index `29`)  |
| `north_` | `30`             | North wind (tile index `30`)  |

Convenience constants `FuHan::east`, `FuHan::south`, `FuHan::west`,
`FuHan::north` are provided. The underlying values intentionally
match the library's internal tile encoding, so a `Wind` value can be
interpreted directly as a wind tile.

### `FuHan::Context`

Enum struct with underlying type `std::uint_fast16_t`. Each
enumerator is a distinct bit, so values can be combined with the
overloaded bitwise operators `|`, `&`, `|=`, and `&=`.

| Flag              | Meaning                                                             |
|-------------------|---------------------------------------------------------------------|
| `tsumo`           | Winning tile was self-drawn.                                        |
| `ron`             | Winning tile was claimed from a discard.                            |
| `riichi`          | Riichi was declared.                                                |
| `chankan`         | Win was achieved by robbing a kan.                                  |
| `rinshan_kaihou`  | Winning tile was drawn from the dead wall after a kan.              |
| `haitei_raoyue`   | Winning tile was the last tile drawn from the live wall.            |
| `houtei_raoyui`   | Winning tile was the final discard of the round.                    |
| `double_riichi`   | Riichi declared on the very first uninterrupted draw.               |
| `ippatsu`         | Win occurred within one go-around after declaring riichi.           |
| `tenhou`          | Dealer won on the initial hand.                                     |
| `chiihou`         | Non-dealer won on the first uninterrupted draw.                     |

### `FuHan::Rules`

A namespace of predefined `FuHan::Rule` configurations matching the
rule sets adopted by several major mahjong organizations, leagues, and
services. Each constant is a complete, **well-formed** `Rule` value: it
selects exactly one alternative for every one of the three options that
`FuHan::Rule` models, so it can be passed directly as the `rule`
argument of [`FuHan::calculateFuHan`](#fuhancalculatefuhan) without any
further combination.

The constants live in their own `FuHan::Rules` namespace both to group
them and to avoid a name clash with `FuHan::tenhou`, the
[`FuHan::Context`](#fuhancontext) flag for the tenhou (天和) yaku, which
is unrelated to the [Tenhou][tenhou] (天鳳) online service. The service
is therefore `FuHan::Rules::tenhou`, whereas the yaku flag remains
`FuHan::tenhou`.

| Constant                   | Models                | Open tanyao (kuitan) | Double yakuman   | Double-wind pair (連風牌) |
|----------------------------|-----------------------|----------------------|------------------|---------------------------|
| `FuHan::Rules::tenhou`       | [Tenhou][tenhou] (天鳳)              | enabled              | not recognised   | 4 fu                      |
| `FuHan::Rules::mahjong_soul` | [Mahjong Soul][mahjong-soul] (雀魂)  | enabled              | recognised       | 4 fu                      |
| `FuHan::Rules::m_league`     | [M.League][m-league] (Mリーグ)       | enabled              | not recognised   | 2 fu                      |

Each constant is defined as the bitwise-OR of one alternative from each
of the three `FuHan::Rule` option pairs, for example:

```cpp
inline constexpr FuHan::Rule tenhou
    = FuHan::kuitan_enabled
    | FuHan::double_yakuman_disabled
    | FuHan::double_wind_pair_4fu;
```

These presets represent only the three options modeled by
`FuHan::Rule`; any other scoring rule used by the corresponding
organization that falls outside that scope is **not** captured here. A
caller that needs a configuration not covered by these presets can
build its own `FuHan::Rule` value by combining one alternative from
each option pair with `operator|` (see
[Error Handling](#error-handling) for the well-formedness requirements).

### `FuHan::Result`

```cpp
struct Result {
    std::uint_fast8_t fu;
    std::uint_fast8_t han;
    std::uint_fast8_t yakuman_multiplier;
};
```

The fields are interpreted by case analysis on the input.
The cases below are mutually exclusive and exhaustive, and match the
[Return value](#return-value) section of `FuHan::calculateFuHan`:

1. **Input does not form a winning shape.** `fu == 0`, `han == 0`,
   `yakuman_multiplier == 0`.
2. **Winning shape but no yaku (excluding dora) established.**
   `fu` is a non-zero fu value computed from the winning shape;
   `han == 0` (dora are *not* reflected here, since dora alone is
   not a yaku); `yakuman_multiplier == 0`.
3. **Non-yakuman yaku established, or counted yakuman (kazoe yakuman).**
   `fu` and `han` are both non-zero (with dora included in `han`);
   `yakuman_multiplier == 0`. Counted yakuman appears as
   `han >= 13` with `yakuman_multiplier == 0`.
4. **Kokushi Musou (thirteen orphans).** `fu == 0` (fu is not
   defined for this hand), `han == 0`, and `yakuman_multiplier` is
   a non-zero value for the recognised form of Kokushi Musou.
5. **Non-counted yakuman other than Kokushi Musou.** `fu` is a
   non-zero fu value, `han == 0`, and `yakuman_multiplier` is the
   non-zero sum of per-yaku yakuman contributions (ordinary yakuman
   `= 1`, double yakuman `= 2`).

`Result` is totally ordered under `operator<` by the score-relevant
candidate ordering documented in [`fuhan/types.hpp`](fuhan/types.hpp):
[yakuman multiplier](#yakuman-multiplier), then `han`, then
`roundUp10(fu) * 2^han` when `han` is tied, then `fu`. This ordering is
used to choose the preferred scoring interpretation and is not a final
point-payment comparison.

#### Discriminating the four outcome categories

The five cases above can be collapsed into the following four
mutually exclusive outcome categories, which a caller typically
needs to distinguish in order to react to the result of
`FuHan::calculateFuHan`:

- Non-counted yakuman.
- Non-yakuman yaku established, or counted yakuman (kazoe yakuman).
- Winning shape but no yaku (excluding dora) established.
- Input does not form a winning shape.

The following pseudocode shows how to discriminate among them given
a `FuHan::Result r`. The order of the tests matters: each branch
assumes that the conditions of all earlier branches have already
been ruled out.

```cpp
FuHan::Result const r = FuHan::calculateFuHan(/* ... */);

if (r.yakuman_multiplier != 0u) {
    // Non-counted yakuman.
    // Covers cases 4 (Kokushi Musou) and 5 (any other non-counted
    // yakuman) from the case analysis above. The final score is
    // determined by `r.yakuman_multiplier` alone; `r.fu` and
    // `r.han` are not used for scoring in this category.
}
else if (r.han != 0u) {
    // Non-yakuman yaku established, or counted yakuman (kazoe yakuman).
    // Covers case 3. The final score is determined by `r.fu` and
    // `r.han`. A counted yakuman appears here as `r.han >= 13`.
}
else if (r.fu != 0u) {
    // Winning shape but no yaku (excluding dora) established.
    // Covers case 2. This is *not* a winning hand for scoring
    // purposes; dora alone do not constitute a yaku.
}
else {
    // Input does not form a winning shape.
    // Covers case 1. The input is not a winning hand under any
    // interpretation.
}
```

Equivalently, a caller that only needs a boolean "is this a winning
hand?" test can check `r.yakuman_multiplier != 0u || r.han != 0u`.

## Yakuman Multiplier

The **yakuman multiplier** is the third field of
[`FuHan::Result`](#fuhanresult) and is the library's mechanism for
reporting **non-counted** yakuman wins. It is a non-negative integer
whose value indicates how many "units" of base yakuman the hand is
worth.

### Definition

For a winning hand, `yakuman_multiplier` is the **sum** of the
per-yaku yakuman contributions of every non-counted yakuman yaku
that is established, where each yaku contributes:

- `1` for an ordinary yakuman (e.g. Daisangen, Suuankou, Tsuuiisou,
  Kokushi Musou, Chinroutou, Ryuuiisou, Shousuushii, Chuuren
  poutou, Suukantsu, Tenhou, Chiihou, ...).
- `2` for a double yakuman (e.g. Suuankou tanki, Kokushi Musou
  juusan menmachi, Daisuushii, Junsei Chuuren poutou — subject to
  the rule set in use).

If no non-counted yakuman is established, `yakuman_multiplier` is
`0`, and the hand is scored from `fu` and `han` instead. In
particular, **counted yakuman (kazoe yakuman)** — a hand that
reaches `13` han or more without any non-counted yakuman yaku — is
**not** reported through `yakuman_multiplier`; for kazoe yakuman,
`yakuman_multiplier == 0` and `han >= 13`.

### Interpretation

| Value | Meaning                                                                          |
|-------|----------------------------------------------------------------------------------|
| `0`   | Not a non-counted yakuman. Score is derived from `fu` and `han` (including kazoe yakuman, and including non-winning hands where `fu == 0` and `han == 0`). |
| `1`   | A single yakuman (e.g. exactly one ordinary yakuman established).                |
| `2`   | A double yakuman, **or** two ordinary yakuman stacked together.                  |
| `3`   | A triple yakuman (e.g. one ordinary + one double, or three ordinary stacked).    |
| `n`   | An `n`-fold yakuman, obtained by summing the contributions of every established yakuman. |

The final yakuman score is computed by the caller as the base score
of a single yakuman multiplied by `yakuman_multiplier`.

### Relation to `han`

When `yakuman_multiplier > 0`, `han` is set to `0`; non-counted
yakuman hands do not also report a han count. Conversely, when
`yakuman_multiplier == 0`, `han` carries the full doubles count
(possibly `>= 13` for kazoe yakuman). The two fields are therefore
mutually exclusive scoring modes: a caller computing the final score
should branch on `yakuman_multiplier == 0` and use either the
`fu`/`han` path or the yakuman path accordingly.

## Tile Encoding

Throughout the public API, tiles are identified by a single
`std::uint_fast8_t` index in `[0, 34)`:

| Range     | Tiles                                                               |
|-----------|---------------------------------------------------------------------|
| `0`–`8`   | Manzu (characters) 1m–9m                                            |
| `9`–`17`  | Pinzu (dots) 1p–9p                                                  |
| `18`–`26` | Souzu (bamboos) 1s–9s                                               |
| `27`–`30` | Winds: East (`27`), South (`28`), West (`29`), North (`30`)         |
| `31`–`33` | Dragons: White (`31`), Green (`32`), Red (`33`)                     |

## Chii Index Encoding

`chii_list` is a `std::array<std::uint_fast8_t, 21>` whose index
identifies a specific chii (sequence) meld by its **base tile** —
the lowest-ranked tile of the three consecutive tiles that form the
sequence. The value stored at each index is the number of chii melds
of that specific shape that the winning player has called.

Honors (winds and dragons) cannot form sequences, so only the three
numbered suits contribute base tiles. Within each suit, the base tile
of a sequence ranges from `1` (forming `1-2-3`) to `7` (forming
`7-8-9`), giving `7 × 3 = 21` entries. The indices are laid out
contiguously, suit by suit, in the order manzu → pinzu → souzu:

| Index | Sequence    | Base tile (internal tile index) |
|-------|-------------|---------------------------------|
| `0`   | 1m–2m–3m    | 1m (`0`)                        |
| `1`   | 2m–3m–4m    | 2m (`1`)                        |
| `2`   | 3m–4m–5m    | 3m (`2`)                        |
| `3`   | 4m–5m–6m    | 4m (`3`)                        |
| `4`   | 5m–6m–7m    | 5m (`4`)                        |
| `5`   | 6m–7m–8m    | 6m (`5`)                        |
| `6`   | 7m–8m–9m    | 7m (`6`)                        |
| `7`   | 1p–2p–3p    | 1p (`9`)                        |
| `8`   | 2p–3p–4p    | 2p (`10`)                       |
| `9`   | 3p–4p–5p    | 3p (`11`)                       |
| `10`  | 4p–5p–6p    | 4p (`12`)                       |
| `11`  | 5p–6p–7p    | 5p (`13`)                       |
| `12`  | 6p–7p–8p    | 6p (`14`)                       |
| `13`  | 7p–8p–9p    | 7p (`15`)                       |
| `14`  | 1s–2s–3s    | 1s (`18`)                       |
| `15`  | 2s–3s–4s    | 2s (`19`)                       |
| `16`  | 3s–4s–5s    | 3s (`20`)                       |
| `17`  | 4s–5s–6s    | 4s (`21`)                       |
| `18`  | 5s–6s–7s    | 5s (`22`)                       |
| `19`  | 6s–7s–8s    | 6s (`23`)                       |
| `20`  | 7s–8s–9s    | 7s (`24`)                       |

The mapping between a `chii_list` index `c` and the internal tile
index `t` of its base tile is therefore:

- `c ∈ [0, 7)`   → manzu, `t = c`             (`1m`..`7m`).
- `c ∈ [7, 14)`  → pinzu, `t = c - 7 + 9`     (`1p`..`7p`).
- `c ∈ [14, 21)` → souzu, `t = c - 14 + 18`   (`1s`..`7s`).

Note the deliberate offset between the chii index space and the
tile-index space: the chii index space has no gap between suits
(because the unusable `8m`/`9m`, `8p`/`9p`, and `8s`/`9s` bases are
excluded), whereas the tile-index space numbers all nine tiles of
each suit consecutively. In particular, `chii_list[7]` corresponds to
the `1p–2p–3p` sequence whose base tile is `1p` (tile index `9`),
**not** to a sequence whose base tile has tile index `7`.

## Error Handling

All error reporting goes through C++ exceptions.

- `FuHan::calculateFuHan` throws `std::invalid_argument` if any
  structural invariant of the inputs or of the `Context` is violated.
  The `what()` string identifies the offending quantity.
- `FuHan::calculateFuHan` throws `std::overflow_error` if any internal
  han-related calculation, including the addition of `num_dora` or the
  comparison of candidate results, may exceed the upper limit required
  by that calculation step. The effective upper limit is the strictest
  limit imposed by the internal steps performed for the input.
- Other exceptions originating from the standard library (e.g.
  `std::bad_alloc`) may propagate out.
- The function does **not** use error codes, `std::optional`, or
  `std::expected`.

A "non-winning hand" is **not** an error: it is reported through the
returned `Result`. Such hands always satisfy `han == 0` and
`yakuman_multiplier == 0`, and the two sub-cases are distinguished by
`fu` (see [Return value](#return-value)):

- `fu == 0` — the input does **not** form a winning shape under any
  interpretation (case 1).
- `fu != 0` — the input *does* form a winning shape but no yaku
  (excluding dora) is established (case 2).

Callers that only need a boolean "is this a winning hand?" test can
therefore check whether `yakuman_multiplier != 0 || han != 0`.

## Thread Safety

`FuHan::calculateFuHan` is a pure function: it reads only its
arguments, holds no global or mutable static state, and returns a
value by copy. As a result:

- Calling `calculateFuHan` concurrently from multiple threads with
  independent arguments is safe and requires no synchronisation.
- Calling `calculateFuHan` concurrently with shared `const` arguments
  is also safe.

## Performance Notes

- The library is header-only; with optimisation enabled the call
  reduces to inlined work on small, fixed-size arrays.
- All input arrays are fixed-size (`std::array`), so there is no
  heap allocation on the input side. The implementation may allocate
  internally for transient bookkeeping.
- Complexity: each call is bounded by the (finite) number of
  decompositions of a 14-tile hand into melds and pairs, so the
  per-call cost is bounded by a small constant.
- TODO: Provide measured throughput numbers on representative
  hardware once a benchmark harness exists.

## Examples

- TODO: Add `examples/` directory with at least:
  - A minimal "first call" example (essentially the
    [Quick Start](#quick-start) snippet).
  - A worked example for each of the three winning shapes
    (standard, Chiitoitsu, Kokushi Musou).
  - A worked example demonstrating yakuman and counted yakuman.
  - A worked example demonstrating dora handling.

## Integration Guide

Because the library is header-only, the simplest integration is to
copy this repository, or add it with
[`git submodule add`][git-submodule], under your project and add its
root to your include path.

```cpp
#include <fuhan/fuhan.hpp>
```

If your project uses [CMake][cmake], prefer linking the bundled `fuhan`
`INTERFACE` target rather than managing include paths by hand (see
[CMake Usage Example](#cmake-usage-example)); the project also
provides an `install()` rule for installing the headers to a prefix
(see [Installation](#installation)).

## CMake Usage Example

The repository's own `CMakeLists.txt` defines an `INTERFACE` library
target named `fuhan`. Its public headers are attached as a header
`FILE_SET`, so linking against the target automatically adds the
repository root (or, for an installed copy, the install prefix's
include directory) to your include path; you do not need to call
`target_include_directories` yourself. That same target backs the
`install()` rule described under [Installation](#installation).

For consumers vendoring the sources (e.g. via a submodule under
`third_party/`), the typical pattern is:

```cmake
# In your CMakeLists.txt
add_subdirectory(third_party/fuhan EXCLUDE_FROM_ALL)
target_link_libraries(my_target PRIVATE fuhan)
target_compile_features(my_target PRIVATE cxx_std_23)
```

Linking `fuhan` is sufficient for `#include <fuhan/fuhan.hpp>` to
resolve; because `fuhan` is an `INTERFACE` (header-only) target,
nothing from the library itself is compiled or linked. C++23 must
still be requested for `my_target`, as shown above.

- TODO: Export the target under a namespaced alias (e.g.
  `FuHan::FuHan`) and ship a `FuHanConfig.cmake` (via
  `install(EXPORT ...)`) so that installed copies can be consumed
  with `find_package(FuHan)`.

## Notes for Library Consumers

- The only header you need to include is `fuhan/fuhan.hpp`. It
  transitively includes everything else under `fuhan/`.
- Headers and identifiers ending with an underscore (e.g.
  `FuHan::Standard_`, `FuHan::SevenPairs_`,
  `FuHan::ThirteenOrphans_`, `tsumo_`, `ron_`, etc.) are
  implementation details. Use the unsuffixed convenience constants
  (`FuHan::tsumo`, `FuHan::ron`, …) and the public types only.

## Notes for Contributors

### Development setup

Install a [C++23][cpp23]-capable toolchain ([GCC][gcc] or
[Clang][clang]), [CMake][cmake] 3.25 or newer, and the
[POSIX shell][posix-shell] utilities listed in
[Requirements](#requirements). With those in place, build and test the
project using the commands in
[Building from Source](#building-from-source) and
[Running Tests](#running-tests).

- TODO: Document a reproducible containerised or
  VS Code dev container workflow, if one is provided in the future.

### Code formatting / linting / testing policy

- The project builds with `-Werror` on [GCC][gcc] and
  [Clang][clang]; new code must not introduce warnings.
- Debug builds enable `_GLIBCXX_DEBUG`, `_GLIBCXX_DEBUG_PEDANTIC`,
  [AddressSanitizer][asan], and [UndefinedBehaviorSanitizer][ubsan]
  by default. Run the test suite at least once in a Debug build before
  submitting a change.
- The Mahjong Soul scoring corpus (`test/majsoul-scoring-corpus/*.tsv*`)
  must pass after any change to the public behaviour.
- TODO: Document the formatting tool of record (e.g.
  [`clang-format`][clang-format]) and add a configuration file if one
  is intended.
- TODO: Document the linting tool of record (e.g.
  [`clang-tidy`][clang-tidy]).

### Commit messages

- This repository uses [Conventional Commits][conventional-commits]
  for commit messages.
- Keep every commit-message line at or below 72 characters, including
  the subject, body, and any [Git trailers][git-trailers].

## Versioning Policy

TODO: Adopt and document an explicit policy.
[Semantic Versioning][semver] (`MAJOR.MINOR.PATCH`) is recommended for
a library with a single public function: any change to the signature or
observable behaviour of `FuHan::calculateFuHan`, `FuHan::Wind`,
`FuHan::Context`, or `FuHan::Result` would constitute a breaking
change.

## Compatibility Policy

- **Language standard:** [C++23][cpp23]. Older standards are not
  supported.
- **API stability:** TODO. Define when (if ever) the signature of
  `calculateFuHan` may change, and how deprecations will be
  signalled.
- **ABI stability:** Not applicable in the usual sense, because the
  library is header-only.
- **Platforms:** TODO. Confirm officially supported platforms
  (Linux/macOS/Windows) and toolchains.

## Limitations

- The library returns `fu`, `han`, and `yakuman_multiplier` only; it
  does not produce a final score in points.
- The library does not enumerate which yaku are present.
- Dora must be counted by the caller and passed via `num_dora`.
- Only the three winning shapes listed above are recognised. Local
  yaku or alternative winning shapes are out of scope unless
  explicitly added.
- The configurable rule variations are limited to the three options
  listed under [Supported Rule Variants](#supported-rule-variants);
  other rule variants are enumerated under
  [Rules not supported](#rules-not-supported).

## Roadmap

- TODO: Define a public roadmap. Candidates include: shipping an
  installable CMake package — a namespaced `FuHan::FuHan` alias and a
  `FuHanConfig.cmake` exported via `install(EXPORT ...)`, building on
  the existing `fuhan` `INTERFACE` target and `install()` rule;
  benchmarks; an enumeration of applied yaku in `Result`; optional
  support for local rule variants.

## FAQ

## References

- TODO: List the rule references used (e.g. the Riichi Mahjong
  rules of a specific organisation, or the published Mahjong Soul rules).
- TODO: Cite the source of the bundled scoring corpus if it
  derives from a third party.

## License

This project is licensed under the **[MIT License][mit-license]**. See
[`LICENSE`](LICENSE) for the full text.

The [SPDX license identifier][spdx-license-id] is:

```
SPDX-License-Identifier: MIT
```

## Acknowledgements

TODO: Add acknowledgements here.

[asan]: https://clang.llvm.org/docs/AddressSanitizer.html
[boost-python]: https://www.boost.org/doc/libs/release/libs/python/
[bzip2]: https://sourceware.org/bzip2/
[clang]: https://clang.llvm.org/
[clang-format]: https://clang.llvm.org/docs/ClangFormat.html
[clang-tidy]: https://clang.llvm.org/extra/clang-tidy/
[cmake]: https://cmake.org/cmake/help/latest/
[conventional-commits]: https://www.conventionalcommits.org/en/v1.0.0/
[cpp23]: https://isocpp.org/std/the-standard
[ctest]: https://cmake.org/cmake/help/latest/manual/ctest.1.html
[gcc]: https://gcc.gnu.org/
[git-submodule]: https://git-scm.com/docs/git-submodule
[git-trailers]: https://git-scm.com/docs/git-interpret-trailers
[gzip]: https://www.gnu.org/software/gzip/
[mahjong-soul]: https://mahjongsoul.com/
[mit-license]: https://spdx.org/licenses/MIT.html
[m-league]: https://m-league.jp/
[msvc]: https://learn.microsoft.com/cpp/build/reference/compiler-options
[posix-shell]: https://pubs.opengroup.org/onlinepubs/9799919799/
[python-mahjong]: https://pypi.org/project/mahjong/
[python3]: https://docs.python.org/3/
[semver]: https://semver.org/
[spdx-license-id]: https://spdx.dev/ids/
[tenhou]: https://tenhou.net/
[tsan]: https://clang.llvm.org/docs/ThreadSanitizer.html
[ubsan]: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
[xz]: https://tukaani.org/xz/
