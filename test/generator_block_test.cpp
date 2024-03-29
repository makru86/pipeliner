#include <pipeliner/generator_block.h>

#include "catch.hpp"

namespace pipeliner {

    TEST_CASE("CsvReaderBlock", "[GeneratorBlock]") {
        SECTION("Reading 16 columns, 100 rows csv file") {
            const auto csvFile = Path{} / "data" / "MOCK_DATA_16_100.csv";

            SECTION("Check first 5 rows of csv file") {
                std::vector<Uint8> expected{
                        1, 114, 102, 69, 4, 169, 113, 58, 172, 115, 88, 177, 199, 212, 159, 22,
                        130, 233, 72, 31, 121, 34, 226, 49, 235, 214, 17, 120, 146, 115, 226, 143,
                        102, 189, 20, 86, 131, 201, 29, 67, 253, 65, 102, 143, 181, 101, 70, 228,
                        145, 35, 131, 221, 164, 134, 140, 249, 149, 75, 252, 59, 69, 7, 99, 179,
                        218, 242, 204, 3, 77, 32, 18, 42, 202, 161, 112, 226, 206, 51, 6, 43};

                GeneratorProcessor p{csvFile};

                std::vector<Uint8> actual{};
                while (expected.size() != actual.size()) {
                    const auto c = p.processChunk({});
                    actual.push_back(c.data1);
                    actual.push_back(c.data2);
                }

                REQUIRE(expected == actual);
            }

            SECTION("Read file until the end") {
                GeneratorBlock b{csvFile};
                b.start();

                int elementsCount{0};
                while (true) {
                    DataChunk c{};
                    b.getProcessor().getQueue().wait_dequeue(c);
                    if (c.getType() == DataChunk::End) {
                        break;
                    }
                    elementsCount += 2;
                }

                b.stop();

                // 16 columns, 100 rows
                REQUIRE(16 * 100 == elementsCount);
            }
        }
    }

    TEST_CASE("RandomNumberGeneratorBlock", "[GeneratorBlock]") {
        GeneratorProcessor p{};
        const auto c = p.processChunk({});
        REQUIRE(DataChunk::Data == c.getType());
    }

}