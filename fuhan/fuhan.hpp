// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#if !defined(FUHAN_FUHAN_HPP_INCLUDE_GUARD)
#define FUHAN_FUHAN_HPP_INCLUDE_GUARD

#include "thirteen_orphans/fuhan.hpp"
#include "seven_pairs/fuhan.hpp"
#include "standard/fuhan.hpp"
#include "types.hpp"
#include <sstream>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <cstdint>


namespace FuHan{

inline constexpr FuHan::Context valid_mask_
  = Context::tsumo_
  | Context::ron_
  | Context::riichi_
  | Context::chankan_
  | Context::rinshan_kaihou_
  | Context::haitei_raoyue_
  | Context::houtei_raoyui_
  | Context::double_riichi_
  | Context::ippatsu_
  | Context::tenhou_
  | Context::chiihou_;

/**
 * @brief Validates the inputs to `calculateFuHan` and throws on error.
 *
 * Performs a battery of structural sanity checks on the arguments that
 * describe a candidate winning hand. The function reports the first
 * inconsistency it finds by throwing `std::invalid_argument` with a
 * message that identifies the offending quantity. If every check
 * passes, the function returns normally. No value is returned and the
 * arguments are not modified.
 *
 * The following invariants are checked:
 * - @p round_wind and @p seat_wind are valid `FuHan::Wind`
 *   enumerators (i.e. one of east, south, west, north).
 * - For every tile in @p concealed_hand, the per-tile count does not
 *   exceed `4`, and the total concealed-tile count plus the winning
 *   tile does not exceed `14`.
 * - For every entry in @p chii_list, the per-base-tile count does not
 *   exceed `4`, and the total chii count does not exceed `4`.
 * - For every entry in @p pon_list, @p open_kan_list, and
 *   @p ankan_list, the per-tile count does not exceed `1`, and the
 *   per-list total does not exceed `4`.
 * - The total number of called melds (chii + pon + open kan + ankan)
 *   does not exceed `4`.
 * - The structural tile count, obtained by counting each kan as `3`
 *   tiles (so that every meld contributes the same number of tiles),
 *   plus the winning tile is exactly `14` (= 4 melds * 3 + 1 pair * 2).
 *   This rules out tile counts that are otherwise within bounds but
 *   cannot correspond to "four melds plus one pair" once kans are
 *   collapsed.
 * - @p winning_tile is a valid internal tile index (i.e. strictly
 *   less than `34`).
 * - For every tile kind, the total physical tile count across
 *   @p concealed_hand, @p chii_list, @p pon_list, @p open_kan_list,
 *   @p ankan_list, and @p winning_tile does not exceed `4`. For this
 *   check, each chii contributes one copy of each tile in its sequence,
 *   each pon contributes three copies, each kan contributes four
 *   copies, and the winning tile contributes one copy.
 * - @p context contains only defined `FuHan::Context` flags; no bits
 *   outside the supported flag mask are set.
 * - @p context is internally consistent and consistent with
 *   @p seat_wind and the open/closed status of the hand. In particular:
 *   - exactly one of `tsumo` and `ron` is set;
 *   - `ippatsu` is set only when either `riichi` or `double_riichi`
 *     is also set;
 *   - `tenhou` is set only for east seat wind, and `chiihou` is set
 *     only for non-east seat wind;
 *   - an open hand (one with at least one chii, pon, or open kan;
 *     ankan does not open the hand) does not combine with `riichi`,
 *     `double_riichi`, `ippatsu`, `tenhou`, or `chiihou`;
 *   - no pair of mutually exclusive `Context` flags is set
 *     simultaneously. The mutually exclusive pairs that are rejected
 *     are:
 *     - `tsumo` with `chankan`, or with `houtei_raoyui`;
 *     - `ron` with `rinshan_kaihou`, `haitei_raoyue`, `tenhou`, or
 *       `chiihou`;
 *     - `riichi` with `double_riichi`, `tenhou`, or `chiihou`;
 *     - `chankan` with `rinshan_kaihou`, `haitei_raoyue`,
 *       `houtei_raoyui`, `tenhou`, or `chiihou`;
 *     - `rinshan_kaihou` with `haitei_raoyue`, `houtei_raoyui`,
 *       `ippatsu`, `tenhou`, or `chiihou`;
 *     - `haitei_raoyue` with `houtei_raoyui`, `tenhou`, or `chiihou`;
 *     - `houtei_raoyui` with `ippatsu`, `tenhou`, or `chiihou`;
 *     - `double_riichi` with `tenhou` or `chiihou`;
 *     - `ippatsu` with `tenhou` or `chiihou`;
 *     - `tenhou` with `chiihou`.
 *
 * @param round_wind     The prevailing round wind.
 * @param seat_wind      The seat wind of the winning player.
 * @param concealed_hand Tile counts of the concealed portion of the
 *                       hand, excluding the winning tile.
 * @param chii_list      Counts of chii melds indexed by sequence base
 *                       tile (in `[0, 21)`).
 * @param pon_list       Counts of pon melds indexed by tile
 *                       (in `[0, 34)`).
 * @param open_kan_list  Counts of open kans (daiminkan or kakan)
 *                       indexed by tile (in `[0, 34)`).
 * @param ankan_list     Counts of concealed kans (ankan) indexed by
 *                       tile (in `[0, 34)`).
 * @param winning_tile   The internal tile index of the winning tile,
 *                       in `[0, 34)`.
 * @param context        A bitmask of `FuHan::Context` flags
 *                       describing the situational context of the
 *                       win. Must contain only defined `Context`
 *                       flags and must satisfy the context consistency
 *                       rules listed above.
 *
 * @throws std::invalid_argument If any of the invariants listed above
 *         is violated. The exception message identifies the offending
 *         quantity.
 */
inline void checkInput_(
  FuHan::Wind const round_wind,
  FuHan::Wind const seat_wind,
  std::array<std::uint_fast8_t, 34u> const &concealed_hand,
  std::array<std::uint_fast8_t, 21u> const &chii_list,
  std::array<std::uint_fast8_t, 34u> const &pon_list,
  std::array<std::uint_fast8_t, 34u> const &open_kan_list,
  std::array<std::uint_fast8_t, 34u> const &ankan_list,
  std::uint_fast8_t const winning_tile,
  FuHan::Context const context)
{
  if (round_wind < Wind::east_ || round_wind > Wind::north_) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(round_wind) << ": An invalid round wind.";
    throw std::invalid_argument(oss.str());
  }

  if (seat_wind < Wind::east_ || seat_wind > Wind::north_) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(seat_wind) << ": An invalid seat wind.";
    throw std::invalid_argument(oss.str());
  }

  std::uint_fast8_t total_num_concealed_tiles = 0u;
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (concealed_hand[i] > 4u) {
      std::ostringstream oss;
      oss << static_cast<unsigned>(i) << ": Too many tiles of this kind in the concealed hand.";
      throw std::invalid_argument(oss.str());
    }
    total_num_concealed_tiles += concealed_hand[i];
  }
  if (total_num_concealed_tiles + 1u > 14u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(total_num_concealed_tiles) << ": Too many tiles in the concealed hand.";
    throw std::invalid_argument(oss.str());
  }

  std::uint_fast8_t total_num_chii = 0u;
  for (std::uint_fast8_t i = 0u; i < 21u; ++i) {
    if (chii_list[i] > 4u) {
      std::ostringstream oss;
      oss << static_cast<unsigned>(i) << ": Too many chii of this base tile.";
      throw std::invalid_argument(oss.str());
    }
    total_num_chii += chii_list[i];
  }
  if (total_num_chii > 4u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(total_num_chii) << ": Too many chii.";
    throw std::invalid_argument(oss.str());
  }

  std::uint_fast8_t total_num_pon = 0u;
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (pon_list[i] > 1u) {
      std::ostringstream oss;
      oss << static_cast<unsigned>(i) << ": Too many pon of this tile.";
      throw std::invalid_argument(oss.str());
    }
    total_num_pon += pon_list[i];
  }
  if (total_num_pon > 4u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(total_num_pon) << ": Too many pon.";
    throw std::invalid_argument(oss.str());
  }

  std::uint_fast8_t total_num_open_kan = 0u;
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (open_kan_list[i] > 1u) {
      std::ostringstream oss;
      oss << static_cast<unsigned>(i) << ": Too many open kan of this tile.";
      throw std::invalid_argument(oss.str());
    }
    total_num_open_kan += open_kan_list[i];
  }
  if (total_num_open_kan > 4u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(total_num_open_kan) << ": Too many open kan.";
    throw std::invalid_argument(oss.str());
  }

  std::uint_fast8_t total_num_ankan = 0u;
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    if (ankan_list[i] > 1u) {
      std::ostringstream oss;
      oss << static_cast<unsigned>(i) << ": Too many ankan of this tile.";
      throw std::invalid_argument(oss.str());
    }
    total_num_ankan += ankan_list[i];
  }
  if (total_num_ankan > 4u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(total_num_ankan) << ": Too many ankan.";
    throw std::invalid_argument(oss.str());
  }

  bool const is_open_hand = (total_num_chii + total_num_pon + total_num_open_kan) != 0u;

  std::uint_fast8_t const total_num_melds
    = total_num_chii + total_num_pon + total_num_open_kan + total_num_ankan;
  if (total_num_melds > 4u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(total_num_melds) << ": Too many melds.";
    throw std::invalid_argument(oss.str());
  }

  std::uint_fast8_t const total_num_tiles_with_kans_as_triplets
    = total_num_concealed_tiles
    + 3u * total_num_chii
    + 3u * total_num_pon
    + 3u * total_num_open_kan
    + 3u * total_num_ankan
    + 1u; // winning tile
  if (total_num_tiles_with_kans_as_triplets != 14u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(total_num_tiles_with_kans_as_triplets)
        << ": The total number of tiles is inconsistent with the number of melds.";
    throw std::invalid_argument(oss.str());
  }

  if (winning_tile >= 34u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(winning_tile) << ": An invalid winning tile.";
    throw std::invalid_argument(oss.str());
  }

  {
    std::array<std::uint_fast8_t, 34u> total_tile_counts = concealed_hand;
    for (std::uint_fast8_t i = 0u; i < 21u; ++i) {
      std::uint_fast8_t const suit = i / 7u;
      std::uint_fast8_t const base_rank = i % 7u;
      std::uint_fast8_t const base_tile = suit * 9u + base_rank;
      total_tile_counts[base_tile] += chii_list[i];
      total_tile_counts[base_tile + 1u] += chii_list[i];
      total_tile_counts[base_tile + 2u] += chii_list[i];
    }
    for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
      total_tile_counts[i] += 3u * pon_list[i];
      total_tile_counts[i] += 4u * open_kan_list[i];
      total_tile_counts[i] += 4u * ankan_list[i];
    }
    ++total_tile_counts[winning_tile];
    for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
      if (total_tile_counts[i] > 4u) {
        std::ostringstream oss;
        oss << static_cast<unsigned>(i)
            << ": Too many tiles of this kind once all melds and the winning tile are added.";
        throw std::invalid_argument(oss.str());
      }
    }
  }

  if ((context & ~valid_mask_) != Context{}) {
    throw std::invalid_argument("`context` contains invalid flags.");
  }

  if (!isTsumo(context) && !isRon(context)) {
    throw std::invalid_argument("A win must be either tsumo or ron.");
  }
  if (isTsumo(context) && isRon(context)) {
    throw std::invalid_argument("A win cannot be both tsumo and ron.");
  }
  if (isTsumo(context) && isChankan(context)) {
    throw std::invalid_argument("A win cannot be both tsumo and chankan.");
  }
  if (isTsumo(context) && isHouteiRaoyui(context)) {
    throw std::invalid_argument("A win cannot be both tsumo and houtei raoyui.");
  }
  if (isRon(context) && isRinshanKaihou(context)) {
    throw std::invalid_argument("A win cannot be both ron and rinshan kaihou.");
  }
  if (isRon(context) && isHaiteiRaoyue(context)) {
    throw std::invalid_argument("A win cannot be both ron and haitei raoyue.");
  }
  if (isRon(context) && isTenhou(context)) {
    throw std::invalid_argument("A win cannot be both ron and tenhou.");
  }
  if (isRon(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both ron and chiihou.");
  }
  if (isRiichi(context) && isDoubleRiichi(context)) {
    throw std::invalid_argument("A win cannot be both riichi and double riichi.");
  }
  if (!isRiichi(context) && !isDoubleRiichi(context) && isIppatsu(context)) {
    throw std::invalid_argument(
      "Ippatsu cannot be set if neither riichi nor double riichi is set.");
  }
  if (isRiichi(context) && isTenhou(context)) {
    throw std::invalid_argument("A win cannot be both riichi and tenhou.");
  }
  if (isRiichi(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both riichi and chiihou.");
  }
  if (isChankan(context) && isRinshanKaihou(context)) {
    throw std::invalid_argument("A win cannot be both chankan and rinshan kaihou.");
  }
  if (isChankan(context) && isHaiteiRaoyue(context)) {
    throw std::invalid_argument("A win cannot be both chankan and haitei raoyue.");
  }
  if (isChankan(context) && isHouteiRaoyui(context)) {
    throw std::invalid_argument("A win cannot be both chankan and houtei raoyui.");
  }
  if (isChankan(context) && isTenhou(context)) {
    throw std::invalid_argument("A win cannot be both chankan and tenhou.");
  }
  if (isChankan(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both chankan and chiihou.");
  }
  if (isRinshanKaihou(context) && isHaiteiRaoyue(context)) {
    throw std::invalid_argument("A win cannot be both rinshan kaihou and haitei raoyue.");
  }
  if (isRinshanKaihou(context) && isHouteiRaoyui(context)) {
    throw std::invalid_argument("A win cannot be both rinshan kaihou and houtei raoyui.");
  }
  if (isRinshanKaihou(context) && isIppatsu(context)) {
    throw std::invalid_argument("A win cannot be both rinshan kaihou and ippatsu.");
  }
  if (isRinshanKaihou(context) && isTenhou(context)) {
    throw std::invalid_argument("A win cannot be both rinshan kaihou and tenhou.");
  }
  if (isRinshanKaihou(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both rinshan kaihou and chiihou.");
  }
  if (isHaiteiRaoyue(context) && isHouteiRaoyui(context)) {
    throw std::invalid_argument("A win cannot be both haitei raoyue and houtei raoyui.");
  }
  if (isHaiteiRaoyue(context) && isTenhou(context)) {
    throw std::invalid_argument("A win cannot be both haitei raoyue and tenhou.");
  }
  if (isHaiteiRaoyue(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both haitei raoyue and chiihou.");
  }
  if (isHouteiRaoyui(context) && isIppatsu(context)) {
    throw std::invalid_argument("A win cannot be both houtei raoyui and ippatsu.");
  }
  if (isHouteiRaoyui(context) && isTenhou(context)) {
    throw std::invalid_argument("A win cannot be both houtei raoyui and tenhou.");
  }
  if (isHouteiRaoyui(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both houtei raoyui and chiihou.");
  }
  if (isDoubleRiichi(context) && isTenhou(context)) {
    throw std::invalid_argument("A win cannot be both double riichi and tenhou.");
  }
  if (isDoubleRiichi(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both double riichi and chiihou.");
  }
  if (isIppatsu(context) && isTenhou(context)) {
    throw std::invalid_argument("A win cannot be both ippatsu and tenhou.");
  }
  if (isIppatsu(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both ippatsu and chiihou.");
  }
  if (isTenhou(context) && isChiihou(context)) {
    throw std::invalid_argument("A win cannot be both tenhou and chiihou.");
  }

  if (seat_wind != east && isTenhou(context)) {
    throw std::invalid_argument("Tenhou cannot be set if the seat wind is not east.");
  }
  if (seat_wind == east && isChiihou(context)) {
    throw std::invalid_argument("Chiihou cannot be set if the seat wind is east.");
  }

  if (is_open_hand) {
    if (isRiichi(context)) {
      throw std::invalid_argument("A win cannot be both open hand and riichi.");
    }
    if (isDoubleRiichi(context)) {
      throw std::invalid_argument("A win cannot be both open hand and double riichi.");
    }
    if (isIppatsu(context)) {
      throw std::invalid_argument("A win cannot be both open hand and ippatsu.");
    }
  }
  if (total_num_melds >= 1u && (isTenhou(context) || isChiihou(context))) {
    throw std::invalid_argument(
      "Tenhou and chiihou cannot be set if the hand is not completely concealed.");
  }

  if (total_num_open_kan + total_num_ankan == 0u && isRinshanKaihou(context)) {
    throw std::invalid_argument("Rinshan kaihou cannot be set if there are no kans.");
  }
}

/**
 * @brief Computes the fu, han, and yakuman multiplier of a mahjong
 *        winning hand under all supported winning shapes.
 *
 * Public entry point of the score calculator. Given a fully-specified
 * mahjong hand, evaluates the score under each supported winning
 * shape and returns the highest-scoring `FuHan::Result` among them.
 *
 * The shapes considered are:
 *
 * - The standard "four melds plus one pair" shape, via
 *   `FuHan::Standard_::calculateFuHan`. Receives the full set of
 *   parameters (winds, concealed hand, the four meld lists, the
 *   winning tile, the dora count, and the context).
 * - Chiitoitsu (seven pairs, 七対子), via
 *   `FuHan::SevenPairs_::calculateFuHan`. Receives only the
 *   concealed hand, the winning tile, the dora count, and the
 *   context, since Chiitoitsu is incompatible with any called meld
 *   and does not depend on the round or seat winds.
 * - Kokushi musou (thirteen orphans, 国士無双), via
 *   `FuHan::ThirteenOrphans_::calculateFuHan`. Receives only the
 *   concealed hand, the winning tile, and the context; dora do not
 *   contribute because Kokushi musou is always scored as a yakuman.
 *
 * The maximum is taken under the strict weak ordering defined by
 * `operator<(Result const &, Result const &)`, which selects the
 * outcome that yields the higher score. When the input hand admits
 * more than one winning shape (e.g. a tanyao-only hand that also
 * happens to be a valid Chiitoitsu), the higher-scoring
 * interpretation is returned.
 *
 * The inputs are validated up front via `checkInput_`, which throws
 * `std::invalid_argument` if any structural invariant of the hand or
 * the context is violated. See `checkInput_` for the full list of
 * invariants enforced.
 *
 * Each tile-count parameter follows the library's standard
 * convention: indices `[0, 34)` correspond to the 34 distinct tiles
 * (see `FuHan::tileToString_`), and the value at each index is the
 * number of tiles of that kind in the corresponding part of the
 * hand. `chii_list` is indexed by the base (smallest) tile of a
 * sequence and therefore has only 21 entries.
 *
 * @param round_wind     The prevailing round wind. Used by the
 *                       standard-shape evaluator for yakuhai and
 *                       certain situational yaku.
 * @param seat_wind      The seat wind of the winning player. Used by
 *                       the standard-shape evaluator for yakuhai and
 *                       to identify the dealer for scoring purposes.
 * @param concealed_hand Tile counts of the concealed portion of the
 *                       hand, indexed by internal tile index in
 *                       `[0, 34)`, excluding any tile that is part
 *                       of a called meld and excluding the winning
 *                       tile itself. The winning tile is supplied
 *                       separately via @p winning_tile and must not
 *                       be counted here, regardless of whether the
 *                       win is tsumo or ron.
 * @param chii_list      For each valid sequence base tile (in
 *                       `[0, 21)`), the number of chii melds whose
 *                       smallest tile is that tile. Must be empty
 *                       (all zeros) for a Chiitoitsu or Kokushi
 *                       musou interpretation to be selected.
 * @param pon_list       For each tile in `[0, 34)`, the number of
 *                       pon melds of that tile (0 or 1 for any
 *                       given tile).
 * @param open_kan_list  For each tile in `[0, 34)`, the number of
 *                       open kans (daiminkan or kakan) of that tile.
 * @param ankan_list     For each tile in `[0, 34)`, the number of
 *                       concealed kans (ankan) of that tile.
 * @param winning_tile   The internal tile index of the winning tile,
 *                       in `[0, 34)`.
 * @param num_dora       The total number of dora the hand counts,
 *                       including regular dora, aka dora, and ura
 *                       dora (when applicable). Reflected in
 *                       `FuHan::Result::han` only in case 3 of the
 *                       return-value case analysis below (i.e. when
 *                       at least one non-yakuman yaku is established,
 *                       possibly producing a counted yakuman). In
 *                       case 2 (winning shape but no yaku), dora are
 *                       *not* reflected in `FuHan::Result::han`,
 *                       because dora alone does not constitute a
 *                       yaku and such a hand is not a winning hand
 *                       for scoring purposes. Does not affect the
 *                       Kokushi musou interpretation, which is
 *                       always scored as a yakuman.
 * @param context        A bitmask of `FuHan::Context` flags
 *                       describing the situational context of the
 *                       win (tsumo/ron, riichi, ippatsu, chankan,
 *                       rinshan kaihou, haitei/houtei,
 *                       tenhou/chiihou, etc.). Multiple flags may be
 *                       combined with `operator|`. Must contain only
 *                       defined `Context` flags, must specify exactly
 *                       one of `tsumo` or `ron`, and must not combine
 *                       mutually exclusive flags (see `checkInput_`).
 *
 * @return The highest-scoring `FuHan::Result` among the standard,
 *         Chiitoitsu, and Kokushi musou interpretations. The returned
 *         value falls into exactly one of the five mutually exclusive
 *         and exhaustive cases enumerated by `FuHan::Result`:
 *
 *         -# **The input does not form a winning shape under any
 *            interpretation.** `fu`, `han`, and `yakuman_multiplier`
 *            are all `0`.
 *         -# **The input forms a winning shape under some
 *            interpretation, but no yaku (excluding dora) is
 *            established under any interpretation.** `fu` is the
 *            non-zero fu value computed from the best-scoring
 *            winning shape, while `han` and `yakuman_multiplier` are
 *            both `0`. Dora are not reflected in `han` here, because
 *            dora alone does not constitute a yaku.
 *         -# **At least one non-yakuman yaku is established (which
 *            may, together with dora, escalate to a counted
 *            yakuman).** `fu` and `han` are the non-zero values of
 *            the best-scoring interpretation (with dora included in
 *            `han`), and `yakuman_multiplier` is `0`. A counted
 *            yakuman (kazoe yakuman) manifests as `han >= 13` with
 *            `yakuman_multiplier == 0`.
 *         -# **Kokushi musou (thirteen orphans) is established.**
 *            `fu` and `han` are both `0` (fu is not defined for this
 *            hand and yakuman wins do not use `han`), and
 *            `yakuman_multiplier` is the appropriate non-zero value
 *            for the recognised form of Kokushi musou.
 *         -# **A non-counted yakuman other than Kokushi musou is
 *            established.** `fu` is the non-zero fu value computed
 *            from the winning shape, `han` is `0`, and
 *            `yakuman_multiplier` is the appropriate non-zero sum of
 *            per-yaku yakuman contributions (each ordinary yakuman
 *            contributes `1` and each double yakuman contributes
 *            `2`).
 *
 *         See `FuHan::Result` for the full case-by-case
 *         specification of each field.
 *
 * @throws std::invalid_argument If any of the invariants enforced by
 *         `checkInput_` is violated.
 * @throws std::overflow_error If any internal han-related calculation,
 *         including the addition of @p num_dora or the comparison of
 *         candidate results, may exceed the upper limit required by
 *         that calculation step. The effective upper limit is the
 *         strictest limit imposed by the internal steps performed for
 *         the input.
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
  FuHan::Context const context)
{
  checkInput_(
    round_wind,
    seat_wind,
    concealed_hand,
    chii_list,
    pon_list,
    open_kan_list,
    ankan_list,
    winning_tile,
    context);

  FuHan::Result result0 = FuHan::Standard_::calculateFuHan(
    round_wind,
    seat_wind,
    concealed_hand,
    chii_list,
    pon_list,
    open_kan_list,
    ankan_list,
    winning_tile,
    num_dora,
    context);

  FuHan::Result result1 = FuHan::SevenPairs_::calculateFuHan(
    concealed_hand,
    winning_tile,
    num_dora,
    context);

  FuHan::Result result2 = FuHan::ThirteenOrphans_::calculateFuHan(
    concealed_hand,
    winning_tile,
    context);

  return std::max({result0, result1, result2});
}

} // namespace FuHan

#endif // !defined(FUHAN_FUHAN_HPP_INCLUDE_GUARD)
