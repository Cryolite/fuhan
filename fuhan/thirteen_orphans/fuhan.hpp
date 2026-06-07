// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#if !defined(FUHAN_THIRTEEN_ORPHANS_FUHAN_HPP_INCLUDE_GUARD)
#define FUHAN_THIRTEEN_ORPHANS_FUHAN_HPP_INCLUDE_GUARD

#include "../types.hpp"
#include "../internal.hpp"
#include <sstream>
#include <array>
#include <stdexcept>
#include <cstdint>


namespace FuHan::ThirteenOrphans_{

/**
 * @brief Computes the fu, han, and yakuman multiplier of a Kokushi
 *        musou (thirteen orphans, 国士無双) winning hand.
 *
 * Tests whether the given fully-concealed hand forms Kokushi musou,
 * and, if so, returns the corresponding scoring `Result`. Kokushi
 * musou is a yakuman hand composed of one of each of the 13
 * terminal-and-honor tiles (the four winds, the three dragons, and
 * the terminals 1 and 9 of each suit) plus one duplicate of any of
 * those 13 tiles.
 *
 * Because Kokushi musou can only be formed from a fully concealed
 * hand with no called melds, the function takes only the concealed
 * tile counts (excluding the winning tile) and the winning tile
 * itself. Round wind, seat wind, the meld lists, and the dora count
 * are not parameters of this entry point: they are irrelevant to
 * Kokushi musou scoring. Dora in particular does not contribute,
 * since the scoring is determined entirely by the yakuman
 * multiplier (see @ref FuHan::Result::yakuman_multiplier).
 *
 * The tile-count convention matches the rest of the library: index
 * `i` in @p concealed_hand corresponds to the internal tile index
 * `i` (in `[0, 34)`), and the value at each index is the number of
 * tiles of that kind held concealed. The winning tile is supplied
 * separately via @p winning_tile and must not be counted in
 * @p concealed_hand, regardless of whether the win is tsumo or ron.
 *
 * Two variants of Kokushi musou are recognised:
 * - The 13-tile (pair) wait, where the concealed portion already
 *   contains one of each terminal-and-honor tile and the winning
 *   tile completes the pair. This is treated as a double yakuman,
 *   so the returned `Result` has
 *   `yakuman_multiplier == 2`.
 * - The single-tile wait, where the concealed portion already
 *   contains a pair of one terminal-and-honor tile plus singletons
 *   of the other twelve, and the winning tile is the missing
 *   thirteenth kind. This is an ordinary yakuman, so the returned
 *   `Result` has `yakuman_multiplier == 1`.
 *
 * In both cases @ref FuHan::Result::fu and @ref FuHan::Result::han
 * are set to `0`: fu is not defined for Kokushi musou (see
 * @ref FuHan::Result::fu), and the score is determined from the
 * yakuman multiplier rather than from han.
 *
 * @param concealed_hand Tile counts of the concealed portion of the
 *                       hand, indexed by internal tile index in
 *                       `[0, 34)` and excluding the winning tile.
 * @param winning_tile   The internal tile index of the winning tile,
 *                       in `[0, 34)`.
 * @param context        A bitmask of `FuHan::Context` flags
 *                       describing the situational context of the
 *                       win. Only flags that bear on Kokushi musou
 *                       recognition (such as `tsumo` vs `ron`, and
 *                       whether the win is the wait-on-pair variant)
 *                       affect the outcome; other flags are
 *                       accepted but do not alter the result.
 * @return A `Result` describing the score of the hand. If the input
 *         forms Kokushi musou, `fu` and `han` are `0` and
 *         `yakuman_multiplier` is `1` (single yakuman) or `2`
 *         (double yakuman, 13-tile wait). If the input does not
 *         form Kokushi musou, all three fields of the returned
 *         `Result` are `0`, indicating that the hand is not a
 *         winning hand for scoring purposes under this entry point.
 */
inline FuHan::Result calculateFuHan(
  std::array<std::uint_fast8_t, 34u> const &concealed_hand,
  std::uint_fast8_t const winning_tile,
  FuHan::Context const context)
{
  std::uint_fast8_t total_num_concealed_tiles = 0u;
  for (std::uint_fast8_t const count : concealed_hand) {
    total_num_concealed_tiles += count;
  }
  if (total_num_concealed_tiles != 13u) {
    return FuHan::Result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
  }

  // Kokushi musou can only be composed of yaochuu (terminal/honor) tiles.
  // If any non-yaochuu tile appears in the full 14-tile hand, the hand is
  // not Kokushi musou.
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (!FuHan::isYaochuuTile_(i) && concealed_hand[i] >= 1u) {
      return FuHan::Result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
    }
  }
  if (!FuHan::isYaochuuTile_(winning_tile)) {
    return FuHan::Result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
  }

  // Build the full 14-tile distribution (concealed hand + winning tile) and
  // verify that it consists of exactly the 13 yaochuu tiles, with one of
  // them duplicated.
  std::array<std::uint_fast8_t, 34u> full_hand = concealed_hand;
  ++full_hand[winning_tile];

  constexpr std::array<std::uint_fast8_t, 13u> yaochuu_tiles = {
    0u, 8u, 9u, 17u, 18u, 26u, 27u, 28u, 29u, 30u, 31u, 32u, 33u
  };
  std::uint_fast8_t num_pairs = 0u;
  for (std::uint_fast8_t const tile : yaochuu_tiles) {
    std::uint_fast8_t const count = full_hand[tile];
    if (count == 0u || count > 2u) {
      return FuHan::Result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
    }
    if (count == 2u) {
      ++num_pairs;
    }
  }
  if (num_pairs != 1u) {
    return FuHan::Result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
  }

  // The hand is Kokushi musou. If the concealed portion alone already
  // contains one of each yaochuu tile, the winning tile completed the
  // pair, which is the 13-tile (pair) wait and counts as a double
  // yakuman. Otherwise it is the single-tile wait and counts as a
  // single yakuman.
  bool is_thirteen_wait = true;
  for (std::uint_fast8_t const tile : yaochuu_tiles) {
    if (concealed_hand[tile] != 1u) {
      is_thirteen_wait = false;
      break;
    }
  }

  std::uint_fast8_t yakuman_multiplier = is_thirteen_wait ? 2u : 1u;
  bool const is_tenhou = isTenhou(context);
  bool const is_chiihou = isChiihou(context);
  yakuman_multiplier += is_tenhou || is_chiihou ? 1u : 0u;
  return FuHan::Result{
    .fu = 0u,
    .han = 0u,
    .yakuman_multiplier = yakuman_multiplier,
  };
}

} // namespace FuHan::ThirteenOrphans_

#endif // !defined(FUHAN_THIRTEEN_ORPHANS_FUHAN_HPP_INCLUDE_GUARD)
