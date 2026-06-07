// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#if !defined(FUHAN_TYPES_HPP_INCLUDE_GUARD)
#define FUHAN_TYPES_HPP_INCLUDE_GUARD

#include <utility>
#include <stdexcept>
#include <climits>
#include <cstdint>


namespace FuHan{

/**
 * @brief An enumeration representing the four winds in mahjong.
 *
 * The enumerators correspond to the four wind tiles (East, South, West, North)
 * and are used to specify a round wind or a seat wind. The underlying integer
 * values (27-30) match the tile indices used internally by the library, so a
 * `Wind` value can be interpreted directly as a wind tile.
 */
enum struct Wind
  : std::uint_fast8_t
{
  east_  = 27u, ///< East wind (tile index 27).
  south_ = 28u, ///< South wind (tile index 28).
  west_  = 29u, ///< West wind (tile index 29).
  north_ = 30u, ///< North wind (tile index 30).
}; // enum struct Wind

/**
 * @brief A `Wind` constant representing the East wind.
 *
 * Used to specify East as a round wind or seat wind.
 */
inline constexpr Wind east = Wind::east_;
/**
 * @brief A `Wind` constant representing the South wind.
 *
 * Used to specify South as a round wind or seat wind.
 */
inline constexpr Wind south = Wind::south_;
/**
 * @brief A `Wind` constant representing the West wind.
 *
 * Used to specify West as a round wind or seat wind.
 */
inline constexpr Wind west = Wind::west_;
/**
 * @brief A `Wind` constant representing the North wind.
 *
 * Used to specify North as a round wind or seat wind.
 */
inline constexpr Wind north = Wind::north_;

/**
 * @brief A bit-flag enumeration describing the situational context of a winning hand.
 *
 * Each enumerator is a distinct bit, so multiple flags can be combined using
 * the bitwise `|` operator (see `operator|(Context, Context)`) to describe a
 * situation in which several conditions hold simultaneously (e.g. a riichi
 * tsumo with ippatsu). Individual flags can be tested with the bitwise `&`
 * operator.
 *
 * These flags are consumed by the score calculator to determine which yaku
 * apply to a winning hand and to compute the resulting fu and han.
 */
enum struct Context
  : std::uint_fast16_t
{
  tsumo_          = 1u <<  1u, ///< The winning tile was self-drawn (tsumo).
  ron_            = 1u <<  2u, ///< The winning tile was claimed from a discard (ron).
  riichi_         = 1u <<  3u, ///< Riichi was declared.
  chankan_        = 1u <<  4u, ///< The win was achieved by robbing a kan (chankan).
  rinshan_kaihou_ = 1u <<  5u, ///< The winning tile was drawn from the dead wall after a kan (rinshan kaihou).
  haitei_raoyue_  = 1u <<  6u, ///< The winning tile was the last tile drawn from the wall (haitei raoyue).
  houtei_raoyui_  = 1u <<  7u, ///< The winning tile was the final discard of the round (houtei raoyui).
  double_riichi_  = 1u <<  8u, ///< Riichi was declared on the very first uninterrupted draw (double riichi).
  ippatsu_        = 1u <<  9u, ///< The win occurred within one go-around after declaring riichi (ippatsu).
  tenhou_         = 1u << 10u, ///< The dealer won on the initial hand (tenhou).
  chiihou_        = 1u << 11u  ///< A non-dealer won on the first uninterrupted draw (chiihou).
}; // enum struct Context

/**
 * @brief A `Context` flag indicating that the winning tile was self-drawn (tsumo).
 */
inline constexpr Context tsumo = Context::tsumo_;
/**
 * @brief A `Context` flag indicating that the winning tile was claimed from an opponent's discard (ron).
 */
inline constexpr Context ron = Context::ron_;
/**
 * @brief A `Context` flag indicating that riichi was declared.
 */
inline constexpr Context riichi = Context::riichi_;
/**
 * @brief A `Context` flag indicating that the win was achieved by robbing a kan (chankan).
 */
inline constexpr Context chankan = Context::chankan_;
/**
 * @brief A `Context` flag indicating that the winning tile was drawn from the dead wall after a kan (rinshan kaihou).
 */
inline constexpr Context rinshan_kaihou = Context::rinshan_kaihou_;
/**
 * @brief A `Context` flag indicating that the winning tile was the last tile drawn from the live wall (haitei raoyue).
 */
inline constexpr Context haitei_raoyue = Context::haitei_raoyue_;
/**
 * @brief A `Context` flag indicating that the winning tile was the final discard of the round (houtei raoyui).
 */
inline constexpr Context houtei_raoyui = Context::houtei_raoyui_;
/**
 * @brief A `Context` flag indicating that riichi was declared on the very first uninterrupted draw (double riichi).
 */
inline constexpr Context double_riichi = Context::double_riichi_;
/**
 * @brief A `Context` flag indicating that the win occurred within one go-around after declaring riichi (ippatsu).
 */
inline constexpr Context ippatsu = Context::ippatsu_;
/**
 * @brief A `Context` flag indicating that the dealer won on the initial hand (tenhou).
 */
inline constexpr Context tenhou = Context::tenhou_;
/**
 * @brief A `Context` flag indicating that a non-dealer won on the first uninterrupted draw (chiihou).
 */
inline constexpr Context chiihou = Context::chiihou_;

/**
 * @brief Computes the bitwise AND of two `Context` flag sets.
 *
 * Treats the operands as bitmasks over the `Context` flag bits and returns a
 * `Context` value containing only the flags set in both @p lhs and @p rhs.
 * This is typically used to test whether a particular flag (or set of flags)
 * is present in a combined `Context` value.
 *
 * @param lhs The left-hand `Context` operand.
 * @param rhs The right-hand `Context` operand.
 * @return A `Context` value whose bits are the bitwise AND of the operands'
 *         underlying representations.
 */
inline constexpr Context operator&(Context const lhs, Context const rhs)
{
  return static_cast<Context>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

/**
 * @brief Computes the bitwise OR of two `Context` flag sets.
 *
 * Treats the operands as bitmasks over the `Context` flag bits and returns a
 * `Context` value containing every flag set in either @p lhs or @p rhs. This
 * is typically used to combine multiple `Context` flags into a single value
 * describing the full situational context of a winning hand (e.g.
 * `riichi | tsumo | ippatsu`).
 *
 * @param lhs The left-hand `Context` operand.
 * @param rhs The right-hand `Context` operand.
 * @return A `Context` value whose bits are the bitwise OR of the operands'
 *         underlying representations.
 */
inline constexpr Context operator|(Context const lhs, Context const rhs)
{
  return static_cast<Context>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

/**
 * @brief Compound bitwise AND assignment for `Context` flag sets.
 *
 * Replaces @p lhs with the bitwise AND of @p lhs and @p rhs, retaining only
 * the flags that are set in both operands. This is the in-place counterpart
 * to `operator&(Context, Context)` and is typically used to mask a `Context`
 * value down to a subset of flags of interest.
 *
 * @param lhs The `Context` value to update in place.
 * @param rhs The `Context` mask to AND into @p lhs.
 * @return A reference to @p lhs after the update.
 */
inline constexpr Context &operator&=(Context &lhs, Context const rhs)
{
  lhs = lhs & rhs;
  return lhs;
}

/**
 * @brief Compound bitwise OR assignment for `Context` flag sets.
 *
 * Replaces @p lhs with the bitwise OR of @p lhs and @p rhs, adding every
 * flag set in @p rhs to @p lhs. This is the in-place counterpart to
 * `operator|(Context, Context)` and is typically used to incrementally
 * accumulate `Context` flags into a single value.
 *
 * @param lhs The `Context` value to update in place.
 * @param rhs The `Context` flags to OR into @p lhs.
 * @return A reference to @p lhs after the update.
 */
inline constexpr Context &operator|=(Context &lhs, Context const rhs)
{
  lhs = lhs | rhs;
  return lhs;
}

/**
 * @brief Computes the bitwise complement of a `Context` flag set.
 *
 * Treats @p context as a bitmask over the underlying `Context`
 * representation and returns a `Context` value whose bits are the
 * bitwise inverse of @p context. This is primarily useful when building
 * masks, for example to remove a subset of flags with
 * `context & ~flags_to_remove`.
 *
 * The complement is taken over the full underlying integer
 * representation, not only over the currently defined `Context` flag
 * bits. Callers that need a value containing only known flags should
 * additionally mask the result with the desired set of valid flags.
 *
 * @param context The `Context` value to complement.
 * @return A `Context` value whose underlying representation is the
 *         bitwise complement of @p context.
 */
inline constexpr Context operator~(Context const context)
{
  return static_cast<Context>(~std::to_underlying(context));
}

/**
 * @brief Tests whether the tsumo flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref tsumo is set in @p context, otherwise `false`.
 */
inline constexpr bool isTsumo(Context const context)
{
  return (context & tsumo) == tsumo;
}

/**
 * @brief Tests whether the ron flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref ron is set in @p context, otherwise `false`.
 */
inline constexpr bool isRon(Context const context)
{
  return (context & ron) == ron;
}

/**
 * @brief Tests whether the riichi flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref riichi is set in @p context, otherwise `false`.
 */
inline constexpr bool isRiichi(Context const context)
{
  return (context & riichi) == riichi;
}

/**
 * @brief Tests whether the chankan flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref chankan is set in @p context, otherwise `false`.
 */
inline constexpr bool isChankan(Context const context)
{
  return (context & chankan) == chankan;
}

/**
 * @brief Tests whether the rinshan kaihou flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref rinshan_kaihou is set in @p context, otherwise `false`.
 */
inline constexpr bool isRinshanKaihou(Context const context)
{
  return (context & rinshan_kaihou) == rinshan_kaihou;
}

/**
 * @brief Tests whether the haitei raoyue flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref haitei_raoyue is set in @p context, otherwise `false`.
 */
inline constexpr bool isHaiteiRaoyue(Context const context)
{
  return (context & haitei_raoyue) == haitei_raoyue;
}

/**
 * @brief Tests whether the houtei raoyui flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref houtei_raoyui is set in @p context, otherwise `false`.
 */
inline constexpr bool isHouteiRaoyui(Context const context)
{
  return (context & houtei_raoyui) == houtei_raoyui;
}

/**
 * @brief Tests whether the double riichi flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref double_riichi is set in @p context, otherwise `false`.
 */
inline constexpr bool isDoubleRiichi(Context const context)
{
  return (context & double_riichi) == double_riichi;
}

/**
 * @brief Tests whether the ippatsu flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref ippatsu is set in @p context, otherwise `false`.
 */
inline constexpr bool isIppatsu(Context const context)
{
  return (context & ippatsu) == ippatsu;
}

/**
 * @brief Tests whether the tenhou flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref tenhou is set in @p context, otherwise `false`.
 */
inline constexpr bool isTenhou(Context const context)
{
  return (context & tenhou) == tenhou;
}

/**
 * @brief Tests whether the chiihou flag is set in a `Context` value.
 *
 * @param context The `Context` value to inspect.
 * @return `true` if @ref chiihou is set in @p context, otherwise `false`.
 */
inline constexpr bool isChiihou(Context const context)
{
  return (context & chiihou) == chiihou;
}

/**
 * @brief The outcome of a score calculation for a winning hand.
 *
 * Aggregates the three quantities required to determine the score of a
 * mahjong winning hand: the fu (minipoints), the han (doubles), and the
 * yakuman multiplier.
 *
 * The fields are interpreted by case analysis on the input. The cases
 * below are evaluated in order; the first matching case determines the
 * values of all three fields, and the cases are mutually exclusive and
 * exhaustive:
 *
 * -# **The input does not form a winning shape.** Its tiles cannot be
 *    partitioned into any valid winning shape (standard four-melds-
 *    plus-one-pair, Chiitoitsu, or Kokushi musou). In this case
 *    @ref fu, @ref han, and @ref yakuman_multiplier are all `0`.
 * -# **The input forms a winning shape but no yaku (excluding dora) is
 *    established.** @ref fu is set to the non-zero fu value computed
 *    from the winning shape, while @ref han and @ref yakuman_multiplier
 *    are both `0`. Dora (including aka dora and ura dora) are *not*
 *    reflected in @ref han here, because dora alone does not
 *    constitute a yaku and the hand is therefore not a winning hand
 *    for scoring purposes.
 * -# **The input establishes one or more non-yakuman yaku, or a
 *    counted yakuman (kazoe yakuman).** @ref fu and @ref han are both
 *    set to the non-zero values computed from the winning shape and
 *    the yaku set (with dora included in @ref han), and
 *    @ref yakuman_multiplier is `0`. A counted yakuman manifests as
 *    @ref han `>= 13` together with @ref yakuman_multiplier `== 0`.
 * -# **The input establishes Kokushi musou (thirteen orphans).**
 *    @ref fu and @ref han are both `0` (fu is not defined for this
 *    hand and yakuman wins do not use @ref han), and
 *    @ref yakuman_multiplier is the appropriate non-zero value for
 *    the recognised form of Kokushi musou.
 * -# **The input establishes a non-counted yakuman other than
 *    Kokushi musou.** @ref fu is set to the non-zero fu value
 *    computed from the winning shape, @ref han is `0`, and
 *    @ref yakuman_multiplier is the appropriate non-zero sum of
 *    per-yaku yakuman contributions (each ordinary yakuman contributes
 *    `1` and each double yakuman contributes `2`).
 *
 * The above cases are exhaustive.
 */
struct Result
{
  /**
   * @brief The fu (minipoints) of the winning hand.
   *
   * Following the case analysis on the parent @ref Result:
   * - Case 1 (no winning shape): `0`.
   * - Case 2 (winning shape, no yaku): a non-zero fu value computed
   *   from the winning shape.
   * - Case 3 (non-yakuman yaku, or counted yakuman): a non-zero fu
   *   value computed from the winning shape and yaku set.
   * - Case 4 (Kokushi musou): `0`, because fu is not defined for
   *   this hand.
   * - Case 5 (non-counted yakuman other than Kokushi musou): a
   *   non-zero fu value computed from the winning shape.
   */
  std::uint_fast8_t fu;
  /**
   * @brief The han (doubles) of the winning hand.
   *
   * Following the case analysis on the parent @ref Result:
   * - Case 1 (no winning shape): `0`.
   * - Case 2 (winning shape, no yaku): `0`. Dora (including aka
   *   dora and ura dora) are not reflected here, because dora alone
   *   does not constitute a yaku and the hand is not a winning hand
   *   for scoring purposes.
   * - Case 3 (non-yakuman yaku, or counted yakuman): a non-zero han
   *   count that aggregates every established non-yakuman yaku and
   *   every dora. A counted yakuman (kazoe yakuman) appears as a
   *   value `>= 13` with @ref yakuman_multiplier `== 0`.
   * - Case 4 (Kokushi musou): `0`, because yakuman wins do not use
   *   @ref han for scoring.
   * - Case 5 (non-counted yakuman other than Kokushi musou): `0`,
   *   for the same reason as case 4.
   */
  std::uint_fast8_t han;
  /**
   * @brief The cumulative yakuman multiplier of the winning hand.
   *
   * Indicates how many "units" of yakuman the hand is worth. The
   * value is the sum of the multipliers contributed by every
   * non-counted yakuman yaku that is established, where each ordinary
   * yakuman contributes `1` and each double yakuman contributes `2`.
   *
   * Following the case analysis on the parent @ref Result:
   * - Case 1 (no winning shape): `0`.
   * - Case 2 (winning shape, no yaku): `0`.
   * - Case 3 (non-yakuman yaku, or counted yakuman): `0`. In
   *   particular, counted yakuman (kazoe yakuman) is *not* reported
   *   here; it is represented by @ref han `>= 13` with this field
   *   set to `0`.
   * - Case 4 (Kokushi musou): the appropriate non-zero value for
   *   the recognised form of Kokushi musou.
   * - Case 5 (non-counted yakuman other than Kokushi musou): the
   *   appropriate non-zero value, namely the sum of the per-yaku
   *   yakuman contributions described above.
   *
   * The non-zero values follow the standard scale:
   * - `1`: a single yakuman.
   * - `2`: a double yakuman, or two single yakuman stacked together.
   * - `3` or more: the corresponding multiple yakuman (triple
   *   yakuman, quadruple yakuman, etc.), obtained by summing the
   *   contributions of every established yakuman.
   *
   * The base score of a single yakuman is multiplied by this value
   * to obtain the final yakuman score.
   */
  std::uint_fast8_t yakuman_multiplier;
}; // struct Result

/**
 * @brief Strict weak ordering on `Result` by score-relevant magnitude.
 *
 * Compares two scoring outcomes so that the "smaller" result is the one
 * that ranks lower for choosing the best interpretation of a hand. The
 * comparison proceeds in stages:
 *
 * -# `yakuman_multiplier` is compared first. A result with a strictly
 *    larger multiplier is always considered greater, since any
 *    (non-counted) yakuman dominates any non-yakuman result and, among
 *    yakuman results, a higher multiplier yields a higher score.
 * -# When the multipliers are equal, @ref Result::han is compared next.
 *    A result with more han ranks higher. This makes any non-yakuman
 *    yaku-bearing result (`han > 0`) rank above a no-yaku winning shape
 *    (`han == 0`) when neither side is a non-counted yakuman.
 * -# When the multipliers and han are both equal, the products
 *    `roundUp10(fu) * 2^han` are compared, where `roundUp10(fu)` denotes
 *    @ref Result::fu rounded up to the nearest multiple of 10. Since
 *    @ref Result::han is equal at this stage, this separates results by
 *    their rounded-fu contribution to the base-score proxy. Rounding
 *    @ref Result::fu first ensures that two decompositions whose
 *    unrounded fu values differ only within the same 10-fu bucket are
 *    not artificially separated by this stage.
 * -# When the previous stages all tie, @ref Result::fu is compared in
 *    ascending order as a final tie-breaker.
 *
 * @note This is *not* a comparison of final scores. In particular, two
 *       results that both saturate to the same capped score (e.g. both
 *       mangan) may still compare unequal under this ordering, and han
 *       is intentionally considered before the fu-derived base-score
 *       proxy. The intended use is to choose the preferred scoring
 *       interpretation among candidates produced by the library, not to
 *       compute a point payment.
 *
 * @param lhs The left-hand `Result` operand.
 * @param rhs The right-hand `Result` operand.
 * @return `true` if @p lhs ranks strictly below @p rhs under the
 *         ordering described above, otherwise `false`.
 * @throws std::overflow_error If @ref Result::han is too large for the
 *         internal product calculation performed after both operands
 *         have the same yakuman multiplier and han, or if
 *         `roundUp10(fu) * 2^han` may overflow `unsigned long long`.
 */
inline bool operator<(Result const &lhs, Result const &rhs)
{
  if (lhs.yakuman_multiplier != rhs.yakuman_multiplier) {
    return lhs.yakuman_multiplier < rhs.yakuman_multiplier;
  }

  if (lhs.han != rhs.han) {
    return lhs.han < rhs.han;
  }

  // Chiitoitsu stays at 25 fu and is not rounded up to the next multiple of 10.
  std::uint_fast8_t const lhs_rounded_fu = lhs.fu == 25u ? 25u : (lhs.fu + 9u) / 10u * 10u;
  std::uint_fast8_t const rhs_rounded_fu = rhs.fu == 25u ? 25u : (rhs.fu + 9u) / 10u * 10u;
  if (lhs.han >= 64u) {
    throw std::overflow_error("Han value exceeds maximum representable value of 63.");
  }
  if (ULLONG_MAX / (1ull << lhs.han) <= lhs_rounded_fu) {
    throw std::overflow_error("Fu and han values are too large to compare without overflow.");
  }
  if (rhs.han >= 64u) {
    throw std::overflow_error("Han value exceeds maximum representable value of 63.");
  }
  if (ULLONG_MAX / (1ull << rhs.han) <= rhs_rounded_fu) {
    throw std::overflow_error("Fu and han values are too large to compare without overflow.");
  }
  unsigned long long const lhs_product = lhs_rounded_fu * (1ull << lhs.han);
  unsigned long long const rhs_product = rhs_rounded_fu * (1ull << rhs.han);
  if (lhs_product != rhs_product) {
    return lhs_product < rhs_product;
  }
  return lhs.fu < rhs.fu;
}

/**
 * @brief Strict-greater ordering on `Result`, delegating to `operator<`.
 *
 * Equivalent to `rhs < lhs`. See @ref operator<(Result const &, Result const &)
 * for the precise definition of the ordering.
 *
 * @param lhs The left-hand `Result` operand.
 * @param rhs The right-hand `Result` operand.
 * @return `true` if @p lhs ranks strictly above @p rhs, otherwise `false`.
 */
inline bool operator>(Result const &lhs, Result const &rhs)
{
  return rhs < lhs;
}

/**
 * @brief Non-strict-less ordering on `Result`, delegating to `operator<`.
 *
 * Equivalent to `!(rhs < lhs)`. See
 * @ref operator<(Result const &, Result const &) for the precise
 * definition of the ordering.
 *
 * @param lhs The left-hand `Result` operand.
 * @param rhs The right-hand `Result` operand.
 * @return `true` if @p lhs does not rank strictly above @p rhs,
 *         otherwise `false`.
 */
inline bool operator<=(Result const &lhs, Result const &rhs)
{
  return !(rhs < lhs);
}

/**
 * @brief Non-strict-greater ordering on `Result`, delegating to `operator<`.
 *
 * Equivalent to `!(lhs < rhs)`. See
 * @ref operator<(Result const &, Result const &) for the precise
 * definition of the ordering.
 *
 * @param lhs The left-hand `Result` operand.
 * @param rhs The right-hand `Result` operand.
 * @return `true` if @p lhs does not rank strictly below @p rhs,
 *         otherwise `false`.
 */
inline bool operator>=(Result const &lhs, Result const &rhs)
{
  return !(lhs < rhs);
}

} // namespace FuHan

#endif // !defined(FUHAN_TYPES_HPP_INCLUDE_GUARD)
