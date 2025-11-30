#include "VulkanWrapper/Memory/IntervalSet.h"

namespace vw {

// ============================================================================
// BufferIntervalSet Implementation
// ============================================================================

void BufferIntervalSet::add(const BufferInterval &interval) {
    if (interval.empty()) {
        return;
    }

    m_intervals.push_back(interval);
    mergeSorted();
}

void BufferIntervalSet::remove(const BufferInterval &interval) {
    if (interval.empty()) {
        return;
    }

    std::vector<BufferInterval> newIntervals;

    for (const auto &existing : m_intervals) {
        if (!existing.overlaps(interval)) {
            // No overlap, keep the interval as is
            newIntervals.push_back(existing);
        } else {
            // There's overlap, need to split the existing interval
            // Keep the part before the removed interval
            if (existing.offset < interval.offset) {
                BufferInterval before(existing.offset,
                                      interval.offset - existing.offset);
                newIntervals.push_back(before);
            }

            // Keep the part after the removed interval
            if (existing.end() > interval.end()) {
                BufferInterval after(interval.end(),
                                     existing.end() - interval.end());
                newIntervals.push_back(after);
            }
        }
    }

    m_intervals = std::move(newIntervals);
}

std::vector<BufferInterval>
BufferIntervalSet::findOverlapping(const BufferInterval &interval) const {
    std::vector<BufferInterval> result;

    for (const auto &existing : m_intervals) {
        if (existing.overlaps(interval)) {
            result.push_back(existing);
        }
    }

    return result;
}

bool BufferIntervalSet::hasOverlap(const BufferInterval &interval) const {
    for (const auto &existing : m_intervals) {
        if (existing.overlaps(interval)) {
            return true;
        }
    }
    return false;
}

void BufferIntervalSet::mergeSorted() {
    if (m_intervals.size() <= 1) {
        return;
    }

    // Sort by offset
    std::sort(m_intervals.begin(), m_intervals.end(),
              [](const BufferInterval &a, const BufferInterval &b) {
                  return a.offset < b.offset;
              });

    // Merge overlapping/adjacent intervals
    std::vector<BufferInterval> merged;
    merged.push_back(m_intervals[0]);

    for (size_t i = 1; i < m_intervals.size(); ++i) {
        auto mergeResult = merged.back().merge(m_intervals[i]);
        if (mergeResult.has_value()) {
            merged.back() = *mergeResult;
        } else {
            merged.push_back(m_intervals[i]);
        }
    }

    m_intervals = std::move(merged);
}

// ============================================================================
// ImageIntervalSet Implementation
// ============================================================================

void ImageIntervalSet::add(const ImageInterval &interval) {
    if (interval.empty()) {
        return;
    }

    m_intervals.push_back(interval);
    mergeCompatible();
}

void ImageIntervalSet::remove(const ImageInterval &interval) {
    if (interval.empty()) {
        return;
    }

    std::vector<ImageInterval> newIntervals;

    for (const auto &existing : m_intervals) {
        if (!existing.overlaps(interval)) {
            // No overlap, keep the interval as is
            newIntervals.push_back(existing);
        } else {
            // There's overlap - for images, this is complex
            // For simplicity, we'll remove the entire interval if there's any overlap
            // A more sophisticated implementation would split the interval
            auto intersection = existing.intersect(interval);
            if (intersection.has_value() && *intersection == existing) {
                // Completely contained, remove it
                continue;
            } else {
                // Partial overlap - for now, keep the existing interval
                // TODO: Implement proper splitting for image intervals
                newIntervals.push_back(existing);
            }
        }
    }

    m_intervals = std::move(newIntervals);
}

std::vector<ImageInterval>
ImageIntervalSet::findOverlapping(const ImageInterval &interval) const {
    std::vector<ImageInterval> result;

    for (const auto &existing : m_intervals) {
        if (existing.overlaps(interval)) {
            result.push_back(existing);
        }
    }

    return result;
}

bool ImageIntervalSet::hasOverlap(const ImageInterval &interval) const {
    for (const auto &existing : m_intervals) {
        if (existing.overlaps(interval)) {
            return true;
        }
    }
    return false;
}

void ImageIntervalSet::mergeCompatible() {
    if (m_intervals.size() <= 1) {
        return;
    }

    // Try to merge intervals with the same aspect mask
    std::vector<ImageInterval> merged;
    std::vector<bool> used(m_intervals.size(), false);

    for (size_t i = 0; i < m_intervals.size(); ++i) {
        if (used[i]) {
            continue;
        }

        ImageInterval current = m_intervals[i];
        used[i] = true;

        // Try to merge with all subsequent intervals
        bool didMerge;
        do {
            didMerge = false;
            for (size_t j = i + 1; j < m_intervals.size(); ++j) {
                if (used[j]) {
                    continue;
                }

                auto mergeResult = current.merge(m_intervals[j]);
                if (mergeResult.has_value()) {
                    current = *mergeResult;
                    used[j] = true;
                    didMerge = true;
                }
            }
        } while (didMerge);

        merged.push_back(current);
    }

    m_intervals = std::move(merged);
}

} // namespace vw
