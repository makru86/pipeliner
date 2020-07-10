#pragma once

#include <cstdint>
#include <algorithm>
#include <set>
#include <mutex>
#include <cassert>

#include <pipeliner/filter_block.h>

namespace pipeliner {

    using Uint16 = std::uint16_t;

    struct Merge {
        Uint16 label1;
        Uint16 label2;
    };

    // LabelSet contains set of available labels.
    class LabelSet {
    public:
        LabelSet(Uint16 size) : next_{size + 1} {
            for (Uint16 label{1}; label != size + 1; ++label) {
                labels_.insert(label);
            }
        }

        Uint16 get() {
            if (labels_.empty()) {
                labels_.insert(next_++);
            }
            auto iter = labels_.begin();
            auto label = *iter;
            labels_.erase(iter);
            return label;
        }

        void put(Uint16 label) {
            auto p = labels_.insert(label);
        }

    private:
        std::set<Uint16> labels_;
        Uint16 next_;
    };

    class LabelledChunk : public DataChunk {
    public:
        LabelledChunk(DataChunk::Type type = DataChunk::Data) : DataChunk{type} {}

        Pos pos = {};
        Uint16 labels[2] = {0, 0};
        std::vector<Merge> merges;
    };

    class LabelReuseChunk : public DataChunk {
    public:
        LabelReuseChunk() : DataChunk{DataChunk::Data} {}

        std::vector<Uint16> labels;
    };

    class LabellingBlock : public BasicBlock {
    public:
        LabellingBlock(Uint16 width, BasicBlock *prev)
                : BasicBlock{prev}, width_{width}, labelSet_{width}, pos_{} {
            prevRow_.resize(width_, 0);
            curRow_.resize(width_, 0);
        }

        std::unique_ptr<DataChunk>
        processChunk(std::unique_ptr<DataChunk> chunk) override {
            if (chunk->getType() == DataChunk::End) {
                return std::move(chunk);
            }
            auto filteredChunk = dynamic_cast<FilteredChunk *>(chunk.get());
            if (!filteredChunk) {
                return nullptr;
            }

            auto labelledChunk = std::make_unique<LabelledChunk>();
            labelledChunk->pos = pos_;

            labelledChunk->labels[0] = processElement(filteredChunk->filt1, labelledChunk->merges);
            labelledChunk->labels[1] = processElement(filteredChunk->filt2, labelledChunk->merges);

            for (const auto &merge : labelledChunk->merges) {
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
                // Here we consider west, north-west, north and north-east neibhors
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
                    // Element has no neibhors, assign new label
                    label = labelSet_.get();
                } else {
                    // Assign minimal label and merge with neibhors (there might be duplicate merges,
                    // which will be ignored by the ComputationBlock
                    std::sort(neibhors.begin(), neibhors.end());
                    label = neibhors[0];
                    for (auto n : neibhors) {
                        if (label != n) {
                            merge.push_back(Merge{label, n});
                        }
                    }

                    updateNeibhorsLabels(label);
                }
            }

            if (label) {
                PILI_DEBUG_ADDTEXT(pos_.col << "(" << label << ") ");
            }

            curRow_[pos_.col] = label;
            ++pos_.col;
            return label;
        }

        void processReverseChunk(std::unique_ptr<DataChunk> chunk) override {
            if (auto c = dynamic_cast<LabelReuseChunk *>(chunk.get())) {
                for (auto label : c->labels) {
                    labelSet_.put(label);
                }
            }
        }

    private:
        void updateNeibhorsLabels(Uint16 label) {
            if (pos_.col != 0) {
                auto &n1 = curRow_[pos_.col - 1];
                if (n1 != 0) { n1 = label; }

                auto &n2 = prevRow_[pos_.col - 1];
                if (n2 != 0) { n2 = label; }
            }
            auto &n3 = prevRow_[pos_.col];
            if (n3 != 0) { n3 = label; }

            if (pos_.col + 1 != width_) {
                auto &n4 = prevRow_[pos_.col + 1];
                if (n4 != 0) { n4 = label; }
            }
        }

        const Uint16 width_;
        std::vector<Uint16> prevRow_, curRow_;
        LabelSet labelSet_;
        Pos pos_;
    };
}
