#pragma once

#include <cstdint>
#include <algorithm>
#include <set>

#include <pipeliner/filter_block.h>

namespace pipeliner {

    using Uint16 = std::uint16_t;

    struct Pos {
        Size row;
        Size col;
    };

    struct Merge {
        Uint16 label1;
        Uint16 label2;
    };

    class LabelledChunk : public DataChunk {
    public:
        LabelledChunk(DataChunk::Type type = DataChunk::Data) : DataChunk{type} {}

        Pos pos = {};
        Uint16 labels[2] = {0, 0};
        std::vector<Merge> merge;
    };

    class LabellingBlock : public BasicBlock {
    public:
        LabellingBlock(Size width, BasicBlock *prev)
                : BasicBlock{prev}, width_{width}, label_{1}, pos_{} {
            prevRow_.resize(width_, 0);
            curRow_.resize(width_, 0);
        }

        std::unique_ptr<DataChunk>
        processChunk(std::unique_ptr<DataChunk> chunk) override {
            if (chunk->getType() == DataChunk::End) {
                return nullptr;
            }
            auto filteredChunk = dynamic_cast<FilteredChunk *>(chunk.get());
            if (!filteredChunk) {
                return nullptr;
            }

            auto labelledChunk = std::make_unique<LabelledChunk>();
            labelledChunk->pos = pos_;

            labelledChunk->labels[0] = processElement(filteredChunk->filt1, labelledChunk->merge);
            labelledChunk->labels[1] = processElement(filteredChunk->filt2, labelledChunk->merge);

            for (const auto &merge : labelledChunk->merge) {
                   PILI_DEBUG_ADDTEXT("M(" << merge.label1 << "," << merge.label2 << ") ");
            }

            if (pos_.col >= width_) {
                pos_.col = 0;
                pos_.row += 1;
                std::swap(prevRow_, curRow_);
                PILI_DEBUG_NEWLINE();
            }

            PILI_DEBUG_ADDTEXT("; ");

            return std::move(labelledChunk);
        }

        Uint16 processElement(bool elem, std::vector<Merge> &merge) {
            Uint16 label{};
            std::vector<Uint16> neibhors;

            if (elem) {
                if (pos_.col != 0) {
                    const auto n1 = curRow_[pos_.col - 1];
                    if (n1 != 0) { neibhors.push_back(n1); }

                    const auto n2 = prevRow_[pos_.col - 1];
                    if (n2 != 0) { neibhors.push_back(n2); }
                }
                const auto n3 = prevRow_[pos_.col];
                if (n3 != 0) { neibhors.push_back(n3); }

                if (pos_.col + 1 != width_) {
                    const auto n4 = prevRow_[pos_.col + 1];
                    if (n4 != 0) { neibhors.push_back(n4); }
                }
                if (neibhors.empty()) {
                    label = label_++;
                } else {
                    std::sort(neibhors.begin(), neibhors.end());
                    label = neibhors[0];
                    for (auto n : neibhors) {
                        if (label != n) {
                            merge.push_back(Merge{label, n});
                        }
                    }
                }
            }

            if (label) {
                PILI_DEBUG_ADDTEXT(pos_.col << "(" << label << ") ");
            }

            curRow_[pos_.col] = label;
            ++pos_.col;
            return label;
        }

    private:
        const Size width_;
        std::vector<Uint16> prevRow_, curRow_;
        Uint16 label_;
        Pos pos_;
    };
}