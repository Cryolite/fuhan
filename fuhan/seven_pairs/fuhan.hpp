// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#if !defined(FUHAN_SEVEN_PAIRS_FUHAN_HPP_INCLUDE_GUARD)
#define FUHAN_SEVEN_PAIRS_FUHAN_HPP_INCLUDE_GUARD

#include "../internal.hpp"
#include "../types.hpp"
#include <array>
#include <stdexcept>
#include <cstdint>


namespace FuHan::SevenPairs_{

/**
 * @brief Tests whether the hand establishes Tsuuiisou (all honors, 字一色).
 *
 * Tsuuiisou is the yakuman that requires every tile in the hand to be
 * an honor tile (winds or dragons, tile indices `27`..`33`). In the
 * Seven Pairs (Chiitoitsu, 七対子) context, the hand is structured as
 * seven distinct pairs encoded in @p full_hand by having `2` at each
 * pair-tile index and `0` everywhere else, so the test reduces to
 * verifying that no numbered-suit tile (index `< 27`) appears as a
 * pair.
 *
 * The caller is responsible for ensuring that @p full_hand is a valid
 * Chiitoitsu shape, i.e. that every nonzero entry has the value `2`
 * and that there are exactly seven such entries; this function does
 * not re-check that invariant.
 *
 * @param full_hand Tile counts of the full 14-tile hand (concealed
 *                  hand plus winning tile), indexed by internal tile
 *                  index in `[0, 34)`. Each entry is either `0` or
 *                  `2` for a valid Chiitoitsu hand.
 * @return `1` if Tsuuiisou is established (every pair is on an honor
 *         tile), otherwise `0`. The return value is the yakuman
 *         multiplier contributed by this yaku and can be added
 *         directly to a running multiplier total.
 */
inline std::uint_fast8_t checkTsuuiisou(std::array<std::uint_fast8_t, 34u> const &full_hand)
{
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (full_hand[i] == 2u && i < 27u) {
      return 0u;
    }
  }
  return 1u;
}

/**
 * @brief Checks the Menzen tsumo (門前清自摸和) yaku and returns its han.
 *
 * Menzen tsumo is a 1-han yaku that is established when the win is
 * achieved by self-draw on a fully concealed hand. In the Seven Pairs
 * (Chiitoitsu, 七対子) context the menzen requirement is automatic,
 * because Chiitoitsu can only be formed from a fully concealed hand
 * (no chii, pon, or open kan is compatible with a seven-pairs shape).
 * Consequently the yaku reduces to a single test on @p context: it
 * is established iff the `tsumo` flag is set.
 *
 * The caller is responsible for ensuring that @p context is internally
 * consistent (see `FuHan::checkInput_`); in particular, exactly one of
 * `tsumo` and `ron` should be set. This function only inspects the
 * `tsumo` bit and does not cross-check the `ron` bit.
 *
 * @param context A bitmask of `FuHan::Context` flags describing the
 *                situational context of the win.
 * @return `1` when Menzen tsumo is established (the `tsumo` flag is
 *         set in @p context); `0` otherwise.
 */
inline std::uint_fast8_t checkMenzenTsumo(FuHan::Context const context)
{
  return isTsumo(context) ? 1u : 0u;
}

/**
 * @brief Checks the Tanyao (断么九, all simples) yaku and returns its han.
 *
 * Tanyao is a 1-han yaku that is established when the hand contains
 * no yaochuu tiles (terminals and honors). In the Seven Pairs
 * (Chiitoitsu, 七対子) context the hand is structured as seven
 * distinct pairs encoded in @p full_hand by having `2` at each
 * pair-tile index and `0` everywhere else, so the test reduces to
 * verifying that no pair sits on a yaochuu tile (as determined by
 * `FuHan::isYaochuuTile_`).
 *
 * The caller is responsible for ensuring that @p full_hand is a valid
 * Chiitoitsu shape, i.e. that every nonzero entry has the value `2`
 * and that there are exactly seven such entries; this function does
 * not re-check that invariant.
 *
 * @param full_hand Tile counts of the full 14-tile hand (concealed
 *                  hand plus winning tile), indexed by internal tile
 *                  index in `[0, 34)`. Each entry is either `0` or
 *                  `2` for a valid Chiitoitsu hand.
 * @return `1` if Tanyao is established (no pair is on a yaochuu
 *         tile); `0` otherwise. The return value is the han
 *         contributed by this yaku and can be added directly to a
 *         running han total.
 */
inline std::uint_fast8_t checkTanyao(std::array<std::uint_fast8_t, 34u> const &full_hand)
{
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (full_hand[i] == 2u && FuHan::isYaochuuTile_(i)) {
      return 0u;
    }
  }
  return 1u;
}

/**
 * @brief Checks the Honroutou (混老頭, all terminals and honors) yaku
 *        and returns its han.
 *
 * Honroutou is a 2-han yaku that is established when every tile in
 * the hand is a yaochuu tile (a terminal `1` or `9` of a numbered
 * suit, or an honor tile). In the Seven Pairs (Chiitoitsu, 七対子)
 * context the hand is structured as seven distinct pairs encoded in
 * @p full_hand by having `2` at each pair-tile index and `0`
 * everywhere else, so the test reduces to verifying that every pair
 * sits on a yaochuu tile (as determined by `FuHan::isYaochuuTile_`).
 *
 * Note that Honroutou and Tsuuiisou (字一色) are *not* mutually
 * exclusive at the shape level: a Chiitoitsu of seven honor pairs
 * is also Honroutou. Because Tsuuiisou is a yakuman, the caller is
 * expected to short-circuit yakuman evaluation before invoking this
 * function (so that Honroutou's han is not double-counted with a
 * yakuman); this function itself performs no such check.
 *
 * The caller is also responsible for ensuring that @p full_hand is a
 * valid Chiitoitsu shape, i.e. that every nonzero entry has the
 * value `2` and that there are exactly seven such entries; this
 * function does not re-check that invariant.
 *
 * @param full_hand Tile counts of the full 14-tile hand (concealed
 *                  hand plus winning tile), indexed by internal tile
 *                  index in `[0, 34)`. Each entry is either `0` or
 *                  `2` for a valid Chiitoitsu hand.
 * @return `2` if Honroutou is established (every pair is on a
 *         yaochuu tile); `0` otherwise. The return value is the han
 *         contributed by this yaku and can be added directly to a
 *         running han total.
 */
inline std::uint_fast8_t checkHonroutou(std::array<std::uint_fast8_t, 34u> const &full_hand)
{
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (full_hand[i] == 2u && !FuHan::isYaochuuTile_(i)) {
      return 0u;
    }
  }
  return 2u;
}

/**
 * @brief Checks the one-suit yaku family (Chinitsu / Honitsu) and
 *        returns its han contribution.
 *
 * The one-suit ("isshoku", 一色) yaku family consists of two mutually
 * exclusive yaku that depend only on which suits appear in the hand:
 *
 * - Chinitsu (清一色, "pure one suit"): every tile of the hand comes
 *   from a *single* numbered suit (manzu, pinzu, or souzu) and no
 *   honor tile is present. Worth `6` han.
 * - Honitsu (混一色, "half flush"): every numbered tile comes from a
 *   *single* numbered suit, but at least one honor tile is also
 *   present. Worth `3` han.
 *
 * Both yaku are worth their menzen value because Chiitoitsu can only
 * be formed from a fully concealed hand, so the open/closed
 * kuisagari distinction does not arise.
 *
 * Detection iterates over @p full_hand and classifies each pair-tile
 * index by its suit (`i / 9`). Honor pairs (suit index `3`) are
 * tolerated but disqualify Chinitsu. The first numbered pair fixes
 * the candidate suit; every subsequent numbered pair must match that
 * suit, otherwise neither yaku is established.
 *
 * Note that this function will report Honitsu (`3`) even when every
 * pair is on an honor tile (an "all honors" shape). Such a hand is
 * Tsuuiisou (字一色), a yakuman, and the caller is expected to
 * short-circuit yakuman evaluation before invoking this function so
 * that its `3` han is not stacked on top of a yakuman; this function
 * itself performs no such check.
 *
 * The caller is also responsible for ensuring that @p full_hand is a
 * valid Chiitoitsu shape, i.e. that every nonzero entry has the
 * value `2` and that there are exactly seven such entries; this
 * function does not re-check that invariant.
 *
 * @param full_hand Tile counts of the full 14-tile hand (concealed
 *                  hand plus winning tile), indexed by internal tile
 *                  index in `[0, 34)`. Each entry is either `0` or
 *                  `2` for a valid Chiitoitsu hand.
 * @return `6` for Chinitsu, `3` for Honitsu, and `0` when neither
 *         yaku is established (i.e. the hand uses tiles from two or
 *         more numbered suits). The return value is the han
 *         contributed by this yaku and can be added directly to a
 *         running han total.
 */
inline std::uint_fast8_t checkIsshoku(std::array<std::uint_fast8_t, 34u> const &full_hand)
{
  bool is_chiniisou = true;
  std::uint_fast8_t color = UINT_FAST8_MAX;
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (full_hand[i] == 0u) {
      continue;
    }
    std::uint_fast8_t const current_color = i / 9u;
    if (current_color == 3u) {
      is_chiniisou = false;
      continue;
    }
    if (color == UINT_FAST8_MAX) {
      color = current_color;
      continue;
    }
    if (current_color != color) {
      return 0u;
    }
  }
  return is_chiniisou ? 6u : 3u;
}

/**
 * @brief Computes the fu, han, and yakuman multiplier of a Chiitoitsu
 *        (seven pairs, 七対子) winning hand.
 *
 * Tests whether the given fully-concealed hand forms Chiitoitsu and,
 * if so, returns the corresponding scoring `Result`. Chiitoitsu is a
 * 2-han yaku composed of seven distinct pairs. Because Chiitoitsu can
 * only be formed from a fully concealed hand (no chii, pon, or open
 * kan is compatible with a seven-pairs shape), the function takes
 * only the concealed tile counts (excluding the winning tile) and the
 * winning tile itself.
 *
 * Validity of the seven-pairs shape is determined by combining
 * @p concealed_hand with @p winning_tile to form the full 14-tile
 * hand and checking that every nonzero entry is exactly `2` and that
 * the total tile count is `14`. This rejects shapes that contain a
 * triplet or quad (which would yield a count of `3` or `4` at some
 * index) as well as any shape that does not partition into seven
 * pairs.
 *
 * Scoring proceeds in two stages:
 * -# Non-counted yakuman are evaluated. Tenhou (`tenhou`) and Chiihou
 *    (`chiihou`) are read directly from @p context, and Tsuuiisou
 *    (`checkTsuuiisou`) is detected from the full-hand distribution.
 *    Each contributes `1` to `yakuman_multiplier`. If at least one
 *    yakuman is established, the function returns immediately with
 *    `han == 0`, `fu == 25`, and the accumulated multiplier; no
 *    ordinary yaku, situational yaku, or dora are counted in that
 *    case.
 * -# Otherwise, the base `2` han of Chiitoitsu is accumulated together
 *    with every ordinary yaku that is compatible with the seven-pairs
 *    shape: Menzen tsumo (`checkMenzenTsumo`), Tanyao
 *    (`checkTanyao`), Honroutou (`checkHonroutou`), and the one-suit
 *    family Chinitsu / Honitsu (`checkIsshoku`). Situational yaku are
 *    added via `FuHan::calculateSituationalHan_(context)`, and
 *    @p num_dora is added on top. The returned `fu` is fixed at `25`,
 *    the published fu value for Chiitoitsu.
 *
 * The tile-count convention matches the rest of the library: index
 * `i` in @p concealed_hand corresponds to the internal tile index
 * `i` (in `[0, 34)`). The winning tile is supplied separately via
 * @p winning_tile and must not be counted in @p concealed_hand,
 * regardless of whether the win is tsumo or ron.
 *
 * @param concealed_hand Tile counts of the concealed portion of the
 *                       hand, indexed by internal tile index in
 *                       `[0, 34)` and excluding the winning tile.
 * @param winning_tile   The internal tile index of the winning tile,
 *                       in `[0, 34)`.
 * @param num_dora       The total number of dora the hand counts,
 *                       including regular dora, aka dora, and ura
 *                       dora (when applicable). Contributes to
 *                       `Result::han` only when the hand actually
 *                       forms Chiitoitsu and is not a yakuman; in
 *                       particular, dora alone do not form a winning
 *                       hand (see `FuHan::Result::han`).
 * @param context        A bitmask of `FuHan::Context` flags
 *                       describing the situational context of the
 *                       win.
 * @return A `Result` describing the score of the hand. If the input
 *         forms Chiitoitsu and at least one yakuman is established,
 *         `fu == 25`, `han == 0`, and `yakuman_multiplier` holds
 *         the accumulated multiplier. If the input forms Chiitoitsu
 *         without a yakuman, `fu == 25` and `han` is the sum of the
 *         Chiitoitsu base, the applicable ordinary yaku, the
 *         situational yaku, and the dora count. If the input does
 *         not form Chiitoitsu, all three fields of the returned
 *         `Result` are `0`, indicating that the hand is not a
 *         winning hand for scoring purposes under this entry point.
 */
inline FuHan::Result calculateFuHan(
  std::array<std::uint_fast8_t, 34u> const &concealed_hand,
  std::uint_fast8_t const winning_tile,
  std::uint_fast8_t const num_dora,
  FuHan::Context const context)
{
  std::array<std::uint_fast8_t, 34u> full_hand = concealed_hand;
  ++full_hand[winning_tile];

  std::uint_fast8_t total_num_concealed_tiles = 0u;
  for (std::uint_fast8_t const count : full_hand) {
    if (count != 0u && count != 2u) {
      return FuHan::Result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
    }
    total_num_concealed_tiles += count;
  }
  if (total_num_concealed_tiles != 14u) {
    return FuHan::Result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
  }

  std::uint_fast8_t yakuman_multiplier = 0u;
  yakuman_multiplier += isTenhou(context) ? 1u : 0u;
  yakuman_multiplier += isChiihou(context) ? 1u : 0u;
  yakuman_multiplier += checkTsuuiisou(full_hand);
  if (yakuman_multiplier >= 1u) {
    return FuHan::Result{
      .fu = 25u,
      .han = 0u,
      .yakuman_multiplier = yakuman_multiplier,
    };
  }

  std::uint_fast8_t han = 2u;
  han += checkMenzenTsumo(context);
  han += checkTanyao(full_hand);
  han += checkHonroutou(full_hand);
  han += checkIsshoku(full_hand);
  han += FuHan::calculateSituationalHan_(context);
  if (han > UINT_FAST8_MAX - num_dora) {
    throw std::overflow_error("Han overflow: `han + num_dora` exceeds `UINT_FAST8_MAX`.");
  }
  han += num_dora;
  return FuHan::Result{
    .fu = 25u,
    .han = han,
    .yakuman_multiplier = 0u,
  };
}

}; // namespace FuHan::SevenPairs_

#endif // !defined(FUHAN_SEVEN_PAIRS_FUHAN_HPP_INCLUDE_GUARD)
