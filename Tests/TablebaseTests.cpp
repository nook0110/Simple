#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>

#include "PositionFactory.h"
#include "ExitCondition.h"
#include "NodeType.h"
#include "Searcher.h"
#include "Tablebase.h"
#include "TablebaseFile.h"
#include "TablebaseGenerator.h"
#include "TablebasePosition.h"

using namespace SimpleChessEngine;
using namespace SimpleChessEngine::Tablebase;

namespace TablebaseTests {

TEST(PieceCode, RoundTripsPieceAndColor) {
  EXPECT_EQ(FlipColor(PieceCode::kWhitePawn), PieceCode::kBlackPawn);
  EXPECT_EQ(FlipColor(PieceCode::kBlackQueen), PieceCode::kWhiteQueen);
  for (const auto color : {Player::kWhite, Player::kBlack}) {
    for (const auto piece : {Piece::kPawn, Piece::kKnight, Piece::kBishop,
                             Piece::kRook, Piece::kQueen}) {
      const auto code = EncodePiece(piece, color);
      EXPECT_EQ(DecodePiece(code), piece);
      EXPECT_EQ(DecodeColor(code), color);
      EXPECT_EQ(FlipColor(FlipColor(code)), code);
    }
  }
}

TEST(PositionIndexer, ColorInversionHasSameCanonicalIndex) {
  const auto white = PositionFactory{}(
      "7k/8/8/8/8/8/8/KQ6 w - - 0 1");
  const auto black = PositionFactory{}(
      "kq6/8/8/8/8/8/8/7K b - - 0 1");

  const auto white_canonical = PositionIndexer::Canonicalize(white);
  const auto black_canonical = PositionIndexer::Canonicalize(black);

  ASSERT_TRUE(white_canonical.has_value());
  ASSERT_TRUE(black_canonical.has_value());
  EXPECT_EQ(*white_canonical, *black_canonical);
  EXPECT_EQ(PositionIndexer::RawIndex(*white_canonical),
            PositionIndexer::RawIndex(*black_canonical));
}

TEST(PositionIndexer, PawnlessGeometryUsesAllBoardSymmetries) {
  const auto first = PositionFactory{}(
      "7k/8/8/8/8/8/8/KR6 w - - 0 1");
  const auto rotated = PositionFactory{}(
      "k7/8/8/8/8/8/8/6RK w - - 0 1");

  EXPECT_EQ(PositionIndexer::Canonicalize(first),
            PositionIndexer::Canonicalize(rotated));
}

TEST(PositionIndexer, PawnfulGeometryMirrorsFiles) {
  const auto queenside = PositionFactory{}(
      "7k/8/8/8/8/8/P7/K7 w - - 0 1");
  const auto kingside = PositionFactory{}(
      "k7/8/8/8/8/8/7P/7K w - - 0 1");

  EXPECT_EQ(PositionIndexer::Canonicalize(queenside),
            PositionIndexer::Canonicalize(kingside));
}

TEST(PositionIndexer, RawIndexRoundTrips) {
  const MaterialClass material{{PieceCode::kWhiteRook,
                                PieceCode::kBlackKnight},
                               2};
  const auto size = PositionIndexer::RawSize(material);
  const std::array<std::uint64_t, 9> samples = {
      0, 1, 2, 127, 128, 4095, 65535, size - 2, size - 1};

  for (const auto raw_index : samples) {
    const auto position = PositionIndexer::FromRawIndex(material, raw_index);
    ASSERT_TRUE(position.has_value());
    EXPECT_EQ(PositionIndexer::RawIndex(*position), raw_index);
  }
}

TEST(PositionIndexer, ExhaustivelyRoundTripsThreePieceRawSpace) {
  const MaterialClass material{
      {PieceCode::kWhiteQueen, PieceCode::kNone}, 1};
  const auto size = PositionIndexer::RawSize(material);
  for (std::uint64_t raw_index = 0; raw_index < size; ++raw_index) {
    const auto position = PositionIndexer::FromRawIndex(material, raw_index);
    ASSERT_TRUE(position.has_value());
    ASSERT_EQ(PositionIndexer::RawIndex(*position), raw_index);
  }
}

TEST(MaterialClasses, EnumeratesAllCanonicalThreeAndFourPieceClasses) {
  const auto classes = AllMaterialClasses();
  EXPECT_EQ(classes.size(), 36);
  EXPECT_EQ(std::ranges::count_if(classes, [](const MaterialClass& material) {
              return material.PieceCount() == 2;
            }),
            1);
  EXPECT_EQ(std::ranges::count_if(classes, [](const MaterialClass& material) {
              return material.PieceCount() == 3;
            }),
            5);
  EXPECT_EQ(std::ranges::count_if(classes, [](const MaterialClass& material) {
              return material.PieceCount() == 4;
            }),
            30);
}

TEST(PositionBuilder, RejectsIllegalAndAcceptsLegalRawPositions) {
  const MaterialClass material{
      {PieceCode::kWhiteQueen, PieceCode::kNone}, 1};
  const CanonicalPosition adjacent_kings{material, {0, 1, 2, 0},
                                         Player::kWhite};
  const CanonicalPosition legal{material, {0, 63, 1, 0}, Player::kWhite};

  EXPECT_FALSE(PositionBuilder::Build(adjacent_kings).has_value());
  EXPECT_TRUE(PositionBuilder::Build(legal).has_value());
}

TEST(PositionBuilder, CanonicalLegalRawRoundTrip) {
  const auto position = PositionFactory{}(
      "7k/8/8/8/8/8/8/KQ6 w - - 0 1");
  const auto canonical = PositionIndexer::Canonicalize(position);
  ASSERT_TRUE(canonical.has_value());
  const auto raw_index = PositionIndexer::RawIndex(*canonical);

  EXPECT_TRUE(PositionBuilder::IsCanonicalLegalRaw(canonical->material,
                                                   raw_index));
}

TEST(PositionBuilder, KeepsLegalCheckedRookVersusKnightPositions) {
  for (const auto* fen : {
           "8/k4N2/6r1/8/8/8/3K4/8 b - - 1 1",
           "8/k2N4/6r1/8/8/8/3K4/8 b - - 1 1",
       }) {
    const auto position = PositionFactory{}(fen);
    const auto canonical = PositionIndexer::Canonicalize(position);
    ASSERT_TRUE(canonical.has_value()) << fen;
    const auto raw_index = PositionIndexer::RawIndex(*canonical);
    EXPECT_TRUE(PositionBuilder::IsCanonicalLegalRaw(canonical->material,
                                                     raw_index))
        << fen;
  }
}

TEST(PositionBuilder, KeepsLegalPromotionChildAfterColorCanonicalization) {
  const MaterialClass material{
      {PieceCode::kWhitePawn, PieceCode::kBlackQueen}, 2};
  const CanonicalPosition encoded{material, {58, 56, 25, 0},
                                  Player::kWhite};

  EXPECT_TRUE(PositionBuilder::Build(encoded).has_value());
  EXPECT_TRUE(PositionBuilder::IsCanonicalLegalRaw(
      material, PositionIndexer::RawIndex(encoded)));
}

TEST(PositionIndexer, RejectsUnsupportedPositionState) {
  const auto too_many_pieces = PositionFactory{}();
  const auto castling = PositionFactory{}(
      "4k3/8/8/8/8/8/8/4K2R w K - 0 1");
  const auto en_passant = PositionFactory{}(
      "7k/8/8/8/3pP3/8/8/K7 b - e3 0 1");

  EXPECT_FALSE(PositionIndexer::Canonicalize(too_many_pieces).has_value());
  EXPECT_FALSE(PositionIndexer::Canonicalize(castling).has_value());
  EXPECT_FALSE(PositionIndexer::Canonicalize(en_passant).has_value());
}

TEST(RankSelect, MapsSetBitsToDenseIndices) {
  const std::array<std::uint64_t, 10> occupancy = {
      0b10101, 0, 0, 0, 0, 0, 0, 0, 0b10, 0};
  const auto checkpoints = BuildRankCheckpoints(occupancy);

  EXPECT_EQ(DenseIndex(0, 640, occupancy, checkpoints), 0);
  EXPECT_EQ(DenseIndex(2, 640, occupancy, checkpoints), 1);
  EXPECT_EQ(DenseIndex(4, 640, occupancy, checkpoints), 2);
  EXPECT_EQ(DenseIndex(513, 640, occupancy, checkpoints), 3);
  EXPECT_FALSE(DenseIndex(1, 640, occupancy, checkpoints).has_value());
  EXPECT_FALSE(DenseIndex(640, 640, occupancy, checkpoints).has_value());
}

TEST(TablebaseFile, WritesMapsProbesAndChecksIntegrity) {
  const auto position = PositionFactory{}(
      "7k/8/8/8/8/8/8/KQ6 w - - 0 1");
  const auto canonical = PositionIndexer::Canonicalize(position);
  ASSERT_TRUE(canonical.has_value());
  const auto raw_index = PositionIndexer::RawIndex(*canonical);

  ClassData data;
  data.material = canonical->material;
  data.raw_size = PositionIndexer::RawSize(data.material);
  data.occupancy.resize((data.raw_size + 63) / 64);
  data.occupancy[raw_index / 64] |= 1ULL << (raw_index % 64);
  data.rank_checkpoints = BuildRankCheckpoints(data.occupancy);
  data.wdl = {static_cast<std::int8_t>(Wdl::kWin)};
  data.dtz = {66};

  const auto suffix = std::chrono::steady_clock::now()
                          .time_since_epoch()
                          .count();
  const auto path = std::filesystem::temp_directory_path() /
                    ("sce-tb-" + std::to_string(suffix) + ".scetb");
  ASSERT_TRUE(FileWriter::Write(path, std::span{&data, 1}));
  auto tablebase = MappedFile::Open(path);
  ASSERT_TRUE(tablebase.has_value());
  EXPECT_EQ(tablebase->ClassCount(), 1);
  EXPECT_TRUE(tablebase->VerifyChecksums());
  EXPECT_EQ(tablebase->Probe(position),
            (ProbeResult{Wdl::kWin, 66}));
  const auto at_fifty_move_boundary = PositionFactory{}(
      "7k/8/8/8/8/8/8/KQ6 w - - 34 1");
  const auto beyond_fifty_move_boundary = PositionFactory{}(
      "7k/8/8/8/8/8/8/KQ6 w - - 35 1");
  EXPECT_EQ(tablebase->Probe(at_fifty_move_boundary),
            (ProbeResult{Wdl::kWin, 66}));
  EXPECT_EQ(tablebase->Probe(beyond_fifty_move_boundary),
            (ProbeResult{Wdl::kCursedWin, 66}));
  EXPECT_EQ(tablebase->ProbeWdl(beyond_fifty_move_boundary),
            Wdl::kCursedWin);
  EXPECT_FALSE(tablebase->RootMove(position).has_value());

  auto shared_tablebase =
      std::make_shared<MappedFile>(std::move(*tablebase));
  Searcher searcher(position,
                    std::make_shared<Searcher::SearcherTranspositionTable>(),
                    shared_tablebase);
  DepthCondition condition{2};
  const auto search_result = searcher.Search<NodeType::kPV>(
      condition, 2, 1, -kTablebaseWinValue, kTablebaseWinValue);
  ASSERT_TRUE(search_result.has_value());
  EXPECT_EQ(*search_result, kTablebaseWinValue - 1);

  Searcher cursed_searcher(
      beyond_fifty_move_boundary,
      std::make_shared<Searcher::SearcherTranspositionTable>(),
      shared_tablebase);
  const auto cursed_result = cursed_searcher.Search<NodeType::kPV>(
      condition, 2, 1, -kTablebaseWinValue, kTablebaseWinValue);
  ASSERT_TRUE(cursed_result.has_value());
  EXPECT_EQ(*cursed_result, kDrawValue);

  std::filesystem::remove(path);
}

TEST(TablebaseFile, RejectsCorruptHeader) {
  const auto path = std::filesystem::temp_directory_path() /
                    "sce-tb-corrupt.scetb";
  {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << "not a tablebase";
  }
  EXPECT_FALSE(MappedFile::Open(path).has_value());
  std::filesystem::remove(path);
}

TEST(TablebaseGenerator, SolvesAndPersistsBareKings) {
  const MaterialClass bare_kings;
  Generator::Stats stats;
  const auto generated =
      Generator::GenerateWdl(std::span{&bare_kings, 1}, 2, &stats);
  ASSERT_TRUE(generated.has_value());
  EXPECT_GT(stats.legal_positions, 0);
  EXPECT_EQ(stats.terminal_losses, 0);
  EXPECT_EQ(stats.resolved_wins, 0);
  EXPECT_EQ(stats.resolved_losses, 0);

  const auto position = PositionFactory{}(
      "7k/8/8/8/8/8/8/K7 w - - 0 1");
  EXPECT_EQ(generated->Probe(position),
            (ProbeResult{Wdl::kDraw, 0}));

  const auto path = std::filesystem::temp_directory_path() /
                    "sce-tb-kings.scetb";
  ASSERT_TRUE(generated->Write(path));
  auto mapped = MappedFile::Open(path);
  ASSERT_TRUE(mapped.has_value());
  EXPECT_TRUE(mapped->VerifyChecksums());
  EXPECT_EQ(mapped->Probe(position),
            (ProbeResult{Wdl::kDraw, 0}));
  std::filesystem::remove(path);
}

}  // namespace TablebaseTests
