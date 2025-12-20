#pragma once

#include "VulkanWrapper/Memory/Interval.h"
#include <algorithm>
#include <vector>

namespace vw {

/**
 * Manages a collection of non-overlapping buffer intervals.
 * Automatically merges overlapping or adjacent intervals when adding.
 */
class BufferIntervalSet {
  public:
    BufferIntervalSet() = default;

    /**
     * Adds an interval to the set, merging with existing intervals if
     * necessary.
     */
    void add(const BufferInterval &interval);

    /**
     * Removes an interval from the set.
     * This may split existing intervals if the removed interval is in the
     * middle.
     */
    void remove(const BufferInterval &interval);

    /**
     * Finds all intervals that overlap with the given interval.
     */
    [[nodiscard]] std::vector<BufferInterval>
    findOverlapping(const BufferInterval &interval) const;

    /**
     * Checks if the set contains any interval that overlaps with the given
     * interval.
     */
    [[nodiscard]] bool hasOverlap(const BufferInterval &interval) const;

    /**
     * Returns all intervals in the set.
     */
    [[nodiscard]] const std::vector<BufferInterval> &intervals() const {
        return m_intervals;
    }

    /**
     * Checks if the set is empty.
     */
    [[nodiscard]] bool empty() const { return m_intervals.empty(); }

    /**
     * Returns the number of intervals in the set.
     */
    [[nodiscard]] size_t size() const { return m_intervals.size(); }

    /**
     * Clears all intervals from the set.
     */
    void clear() { m_intervals.clear(); }

  private:
    // Intervals are kept sorted by offset for efficient operations
    std::vector<BufferInterval> m_intervals;

    // Maintains the sorted invariant and merges adjacent/overlapping intervals
    void mergeSorted();
};

/**
 * Manages a collection of non-overlapping image intervals.
 * Automatically merges overlapping or adjacent intervals when adding.
 */
class ImageIntervalSet {
  public:
    ImageIntervalSet() = default;

    /**
     * Adds an interval to the set, merging with existing intervals if
     * necessary.
     */
    void add(const ImageInterval &interval);

    /**
     * Removes an interval from the set.
     * This may split existing intervals if the removed interval is in the
     * middle.
     */
    void remove(const ImageInterval &interval);

    /**
     * Finds all intervals that overlap with the given interval.
     */
    [[nodiscard]] std::vector<ImageInterval>
    findOverlapping(const ImageInterval &interval) const;

    /**
     * Checks if the set contains any interval that overlaps with the given
     * interval.
     */
    [[nodiscard]] bool hasOverlap(const ImageInterval &interval) const;

    /**
     * Returns all intervals in the set.
     */
    [[nodiscard]] const std::vector<ImageInterval> &intervals() const {
        return m_intervals;
    }

    /**
     * Checks if the set is empty.
     */
    [[nodiscard]] bool empty() const { return m_intervals.empty(); }

    /**
     * Returns the number of intervals in the set.
     */
    [[nodiscard]] size_t size() const { return m_intervals.size(); }

    /**
     * Clears all intervals from the set.
     */
    void clear() { m_intervals.clear(); }

  private:
    std::vector<ImageInterval> m_intervals;

    // Merges overlapping/adjacent intervals with matching aspect masks
    void mergeCompatible();
};

} // namespace vw
