// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#if !defined(FUHAN_STANDARD_FUHAN_HPP_INCLUDE_GUARD)
#define FUHAN_STANDARD_FUHAN_HPP_INCLUDE_GUARD

#include "../types.hpp"
#include "decomposition.hpp"
#include "internal.hpp"
#include "../internal.hpp"
#include <sstream>
#include <algorithm>
#include <vector>
#include <array>
#include <utility>
#include <stdexcept>
#include <cstdint>
#include <cassert>


namespace FuHan::Standard_{

/**
 * @brief Tests whether a hand is closed (menzen).
 *
 * A hand is *closed* iff the player has made no call that exposes any
 * tile to the other players. Concretely, this means there is no chii,
 * no pon, and no open kan (daiminkan or kakan). Concealed kans (ankan)
 * are compatible with a closed hand and are therefore *not* considered
 * here: a hand with only ankan calls is still closed.
 *
 * This predicate operates purely on the meld-list inputs; the
 * concealed hand and the winning tile play no role and need not be
 * supplied. Whether the win is by tsumo or ron is likewise irrelevant
 * to the closed/open status of the hand itself.
 *
 * @param chii_list     For each valid sequence base tile (in `[0, 21)`),
 *                      the number of chii melds whose smallest tile is
 *                      that tile.
 * @param pon_list      For each tile in `[0, 34)`, the number of pon
 *                      melds of that tile.
 * @param open_kan_list For each tile in `[0, 34)`, the number of open
 *                      kans (daiminkan or kakan) of that tile.
 * @return `true` if every entry of @p chii_list, @p pon_list, and
 *         @p open_kan_list is `0` (i.e. the hand has no exposed call),
 *         otherwise `false`.
 */
inline bool isClosedHand(
  std::array<std::uint_fast8_t, 21u> const &chii_list,
  std::array<std::uint_fast8_t, 34u> const &pon_list,
  std::array<std::uint_fast8_t, 34u> const &open_kan_list)
{
  for (std::uint_fast8_t i = 0u; i < 21u; ++i) {
    if (chii_list[i] != 0u) {
      return false;
    }
  }
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (pon_list[i] != 0u) {
      return false;
    }
    if (open_kan_list[i] != 0u) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Tests whether a decomposition has the structural shape of pinfu.
 *
 * Checks whether the given block decomposition satisfies the
 * *shape-only* requirements of the pinfu yaku, namely:
 *
 * - The winning tile completes the hand on a two-sided wait
 *   (`WaitType::two_sided`).
 * - Every meld block is a sequence; that is, no block is a triplet or
 *   a quad (kan automatically disqualifies pinfu).
 * - The pair is not made of a *value tile* (yakuhai), i.e. it is none
 *   of:
 *   - the prevailing round wind (@p round_wind),
 *   - the player's seat wind (@p seat_wind), or
 *   - any of the three dragon tiles (white, green, red); these are
 *     identified by `base_tile >= 31` under the internal tile
 *     encoding.
 *
 * Conditions that pinfu also requires but are *not* checked here
 * include: the hand being closed (menzen) and at least one yaku being
 * established overall. The caller is responsible for enforcing those
 * separately (e.g. via `isClosedHand`).
 *
 * @param round_wind    The prevailing round wind, used to identify the
 *                      round-wind value tile.
 * @param seat_wind     The seat wind of the winning player, used to
 *                      identify the seat-wind value tile.
 * @param decomposition A block decomposition produced by
 *                      `decompose()`. Must contain exactly four meld
 *                      blocks and one pair block.
 * @return `true` if @p decomposition has pinfu shape under the rules
 *         above, otherwise `false`.
 */
inline bool isPinfuShape(
  FuHan::Wind const round_wind,
  FuHan::Wind const seat_wind,
  FuHan::Standard_::DecompositionElement const &decomposition)
{
  if (decomposition.wait_type != FuHan::Standard_::WaitType::two_sided) {
    return false;
  }

  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isTriplet() || block.isQuad()) {
      return false;
    }
    if (block.isPair()) {
      std::uint_fast8_t const base_tile = block.getBaseTile();
      if (base_tile == static_cast<std::uint_fast8_t>(round_wind)
          || base_tile == static_cast<std::uint_fast8_t>(seat_wind)
          || base_tile >= 31u) {
        return false;
      }
    }
  }

  return true;
}

/**
 * @brief Computes the fu (minipoints) of a single block decomposition.
 *
 * Evaluates the raw fu count for the given interpretation of a standard
 * winning hand. The result is *not* rounded up to the nearest 10; any
 * such rounding is the responsibility of the caller (it is typically
 * deferred until just before the base-score formula is applied).
 *
 * The calculation distinguishes the two flat pinfu cases first and
 * then accumulates fu contributions component by component:
 *
 * - **Pinfu fixed values.** If @p is_pinfu_shape is `true`:
 *   - tsumo + closed hand returns `20` fu (pinfu tsumo);
 *   - ron returns `30` fu (pinfu ron, i.e. 20 fu + 10 fu menzen-ron
 *     bonus baked in).
 *
 *   Other combinations fall through to the general calculation below;
 *   in particular, a pinfu shape on a tsumo with an open hand cannot
 *   occur because pinfu requires menzen, but the function does not
 *   enforce that here.
 *
 * - **General case.** Starts from `20` fu (futei) and adds:
 *   - **Per-block fu** for every meld in @p decomposition.blocks:
 *     - Sequences contribute `0`.
 *     - Triplets contribute `2` (open, simple), `4` (closed simple, or
 *       open yaochuu), or `8` (closed yaochuu). Yaochuu is determined
 *       by `FuHan::isYaochuuTile_`.
 *     - Quads contribute `8` (open simple), `16` (closed simple, or
 *       open yaochuu), or `32` (closed yaochuu).
 *   - **Pair fu** when the pair is a value tile:
 *     - `2` for a single yakuhai pair (round wind alone, seat wind
 *       alone, or one of the three dragon tiles, identified by
 *       `base_tile >= 31`).
 *     - `2` or `4` when the pair is simultaneously the round wind and
 *       the seat wind (double-wind / renpuu pair): `4` under the
 *       `double_wind_pair_4fu` rule, or `2` under `double_wind_pair_2fu`.
 *   - **Wait fu** based on `decomposition.wait_type`:
 *     - `0` for two-sided (ryanmen) and double-pair (shanpon).
 *     - `2` for inside (kanchan), edge (penchan), and pair (tanki).
 *   - **Menzen-ron bonus** of `10` if @p is_ron and @p is_closed_hand
 *     are both `true`.
 *   - **Tsumo bonus** of `2` if @p is_ron is `false`. A rinshan kaihou
 *     self-draw is treated as an ordinary self-draw and likewise
 *     receives this `2` fu bonus.
 *
 * @param round_wind     The prevailing round wind, used to identify
 *                       the round-wind value tile when the pair is
 *                       evaluated.
 * @param seat_wind      The seat wind of the winning player, used to
 *                       identify the seat-wind value tile when the
 *                       pair is evaluated.
 * @param decomposition  The block decomposition under consideration.
 *                       Must contain exactly four meld blocks and one
 *                       pair block, together with the wait type that
 *                       links the winning tile to the decomposition.
 * @param is_ron         `true` iff the win is by discard (ron). Used
 *                       for the menzen-ron and tsumo bonuses and for
 *                       the pinfu fixed-value branch.
 * @param is_closed_hand `true` iff the hand is menzen. Used for the
 *                       menzen-ron and pinfu fixed-value branches.
 * @param is_pinfu_shape `true` iff @p decomposition has pinfu shape,
 *                       as determined by `isPinfuShape`. Selects the
 *                       flat 20/30 fu branch when applicable.
 * @param rule           The active rule configuration. Selects the
 *                       fu awarded for a double-wind pair (2 vs 4).
 *
 * @return The fu (minipoints) of @p decomposition, *unrounded*. The
 *         value is at least `20` for any input.
 *
 * @throws std::logic_error If a block of an unknown type is
 *         encountered, or if @p decomposition.wait_type is not a valid
 *         wait kind. These indicate inconsistent inputs and should not
 *         occur for decompositions produced by `decompose()`.
 */
inline std::uint_fast8_t calculateFu(
  FuHan::Wind const round_wind,
  FuHan::Wind const seat_wind,
  FuHan::Standard_::DecompositionElement const &decomposition,
  bool const is_ron,
  bool const is_closed_hand,
  bool const is_pinfu_shape,
  FuHan::Rule const rule)
{
  if (is_pinfu_shape) {
    if (!is_ron && is_closed_hand) {
      return 20u;
    }
    if (is_ron) {
      return 30u;
    }
  }

  std::uint_fast8_t fu = 20u;

  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    std::uint_fast8_t const base_tile = block.getBaseTile();
    if (block.isSequence()) {
      continue;
    }
    if (block.isTriplet()) {
      if (block.isOpen()) {
        fu += FuHan::isYaochuuTile_(base_tile) ? 4u : 2u;
      }
      else {
        fu += FuHan::isYaochuuTile_(base_tile) ? 8u : 4u;
      }
      continue;
    }
    if (block.isQuad()) {
      if (block.isOpen()) {
        fu += FuHan::isYaochuuTile_(base_tile) ? 16u : 8u;
      }
      else {
        fu += FuHan::isYaochuuTile_(base_tile) ? 32u : 16u;
      }
      continue;
    }
    if (block.isPair()) {
      std::uint_fast8_t const round_wind_ = std::to_underlying(round_wind);
      std::uint_fast8_t const seat_wind_ = std::to_underlying(seat_wind);
      if (base_tile == round_wind_ || base_tile == seat_wind_ || base_tile >= 31u) {
        bool const is_double_wind_pair = base_tile == round_wind_ && base_tile == seat_wind_;
        fu += is_double_wind_pair ? (isDoubleWindPair4Fu(rule) ? 4u : 2u) : 2u;
      }
      continue;
    }
    throw std::logic_error("The control flow must not reach here.");
  }

  switch (decomposition.wait_type) {
    case FuHan::Standard_::WaitType::two_sided:
    case FuHan::Standard_::WaitType::double_pair:
      break;
    case FuHan::Standard_::WaitType::inside:
    case FuHan::Standard_::WaitType::edge:
    case FuHan::Standard_::WaitType::pair:
      fu += 2u;
      break;
    default:
      throw std::logic_error("The control flow must not reach here.");
  }

  if (is_ron && is_closed_hand) {
    fu += 10u;
  }

  if (!is_ron) {
    fu += 2u;
  }

  return fu;
}

/**
 * @brief Tests whether the decomposition establishes Daisangen (big three dragons, 大三元).
 *
 * Daisangen is the yakuman that requires the hand to contain a triplet or
 * quad of each of the three dragon tiles (white, green, and red, with tile
 * indices 31, 32, and 33). The pair and the remaining meld are unrestricted.
 *
 * The check counts the meld blocks of @p decomposition whose base tile is a
 * dragon tile; both open and closed triplets and quads qualify, since
 * Daisangen does not require the dragon melds to be concealed. Because a
 * standard decomposition contains exactly four meld blocks and there are
 * only three distinct dragon tiles (and thus at most three dragon melds),
 * a count of three is both necessary and sufficient.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 * @return `1` if @p decomposition establishes Daisangen, otherwise `0`.
 *         The return value is the yakuman multiplier contributed by this
 *         yaku and can be added directly to a running multiplier total.
 */
inline std::uint_fast8_t checkDaisangen(FuHan::Standard_::DecompositionElement const &decomposition)
{
  std::uint_fast8_t count = 0u;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isTriplet() || block.isQuad()) {
      std::uint_fast8_t const base_tile = block.getBaseTile();
      if (base_tile == 31u || base_tile == 32u || base_tile == 33u) {
        ++count;
      }
    }
  }

  return count >= 3u ? 1u : 0u;
}

/**
 * @brief Tests whether the decomposition establishes Suuankou (four concealed triplets, 四暗刻).
 *
 * Suuankou is the yakuman that requires the hand to contain four concealed
 * triplets or quads (ankou / ankan). The remaining block is the pair, which
 * is unrestricted. The wait further distinguishes two variants:
 *
 * - **Suuankou tanki (single-wait four concealed triplets, 四暗刻単騎):** the win
 *   completes the pair (`WaitType::pair`). This is a double yakuman in this
 *   library and contributes a multiplier of `2`.
 * - **Suuankou (plain, 四暗刻):** the win completes one of the four triplets while
 *   the pair was already complete. This contributes a multiplier of `1`.
 *
 * The check counts the meld blocks of @p decomposition that are triplets or
 * quads and are *not* open. The classification of the winning meld block as
 * open or closed (in particular, whether a triplet completed by ron is
 * treated as a minkou and therefore disqualifies Suuankou) is delegated to
 * the decomposition: this function relies on `Block::isOpen()` alone and
 * does not separately consider the tsumo/ron status of the win.
 *
 * Because a standard decomposition contains exactly four meld blocks, a
 * count of four is both necessary and sufficient for the meld condition.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 * @param rule          The active rule configuration. When it selects
 *                      `double_yakuman_disabled`, Suuankou tanki is
 *                      capped to a multiplier of `1` instead of `2`.
 * @return `2` if @p decomposition establishes Suuankou tanki and @p rule
 *         selects `double_yakuman_enabled`; `1` if it establishes
 *         Suuankou tanki under `double_yakuman_disabled` or plain
 *         Suuankou; and `0` otherwise. The return value is the yakuman
 *         multiplier contributed by this yaku and can be added directly
 *         to a running multiplier total.
 */
inline std::uint_fast8_t checkSuuankou(
  FuHan::Standard_::DecompositionElement const &decomposition,
  FuHan::Rule const rule)
{
  std::uint_fast8_t count = 0u;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if ((block.isTriplet() || block.isQuad()) && !block.isOpen()) {
      ++count;
    }
  }

  if (count < 4u) {
    return 0u;
  }
  bool const is_tanki = decomposition.wait_type == FuHan::Standard_::WaitType::pair;
  return is_tanki && isDoubleYakumanEnabled(rule) ? 2u : 1u;
}

/**
 * @brief Tests whether the decomposition establishes Tsuuiisou (all honors, 字一色).
 *
 * Tsuuiisou is the yakuman that requires every tile in the hand to be an
 * honor tile (wind or dragon). With tile indices 0-26 reserved for the
 * numbered suits (manzu, pinzu, souzu) and 27-33 for the honors (winds and
 * dragons), the condition reduces to checking that every block has a base
 * tile index of at least 27. Because honors cannot form sequences, a
 * Tsuuiisou hand necessarily consists of triplets, quads, and a pair; the
 * shape constraint is implied by the base-tile check rather than enforced
 * separately.
 *
 * The check is performed against every block in @p decomposition, including
 * the pair block. Both open and closed melds qualify; Tsuuiisou does not
 * require the hand to be concealed.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 * @return `1` if @p decomposition establishes Tsuuiisou, otherwise `0`.
 *         The return value is the yakuman multiplier contributed by this
 *         yaku and can be added directly to a running multiplier total.
 */
inline std::uint_fast8_t checkTsuuiisou(FuHan::Standard_::DecompositionElement const &decomposition)
{
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.getBaseTile() < 27u) {
      return 0u;
    }
  }
  return 1u;
}

/**
 * @brief Tests whether the decomposition establishes Ryuuiisou (all green, 緑一色).
 *
 * Ryuuiisou is the yakuman that requires every tile in the hand to be one
 * of the "green" tiles: 2s, 3s, 4s, 6s, 8s, and the green dragon (hatsu).
 * With the library's tile encoding, these correspond to the tile indices
 * 19, 20, 21, 23, 25, and 32, respectively.
 *
 * Each block in @p decomposition is checked twice:
 * - Triplets, quads, and the pair are accepted only when their base tile
 *   is one of the six green tile indices listed above. This covers the
 *   pair as well as every non-sequence meld.
 * - Sequences are additionally constrained to have base tile `19`, i.e.
 *   the 2s-3s-4s run, because that is the only sequence composed entirely
 *   of green tiles (3s-4s-5s, 4s-5s-6s, 6s-7s-8s and 7s-8s-9s all contain
 *   a non-green tile).
 *
 * Both open and closed melds qualify; Ryuuiisou does not require the hand
 * to be concealed.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 * @return `1` if @p decomposition establishes Ryuuiisou, otherwise `0`.
 *         The return value is the yakuman multiplier contributed by this
 *         yaku and can be added directly to a running multiplier total.
 */
inline std::uint_fast8_t checkRyuuiisou(FuHan::Standard_::DecompositionElement const &decomposition)
{
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    std::uint_fast8_t const base_tile = block.getBaseTile();
    if (block.isSequence() && base_tile != 19u) {
      return 0u;
    }
    if (base_tile != 19u
        && base_tile != 20u
        && base_tile != 21u
        && base_tile != 23u
        && base_tile != 25u
        && base_tile != 32u) {
      return 0u;
    }
  }
  return 1u;
}

/**
 * @brief Tests whether the decomposition establishes Chinroutou (all terminals, 清老頭).
 *
 * Chinroutou is the yakuman that requires every tile in the hand to be a
 * terminal tile, i.e. a 1 or a 9 of any numbered suit. With the library's
 * tile encoding, these correspond to the tile indices 0, 8, 9, 17, 18, and
 * 26 (1m, 9m, 1p, 9p, 1s, and 9s, respectively). Honor tiles are *not*
 * permitted; a hand consisting of terminals and honors is Honroutou
 * (混老頭), not Chinroutou.
 *
 * Each block in @p decomposition is checked as follows:
 * - If any block is a sequence, the hand cannot be Chinroutou, since no
 *   three consecutive numbered tiles are all terminals.
 * - For triplets, quads, and the pair, the base tile must be one of the
 *   six terminal indices listed above.
 *
 * Both open and closed melds qualify; Chinroutou does not require the hand
 * to be concealed.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 * @return `1` if @p decomposition establishes Chinroutou, otherwise `0`.
 *         The return value is the yakuman multiplier contributed by this
 *         yaku and can be added directly to a running multiplier total.
 */
inline std::uint_fast8_t checkChinroutou(FuHan::Standard_::DecompositionElement const &decomposition)
{
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isSequence()) {
      return 0u;
    }
    std::uint_fast8_t const base_tile = block.getBaseTile();
    if (base_tile != 0u
        && base_tile != 8u
        && base_tile != 9u
        && base_tile != 17u
        && base_tile != 18u
        && base_tile != 26u) {
      return 0u;
    }
  }
  return 1u;
}

/**
 * @brief Tests whether the decomposition establishes Suushii (four winds).
 *
 * Suushii is the family of yakuman that requires the hand to be built
 * around all four wind tiles (East, South, West, North, with tile indices
 * 27-30). It has two variants:
 *
 * - **Daisuushii (大四喜, big four winds):** the hand contains a triplet
 *   or quad of *each* of the four winds. The pair is then necessarily a
 *   non-wind tile. This is a double yakuman in this library and
 *   contributes a multiplier of `2`.
 * - **Shousuushii (小四喜, little four winds):** the hand contains
 *   triplets or quads of three of the winds and a pair of the remaining
 *   wind. This contributes a multiplier of `1`.
 *
 * The function counts, among the blocks of @p decomposition, how many
 * triplets/quads have a wind base tile (`triplet_count`) and how many
 * pair blocks have a wind base tile (`pair_count`). Daisuushii is
 * recognised by `triplet_count == 4`, and Shousuushii by
 * `triplet_count == 3 && pair_count == 1`. Blocks whose base tile is not
 * a wind are ignored.
 *
 * Both open and closed melds qualify; neither Suushii variant requires
 * the hand to be concealed.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 * @param rule          The active rule configuration. When it selects
 *                      `double_yakuman_disabled`, Daisuushii is capped
 *                      to a multiplier of `1` instead of `2`.
 * @return `2` if @p decomposition establishes Daisuushii and @p rule
 *         selects `double_yakuman_enabled`; `1` if it establishes
 *         Daisuushii under `double_yakuman_disabled` or Shousuushii;
 *         and `0` otherwise. The return value is the yakuman multiplier
 *         contributed by this yaku and can be added directly to a
 *         running multiplier total.
 */
inline std::uint_fast8_t checkSuushii(
  FuHan::Standard_::DecompositionElement const &decomposition,
  FuHan::Rule const rule)
{
  std::uint_fast8_t triplet_count = 0u;
  std::uint_fast8_t pair_count = 0u;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    std::uint_fast8_t const base_tile = block.getBaseTile();
    if (!(27u <= base_tile && base_tile <= 30u)) {
      continue;
    }
    if (block.isTriplet() || block.isQuad()) {
      ++triplet_count;
    }
    else if (block.isPair()) {
      ++pair_count;
    }
  }

  return triplet_count == 4u ? (isDoubleYakumanEnabled(rule) ? 2u : 1u) : (triplet_count == 3u && pair_count == 1u ? 1u : 0u);
}

/**
 * @brief Tests whether the decomposition establishes Suukantsu (four kans, 四槓子).
 *
 * Suukantsu is the yakuman that requires the hand to contain four quads
 * (kans). The pair is unrestricted, and any of the four kans may be open
 * (minkan / shouminkan) or closed (ankan); Suukantsu does not require the
 * hand to be concealed.
 *
 * The check counts the meld blocks of @p decomposition that are quads.
 * Because a standard decomposition contains exactly four meld blocks, a
 * count of four is both necessary and sufficient.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 * @return `1` if @p decomposition establishes Suukantsu, otherwise `0`.
 *         The return value is the yakuman multiplier contributed by this
 *         yaku and can be added directly to a running multiplier total.
 */
inline std::uint_fast8_t checkSuukantsu(FuHan::Standard_::DecompositionElement const &decomposition)
{
  std::uint_fast8_t count = 0u;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isQuad()) {
      ++count;
    }
  }
  return count >= 4u ? 1u : 0u;
}

/**
 * @brief Tests whether the hand establishes Chuuren Poutou (nine gates, 九蓮宝燈).
 *
 * Chuuren Poutou is the yakuman that requires a fully concealed hand
 * composed entirely of a single numbered suit (manzu, pinzu, or souzu)
 * whose tile counts, after including the winning tile, match the
 * canonical "1112345678999 + one extra" shape. It has two variants
 * distinguished by which tile is the winning tile:
 *
 * - **Junsei Chuuren Poutou (pure nine gates, 純正九蓮宝燈):** the
 *   thirteen-tile waiting shape is exactly `3-1-1-1-1-1-1-1-3` (three
 *   1s, one each of 2-8, and three 9s), and the win completes the
 *   9-way wait on any tile of the suit. This is a double yakuman in
 *   this library and contributes a multiplier of `2`.
 * - **Plain Chuuren Poutou (九蓮宝燈):** the completed 14-tile shape is
 *   `1112345678999` plus a duplicate of one of the nine ranks, but the
 *   thirteen-tile waiting shape was not pure (i.e. the winning tile is
 *   not on a 9-way wait). This contributes a multiplier of `1`.
 *
 * Unlike the other yakuman checks in this file, this function operates
 * directly on the raw 34-element tile-count vector rather than on a
 * `DecompositionElement`. This avoids any dependence on which
 * decomposition is being considered; Chuuren Poutou is a property of
 * the multiset of tiles alone (within a single numbered suit) and is
 * independent of how the hand is partitioned into melds.
 *
 * The function takes a copy of @p concealed_hand, adds @p winning_tile
 * to it, and then:
 * -# Verifies that every present tile shares a single numbered suit
 *    (returns `0` if any honor tile is present or if more than one
 *    suit appears).
 * -# Compares the per-rank counts of that suit against the canonical
 *    pure shape (3-1-1-1-1-1-1-1-3) with the winning tile removed; a
 *    match yields a multiplier of `2`.
 * -# Otherwise compares the per-rank counts against the nine non-pure
 *    completed shapes (canonical shape plus one extra tile at each of
 *    ranks 1 through 9); a match yields a multiplier of `1`.
 *
 * Chuuren Poutou requires a fully concealed hand: if @p is_closed_hand
 * is `false`, the function returns `0` immediately without inspecting
 * the tile counts. Because the function operates on the concealed
 * tile-count vector alone, @p is_closed_hand is the only signal it has
 * about open melds, and it must be supplied by the caller.
 *
 * @param concealed_hand The 34-element tile-count vector of the
 *                       concealed portion of the hand, *not* yet
 *                       including the winning tile.
 * @param winning_tile   The index (0-33) of the winning tile.
 * @param is_closed_hand `true` iff the hand is menzen (no chii, pon,
 *                       or open/added kan has been called). The
 *                       function returns `0` when this is `false`,
 *                       since Chuuren Poutou cannot be established
 *                       with an open hand.
 * @param rule           The active rule configuration. When it selects
 *                       `double_yakuman_disabled`, Junsei Chuuren
 *                       Poutou is capped to a multiplier of `1` instead
 *                       of `2`.
 *
 * @return `2` if the hand establishes Junsei Chuuren Poutou and @p rule
 *         selects `double_yakuman_enabled`; `1` if it establishes
 *         Junsei Chuuren Poutou under `double_yakuman_disabled` or plain
 *         Chuuren Poutou; and `0` otherwise. The return value is the
 *         yakuman multiplier contributed by this yaku and can be added
 *         directly to a running multiplier total.
 */
inline std::uint_fast8_t checkChuurenPoutou(
  std::array<std::uint_fast8_t, 34u> const &concealed_hand,
  std::uint_fast8_t const winning_tile,
  bool const is_closed_hand,
  FuHan::Rule const rule)
{
  if (!is_closed_hand) {
    return 0u;
  }

  std::array<std::uint_fast8_t, 34u> hand = concealed_hand;
  ++hand[winning_tile];

  std::uint_fast8_t color = UINT_FAST8_MAX;
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (hand[i] == 0u) {
      continue;
    }
    std::uint_fast8_t const color_ = i / 9u;
    if (color_ == 3u) {
      return 0u;
    }
    if (color == UINT_FAST8_MAX) {
      color = color_;
      continue;
    }
    if (color_ != color) {
      return 0u;
    }
  }
  assert((color != UINT_FAST8_MAX));

  std::array<std::uint_fast8_t, 9u> counts{};
  for (std::uint_fast8_t i = 0u; i < 9u; ++i) {
    counts[i] = hand[9u * color + i];
  }

  --counts[winning_tile % 9u];
  if (counts[0u] == 3u && counts[1u] == 1u && counts[2u] == 1u && counts[3u] == 1u && counts[4u] == 1u && counts[5u] == 1u && counts[6u] == 1u && counts[7u] == 1u && counts[8u] == 3u) {
    return isDoubleYakumanEnabled(rule) ? 2u : 1u;
  }
  ++counts[winning_tile % 9u];

  if (counts[0u] == 4u && counts[1u] == 1u && counts[2u] == 1u && counts[3u] == 1u && counts[4u] == 1u && counts[5u] == 1u && counts[6u] == 1u && counts[7u] == 1u && counts[8u] == 3u) {
    return 1u;
  }
  if (counts[0u] == 3u && counts[1u] == 2u && counts[2u] == 1u && counts[3u] == 1u && counts[4u] == 1u && counts[5u] == 1u && counts[6u] == 1u && counts[7u] == 1u && counts[8u] == 3u) {
    return 1u;
  }
  if (counts[0u] == 3u && counts[1u] == 1u && counts[2u] == 2u && counts[3u] == 1u && counts[4u] == 1u && counts[5u] == 1u && counts[6u] == 1u && counts[7u] == 1u && counts[8u] == 3u) {
    return 1u;
  }
  if (counts[0u] == 3u && counts[1u] == 1u && counts[2u] == 1u && counts[3u] == 2u && counts[4u] == 1u && counts[5u] == 1u && counts[6u] == 1u && counts[7u] == 1u && counts[8u] == 3u) {
    return 1u;
  }
  if (counts[0u] == 3u && counts[1u] == 1u && counts[2u] == 1u && counts[3u] == 1u && counts[4u] == 2u && counts[5u] == 1u && counts[6u] == 1u && counts[7u] == 1u && counts[8u] == 3u) {
    return 1u;
  }
  if (counts[0u] == 3u && counts[1u] == 1u && counts[2u] == 1u && counts[3u] == 1u && counts[4u] == 1u && counts[5u] == 2u && counts[6u] == 1u && counts[7u] == 1u && counts[8u] == 3u) {
    return 1u;
  }
  if (counts[0u] == 3u && counts[1u] == 1u && counts[2u] == 1u && counts[3u] == 1u && counts[4u] == 1u && counts[5u] == 1u && counts[6u] == 2u && counts[7u] == 1u && counts[8u] == 3u) {
    return 1u;
  }
  if (counts[0u] == 3u && counts[1u] == 1u && counts[2u] == 1u && counts[3u] == 1u && counts[4u] == 1u && counts[5u] == 1u && counts[6u] == 1u && counts[7u] == 2u && counts[8u] == 3u) {
    return 1u;
  }
  if (counts[0u] == 3u && counts[1u] == 1u && counts[2u] == 1u && counts[3u] == 1u && counts[4u] == 1u && counts[5u] == 1u && counts[6u] == 1u && counts[7u] == 1u && counts[8u] == 4u) {
    return 1u;
  }
  return 0u;
}

/**
 * @brief Checks the Menzen tsumo (門前清自摸和) yaku and returns its han.
 *
 * Menzen tsumo is a 1-han yaku that is established when the win is
 * achieved by self-draw (tsumo) on a fully concealed hand (menzen),
 * i.e. a hand without any chii, pon, or open kan (ankan does not
 * break menzen). This function evaluates the yaku from the two
 * boolean predicates that together encode this condition.
 *
 * The yaku is established iff @p is_ron is `false` (the win is by
 * self-draw rather than by claiming a discard) *and* @p is_closed_hand
 * is `true` (the hand is menzen). In that case the function returns
 * `1`. In every other case it returns `0`.
 *
 * @param is_ron         `true` iff the win is by discard (ron).
 * @param is_closed_hand `true` iff the hand is menzen (no chii, pon,
 *                       or open kan). Ankan does not break menzen and
 *                       should not flip this flag to `false`.
 * @return `1` when Menzen tsumo is established (tsumo on a menzen
 *         hand); `0` otherwise.
 */
inline std::uint_fast8_t checkMenzenTsumo(bool const is_ron, bool const is_closed_hand)
{
  return (!is_ron && is_closed_hand) ? 1u : 0u;
}

/**
 * @brief Counts the total han contributed by yakuhai triplets and quads.
 *
 * Yakuhai (役牌) are value-tile melds that each contribute 1 han when
 * present as a triplet or quad in the hand:
 *
 * - The three dragon tiles (white, green, red — tile indices 31, 32,
 *   and 33), which are always yakuhai irrespective of the seat or
 *   round wind.
 * - The round wind, identified by @p round_wind.
 * - The seat wind, identified by @p seat_wind.
 *
 * When a single meld is simultaneously the round wind *and* the seat
 * wind (i.e. the round wind and seat wind coincide), it counts as both
 * yakuhai (double-wind, 連風牌) and contributes `2` han rather than
 * `1`.
 *
 * The function iterates over the meld blocks of @p decomposition and
 * accumulates `1` han for each non-coincident yakuhai triplet/quad and
 * `2` han for a coincident double-wind triplet/quad. Sequences and the
 * pair block contribute no han here; pair yakuhai contribute fu but
 * not han and are handled separately by `calculateFu`. Open and closed
 * melds are treated identically for yakuhai purposes.
 *
 * @param round_wind    The prevailing round wind, used to identify the
 *                      round-wind value tile.
 * @param seat_wind     The seat wind of the winning player, used to
 *                      identify the seat-wind value tile.
 * @param decomposition The block decomposition under consideration.
 *                      Only its meld blocks are examined.
 *
 * @return The total han contributed by yakuhai triplets and quads in
 *         @p decomposition. The minimum is `0`; the maximum is `5`,
 *         attained when all four melds are yakuhai and one of them is
 *         a double-wind.
 */
inline std::uint_fast8_t countYakuhaiHan(
  FuHan::Wind const round_wind,
  FuHan::Wind const seat_wind,
  FuHan::Standard_::DecompositionElement const &decomposition)
{
  std::uint_fast8_t han = 0u;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isTriplet() || block.isQuad()) {
      std::uint_fast8_t const base_tile = block.getBaseTile();
      std::uint_fast8_t const round_wind_ = std::to_underlying(round_wind);
      std::uint_fast8_t const seat_wind_ = std::to_underlying(seat_wind);
      if (base_tile == round_wind_ || base_tile == seat_wind_ || base_tile >= 31u) {
        han += (base_tile == round_wind_ && base_tile == seat_wind_) ? 2u : 1u;
      }
    }
  }
  return han;
}

/**
 * @brief Tests whether the decomposition establishes Tanyao (all simples, 断么九).
 *
 * Tanyao is the 1-han yaku that requires every tile in the hand to be a
 * simple (2-8 of a numbered suit). Terminals (1 and 9 of any suit) and
 * honors (winds and dragons) are forbidden — equivalently, no block may
 * contain a yaochuu tile.
 *
 * Each block in @p decomposition is checked as follows:
 * - **Sequences:** the run must lie entirely within ranks 2-8. With the
 *   library's encoding, the base tile of a sequence is its lowest rank,
 *   so the run is `[base_rank, base_rank + 2]`; the sequence is rejected
 *   when `base_rank == 0` (1-2-3, which contains a terminal 1) or
 *   `base_rank == 6` (7-8-9, which contains a terminal 9). All other
 *   sequence base ranks are valid.
 * - **Triplets, quads, and the pair:** the base tile must not be a
 *   yaochuu tile, as determined by `FuHan::isYaochuuTile_` (i.e. it must
 *   be a 2-8 of a numbered suit).
 *
 * Both open and closed melds qualify; whether or not kuitan (open
 * tanyao) is allowed by the rule set in use is the responsibility of
 * the caller and is not enforced here.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 * @return `1` if @p decomposition establishes Tanyao, otherwise `0`.
 *         The return value is the han contributed by this yaku and can
 *         be added directly to a running han total.
 */
inline std::uint_fast8_t checkTanyao(FuHan::Standard_::DecompositionElement const &decomposition)
{
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    std::uint_fast8_t const base_tile = block.getBaseTile();
    std::uint_fast8_t const color = base_tile / 9u;
    std::uint_fast8_t const base_rank = color != 3u ? base_tile % 9u : UINT_FAST8_MAX;
    if (block.isSequence()) {
      if (base_rank == 0u || base_rank == 6u) {
        return 0u;
      }
      continue;
    }
    if (FuHan::isYaochuuTile_(base_tile)) {
      return 0u;
    }
  }
  return 1u;
}

/**
 * @brief Counts the han contributed by Iipeikou and Ryanpeikou.
 *
 * Iipeikou (一盃口) and Ryanpeikou (二盃口) are the yaku awarded for
 * pairs of identical sequences (same suit, same starting rank) within
 * a single decomposition:
 *
 * - **Iipeikou** is one pair of identical sequences, contributing
 *   `1` han.
 * - **Ryanpeikou** is two such pairs, contributing `3` han.
 *
 * Both yaku require a concealed hand: if @p is_closed_hand is `false`,
 * the function returns `0` immediately. Note also that the two yaku
 * are mutually exclusive — a hand satisfying Ryanpeikou is *not*
 * additionally counted for Iipeikou.
 *
 * The implementation tallies sequences by a 0-based sequence ID
 * `7 * color + base_rank`, where `color` is the suit index (0-2) and
 * `base_rank` is the rank of the lowest tile in the run (0-6). The
 * possible sequence IDs therefore form the range `[0, 21)`.
 *
 * After counting, the function classifies the multiset of sequences:
 * - If any single sequence ID occurs four times (four copies of the
 *   same run, possible only when both pairs are identical), the result
 *   is treated as Ryanpeikou and `3` is returned.
 * - Otherwise the number of sequence IDs occurring at least twice is
 *   the peikou count: `0` peikou yields `0`, `1` peikou yields `1`
 *   (Iipeikou), and `2` peikou yields `3` (Ryanpeikou).
 *
 * @param decomposition  The block decomposition to test. Only sequence
 *                       blocks contribute; triplets, quads, and the
 *                       pair are ignored.
 * @param is_closed_hand `true` iff the hand is menzen. The function
 *                       returns `0` when this is `false`, since
 *                       neither Iipeikou nor Ryanpeikou can be
 *                       established with an open hand.
 *
 * @return `3` for Ryanpeikou, `1` for Iipeikou, or `0` otherwise. The
 *         return value is the han contributed by this yaku family and
 *         can be added directly to a running han total.
 */
inline std::uint_fast8_t checkPeikou(
  FuHan::Standard_::DecompositionElement const &decomposition, bool const is_closed_hand)
{
  if (!is_closed_hand) {
    return 0u;
  }

  std::array<std::uint_fast8_t, 21u> sequence_counts{};
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isSequence()) {
      std::uint_fast8_t const base_tile = block.getBaseTile();
      std::uint_fast8_t const color = base_tile / 9u;
      std::uint_fast8_t const base_rank = color != 3u ? base_tile % 9u : UINT_FAST8_MAX;
      assert((base_rank != UINT_FAST8_MAX));
      std::uint_fast8_t const sequence_id = color != 3u ? 7u * color + base_rank : UINT_FAST8_MAX;
      assert((sequence_id != UINT_FAST8_MAX));
      ++sequence_counts[sequence_id];
    }
  }

  std::uint_fast8_t peikou_count = 0u;
  for (std::uint_fast8_t count : sequence_counts) {
    if (count >= 4u) {
      return 3u;
    }
    if (count >= 2u) {
      ++peikou_count;
    }
  }
  return peikou_count == 2u ? 3u : (peikou_count == 1u ? 1u : 0u);
}

/**
 * @brief Tests whether the decomposition establishes Pinfu (no-fu hand, 平和).
 *
 * Pinfu is the 1-han yaku awarded for a fully concealed hand whose
 * decomposition has *pinfu shape*: four sequences, a non-yakuhai pair,
 * and a two-sided (ryanmen) wait. The shape itself is determined once
 * per decomposition by `isPinfuShape` (which takes the round and seat
 * winds into account when evaluating the pair); this function merely
 * combines that result with the menzen requirement.
 *
 * The check therefore reduces to the logical conjunction of its two
 * arguments: Pinfu is established iff @p is_closed_hand and
 * @p is_pinfu_shape are both `true`.
 *
 * @param is_closed_hand `true` iff the hand is menzen. Pinfu cannot be
 *                       established with an open hand.
 * @param is_pinfu_shape `true` iff the decomposition under
 *                       consideration has pinfu shape, as determined
 *                       by `isPinfuShape`.
 *
 * @return `1` if @p decomposition establishes Pinfu, otherwise `0`.
 *         The return value is the han contributed by this yaku and can
 *         be added directly to a running han total.
 */
inline std::uint_fast8_t checkPinfu(bool const is_closed_hand, bool const is_pinfu_shape)
{
  return (is_closed_hand && is_pinfu_shape) ? 1u : 0u;
}

/**
 * @brief Counts the han contributed by the yaochuu-meld yaku family.
 *
 * This function unifies the three yaku that require every block of the
 * decomposition to contain at least one yaochuu tile (terminal or
 * honor):
 *
 * - **Chanta (混全帯么九):** every meld and the pair contains at least
 *   one yaochuu tile, and at least one meld is a sequence and at least
 *   one block contains an honor tile. Contributes `2` han when closed
 *   and `1` han when open (kuisagari).
 * - **Junchan (純全帯么九):** like Chanta, but no honor tile appears
 *   anywhere in the hand (every yaochuu tile is a terminal).
 *   Contributes `3` han when closed and `2` han when open (kuisagari).
 * - **Honroutou (混老頭):** every block consists exclusively of
 *   terminals and honors (no sequence is present). Contributes `2`
 *   han, irrespective of whether the hand is closed or open
 *   (Honroutou has no kuisagari).
 *
 * Only one of these yaku is reported at a time; the three are mutually
 * exclusive because Honroutou requires the absence of sequences while
 * Chanta and Junchan each require at least one sequence, and Junchan
 * additionally requires the absence of honors that Chanta would mark.
 *
 * The implementation walks the blocks of @p decomposition and applies
 * the following rules:
 * - Each sequence must have `base_rank == 0` (run 1-2-3) or
 *   `base_rank == 6` (run 7-8-9); any other sequence makes the entire
 *   family fail, and `0` is returned. When at least one sequence is
 *   present, Honroutou is ruled out (`is_honroutou` becomes `false`).
 * - Each non-sequence block (triplet, quad, or pair) must have a
 *   yaochuu base tile, as determined by `FuHan::isYaochuuTile_`. If any
 *   non-sequence block is not yaochuu, `0` is returned. When at least
 *   one non-sequence block uses an honor tile (`base_tile >= 27`),
 *   Junchan is ruled out (`is_pure` becomes `false`).
 *
 * After the scan, the result is determined by `is_honroutou`,
 * `is_pure`, and @p is_closed_hand, as described above. The
 * `is_pure && is_honroutou` combination is impossible by construction
 * (Honroutou requires no sequences, while pure-only configurations
 * with no sequences would also have no honors, leaving only a
 * terminal-only hand — Chinroutou — which is handled as a yakuman and
 * never reaches this function); if it nevertheless occurs, the
 * function throws `std::logic_error`.
 *
 * @param decomposition  The block decomposition to test.
 * @param is_closed_hand `true` iff the hand is menzen. Selects between
 *                       the closed-hand and open-hand (kuisagari)
 *                       values for Chanta and Junchan.
 *
 * @return The han contributed by this yaku family — `3` for closed
 *         Junchan, `2` for open Junchan, closed Chanta, or Honroutou,
 *         `1` for open Chanta, or `0` if none of the three is
 *         established. The return value can be added directly to a
 *         running han total.
 *
 * @throws std::logic_error If the supposedly mutually exclusive flags
 *         `is_pure` and `is_honroutou` end up both `true`. This
 *         indicates an inconsistent input and should not occur for
 *         decompositions produced by `decompose()`.
 */
inline std::uint_fast8_t checkYaochuuYaku(
  FuHan::Standard_::DecompositionElement const &decomposition, bool const is_closed_hand)
{
  bool is_pure = true;
  bool is_honroutou = true;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    std::uint_fast8_t const base_tile = block.getBaseTile();
    if (block.isSequence()) {
      std::uint_fast8_t const color = base_tile / 9u;
      std::uint_fast8_t const number = color != 3u ? base_tile % 9u : UINT_FAST8_MAX;
      assert((number != UINT_FAST8_MAX));
      if (number != 0u && number != 6u) {
        return 0u;
      }
      is_honroutou = false;
      continue;
    }
    if (!FuHan::isYaochuuTile_(base_tile)) {
      return 0u;
    }
    if (base_tile >= 27u) {
      is_pure = false;
    }
  }
  if (is_pure && is_honroutou) {
    throw std::logic_error("`is_pure` and `is_honroutou` are mutually exclusive.");
  }

  if (is_honroutou) {
    return 2u;
  }
  if (is_pure) {
    return is_closed_hand ? 3u : 2u;
  }
  return is_closed_hand ? 2u : 1u;
}

/**
 * @brief Tests whether the decomposition establishes Ikki Tsuukan (pure straight, 一気通貫).
 *
 * Ikki Tsuukan is the yaku awarded for completing the three sequences
 * 1-2-3, 4-5-6, and 7-8-9 within a single numbered suit. Its han value
 * depends on whether the hand is concealed (kuisagari):
 * - Closed hand: `2` han.
 * - Open hand: `1` han.
 *
 * The implementation tallies sequences by a 0-based sequence ID
 * `7 * color + base_rank`, where `color` is the suit index (0-2) and
 * `base_rank` is the rank of the lowest tile in the run (0-6). For each
 * of the three numbered suits, the function then tests whether at least
 * one copy of each of the three required sequences (IDs `7*color + 0`,
 * `7*color + 3`, and `7*color + 6`) is present. As soon as one suit
 * satisfies the condition, the appropriate han value is returned;
 * because a standard decomposition contains only four sequence slots,
 * the condition can be satisfied in at most one suit, so the search
 * order is irrelevant.
 *
 * @param decomposition  The block decomposition to test. Only sequence
 *                       blocks contribute; triplets, quads, and the
 *                       pair are ignored.
 * @param is_closed_hand `true` iff the hand is menzen. Selects between
 *                       the closed-hand (`2`) and open-hand (`1`,
 *                       kuisagari) han values.
 *
 * @return `2` if @p decomposition establishes Ikki Tsuukan and the
 *         hand is closed, `1` if it is established with an open hand,
 *         or `0` otherwise. The return value is the han contributed
 *         by this yaku and can be added directly to a running han
 *         total.
 */
inline std::uint_fast8_t checkIkkiTsuukan(
  FuHan::Standard_::DecompositionElement const &decomposition, bool const is_closed_hand)
{
  std::array<std::uint_fast8_t, 21u> sequence_counts{};
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (!block.isSequence()) {
      continue;
    }
    std::uint_fast8_t const base_tile = block.getBaseTile();
    std::uint_fast8_t const color = base_tile / 9u;
    std::uint_fast8_t const number = color != 3u ? base_tile % 9u : UINT_FAST8_MAX;
    assert((number != UINT_FAST8_MAX));
    std::uint_fast8_t const sequence_id = color != 3u ? 7u * color + number : UINT_FAST8_MAX;
    assert((sequence_id != UINT_FAST8_MAX));
    ++sequence_counts[sequence_id];
  }
  for (std::uint_fast8_t color = 0u; color < 3u; ++color) {
    if (sequence_counts[7u * color + 0u] >= 1u
        && sequence_counts[7u * color + 3u] >= 1u
        && sequence_counts[7u * color + 6u] >= 1u) {
      return is_closed_hand ? 2u : 1u;
    }
  }
  return 0u;
}

/**
 * @brief Tests whether the decomposition establishes Sanshoku Doujun (three-colour straight, 三色同順).
 *
 * Sanshoku Doujun is the yaku awarded for having the same numbered run
 * (e.g. 2-3-4) appear once in each of the three suits (manzu, pinzu,
 * souzu) within a single decomposition. Its han value depends on whether
 * the hand is concealed (kuisagari):
 * - Closed hand: `2` han.
 * - Open hand: `1` han.
 *
 * The implementation tallies sequences by a 0-based sequence ID
 * `7 * color + base_rank`, where `color` is the suit index (0-2) and
 * `base_rank` is the rank of the lowest tile in the run (0-6). For each
 * possible base rank in `[0, 7)`, the function then tests whether at
 * least one sequence with that base rank is present in every suit
 * (IDs `7*0 + base_rank`, `7*1 + base_rank`, `7*2 + base_rank`). As
 * soon as one base rank satisfies the condition, the appropriate han
 * value is returned; because a standard decomposition contains only
 * four sequence slots, the condition can be satisfied at most once, so
 * the iteration order over base ranks is irrelevant.
 *
 * @param decomposition  The block decomposition to test. Only sequence
 *                       blocks contribute; triplets, quads, and the
 *                       pair are ignored.
 * @param is_closed_hand `true` iff the hand is menzen. Selects between
 *                       the closed-hand (`2`) and open-hand (`1`,
 *                       kuisagari) han values.
 *
 * @return `2` if @p decomposition establishes Sanshoku Doujun and the
 *         hand is closed, `1` if it is established with an open hand,
 *         or `0` otherwise. The return value is the han contributed by
 *         this yaku and can be added directly to a running han total.
 */
inline std::uint_fast8_t checkSanshokuDoujun(
  FuHan::Standard_::DecompositionElement const &decomposition, bool const is_closed_hand)
{
  std::array<std::uint_fast8_t, 21u> sequence_counts{};
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (!block.isSequence()) {
      continue;
    }
    std::uint_fast8_t const base_tile = block.getBaseTile();
    std::uint_fast8_t const color = base_tile / 9u;
    std::uint_fast8_t const base_rank = color != 3u ? base_tile % 9u : UINT_FAST8_MAX;
    assert((base_rank != UINT_FAST8_MAX));
    std::uint_fast8_t const sequence_id = color != 3u ? 7u * color + base_rank : UINT_FAST8_MAX;
    assert((sequence_id != UINT_FAST8_MAX));
    ++sequence_counts[sequence_id];
  }

  for (std::uint_fast8_t base_rank = 0u; base_rank < 7u; ++base_rank) {
    if (sequence_counts[0u * 7u + base_rank] >= 1u
        && sequence_counts[1u * 7u + base_rank] >= 1u
        && sequence_counts[2u * 7u + base_rank] >= 1u) {
      return is_closed_hand ? 2u : 1u;
    }
  }
  return 0u;
}

/**
 * @brief Tests whether the decomposition establishes Sanshoku Doukou (three-colour triplets, 三色同刻).
 *
 * Sanshoku Doukou is the 2-han yaku awarded for having a triplet or quad
 * of the same rank appear in each of the three numbered suits (manzu,
 * pinzu, souzu) within a single decomposition. Both open and closed
 * triplets/quads qualify; Sanshoku Doukou has no kuisagari and its han
 * value is `2` whether the hand is concealed or open.
 *
 * The implementation tallies non-sequence numbered-suit blocks by their
 * tile index in a 27-element array (`9 * color + rank`). Honor blocks
 * (`color == 3`) are ignored, as honors cannot participate in
 * three-colour yaku. For each rank in `[0, 9)`, the function then tests
 * whether at least one triplet or quad of that rank is present in every
 * numbered suit. As soon as one rank satisfies the condition, `2` is
 * returned; because a standard decomposition contains only four
 * non-sequence meld slots and one pair, the condition can be satisfied
 * for at most one rank, so the iteration order over ranks is
 * irrelevant.
 *
 * The pair block is *not* counted: Sanshoku Doukou requires three
 * triplets/quads (one per suit), and the remaining block of the
 * decomposition must be the pair, which contributes neither to the yaku
 * nor to its detection.
 *
 * @param decomposition The block decomposition to test. Only triplet
 *                      and quad blocks of numbered suits contribute;
 *                      sequences, honor melds, and the pair are
 *                      ignored.
 *
 * @return `2` if @p decomposition establishes Sanshoku Doukou,
 *         otherwise `0`. The return value is the han contributed by
 *         this yaku and can be added directly to a running han total.
 */
inline std::uint_fast8_t checkSanshokuDoukou(
  FuHan::Standard_::DecompositionElement const &decomposition)
{
  std::array<std::uint_fast8_t, 27u> triplet_counts{};
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isTriplet() || block.isQuad()) {
      std::uint_fast8_t const base_tile = block.getBaseTile();
      std::uint_fast8_t const color = base_tile / 9u;
      if (color == 3u) {
        continue;
      }
      assert((base_tile < 27u));
      ++triplet_counts[base_tile];
    }
  }

  for (std::uint_fast8_t rank = 0u; rank < 9u; ++rank) {
    if (triplet_counts[0u * 9u + rank] >= 1u && triplet_counts[1u * 9u + rank] >= 1u && triplet_counts[2u * 9u + rank] >= 1u) {
      return 2u;
    }
  }
  return 0u;
}

/**
 * @brief Tests whether the decomposition establishes Sankantsu (three kans, 三槓子).
 *
 * Sankantsu is the 2-han yaku awarded for having exactly three quads
 * (kans) in the hand. Any combination of open kans (daiminkan / kakan)
 * and closed kans (ankan) qualifies; Sankantsu has no kuisagari and its
 * han value is `2` whether the hand is concealed or open.
 *
 * The check counts the meld blocks of @p decomposition that are quads.
 * A count of exactly three yields the yaku; four quads is Suukantsu
 * (a yakuman) and is handled separately by `checkSuukantsu`, so this
 * function deliberately returns `0` in that case to avoid
 * double-counting.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 *
 * @return `2` if @p decomposition contains exactly three quads,
 *         otherwise `0`. The return value is the han contributed by
 *         this yaku and can be added directly to a running han total.
 */
inline std::uint_fast8_t checkSankantsu(FuHan::Standard_::DecompositionElement const &decomposition)
{
  std::uint_fast8_t count = 0u;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isQuad()) {
      ++count;
    }
  }
  return count == 3u ? 2u : 0u;
}

/**
 * @brief Tests whether the decomposition establishes Toitoi (all triplets, 対々和).
 *
 * Toitoi is the 2-han yaku awarded for a hand composed entirely of
 * triplets and/or quads (no sequences), plus a pair. Both open and
 * closed triplets/quads qualify; Toitoi has no kuisagari and its han
 * value is `2` whether the hand is concealed or open.
 *
 * The check simply scans the meld blocks of @p decomposition and
 * returns `0` as soon as any sequence is found; otherwise the hand
 * consists of four triplets/quads and a pair, so `2` is returned. The
 * pair block is not inspected by this function, since its presence is
 * guaranteed by the structure of a standard decomposition.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 *
 * @return `2` if @p decomposition contains no sequences, otherwise
 *         `0`. The return value is the han contributed by this yaku
 *         and can be added directly to a running han total.
 */
inline std::uint_fast8_t checkToitoi(FuHan::Standard_::DecompositionElement const &decomposition)
{
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isSequence()) {
      return 0u;
    }
  }
  return 2u;
}

/**
 * @brief Tests whether the decomposition establishes Sanankou (three concealed triplets, 三暗刻).
 *
 * Sanankou is the 2-han yaku awarded for having exactly three concealed
 * triplets or quads (ankou / ankan) in the hand. The remaining meld
 * blocks and the pair are unrestricted: the fourth meld may be a
 * sequence, an open triplet/quad, or even another concealed triplet
 * (in which case the fourth concealed triplet promotes the hand to
 * Suuankou, a yakuman, which is handled separately by `checkSuuankou`).
 *
 * The check counts the meld blocks of @p decomposition that are
 * triplets or quads *and* are not open. Whether the winning meld block
 * counts as concealed (in particular, whether a triplet completed by
 * ron is treated as a minkou and therefore reduces the count) is
 * delegated to the decomposition: this function relies on
 * `Block::isOpen()` alone and does not separately consider the
 * tsumo/ron status of the win.
 *
 * Sanankou has no kuisagari and its han value is `2` whether the hand
 * is otherwise concealed or open. A count of four concealed
 * triplets/quads is Suuankou (a yakuman) and is handled separately, so
 * this function deliberately returns `0` in that case to avoid
 * double-counting.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 *
 * @return `2` if @p decomposition contains exactly three concealed
 *         triplets/quads, otherwise `0`. The return value is the han
 *         contributed by this yaku and can be added directly to a
 *         running han total.
 */
inline std::uint_fast8_t checkSanankou(FuHan::Standard_::DecompositionElement const &decomposition)
{
  std::uint_fast8_t count = 0u;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if ((block.isTriplet() || block.isQuad()) && !block.isOpen()) {
      ++count;
    }
  }
  return count == 3u ? 2u : 0u;
}

/**
 * @brief Tests whether the decomposition establishes Shousangen (little three dragons, 小三元).
 *
 * Shousangen is the 2-han yaku awarded for having triplets/quads of two
 * distinct dragon tiles and a pair of the remaining dragon tile (tile
 * indices 31, 32, and 33 — white, green, and red dragon). The other two
 * meld blocks are unrestricted.
 *
 * Note that Shousangen is *only* the structural 2 han granted by the
 * pair-plus-two-triplets shape itself. The two dragon triplets/quads
 * are additionally counted as yakuhai (`1` han each) by
 * `countYakuhaiHan`, so a Shousangen hand contributes `2 + 2 = 4` han in
 * total from these sources. This split is intentional and matches how
 * the score is built up across the per-yaku helpers; do not combine the
 * yakuhai han into this function's return value.
 *
 * If all three dragons appear as triplets or quads, the hand is
 * Daisangen (a yakuman), which is handled separately by
 * `checkDaisangen`. The condition tested here (two dragon
 * triplets/quads *and* one dragon pair) cannot coexist with that
 * configuration, so this function deliberately returns `0` for a
 * Daisangen hand to avoid double-counting.
 *
 * The implementation tallies, among the blocks of @p decomposition,
 * how many triplets/quads have a dragon base tile (`triplet_count`)
 * and how many pair blocks have a dragon base tile (`pair_count`).
 * Both open and closed dragon melds qualify; Shousangen does not
 * require the hand to be concealed and has no kuisagari.
 *
 * @param decomposition The block decomposition to test. Must contain
 *                      exactly four meld blocks and one pair block.
 *
 * @return `2` if @p decomposition contains two dragon triplets/quads
 *         and one dragon pair, otherwise `0`. The return value is the
 *         han contributed by the Shousangen yaku itself (excluding the
 *         yakuhai han from the dragon melds) and can be added directly
 *         to a running han total.
 */
inline std::uint_fast8_t checkShousangen(FuHan::Standard_::DecompositionElement const &decomposition)
{
  std::uint_fast8_t triplet_count = 0u;
  std::uint_fast8_t pair_count = 0u;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    if (block.isTriplet() || block.isQuad()) {
      std::uint_fast8_t const base_tile = block.getBaseTile();
      if (base_tile >= 31u) {
        ++triplet_count;
      }
      continue;
    }
    if (block.isPair()) {
      std::uint_fast8_t const base_tile = block.getBaseTile();
      if (base_tile >= 31u) {
        ++pair_count;
      }
      continue;
    }
  }
  return (triplet_count == 2u && pair_count == 1u) ? 2u : 0u;
}

/**
 * @brief Checks the one-suit yaku family (Chinitsu / Honitsu) and
 *        returns its han contribution.
 *
 * The one-suit ("isshoku", 一色) yaku family consists of two mutually
 * exclusive yaku that depend only on which suits appear in the
 * decomposed hand:
 *
 * - Chinitsu (清一色, "pure one suit"): every block of the hand uses
 *   tiles from a *single* numbered suit (manzu, pinzu, or souzu) and
 *   no honor tile appears anywhere. Worth `6` han closed and `5` han
 *   open (the standard one-han kuisagari).
 * - Honitsu (混一色, "half flush"): every numbered block uses tiles
 *   from a *single* numbered suit, but at least one honor block
 *   (wind or dragon) is also present. Worth `3` han closed and `2`
 *   han open.
 *
 * Detection iterates over the blocks of @p decomposition, classifying
 * each block by its suit via its base tile (`Block::getBaseTile() / 9`).
 * Honor blocks (suit index `3`) are tolerated but disqualify Chinitsu.
 * The first numbered block fixes the candidate suit; every subsequent
 * numbered block must match that suit, otherwise neither yaku is
 * established. The returned han accounts for the menzen bonus via
 * @p is_closed_hand.
 *
 * No assumption is made about the order of blocks within
 * @p decomposition. The pair block is included in the iteration on the
 * same footing as the meld blocks: the suit of the pair must match the
 * suit of the numbered melds for either yaku to apply, and a honor
 * pair is enough on its own to demote Chinitsu to Honitsu.
 *
 * @param decomposition The block decomposition under consideration.
 *                      Must contain exactly four meld blocks and one
 *                      pair block.
 * @param is_closed_hand `true` iff the hand is menzen (no chii, pon,
 *                      or open kan). Selects the closed/open han
 *                      value for the established yaku.
 * @return `6` for closed Chinitsu, `5` for open Chinitsu, `3` for
 *         closed Honitsu, `2` for open Honitsu, and `0` when neither
 *         yaku is established (i.e. the hand uses tiles from two or
 *         more numbered suits).
 */
inline std::uint_fast8_t checkIsshoku(
  FuHan::Standard_::DecompositionElement const &decomposition, bool const is_closed_hand)
{
  bool is_chiniisou = true;
  std::uint_fast8_t color = UINT_FAST8_MAX;
  for (FuHan::Standard_::Block const &block : decomposition.blocks) {
    std::uint_fast8_t const base_tile = block.getBaseTile();
    std::uint_fast8_t const color_ = base_tile / 9u;
    if (color_ == 3u) {
      is_chiniisou = false;
      continue;
    }
    if (color == UINT_FAST8_MAX) {
      color = color_;
      continue;
    }
    if (color_ != color) {
      return 0u;
    }
  }
  if (is_chiniisou) {
    return is_closed_hand ? 6u : 5u;
  }
  return is_closed_hand ? 3u : 2u;
}

/**
 * @brief Computes the score components of a single block decomposition.
 *
 * Evaluates the fu, han, and yakuman multiplier for *one* particular
 * interpretation of a standard winning hand. This is the per-
 * decomposition core of `calculateFuHan`: the public entry point
 * enumerates every valid decomposition of the hand and invokes this
 * function on each, selecting the highest-scoring result.
 *
 * Evaluation proceeds in three stages:
 * -# A shape-based pinfu test (`isPinfuShape`) is performed once and
 *    its result is fed into the fu calculation
 *    (`calculateFu`), which incorporates the closed/open status and
 *    the tsumo/ron status of the win.
 * -# Every non-counted yakuman is checked. If at least one is
 *    established, the function returns immediately with the
 *    accumulated `yakuman_multiplier` and `han == 0`; no ordinary yaku
 *    are counted in that case.
 * -# Otherwise, every ordinary yaku that depends only on the
 *    decomposition (and the closed/open status) is checked and its
 *    contribution accumulated into `han`. Situational yaku (riichi,
 *    ippatsu, haitei, etc.) and dora are *not* counted here; they are
 *    applied by the caller (`calculateFuHan`) after the best
 *    decomposition has been selected.
 *
 * @param round_wind     The prevailing round wind. Used by both the fu
 *                       calculation and the yakuhai check.
 * @param seat_wind      The seat wind of the winning player. Used by
 *                       both the fu calculation and the yakuhai check.
 * @param concealed_hand Tile counts of the concealed portion of the
 *                       hand, excluding the winning tile. Forwarded to
 *                       `checkChuurenPoutou`, which needs the raw
 *                       concealed-tile distribution rather than a
 *                       block decomposition.
 * @param winning_tile   The internal tile index of the winning tile.
 *                       Forwarded to `checkChuurenPoutou`.
 * @param decomposition  The block decomposition under consideration.
 *                       Must contain exactly four meld blocks and one
 *                       pair block, together with the wait type that
 *                       links the winning tile to the decomposition.
 * @param is_ron         `true` iff the win is by discard (ron). Used
 *                       by the fu calculation.
 * @param is_closed_hand `true` iff the hand is menzen (no chii, pon,
 *                       or open kan). Used by both the fu calculation
 *                       and the yaku checks for which menzen is a
 *                       prerequisite.
 * @param rule           The active rule configuration. Selects open
 *                       tanyao (kuitan), double-yakuman recognition,
 *                       and the double-wind pair fu.
 * @return The score components for this decomposition. If any
 *         non-counted yakuman is established, the returned
 *         `yakuman_multiplier` is positive and `han` is `0`;
 *         otherwise `yakuman_multiplier` is `0` and `han` holds the
 *         sum of the decomposition-derived yaku contributions.
 *         `fu` is always populated.
 */
inline FuHan::Result calculateFuHanImpl(
  FuHan::Wind const round_wind,
  FuHan::Wind const seat_wind,
  std::array<std::uint_fast8_t, 34u> const &concealed_hand,
  std::uint_fast8_t const winning_tile,
  FuHan::Standard_::DecompositionElement const &decomposition,
  bool const is_ron,
  bool const is_closed_hand,
  FuHan::Rule const rule)
{
  bool const is_pinfu_shape = isPinfuShape(round_wind, seat_wind, decomposition);

  std::uint_fast8_t const fu = calculateFu(
    round_wind, seat_wind, decomposition, is_ron, is_closed_hand, is_pinfu_shape, rule);

  std::uint_fast8_t yakuman_multiplier = 0u;
  yakuman_multiplier += checkDaisangen(decomposition);
  yakuman_multiplier += checkSuuankou(decomposition, rule);
  yakuman_multiplier += checkTsuuiisou(decomposition);
  yakuman_multiplier += checkRyuuiisou(decomposition);
  yakuman_multiplier += checkChinroutou(decomposition);
  yakuman_multiplier += checkSuushii(decomposition, rule);
  yakuman_multiplier += checkSuukantsu(decomposition);
  yakuman_multiplier += checkChuurenPoutou(concealed_hand, winning_tile, is_closed_hand, rule);
  if (yakuman_multiplier >= 1u) {
    return FuHan::Result{
      .fu = fu,
      .han = 0u,
      .yakuman_multiplier = yakuman_multiplier
    };
  }

  std::uint_fast8_t han = 0u;
  han += checkMenzenTsumo(is_ron, is_closed_hand);
  han += countYakuhaiHan(round_wind, seat_wind, decomposition);
  if (is_closed_hand || isKuitanEnabled(rule)) {
    han += checkTanyao(decomposition);
  }
  han += checkPeikou(decomposition, is_closed_hand);
  han += checkPinfu(is_closed_hand, is_pinfu_shape);
  han += checkYaochuuYaku(decomposition, is_closed_hand);
  han += checkIkkiTsuukan(decomposition, is_closed_hand);
  han += checkSanshokuDoujun(decomposition, is_closed_hand);
  han += checkSanshokuDoukou(decomposition);
  han += checkSankantsu(decomposition);
  han += checkToitoi(decomposition);
  han += checkSanankou(decomposition);
  han += checkShousangen(decomposition);
  han += checkIsshoku(decomposition, is_closed_hand);

  return FuHan::Result{
    .fu = fu,
    .han = han,
    .yakuman_multiplier = 0u
  };
}

/**
 * @brief Computes the fu, han, and yakuman multiplier of a standard
 *        winning hand.
 *
 * Given a fully-specified standard mahjong winning hand (i.e. a hand
 * that can be partitioned into four melds plus one pair), enumerates
 * every valid block decomposition via `decompose()`, evaluates the
 * applicable yaku, fu, and yakuman for each decomposition, and returns
 * the highest-scoring `Result` according to the ordering defined by
 * @ref operator<(Result const &, Result const &).
 *
 * Each tile-count parameter follows the same convention as
 * `decompose()`: indices `[0, 34)` correspond to the 34 distinct tiles
 * (see `FuHan::tileToString_`), and the value at each index is the
 * number of tiles of that kind in the corresponding part of the hand.
 * The `chii_list` parameter is indexed by the base tile of a sequence
 * and therefore has only 21 entries.
 *
 * @param round_wind     The prevailing round wind (`Wind::east`,
 *                       `Wind::south`, `Wind::west`, or `Wind::north`).
 *                       Used to determine yakuhai for the round-wind
 *                       triplet/quad and certain situational yaku.
 * @param seat_wind      The seat wind of the winning player. Used to
 *                       determine yakuhai for the seat-wind
 *                       triplet/quad and to identify the dealer for
 *                       scoring purposes.
 * @param concealed_hand Tile counts of the concealed portion of the
 *                       hand, excluding tiles that are part of any
 *                       called meld and also excluding the winning
 *                       tile itself (the winning tile is supplied
 *                       separately via @p winning_tile and must not be
 *                       counted here, regardless of whether the win
 *                       is tsumo or ron).
 * @param chii_list      For each valid sequence base tile (in
 *                       `[0, 21)`), the number of chii melds whose
 *                       smallest tile is that tile.
 * @param pon_list       For each tile in `[0, 34)`, the number of pon
 *                       melds of that tile (0 or 1 for any given
 *                       tile).
 * @param open_kan_list  For each tile in `[0, 34)`, the number of
 *                       open kans (daiminkan or kakan) of that tile.
 * @param ankan_list     For each tile in `[0, 34)`, the number of
 *                       concealed kans (ankan) of that tile.
 * @param winning_tile   The internal tile index of the winning tile,
 *                       in `[0, 34)`.
 * @param num_dora       The total number of dora the hand counts,
 *                       including regular dora, aka dora, and ura dora
 *                       (when applicable). This value contributes to
 *                       `Result::han` only when at least one yaku is
 *                       established; otherwise, dora alone do not form
 *                       a winning hand and the returned `han` is `0`
 *                       (see @ref FuHan::Result::han).
 * @param context        A bitmask of `FuHan::Context` flags describing
 *                       the situational context of the win (tsumo/ron,
 *                       riichi, ippatsu, chankan, rinshan kaihou,
 *                       haitei/houtei, tenhou/chiihou, etc.). Multiple
 *                       flags may be combined with `operator|`.
 * @param rule           The active rule configuration. Selects open
 *                       tanyao (kuitan), double-yakuman recognition,
 *                       and the double-wind pair fu.
 * @return The best-scoring `Result` for the given hand and context.
 *         If the hand does not admit a standard winning decomposition,
 *         or if no yaku (and no yakuman) is established, the returned
 *         `Result` has `han == 0` and `yakuman_multiplier == 0`,
 *         indicating that the hand is not a winning hand for scoring
 *         purposes.
 */
inline FuHan::Result calculateFuHan(
  FuHan::Wind const round_wind,
  FuHan::Wind const seat_wind,
  std::array<std::uint_fast8_t, 34u> const &concealed_hand,
  std::array<std::uint_fast8_t, 21u> const &chii_list,
  std::array<std::uint_fast8_t, 34u> const &pon_list,
  std::array<std::uint_fast8_t, 34u> const &open_kan_list,
  std::array<std::uint_fast8_t, 34u> const &ankan_list,
  std::uint_fast8_t const winning_tile,
  std::uint_fast8_t const num_dora,
  FuHan::Context const context,
  FuHan::Rule const rule)
{
  bool const is_ron = isRon(context);

  std::vector<FuHan::Standard_::DecompositionElement> decomposition = FuHan::Standard_::decompose(
    concealed_hand,
    chii_list,
    pon_list,
    open_kan_list,
    ankan_list,
    winning_tile,
    is_ron);
  if (decomposition.empty()) {
    return FuHan::Result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
  }

  bool const is_closed_hand = isClosedHand(chii_list, pon_list, open_kan_list);

  FuHan::Result best_result{.fu = 0u, .han = 0u, .yakuman_multiplier = 0u};
  for (FuHan::Standard_::DecompositionElement const &decomp_elem : decomposition) {
    FuHan::Result const result = calculateFuHanImpl(
      round_wind,
      seat_wind,
      concealed_hand,
      winning_tile,
      decomp_elem,
      is_ron,
      is_closed_hand,
      rule);
    best_result = std::max(best_result, result);
  }
  assert((best_result.fu > 0u));

  best_result.fu = (best_result.fu + 9u) / 10u * 10u;

  best_result.yakuman_multiplier += isTenhou(context) ? 1u : 0u;
  best_result.yakuman_multiplier += isChiihou(context) ? 1u : 0u;
  if (best_result.yakuman_multiplier >= 1u) {
    return {
      .fu = best_result.fu,
      .han = 0u,
      .yakuman_multiplier = best_result.yakuman_multiplier,
    };
  }

  best_result.han += FuHan::calculateSituationalHan_(context);
  if (best_result.han == 0u) {
    return FuHan::Result{.fu = best_result.fu, .han = 0u, .yakuman_multiplier = 0u};
  }
  if (best_result.han > UINT_FAST8_MAX - num_dora) {
    throw std::overflow_error("Han overflow: `han + num_dora` exceeds `UINT_FAST8_MAX`.");
  }
  best_result.han += num_dora;
  return {
    .fu = best_result.fu,
    .han = best_result.han,
    .yakuman_multiplier = 0u,
  };
}

} // namespace FuHan::Standard_

#endif // !defined(FUHAN_STANDARD_FUHAN_HPP_INCLUDE_GUARD)
