// Copyright (c) 2026 Cryolite
// SPDX-License-Identifier: MIT
// This file is part of https://github.com/Cryolite/fuhan.

#include <fuhan/fuhan.hpp>
#include <fuhan/types.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/import.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/long.hpp>
#include <boost/python/object.hpp>
#include <Python.h>
#include <filesystem>
#include <sstream>
#include <iostream>
#include <string>
#include <array>
#include <utility>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>


namespace fs = std::filesystem;
namespace py = boost::python;

namespace{

void initializePython(fs::path const &python_executable_path)
{
  PyStatus py_status;

  PyConfig py_config;
  PyConfig_InitPythonConfig(&py_config);

  std::wstring python_executable_path_wstr = python_executable_path.wstring();
  py_status = PyConfig_SetString(
    &py_config,
    &py_config.program_name,
    python_executable_path_wstr.c_str());
  if (PyStatus_Exception(py_status)) {
    PyConfig_Clear(&py_config);
    throw std::runtime_error("Failed to set Python program name.");
  }

  py_status = Py_InitializeFromConfig(&py_config);
  if (PyStatus_Exception(py_status)) {
    PyConfig_Clear(&py_config);
    throw std::runtime_error("Failed to initialize Python.");
  }

  PyConfig_Clear(&py_config);
}

py::list convertTo136(
  std::uint_fast8_t const suit,
  std::array<std::uint_fast8_t, 9u> const &tile_counts)
{
  py::list result;
  for (std::uint_fast8_t i = 0u; i < 9u; ++i) {
    for (std::uint_fast8_t j = 0u; j < tile_counts[i]; ++j) {
      result.append(36u * suit + 4u * i + j);
    }
  }
  return result;
}

void throwTestFailure(
  FuHan::Context const context,
  std::uint_fast8_t const suit,
  std::array<std::uint_fast8_t, 9u> const &tile_counts,
  std::uint_fast8_t const winning_tile,
  FuHan::Result const &result,
  std::uint_fast8_t const expected_fu,
  std::uint_fast8_t const expected_han)
{
  std::ostringstream oss;
  oss << "is_ron: " << FuHan::isRon(context) << '\n';
  oss << "suit: " << static_cast<unsigned>(suit) << '\n';
  oss << "hand: [";
  {
    bool is_first = true;
    for (std::uint_fast8_t i = 0u; i < 9u; ++i) {
      std::uint_fast8_t tile_count = tile_counts[i];
      for (std::uint_fast8_t j = 0u; j < tile_count; ++j) {
        if (is_first) {
          is_first = false;
        }
        else {
          oss << ", ";
        }
        oss << static_cast<unsigned>(9u * suit + i);
      }
    }
  }
  oss << "]\n";
  oss << "winning_tile: " << static_cast<unsigned>(winning_tile) << '\n';
  oss << "actual:\n";
  oss << "  fu: " << static_cast<unsigned>(result.fu) << '\n';
  oss << "  han: " << static_cast<unsigned>(result.han) << '\n';
  oss << "  yakuman_multiplier: " << static_cast<unsigned>(result.yakuman_multiplier) << '\n';
  oss << "expected:\n";
  oss << "  fu: " << static_cast<unsigned>(expected_fu) << '\n';
  oss << "  han: " << static_cast<unsigned>(expected_han) << '\n';
  throw std::runtime_error(oss.str());
}

void testChiniisouImpl(
  FuHan::Context const context,
  std::uint_fast8_t const suit,
  std::array<std::uint_fast8_t, 9u> const &tile_counts,
  std::uint_fast8_t const winning_tile,
  py::object m_hand_config,
  py::object optional_rules,
  py::object hand_calculator)
{
  py::tuple args = py::make_tuple();
  py::dict kwargs = py::dict();
  if (FuHan::isTsumo(context)) {
    kwargs["is_tsumo"] = true;
  }
  if (FuHan::isRiichi(context)) {
    kwargs["is_riichi"] = true;
  }
  if (FuHan::isIppatsu(context)) {
    kwargs["is_ippatsu"] = true;
  }
  if (FuHan::isRinshanKaihou(context)) {
    kwargs["is_rinshan"] = true;
  }
  if (FuHan::isChankan(context)) {
    kwargs["is_chankan"] = true;
  }
  if (FuHan::isHaiteiRaoyue(context)) {
    kwargs["is_haitei"] = true;
  }
  if (FuHan::isHouteiRaoyui(context)) {
    kwargs["is_houtei"] = true;
  }
  if (FuHan::isDoubleRiichi(context)) {
    kwargs["is_daburu_riichi"] = true;
  }
  if (FuHan::isTenhou(context)) {
    kwargs["is_tenhou"] = true;
  }
  if (FuHan::isChiihou(context)) {
    kwargs["is_chiihou"] = true;
  }
  kwargs["player_wind"] = 27;
  kwargs["round_wind"] = 27;
  kwargs["options"] = optional_rules;
  py::object hand_config = m_hand_config.attr("HandConfig")(*args, **kwargs);

  py::list tiles_136 = convertTo136(suit, tile_counts);

  py::long_ winning_tile_136(36u * suit + 4u * winning_tile);

  args = py::make_tuple(tiles_136, winning_tile_136);
  kwargs = py::dict();
  kwargs["config"] = hand_config;
  py::object hand_response = hand_calculator.attr("estimate_hand_value")(*args, **kwargs);

  auto const [expected_fu, expected_han] = [&]() -> std::pair<std::uint_fast8_t, std::uint_fast8_t> {
    py::object error = hand_response.attr("error");
    if (error == hand_calculator.attr("ERR_HAND_NOT_WINNING")) {
      return {0u, 0u};
    }

    if (error == hand_calculator.attr("ERR_NO_YAKU")) {
      return {0u, 0u};
    }

    if (!error.is_none()) {
      throw std::runtime_error("Unexpected error in hand response.");
    }

    py::object fu = hand_response.attr("fu");
    if (fu.is_none()) {
      throw std::runtime_error("`fu` is `None` in hand response.");
    }
    py::object han = hand_response.attr("han");
    if (han.is_none()) {
      throw std::runtime_error("`han` is `None` in hand response.");
    }
    return {py::extract<std::uint_fast8_t>(fu)(), py::extract<std::uint_fast8_t>(han)()};
  }();

  std::array<std::uint_fast8_t, 34u> concealed_hand{};
  for (std::uint_fast8_t i = 0u; i < 9u; ++i) {
    concealed_hand[9u * suit + i] = tile_counts[i];
  }
  --concealed_hand[9u * suit + winning_tile];
  FuHan::Result const result = FuHan::calculateFuHan(
    FuHan::east,
    FuHan::east,
    concealed_hand,
    {}, // chii_list
    {}, // pon_list
    {}, // open_kan_list
    {}, // ankan_list
    9u * suit + winning_tile,
    0u, // num_dora
    context);

  if (expected_fu == 0u && expected_han == 0u) {
    if (result.han != 0u || result.yakuman_multiplier != 0u) {
      throwTestFailure(context, suit, tile_counts, winning_tile, result, expected_fu, expected_han);
    }
    return;
  }

  if (result.yakuman_multiplier >= 1u) {
    if (result.yakuman_multiplier * 13u != expected_han) {
      throwTestFailure(context, suit, tile_counts, winning_tile, result, expected_fu, expected_han);
    }
    return;
  }

  if (result.fu != expected_fu || result.han != expected_han) {
    throwTestFailure(context, suit, tile_counts, winning_tile, result, expected_fu, expected_han);
  }
}

void testChiniisou(
  FuHan::Context const context,
  std::uint_fast8_t const suit,
  std::uint_fast8_t const i,
  std::array<std::uint_fast8_t, 9u> &tile_counts,
  std::uint_fast8_t const winning_tile,
  std::uint_fast8_t const num_melds,
  bool const has_pair,
  py::object m_hand_config,
  py::object optional_rules,
  py::object hand_calculator)
{
  if (i == 9u) {
    if (num_melds == 4u && has_pair) {
      testChiniisouImpl(
        context,
        suit,
        tile_counts,
        winning_tile,
        m_hand_config,
        optional_rules,
        hand_calculator);
    }
    return;
  }

  if (num_melds <= 3u) {
    if (i <= 6u && tile_counts[i] <= 3u && tile_counts[i + 1u] <= 3u && tile_counts[i + 2u] <= 3u) {
      ++tile_counts[i];
      ++tile_counts[i + 1u];
      ++tile_counts[i + 2u];
      testChiniisou(
        context,
        suit,
        i,
        tile_counts,
        winning_tile,
        num_melds + 1u,
        has_pair,
        m_hand_config,
        optional_rules,
        hand_calculator);
      --tile_counts[i + 2u];
      --tile_counts[i + 1u];
      --tile_counts[i];
    }
    if (1u <= i && i <= 7u
        && tile_counts[i - 1u] <= 3u
        && tile_counts[i] <= 3u
        && tile_counts[i + 1u] <= 3u) {
      ++tile_counts[i - 1u];
      ++tile_counts[i];
      ++tile_counts[i + 1u];
      testChiniisou(
        context,
        suit,
        i,
        tile_counts,
        winning_tile,
        num_melds + 1u,
        has_pair,
        m_hand_config,
        optional_rules,
        hand_calculator);
      --tile_counts[i + 1u];
      --tile_counts[i];
      --tile_counts[i - 1u];
    }
    if (2u <= i && tile_counts[i - 2u] <= 3u && tile_counts[i - 1u] <= 3u && tile_counts[i] <= 3u) {
      ++tile_counts[i - 2u];
      ++tile_counts[i - 1u];
      ++tile_counts[i];
      testChiniisou(
        context,
        suit,
        i,
        tile_counts,
        winning_tile,
        num_melds + 1u,
        has_pair,
        m_hand_config,
        optional_rules,
        hand_calculator);
      --tile_counts[i];
      --tile_counts[i - 1u];
      --tile_counts[i - 2u];
    }
    if (tile_counts[i] <= 1u) {
      tile_counts[i] += 3u;
      testChiniisou(
        context,
        suit,
        i,
        tile_counts,
        winning_tile,
        num_melds + 1u,
        has_pair,
        m_hand_config,
        optional_rules,
        hand_calculator);
      tile_counts[i] -= 3u;
    }
  }

  if (!has_pair && tile_counts[i] <= 2u) {
    tile_counts[i] += 2u;
    testChiniisou(
      context,
      suit,
      i,
      tile_counts,
      winning_tile,
      num_melds,
      true,
      m_hand_config,
      optional_rules,
      hand_calculator);
    tile_counts[i] -= 2u;
  }

  testChiniisou(
    context,
    suit,
    i + 1u,
    tile_counts,
    winning_tile,
    num_melds,
    has_pair,
    m_hand_config,
    optional_rules,
    hand_calculator);
}

} // namespace *unnamed*

int main(int argc, char const * const * const argv)
{
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <python_executable_path>\n";
    return EXIT_FAILURE;
  }

  fs::path python_executable_path(argv[1]);
  if (!fs::exists(python_executable_path)) {
    std::cerr << "Error: Python executable not found at " << python_executable_path << '\n';
    return EXIT_FAILURE;
  }
  if (!fs::is_regular_file(python_executable_path)) {
    std::cerr << "Error: Path " << python_executable_path << " is not a regular file\n";
    return EXIT_FAILURE;
  }

  initializePython(python_executable_path);

  try {
    py::object m_hand_config = py::import("mahjong.hand_calculating.hand_config");
    py::tuple args = py::make_tuple();
    py::dict kwargs;
    kwargs["has_open_tanyao"] = true;
    py::object optional_rules = m_hand_config.attr("OptionalRules")(*args, **kwargs);
    py::object m_hand = py::import("mahjong.hand_calculating.hand");
    py::object hand_calculator = m_hand.attr("HandCalculator")();

    for (std::uint_fast8_t is_ron = 0u; is_ron <= 1u; ++is_ron) {
      FuHan::Context context = is_ron ? FuHan::ron : FuHan::tsumo;
      for (std::uint_fast8_t suit = 0u; suit <= 2u; ++suit) {
        std::array<std::uint_fast8_t, 9u> tile_counts{};
        for (std::uint_fast8_t winning_tile = 0u; winning_tile < 9u; ++winning_tile) {
          if (winning_tile <= 6u) {
            ++tile_counts[winning_tile];
            ++tile_counts[winning_tile + 1u];
            ++tile_counts[winning_tile + 2u];
            testChiniisou(
              context,
              suit,
              0u,
              tile_counts,
              winning_tile,
              1u,
              false,
              m_hand_config,
              optional_rules,
              hand_calculator);
            --tile_counts[winning_tile + 2u];
            --tile_counts[winning_tile + 1u];
            --tile_counts[winning_tile];
          }
          if (1u <= winning_tile && winning_tile <= 7u) {
            ++tile_counts[winning_tile - 1u];
            ++tile_counts[winning_tile];
            ++tile_counts[winning_tile + 1u];
            testChiniisou(
              context,
              suit,
              0u,
              tile_counts,
              winning_tile,
              1u,
              false,
              m_hand_config,
              optional_rules,
              hand_calculator);
            --tile_counts[winning_tile + 1u];
            --tile_counts[winning_tile];
            --tile_counts[winning_tile - 1u];
          }
          if (2u <= winning_tile) {
            ++tile_counts[winning_tile - 2u];
            ++tile_counts[winning_tile - 1u];
            ++tile_counts[winning_tile];
            testChiniisou(
              context,
              suit,
              0u,
              tile_counts,
              winning_tile,
              1u,
              false,
              m_hand_config,
              optional_rules,
              hand_calculator);
            --tile_counts[winning_tile];
            --tile_counts[winning_tile - 1u];
            --tile_counts[winning_tile - 2u];
          }
          {
            tile_counts[winning_tile] += 3u;
            testChiniisou(
              context,
              suit,
              0u,
              tile_counts,
              winning_tile,
              1u,
              false,
              m_hand_config,
              optional_rules,
              hand_calculator);
            tile_counts[winning_tile] -= 3u;
          }
          {
            tile_counts[winning_tile] += 2u;
            testChiniisou(
              context,
              suit,
              0u,
              tile_counts,
              winning_tile,
              0u,
              true,
              m_hand_config,
              optional_rules,
              hand_calculator);
            tile_counts[winning_tile] -= 2u;
          }
        }
      }
    }
  }
  catch (py::error_already_set const &) {
    PyErr_Print();
    Py_FinalizeEx();
    return EXIT_FAILURE;
  }

  Py_FinalizeEx();

  return EXIT_SUCCESS;
}
