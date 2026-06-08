// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#include <fuhan/fuhan.hpp>
#include <fuhan/types.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>


namespace{

struct TestCase
{
  std::string uuid;
  std::uint_fast8_t round_wind;
  std::uint_fast8_t round_index_within_round_wind;
  std::uint_fast8_t num_counter_sticks;
  std::uint_fast8_t seat;
  std::array<std::uint_fast8_t, 34u> concealed_hand;
  std::array<std::uint_fast8_t, 21u> chii_list;
  std::array<std::uint_fast8_t, 34u> pon_list;
  std::array<std::uint_fast8_t, 34u> open_kan_list;
  std::array<std::uint_fast8_t, 34u> ankan_list;
  std::uint_fast8_t winning_tile;
  FuHan::Context context;
  std::uint_fast8_t num_dora;
  std::uint_fast8_t expected_fu;
  std::uint_fast8_t expected_han;
  std::uint_fast8_t expected_yakuman_multiplier;
};

std::vector<std::string> split(std::string const &s, char delimiter)
{
  if (s.empty()) {
    return {};
  }

  std::vector<std::string> result;
  std::string current;
  for (char c : s) {
    if (c == delimiter) {
      result.push_back(current);
      current.clear();
    } else {
      current.push_back(c);
    }
  }
  result.push_back(current);
  return result;
}

std::pair<std::uint_fast8_t, bool> adjustTileIndex(std::uint_fast8_t const tile)
{
  if (tile == 0u) {
    return {4u, true};
  }
  if (1u <= tile && tile <= 9u) {
    return {tile - 1u, false};
  }
  if (tile == 10u) {
    return {13u, true};
  }
  if (11u <= tile && tile <= 19u) {
    return {tile - 2u, false};
  }
  if (tile == 20u) {
    return {22u, true};
  }
  if (21u <= tile && tile < 37u) {
    return {tile - 3u, false};
  }
  throw std::invalid_argument("An invalid tile index: " + std::to_string(tile));
}

std::pair<std::array<std::uint_fast8_t, 34u>, std::uint_fast8_t>
parseConcealedHand(std::vector<std::string> const &fields)
{
  std::array<std::uint_fast8_t, 34u> concealed_hand{};
  std::uint_fast8_t num_red_dora = 0u;
  for (std::string const &tile : fields) {
    unsigned long long tile_index_ull = std::stoull(tile);
    if (tile_index_ull >= 37u) {
      throw std::runtime_error("An invalid tile index: " + tile);
    }
    std::uint_fast8_t const tile_index = static_cast<std::uint_fast8_t>(tile_index_ull);
    if (tile_index >= 37u) {
      throw std::runtime_error("An invalid tile index: " + tile);
    }
    auto [adjusted_index, is_red_dora] = adjustTileIndex(tile_index);
    ++concealed_hand[adjusted_index];
    if (is_red_dora) {
      ++num_red_dora;
    }
  }
  return {concealed_hand, num_red_dora};
}

std::uint_fast8_t countDora(TestCase const &test_case, std::uint_fast8_t const dora_indicator)
{
  std::uint_fast8_t const dora_tile = [dora_indicator]{
    if (dora_indicator <= 8u) {
      return (dora_indicator + 1u) % 9u;
    }
    if (dora_indicator <= 17u) {
      return ((dora_indicator - 9u + 1u) % 9u) + 9u;
    }
    if (dora_indicator <= 26u) {
      return ((dora_indicator - 18u + 1u) % 9u) + 18u;
    }
    if (dora_indicator <= 30u) {
      return ((dora_indicator - 27u + 1u) % 4u) + 27u;
    }
    return (dora_indicator - 31u + 1u) % 3u + 31u;
  }();

  std::uint_fast8_t count = 0u;
  count += test_case.concealed_hand[dora_tile];
  for (std::uint_fast8_t i = 0u; i < 21u; ++i) {
    if (test_case.chii_list[i] == 0u) {
      continue;
    }
    std::uint_fast8_t const color = i / 7u;
    std::uint_fast8_t const base_rank = i % 7u;
    std::uint_fast8_t const base_tile = color * 9u + base_rank;
    for (std::uint_fast8_t j = 0u; j < 3u; ++j) {
      std::uint_fast8_t const tile = base_tile + j;
      if (tile == dora_tile) {
        count += test_case.chii_list[i];
      }
    }
  }
  count += 3u * test_case.pon_list[dora_tile];
  count += 4u * test_case.open_kan_list[dora_tile];
  count += 4u * test_case.ankan_list[dora_tile];
  if (test_case.winning_tile == dora_tile) {
    ++count;
  }
  return count;
}

TestCase parseTestCase(std::string const &line)
{
  std::vector<std::string> columns = split(line, '\t');
  if (columns.size() != 12u) {
    std::ostringstream oss;
    oss << columns.size() << " columns were found, but 12 were expected.\n";
    oss << "Line: " << line;
    throw std::runtime_error(oss.str());
  }

  TestCase test_case{};

  std::vector<std::string> fields = split(columns[0u], ',');
  if (fields.size() != 7u) {
    std::ostringstream oss;
    oss << fields.size() << " fields were found, but 7 were expected.\n";
    oss << "Line: " << line;
    throw std::runtime_error(oss.str());
  }
  test_case.uuid = fields[0u];
  unsigned long long const round_wind_ull = std::stoull(fields[3u]);
  if (round_wind_ull >= 4u) {
    throw std::runtime_error("An invalid round wind: " + line);
  }
  test_case.round_wind = static_cast<std::uint_fast8_t>(round_wind_ull);
  unsigned long long const round_index_within_round_wind_ull = std::stoull(fields[4u]);
  if (round_index_within_round_wind_ull >= 4u) {
    throw std::runtime_error("An invalid round index within round wind: " + line);
  }
  test_case.round_index_within_round_wind
    = static_cast<std::uint_fast8_t>(round_index_within_round_wind_ull);
  unsigned long long const num_counter_sticks_ull = std::stoull(fields[5u]);
  if (num_counter_sticks_ull > UINT_FAST8_MAX) {
    throw std::runtime_error("Too many counter sticks: " + line);
  }
  test_case.num_counter_sticks = static_cast<std::uint_fast8_t>(num_counter_sticks_ull);
  unsigned long long const seat_ull = std::stoull(fields[6u]);
  if (seat_ull >= 4u) {
    throw std::runtime_error("An invalid seat: " + line);
  }
  test_case.seat = static_cast<std::uint_fast8_t>(seat_ull);

  fields = split(columns[1u], ',');
  {
    std::uint_fast8_t num_red_dora = 0u;
    std::tie(test_case.concealed_hand, num_red_dora) = parseConcealedHand(fields);
    if (test_case.num_dora > UINT_FAST8_MAX - num_red_dora) {
      throw std::overflow_error("Too many dora in the test case: " + line);
    }
    test_case.num_dora += num_red_dora;
  }

  fields = split(columns[2u], ',');
  for (std::string const &field : fields) {
    std::vector<std::string> subfields = split(field, '-');
    if (subfields.size() != 3u) {
      throw std::runtime_error("An invalid chii: " + line);
    }
    std::uint_fast8_t base_tile = UINT_FAST8_MAX;
    for (std::uint_fast8_t i = 0u; i < 3u; ++i) {
      unsigned long long const tile_index_ull = std::stoull(subfields[i]);
      if (tile_index_ull >= 37u) {
        throw std::runtime_error("An invalid tile index: " + subfields[i]);
      }
      std::uint_fast8_t t = UINT_FAST8_MAX;
      bool is_red_dora = false;
      std::tie(t, is_red_dora) = adjustTileIndex(static_cast<std::uint_fast8_t>(tile_index_ull));
      base_tile = std::min(base_tile, t);
      if (is_red_dora) {
        if (test_case.num_dora > UINT_FAST8_MAX - 1u) {
          throw std::overflow_error("Too many dora in the test case: " + line);
        }
        ++test_case.num_dora;
      }
    }
    if (base_tile >= 34u) {
      throw std::runtime_error("An invalid base tile in chii: " + line);
    }
    std::uint_fast8_t const color = base_tile / 9u;
    if (color >= 3u) {
      throw std::runtime_error("A chii cannot contain honor tiles: " + line);
    }
    std::uint_fast8_t const base_rank = base_tile % 9u;
    if (base_rank >= 7u) {
      throw std::runtime_error("An invalid chii: " + line);
    }
    std::uint_fast8_t const chii_id = 7u * color + base_rank;
    ++test_case.chii_list[chii_id];
  }

  fields = split(columns[3u], ',');
  for (std::string const &field : fields) {
    std::vector<std::string> subfields = split(field, '-');
    if (subfields.size() != 3u) {
      throw std::runtime_error("An invalid pon: " + line);
    }
    std::uint_fast8_t base_tile = UINT_FAST8_MAX;
    for (std::uint_fast8_t i = 0u; i < 3u; ++i) {
      unsigned long long const tile_index_ull = std::stoull(subfields[i]);
      if (tile_index_ull >= 37u) {
        throw std::runtime_error("An invalid tile index: " + subfields[i]);
      }
      bool is_red_dora = false;
      std::tie(base_tile, is_red_dora)
        = adjustTileIndex(static_cast<std::uint_fast8_t>(tile_index_ull));
      if (is_red_dora) {
        if (test_case.num_dora > UINT_FAST8_MAX - 1u) {
          throw std::overflow_error("Too many dora in the test case: " + line);
        }
        ++test_case.num_dora;
      }
    }
    if (base_tile >= 34u) {
      throw std::runtime_error("An invalid tile index in pon: " + line);
    }
    ++test_case.pon_list[base_tile];
  }

  fields = split(columns[4u], ',');
  for (std::string const &field : fields) {
    std::vector<std::string> subfields = split(field, '-');
    if (subfields.size() != 4u) {
      throw std::runtime_error("An invalid open kan: " + line);
    }
    std::uint_fast8_t base_tile = UINT_FAST8_MAX;
    for (std::uint_fast8_t i = 0u; i < 4u; ++i) {
      unsigned long long const tile_index_ull = std::stoull(subfields[i]);
      if (tile_index_ull >= 37u) {
        throw std::runtime_error("An invalid tile index: " + subfields[i]);
      }
      bool is_red_dora = false;
      std::tie(base_tile, is_red_dora)
        = adjustTileIndex(static_cast<std::uint_fast8_t>(tile_index_ull));
      if (is_red_dora) {
        if (test_case.num_dora > UINT_FAST8_MAX - 1u) {
          throw std::overflow_error("Too many dora in the test case: " + line);
        }
        ++test_case.num_dora;
      }
    }
    if (base_tile >= 34u) {
      throw std::runtime_error("An invalid tile index in open kan: " + line);
    }
    ++test_case.open_kan_list[base_tile];
  }

  fields = split(columns[5u], ',');
  for (std::string const &field : fields) {
    std::vector<std::string> subfields = split(field, '-');
    if (subfields.size() != 4u) {
      throw std::runtime_error("An invalid ankan: " + line);
    }
    std::uint_fast8_t base_tile = UINT_FAST8_MAX;
    for (std::uint_fast8_t i = 0u; i < 4u; ++i) {
      unsigned long long const tile_index_ull = std::stoull(subfields[i]);
      if (tile_index_ull >= 37u) {
        throw std::runtime_error("An invalid tile index: " + subfields[i]);
      }
      bool is_red_dora = false;
      std::tie(base_tile, is_red_dora)
        = adjustTileIndex(static_cast<std::uint_fast8_t>(tile_index_ull));
      if (is_red_dora) {
        if (test_case.num_dora > UINT_FAST8_MAX - 1u) {
          throw std::overflow_error("Too many dora in the test case: " + line);
        }
        ++test_case.num_dora;
      }
    }
    if (base_tile >= 34u) {
      throw std::runtime_error("An invalid tile index in ankan: " + line);
    }
    ++test_case.ankan_list[base_tile];
  }

  std::uint_fast8_t winning_tile = UINT_FAST8_MAX;
  {
    unsigned long long const winning_tile_ull = std::stoull(columns[6u]);
    if (winning_tile_ull >= 37u) {
      throw std::runtime_error("An invalid winning tile index: " + line);
    }
    bool is_red_dora = false;
    std::tie(winning_tile, is_red_dora)
      = adjustTileIndex(static_cast<std::uint_fast8_t>(winning_tile_ull));
    if (is_red_dora) {
      if (test_case.num_dora > UINT_FAST8_MAX - 1u) {
        throw std::overflow_error("Too many dora in the test case: " + line);
      }
      ++test_case.num_dora;
    }
  }
  if (winning_tile >= 34u) {
    throw std::runtime_error("An invalid winning tile index: " + line);
  }
  test_case.winning_tile = winning_tile;

  fields = split(columns[7u], ',');
  if (fields.size() != 10u) {
    throw std::runtime_error("Invalid context: " + line);
  }
  test_case.context |= (fields[0u] == "0") ? FuHan::tsumo : FuHan::ron;
  if (fields[1u] == "1") {
    test_case.context |= FuHan::riichi;
  }
  if (fields[2u] == "1") {
    test_case.context |= FuHan::chankan;
  }
  if (fields[3u] == "1") {
    test_case.context |= FuHan::rinshan_kaihou;
  }
  if (fields[4u] == "1") {
    test_case.context |= FuHan::haitei_raoyue;
  }
  if (fields[5u] == "1") {
    test_case.context |= FuHan::houtei_raoyui;
  }
  if (fields[6u] == "1") {
    test_case.context |= FuHan::double_riichi;
  }
  if (fields[7u] == "1") {
    test_case.context |= FuHan::ippatsu;
  }
  if (fields[8u] == "1") {
    test_case.context |= FuHan::tenhou;
  }
  if (fields[9u] == "1") {
    test_case.context |= FuHan::chiihou;
  }

  fields = split(columns[8u], ',');
  std::vector<std::uint_fast8_t> dora_indicators;
  for (std::string const &field : fields) {
    unsigned long long const tile_index_ull = std::stoull(field);
    if (tile_index_ull >= 37u) {
      throw std::runtime_error("An invalid tile index: " + field);
    }
    auto [tile, is_red_dora] = adjustTileIndex(static_cast<std::uint_fast8_t>(tile_index_ull));
    dora_indicators.push_back(tile);
  }
  for (std::uint_fast8_t const dora_indicator : dora_indicators) {
    std::uint_fast8_t const num_dora = countDora(test_case, dora_indicator);
    if (test_case.num_dora > UINT_FAST8_MAX - num_dora) {
      throw std::overflow_error("Too many dora in the test case: " + line);
    }
    test_case.num_dora += num_dora;
  }

  fields = split(columns[9u], ',');
  std::vector<std::uint_fast8_t> ura_dora_indicators;
  for (std::string const &field : fields) {
    unsigned long long const tile_index_ull = std::stoull(field);
    if (tile_index_ull >= 37u) {
      throw std::runtime_error("An invalid tile index: " + field);
    }
    auto [tile, is_red_dora] = adjustTileIndex(static_cast<std::uint_fast8_t>(tile_index_ull));
    ura_dora_indicators.push_back(tile);
  }
  for (std::uint_fast8_t const ura_dora_indicator : ura_dora_indicators) {
    std::uint_fast8_t const num_dora = countDora(test_case, ura_dora_indicator);
    if (test_case.num_dora > UINT_FAST8_MAX - num_dora) {
      throw std::overflow_error("Too many dora in the test case: " + line);
    }
    test_case.num_dora += num_dora;
  }

  // `columns[10u]` records which yaku were present, but this test only checks
  // the final fu/han/yakuman_multiplier values, so the column is intentionally
  // not parsed.

  fields = split(columns[11u], ',');
  unsigned long long const expected_fu_ull = std::stoull(fields[0u]);
  if (expected_fu_ull > UINT_FAST8_MAX) {
    throw std::runtime_error("An invalid expected fu: " + line);
  }
  test_case.expected_fu = static_cast<std::uint_fast8_t>(expected_fu_ull);
  unsigned long long const expected_han_ull = std::stoull(fields[1u]);
  if (expected_han_ull > UINT_FAST8_MAX) {
    throw std::runtime_error("An invalid expected han: " + line);
  }
  test_case.expected_han = static_cast<std::uint_fast8_t>(expected_han_ull);
  unsigned long long const expected_yakuman_multiplier_ull = std::stoull(fields[2u]);
  if (expected_yakuman_multiplier_ull > UINT_FAST8_MAX) {
    throw std::runtime_error("An invalid expected yakuman multiplier: " + line);
  }
  test_case.expected_yakuman_multiplier
    = static_cast<std::uint_fast8_t>(expected_yakuman_multiplier_ull);
  if (test_case.expected_fu == 25u && test_case.expected_yakuman_multiplier >= 1u) {
    // 雀魂の牌譜データにおいては国士無双の符は常に `25` として記録されている．
    test_case.expected_fu = 0u;
  }

  return test_case;
}

void runTestCase(std::string const &line, TestCase const &test_case)
{
  FuHan::Result result = FuHan::calculateFuHan(
    static_cast<FuHan::Wind>(27u + test_case.round_wind),
    static_cast<FuHan::Wind>(27u + (test_case.seat + 4u - test_case.round_index_within_round_wind) % 4u),
    test_case.concealed_hand,
    test_case.chii_list,
    test_case.pon_list,
    test_case.open_kan_list,
    test_case.ankan_list,
    test_case.winning_tile,
    test_case.num_dora,
    test_case.context,
    FuHan::Rules::mahjong_soul);

  auto on_test_failure = [&]() {
    std::ostringstream test_case_id;
    test_case_id << test_case.uuid
                 << " (" << static_cast<unsigned>(test_case.round_wind)
                 << ',' << static_cast<unsigned>(test_case.round_index_within_round_wind)
                 << ',' << static_cast<unsigned>(test_case.num_counter_sticks)
                 << ',' << static_cast<unsigned>(test_case.seat) << ')';
    std::cerr << "Test case failed: " << test_case_id.str() << std::endl;
    std::cerr << "Expected: fu = " << static_cast<unsigned>(test_case.expected_fu)
              << ", han = " << static_cast<unsigned>(test_case.expected_han)
              << ", yakuman_multiplier = " << static_cast<unsigned>(test_case.expected_yakuman_multiplier)
              << std::endl;
    std::cerr << "Actual: fu = " << static_cast<unsigned>(result.fu)
              << ", han = " << static_cast<unsigned>(result.han)
              << ", yakuman_multiplier = " << static_cast<unsigned>(result.yakuman_multiplier)
              << std::endl;
    std::cerr << "Test case line: " << line << std::endl;
    throw std::runtime_error("Test case failed: " + test_case_id.str());
  };

  if (test_case.expected_yakuman_multiplier >= 1u) {
    // 雀魂の牌譜において，数え役満以外の役満が1つでも成立している場合は
    // 記録されている符が使い物にならないため，符の値を検査から除外する．
    if (result.han != test_case.expected_han
        || result.yakuman_multiplier != test_case.expected_yakuman_multiplier) {
      on_test_failure();
    }
    return;
  }

  if (result.fu != test_case.expected_fu
      || result.han != test_case.expected_han
      || result.yakuman_multiplier != test_case.expected_yakuman_multiplier) {
    on_test_failure();
  }
}

} // namespace *unnamed*

int main()
{
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty() && std::cin.eof()) {
      break;
    }

    TestCase test_case = parseTestCase(line);
    runTestCase(line, test_case);
  }

  return EXIT_SUCCESS;
}
