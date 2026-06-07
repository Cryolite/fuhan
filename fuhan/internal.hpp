// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#if !defined(FUHAN_INTERNAL_HPP_INCLUDE_GUARD)
#define FUHAN_INTERNAL_HPP_INCLUDE_GUARD

#include "types.hpp"
#include <sstream>
#include <string>
#include <stdexcept>
#include <cstdint>


namespace FuHan{

/**
 * @brief Converts an internal tile index into its human-readable string representation.
 *
 * The tile index follows the library's internal encoding:
 * - `0`-`8`:   man-zi (characters) 1m through 9m.
 * - `9`-`17`:  pin-zi (dots)       1p through 9p.
 * - `18`-`26`: suo-zi (bamboos)    1s through 9s.
 * - `27`-`33`: honor tiles, in the order East (`E`), South (`S`), West (`W`),
 *              North (`N`), White Dragon (`P`), Green Dragon (`F`), Red Dragon (`C`).
 *
 * Number tiles are formatted as a digit followed by the suit letter (e.g.
 * `"5p"`), and honor tiles are formatted as a single letter (e.g. `"E"`).
 *
 * @param tile The internal tile index. Must be in the range `[0, 34)`.
 * @return The string representation of @p tile.
 * @throws std::invalid_argument If @p tile is not a valid tile index
 *         (i.e. `tile >= 34`).
 */
inline std::string tileToString_(std::uint_fast8_t const tile)
{
  if (tile < 27u) {
    char const suit_str = "mpsz"[tile / 9u];
    std::uint_fast8_t const rank = tile % 9u + 1u;
    char const rank_char = static_cast<char>('0' + rank);
    return std::string(1u, rank_char) + suit_str;
  }
  if (tile < 34u) {
    char const honor_str = "ESWNPFC"[tile - 27u];
    return std::string(1u, honor_str);
  }
  std::ostringstream oss;
  oss << static_cast<unsigned>(tile) << ": An invalid tile.";
  throw std::invalid_argument(oss.str());
}

/**
 * @brief Tests whether a tile is a yaochuu tile (terminal or honor).
 *
 * Yaochuu tiles (么九牌) are the union of the terminals (1 and 9 of each
 * numbered suit) and the honor tiles (winds and dragons). This predicate
 * is used throughout the score calculator to determine, among other
 * things, the per-meld fu bonus for triplets/quads of yaochuu tiles and
 * to detect yaku such as Tanyao (which forbids yaochuu tiles) and
 * Yaochuu yaku families (which require them).
 *
 * With the library's tile encoding, tiles 0-26 are partitioned by suit
 * into 9-tile groups (manzu, pinzu, souzu) and tiles 27-33 are the
 * honors. The function computes the suit (`tile / 9`) and the rank
 * within the suit (`tile % 9`); a tile is a yaochuu tile iff:
 * - its suit index is at least `3` (i.e. it is an honor tile, regardless
 *   of rank), or
 * - its rank within a numbered suit is `0` (a 1) or `8` (a 9).
 *
 * The caller is responsible for ensuring that @p tile is a valid tile
 * index in the range `[0, 33]`. Values outside this range are not
 * rejected by this function but yield an unspecified boolean result.
 *
 * @param tile The tile index to test, in the range `[0, 33]`.
 * @return `true` if @p tile is a yaochuu tile (terminal or honor),
 *         otherwise `false`.
 */
inline bool isYaochuuTile_(std::uint_fast8_t const tile)
{
  std::uint_fast8_t const color = tile / 9u;
  std::uint_fast8_t const rank = color < 3u ? tile % 9u : UINT_FAST8_MAX;
  return color == 3u || rank == 0u || rank == 8u;
}

/**
 * @brief Computes the han contributed by situational (context-only) yaku.
 *
 * Sums the han contributions of every yaku that is determined solely
 * by the situational `FuHan::Context` flags of a win, independently of
 * the tile composition or block decomposition of the hand. These yaku
 * cannot be detected from the tiles alone, so they are evaluated once
 * per winning hand after the highest-scoring decomposition has been
 * selected, and their total han is added on top of the
 * decomposition-derived han.
 *
 * Each contributing flag adds the standard published han value of the
 * corresponding yaku, namely:
 * - `riichi`         : +1 han.
 * - `chankan`        : +1 han.
 * - `rinshan_kaihou` : +1 han.
 * - `haitei_raoyue`  : +1 han.
 * - `houtei_raoyui`  : +1 han.
 * - `ippatsu`        : +1 han.
 * - `double_riichi`  : +2 han.
 *
 * The `tsumo`, `ron`, `tenhou`, and `chiihou` flags do not contribute
 * here: `tsumo` and `ron` only select the win mode (their menzen-tsumo
 * yaku is handled together with the decomposition-derived yaku, where
 * the menzen status is known), while `tenhou` and `chiihou` are
 * yakuman and are accounted for through `FuHan::Result::yakuman_multiplier`
 * rather than `FuHan::Result::han`.
 *
 * The caller is responsible for ensuring that @p context is internally
 * consistent (see `FuHan::checkInput_`). In particular, mutually
 * exclusive flag combinations (for example `riichi` together with
 * `double_riichi`) are not detected here and would erroneously stack
 * if they were present.
 *
 * @param context A bitmask of `FuHan::Context` flags describing the
 *                situational context of the win.
 * @return The total han contributed by the situational yaku set in
 *         @p context. Returns `0` when no situational yaku is set.
 */
inline std::uint_fast8_t calculateSituationalHan_(FuHan::Context const context)
{
  std::uint_fast8_t han = 0u;
  if (isRiichi(context)) {
    ++han;
  }
  if (isChankan(context)) {
    ++han;
  }
  if (isRinshanKaihou(context)) {
    ++han;
  }
  if (isHaiteiRaoyue(context)) {
    ++han;
  }
  if (isHouteiRaoyui(context)) {
    ++han;
  }
  if (isDoubleRiichi(context)) {
    han += 2u;
  }
  if (isIppatsu(context)) {
    ++han;
  }
  return han;
}

} // namespace FuHan

#endif // !defined(FUHAN_INTERNAL_HPP_INCLUDE_GUARD)
