// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#if !defined(FUHAN_STANDARD_DECOMPOSITION_HPP_INCLUDE_GUARD)
#define FUHAN_STANDARD_DECOMPOSITION_HPP_INCLUDE_GUARD

#include "../types.hpp"
#include "internal.hpp"
#include <sstream>
#include <vector>
#include <array>
#include <functional>
#include <utility>
#include <stdexcept>
#include <cstdint>


namespace FuHan::Standard_{

/**
 * @brief Recursively enumerates standard block decompositions of @p hand.
 *
 * Visits every way to partition the multiset of tiles in @p hand into
 * exactly (`need_pair` ? 1 : 0) pair plus @p num_melds_to_form sequences
 * and/or triplets, calling @p emit on the resulting block list for each
 * complete decomposition. The recursion scans tile indices in ascending
 * order from @p start_tile and only forms blocks whose smallest tile is
 * the current scan position, which is sufficient to enumerate every
 * partition exactly once.
 */
inline void decomposeImpl(
  std::array<std::uint_fast8_t, 34u> &hand,
  std::uint_fast8_t const start_tile,
  bool const need_pair,
  std::uint_fast8_t const num_melds_to_form,
  std::vector<Block> &blocks,
  std::function<void(std::vector<Block> const &)> const &emit)
{
  std::uint_fast8_t i = start_tile;
  while (i < 34u && hand[i] == 0u) {
    ++i;
  }

  if (i == 34u) {
    if (!need_pair && num_melds_to_form == 0u) {
      emit(blocks);
    }
    return;
  }

  if (need_pair && hand[i] >= 2u) {
    hand[i] -= 2u;
    blocks.emplace_back(BlockType::pair, i, false);
    decomposeImpl(hand, i, false, num_melds_to_form, blocks, emit);
    blocks.pop_back();
    hand[i] += 2u;
  }

  if (num_melds_to_form > 0u) {
    if (hand[i] >= 3u) {
      hand[i] -= 3u;
      blocks.emplace_back(BlockType::triplet, i, false);
      decomposeImpl(hand, i, need_pair, num_melds_to_form - 1u, blocks, emit);
      blocks.pop_back();
      hand[i] += 3u;
    }

    if (i < 27u
        && i % 9u <= 6u
        && hand[i + 1u] >= 1u && hand[i + 2u] >= 1u) {
      --hand[i];
      --hand[i + 1u];
      --hand[i + 2u];
      blocks.emplace_back(BlockType::sequence, i, false);
      decomposeImpl(hand, i, need_pair, num_melds_to_form - 1u, blocks, emit);
      blocks.pop_back();
      ++hand[i];
      ++hand[i + 1u];
      ++hand[i + 2u];
    }
  }
}

/**
 * @brief Decomposes a standard winning hand into its constituent blocks.
 *
 * Combines the concealed hand and all melds formed by calls (chii, pon,
 * open kan, ankan) into a single decomposition of the winning hand as a
 * sequence of `Block` values suitable for scoring.
 *
 * This function also accepts hands that are *not* in a standard winning
 * form (i.e. that cannot be partitioned into four melds plus one pair);
 * in that case, an empty vector is returned. Callers may therefore use
 * this function both to enumerate the interpretations of a known
 * winning hand and to test whether an arbitrary hand admits any
 * standard winning decomposition.
 *
 * Each tile-count parameter is indexed by the internal tile encoding (see
 * `FuHan::tileToString_`): indices `[0, 34)` correspond to the 34 distinct
 * tiles, and the value at each index is the number of tiles of that kind
 * contained in the corresponding part of the hand. The `chii_list`
 * parameter is indexed by the base tile of a sequence and therefore has
 * only 21 entries (one per valid sequence start in the three numbered
 * suits: 7 ranks * 3 suits).
 *
 * @param concealed_hand Tile counts of the concealed portion of the hand,
 *                       excluding tiles that are part of any called meld
 *                       and also excluding the winning tile itself (the
 *                       winning tile is supplied separately via
 *                       @p winning_tile and must not be counted here,
 *                       regardless of whether the win is tsumo or ron).
 * @param chii_list      For each valid sequence base tile (in `[0, 21)`),
 *                       the number of chii melds whose smallest tile is
 *                       that tile.
 * @param pon_list       For each tile in `[0, 34)`, the number of pon
 *                       melds of that tile (0 or 1 for any given tile).
 * @param open_kan_list  For each tile in `[0, 34)`, the number of open
 *                       kans (daiminkan or kakan) of that tile.
 * @param ankan_list     For each tile in `[0, 34)`, the number of
 *                       concealed kans (ankan) of that tile.
 * @param winning_tile   The internal tile index of the winning tile, in
 *                       `[0, 34)`.
 * @param is_ron         `true` if the win is by discard (ron), `false`
 *                       if it is a self-draw (tsumo). This determines
 *                       the open/closed status of the concealed-hand
 *                       block that contains the winning tile: it is
 *                       open iff @p is_ron is `true`.
 * @return All valid interpretations of the winning hand, each as a
 *         `DecompositionElement` pairing a five-block decomposition
 *         (four melds plus one pair) with the wait pattern by which the
 *         winning tile completes that decomposition. A standard hand
 *         may admit more than one such interpretation; the score
 *         calculator selects the one that yields the highest score
 *         under the applicable fu/han rules. The order of elements in
 *         the returned vector is not semantically significant. If the
 *         input hand does not form a standard winning hand, an empty
 *         vector is returned.
 */
inline std::vector<FuHan::Standard_::DecompositionElement> decompose(
  std::array<std::uint_fast8_t, 34u> const &concealed_hand,
  std::array<std::uint_fast8_t, 21u> const &chii_list,
  std::array<std::uint_fast8_t, 34u> const &pon_list,
  std::array<std::uint_fast8_t, 34u> const &open_kan_list,
  std::array<std::uint_fast8_t, 34u> const &ankan_list,
  std::uint_fast8_t const winning_tile,
  bool const is_ron)
{
  if (winning_tile >= 34u) {
    std::ostringstream oss;
    oss << static_cast<unsigned>(winning_tile) << ": An invalid winning tile.";
    throw std::invalid_argument(oss.str());
  }

  // Build the fixed (called-meld) portion of every decomposition.
  std::vector<Block> called_blocks;
  for (std::uint_fast8_t i = 0u; i < 21u; ++i) {
    std::uint_fast8_t const base = static_cast<std::uint_fast8_t>(9u * (i / 7u) + (i % 7u));
    for (std::uint_fast8_t j = 0u; j < chii_list[i]; ++j) {
      called_blocks.emplace_back(BlockType::sequence, base, true);
    }
  }
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    for (std::uint_fast8_t j = 0u; j < pon_list[i]; ++j) {
      called_blocks.emplace_back(BlockType::triplet, i, true);
    }
  }
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    for (std::uint_fast8_t j = 0u; j < open_kan_list[i]; ++j) {
      called_blocks.emplace_back(BlockType::quad, i, true);
    }
  }
  for (std::uint_fast8_t i = 0u; i < 34u; ++i) {
    for (std::uint_fast8_t j = 0u; j < ankan_list[i]; ++j) {
      called_blocks.emplace_back(BlockType::quad, i, false);
    }
  }

  if (called_blocks.size() > 4u) {
    std::ostringstream oss;
    oss << called_blocks.size() << ": Too many called blocks.";
    throw std::invalid_argument(oss.str());
  }
  std::uint_fast8_t const concealed_melds_needed = 4u - called_blocks.size();

  std::vector<DecompositionElement> results;

  // For each way the winning tile can complete the hand, remove the
  // tiles of the resulting "winning block" from the concealed hand and
  // enumerate decompositions of what remains.
  auto try_case = [&](
    std::array<std::uint_fast8_t, 34u> remaining,
    Block const &wt_block,
    WaitType const wait_type)
  {
    bool const wt_is_pair = wt_block.isPair();
    bool const need_pair = !wt_is_pair;
    std::uint_fast8_t num_melds_to_form;
    if (wt_is_pair) {
      num_melds_to_form = concealed_melds_needed;
    }
    else {
      if (concealed_melds_needed == 0u) {
        return;
      }
      num_melds_to_form = concealed_melds_needed - 1u;
    }

    std::vector<Block> tmp;
    tmp.reserve(num_melds_to_form + (need_pair ? 1u : 0u));

    decomposeImpl(
      remaining, 0u, need_pair, num_melds_to_form, tmp,
      [&](std::vector<Block> const &enumerated) {
        DecompositionElement element{};
        std::size_t k = 0u;
        element.blocks[k++] = wt_block;
        for (Block const &b : enumerated) {
          element.blocks[k++] = b;
        }
        for (Block const &b : called_blocks) {
          element.blocks[k++] = b;
        }
        element.wait_type = wait_type;
        results.push_back(std::move(element));
      });
  };

  // Case 1: tanki (single-tile / pair wait).
  if (concealed_hand[winning_tile] >= 1u) {
    auto remaining = concealed_hand;
    --remaining[winning_tile];
    try_case(
      remaining,
      Block(BlockType::pair, winning_tile, is_ron),
      WaitType::pair);
  }

  // Case 2: shanpon (double-pair wait); the winning tile completes a triplet.
  if (concealed_hand[winning_tile] >= 2u) {
    auto remaining = concealed_hand;
    remaining[winning_tile] -= 2u;
    try_case(
      remaining,
      Block(BlockType::triplet, winning_tile, is_ron),
      WaitType::double_pair);
  }

  // Case 3: the winning tile completes a sequence (only for number tiles).
  if (winning_tile < 27u) {
    std::uint_fast8_t const rank = winning_tile % 9u;

    // (3a) Winning tile is the smallest of (WT, WT+1, WT+2).
    if (rank <= 6u
        && concealed_hand[winning_tile + 1u] >= 1u
        && concealed_hand[winning_tile + 2u] >= 1u) {
      auto remaining = concealed_hand;
      --remaining[winning_tile + 1u];
      --remaining[winning_tile + 2u];
      WaitType const wt = (rank == 6u) ? WaitType::edge : WaitType::two_sided;
      try_case(
        remaining,
        Block(BlockType::sequence, winning_tile, is_ron),
        wt);
    }

    // (3b) Winning tile is the middle of (WT-1, WT, WT+1).
    if (rank >= 1u && rank <= 7u
        && concealed_hand[winning_tile - 1u] >= 1u
        && concealed_hand[winning_tile + 1u] >= 1u) {
      auto remaining = concealed_hand;
      --remaining[winning_tile - 1u];
      --remaining[winning_tile + 1u];
      try_case(
        remaining,
        Block(BlockType::sequence, static_cast<std::uint_fast8_t>(winning_tile - 1u), is_ron),
        WaitType::inside);
    }

    // (3c) Winning tile is the largest of (WT-2, WT-1, WT).
    if (rank >= 2u
        && concealed_hand[winning_tile - 2u] >= 1u
        && concealed_hand[winning_tile - 1u] >= 1u) {
      auto remaining = concealed_hand;
      --remaining[winning_tile - 2u];
      --remaining[winning_tile - 1u];
      WaitType const wt = (rank == 2u) ? WaitType::edge : WaitType::two_sided;
      try_case(
        remaining,
        Block(BlockType::sequence, static_cast<std::uint_fast8_t>(winning_tile - 2u), is_ron),
        wt);
    }
  }

  return results;
}

} // namespace FuHan::Standard_

#endif // !defined(FUHAN_STANDARD_DECOMPOSITION_HPP_INCLUDE_GUARD)
