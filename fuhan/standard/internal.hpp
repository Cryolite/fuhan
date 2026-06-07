// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#if !defined(FUHAN_STANDARD_INTERNAL_HPP_INCLUDE_GUARD)
#define FUHAN_STANDARD_INTERNAL_HPP_INCLUDE_GUARD

#include "../internal.hpp"
#include <sstream>
#include <ostream>
#include <array>
#include <stdexcept>
#include <cstdint>
#include <cassert>


namespace FuHan::Standard_{

/**
 * @brief An enumeration identifying the kind of block that composes a standard mahjong hand.
 *
 * A standard winning hand is decomposed into a set of blocks, each of which
 * is one of the following structures. `BlockType` tags a `Block` value with
 * the kind of structure it represents and is used by the score calculator
 * to apply the appropriate fu/han rules.
 *
 * The value `invalid` is reserved as a sentinel for default-constructed or
 * uninitialized values and is rejected by the `Block` constructor.
 */
enum struct BlockType
  : std::uint_fast8_t
{
  invalid  = 0u, ///< Sentinel value; not a valid block type.

  sequence = 1u, ///< A sequence (shuntsu): three consecutive tiles of the same suit (e.g. 2m 3m 4m).
  triplet  = 2u, ///< A triplet (koutsu): three identical tiles.
  quad     = 3u, ///< A quad (kantsu): four identical tiles.
  pair     = 4u, ///< A pair (toitsu): two identical tiles, typically the head of the hand.
}; // enum struct BlockType

/**
 * @brief A single block (sequence, triplet, quad, or pair) of a standard mahjong hand.
 *
 * A `Block` is the unit produced by decomposing a standard winning hand into
 * its structural components. It carries three pieces of information:
 * - The kind of block (see @ref BlockType).
 * - The "base tile" of the block, encoded as an internal tile index in the
 *   range `[0, 34)` (see `FuHan::tileToString_`). The base tile identifies
 *   the block as follows:
 *   - For a `sequence`: the smallest tile of the run (e.g. `2m` for the
 *     run `2m-3m-4m`). Must be a number tile (`< 27`) whose rank is at
 *     most 7 (i.e. `base_tile % 9 < 7`).
 *   - For a `triplet`, `quad`, or `pair`: the (identical) tile that the
 *     block consists of.
 * - The "is open" flag (see @ref isOpen). The value of this flag is
 *   determined by the following rules:
 *   - A block formed by a call (chi, pon, daiminkan, or kakan) is open
 *     (`is_open_ == true`).
 *   - A block formed by a concealed kan (ankan) is not open
 *     (`is_open_ == false`).
 *   - For a block composed entirely of tiles from the concealed hand
 *     (i.e. not formed by any call):
 *     - If the win is a self-draw (tsumo), the block is not open
 *       (`is_open_ == false`).
 *     - If the win is by discard (ron) and the winning tile is one of
 *       the constituent tiles of the block, the block is open
 *       (`is_open_ == true`).
 *     - If the win is by discard (ron) and the winning tile is not one
 *       of the constituent tiles of the block, the block is not open
 *       (`is_open_ == false`).
 *
 * `Block` is a value type: it is default-constructible, copyable, and
 * assignable, and its accessors are read-only. A default-constructed
 * `Block` is in an *invalid* state (its type is `BlockType::invalid`)
 * and is intended only as a placeholder for later assignment; calling
 * accessors other than @ref isValid on an invalid block yields
 * unspecified values, and @ref print throws. Use @ref isValid to test
 * whether a `Block` carries a meaningful value.
 */
class Block
{
public:
  /**
   * @brief Default-constructs an invalid `Block`.
   *
   * The resulting block has type `BlockType::invalid` and is intended
   * only as a placeholder (e.g. for elements of a fixed-size array that
   * are filled in later by assignment). @ref isValid returns `false` for
   * such a block, and accessors other than @ref isValid should not be
   * relied upon until the block has been assigned a valid value.
   */
  Block()
    : type_(BlockType::invalid)
    , base_tile_(UINT_FAST8_MAX)
    , is_open_(false)
  {}

  /**
   * @brief Constructs a `Block`.
   *
   * @param type      The kind of block. Must not be `BlockType::invalid`.
   * @param base_tile The base tile of the block as an internal tile index.
   *                  Must be in `[0, 34)`. For `BlockType::sequence`, must
   *                  additionally be a number tile (`< 27`) with rank at most
   *                  7 (`base_tile % 9 < 7`).
   * @param is_open   Whether the block is open. The value must follow the
   *                  rules described in the class documentation: blocks
   *                  formed by chi, pon, daiminkan, or kakan are open;
   *                  blocks formed by ankan are closed; for blocks built
   *                  from concealed-hand tiles, the value is `true` if and
   *                  only if the win is by ron and the block contains the
   *                  winning tile.
   * @throws std::invalid_argument If @p type is `BlockType::invalid`, if
   *         @p base_tile is not a valid tile index, or if @p type is
   *         `BlockType::sequence` and @p base_tile cannot start a valid run.
   */
  Block(BlockType const type, std::uint_fast8_t const base_tile, bool const is_open)
    : type_(type)
    , base_tile_(base_tile)
    , is_open_(is_open)
  {
    if (type_ == BlockType::invalid) {
      throw std::invalid_argument("An invalid block type.");
    }
    if (base_tile_ >= 34u) {
      std::ostringstream oss;
      oss << static_cast<unsigned>(base_tile_) << ": An invalid base tile.";
      throw std::invalid_argument(oss.str());
    }
    if (type_ == BlockType::sequence) {
      if (base_tile_ >= 27u || base_tile_ % 9u >= 7u) {
        std::ostringstream oss;
        oss << static_cast<unsigned>(base_tile_) << ": An invalid base tile for a sequence.";
        throw std::invalid_argument(oss.str());
      }
    }
  }

  Block(Block const &) = default;

  Block &operator=(Block const &) = default;

  /**
   * @brief Tests whether this block carries a valid value.
   *
   * A `Block` is valid if and only if it was constructed through the
   * three-argument constructor (or copied/assigned from such a block).
   * Default-constructed blocks are invalid.
   *
   * @return `true` if @ref getType returns a value other than
   *         `BlockType::invalid`, otherwise `false`.
   */
  bool isValid() const
  {
    return type_ != BlockType::invalid;
  }

  /**
   * @brief Returns the kind of this block.
   *
   * @return The `BlockType` of this block. Returns `BlockType::invalid`
   *         if and only if this block was default-constructed and has
   *         not since been assigned a valid value (see @ref isValid).
   */
  BlockType getType() const
  {
    return type_;
  }

  /**
   * @brief Tests whether this block is a sequence (shuntsu).
   *
   * @return `true` if @ref getType returns `BlockType::sequence`, otherwise `false`.
   */
  bool isSequence() const
  {
    return type_ == BlockType::sequence;
  }

  /**
   * @brief Tests whether this block is a triplet (koutsu).
   *
   * @return `true` if @ref getType returns `BlockType::triplet`, otherwise `false`.
   */
  bool isTriplet() const
  {
    return type_ == BlockType::triplet;
  }

  /**
   * @brief Tests whether this block is a quad (kantsu).
   *
   * @return `true` if @ref getType returns `BlockType::quad`, otherwise `false`.
   */
  bool isQuad() const
  {
    return type_ == BlockType::quad;
  }

  /**
   * @brief Tests whether this block is a pair (toitsu).
   *
   * @return `true` if @ref getType returns `BlockType::pair`, otherwise `false`.
   */
  bool isPair() const
  {
    return type_ == BlockType::pair;
  }

  /**
   * @brief Returns the base tile of this block.
   *
   * For a sequence, this is the smallest tile of the run; for a triplet,
   * quad, or pair, this is the (identical) tile that the block consists of.
   *
   * @return The base tile as an internal tile index in `[0, 34)`.
   */
  std::uint_fast8_t getBaseTile() const
  {
    return base_tile_;
  }

  /**
   * @brief Tests whether this block is open.
   *
   * The open/closed status follows the rules described in the class
   * documentation: blocks formed by chi, pon, daiminkan, or kakan are
   * open; blocks formed by ankan are closed; for blocks built from
   * concealed-hand tiles, the block is open if and only if the win is by
   * ron and the block contains the winning tile.
   *
   * @return `true` if the block is open, `false` if it is closed.
   */
  bool isOpen() const
  {
    return is_open_;
  }

  /**
   * @brief Writes a human-readable representation of this block to @p os.
   *
   * Each block is rendered as its kind name followed by a parenthesised,
   * comma-separated list of the constituent tile strings as produced by
   * `FuHan::tileToString_` (e.g. `"sequence(2m,3m,4m)"`,
   * `"triplet(5p,5p,5p)"`, `"pair(E,E)"`).
   *
   * @param os The output stream to write to.
   * @throws std::invalid_argument If this block is invalid (i.e.
   *         @ref isValid returns `false`).
   * @throws std::logic_error If this block has an unknown internal type
   *         (should not happen for blocks constructed through the public
   *         constructor).
   */
  void print(std::ostream &os) const
  {
    if (type_ == BlockType::invalid) {
      throw std::invalid_argument("Cannot print an invalid block.");
    }
    std::string const tile_str = FuHan::tileToString_(base_tile_);
    switch (type_) {
    case BlockType::sequence:
      os << "sequence(" << tile_str
         << "," << FuHan::tileToString_(base_tile_ + 1u)
         << "," << FuHan::tileToString_(base_tile_ + 2u) << ")";
      break;
    case BlockType::triplet:
      os << "triplet(" << tile_str << "," << tile_str << "," << tile_str << ")";
      break;
    case BlockType::quad:
      os << "quad(" << tile_str << "," << tile_str << "," << tile_str << "," << tile_str << ")";
      break;
    case BlockType::pair:
      os << "pair(" << tile_str << "," << tile_str << ")";
      break;
    default:
      std::ostringstream oss;
      oss << static_cast<unsigned>(type_) << ": An unknown block type.";
      throw std::logic_error(oss.str());
    }
  }

private:
  BlockType type_;
  std::uint_fast8_t base_tile_;
  bool is_open_;
}; // class Block

/**
 * @brief Stream-insertion operator for `Block`.
 *
 * Writes a human-readable representation of @p block to @p os by delegating
 * to `Block::print` (e.g. `"sequence(2m,3m,4m)"`, `"triplet(5p,5p,5p)"`,
 * `"pair(E,E)"`).
 *
 * @param os    The output stream to write to.
 * @param block The block to format.
 * @return A reference to @p os, to allow chaining.
 */
inline std::ostream &operator<<(std::ostream &os, Block const &block)
{
  block.print(os);
  return os;
}

/**
 * @brief An enumeration identifying the wait pattern of a tenpai hand.
 *
 * The wait pattern characterizes how the winning tile completes the hand,
 * and is one of the inputs to the fu calculation: depending on which
 * pattern is used to interpret a winning hand, the awarded fu may differ.
 *
 * The value `invalid` is reserved as a sentinel for default-constructed or
 * uninitialized values.
 */
enum struct WaitType
  : std::uint_fast8_t
{
  invalid     = 0u, ///< Sentinel value; not a valid wait pattern.

  two_sided   = 1u, ///< Two-sided wait (ryanmen, 両面待ち): a partial run waits on either of two tiles (e.g. 2m-3m waits on 1m or 4m).
  double_pair = 2u, ///< Double-pair wait (shanpon/shabo, 双碰待ち): two pairs wait for either to become a triplet.
  inside      = 3u, ///< Closed/inside wait (kanchan, 嵌張待ち): a partial run waits on the middle tile (e.g. 2m-4m waits on 3m).
  edge        = 4u, ///< Edge wait (penchan, 辺張待ち): a partial run waits on a single edge tile (e.g. 1m-2m waits on 3m, or 8m-9m waits on 7m).
  pair        = 5u, ///< Single-tile (pair) wait (tanki, 単騎待ち): a lone tile waits for its partner to form the pair.
}; // enum struct WaitType

/**
 * @brief Stream-insertion operator for `WaitType`.
 *
 * Writes a human-readable representation of @p wait_type to @p os. The
 * output is the lowercase enumerator name (e.g. `"two_sided"`,
 * `"double_pair"`, `"inside"`, `"edge"`, `"pair"`, or `"invalid"`).
 *
 * @param os        The output stream to write to.
 * @param wait_type The wait pattern to format.
 * @return A reference to @p os, to allow chaining.
 * @throws std::logic_error If @p wait_type is not a recognised enumerator
 *         (should not happen for values produced by this library).
 */
inline std::ostream &operator<<(std::ostream &os, WaitType const wait_type)
{
  switch (wait_type) {
  case WaitType::invalid:
    os << "invalid";
    break;
  case WaitType::two_sided:
    os << "two_sided";
    break;
  case WaitType::double_pair:
    os << "double_pair";
    break;
  case WaitType::inside:
    os << "inside";
    break;
  case WaitType::edge:
    os << "edge";
    break;
  case WaitType::pair:
    os << "pair";
    break;
  default:
    std::ostringstream oss;
    oss << static_cast<unsigned>(wait_type) << ": An unknown wait type.";
    throw std::logic_error(oss.str());
  }
  return os;
}

/**
 * @brief A single interpretation of a standard tenpai/winning hand.
 *
 * A standard mahjong hand may admit more than one structural
 * decomposition: the same set of tiles can sometimes be partitioned into
 * blocks in multiple ways, and the winning tile may complete the hand
 * through different wait patterns. A `DecompositionElement` captures
 * exactly one such interpretation by pairing:
 * - a block decomposition of the hand (see @ref blocks), and
 * - the wait pattern under which the winning tile completes that
 *   decomposition (see @ref wait_type).
 *
 * The score calculator enumerates `DecompositionElement` values for a
 * given hand and selects the interpretation that yields the highest
 * score under the applicable fu/han rules.
 */
struct DecompositionElement
{
  /**
   * @brief The block decomposition of the hand under this interpretation.
   *
   * A standard winning hand always consists of exactly five blocks: four
   * melds (sequences, triplets, or quads) plus one pair, including any
   * open blocks formed by calls. The order of blocks within the array is
   * not semantically significant for scoring, but is preserved as
   * produced by the decomposer. See @ref Block for details.
   */
  std::array<Block, 5u> blocks;

  /**
   * @brief The wait pattern by which the winning tile completes @ref blocks.
   *
   * Identifies how the winning tile fits into the decomposition (e.g.
   * two-sided, closed, edge, double-pair, or single-tile wait). Never
   * `WaitType::invalid` for values produced by the library. See
   * @ref WaitType for details.
   */
  WaitType wait_type;
}; // struct DecompositionElement

} // namespace FuHan::Standard_

#endif // !defined(FUHAN_STANDARD_INTERNAL_HPP_INCLUDE_GUARD)
