//
// Created by Ruslan Bazhenov on 20/02/2025.
//

#ifndef SPARSE_VECTOR_HPP
#define SPARSE_VECTOR_HPP

#include <vector>
#include <optional>

template<typename T>
class SparseVector {
    using Storage = std::vector<std::optional<T>>;

    static inline const std::optional<T> s_nullopt {};  // Static member for null returns
    Storage m_data;
    size_t m_size { 0 };
    size_t m_first_empty { 0 }; // Track first empty slot

    void updateFirstEmpty(size_t start_pos = 0) {
        if (m_size >= m_data.size()) {
            m_first_empty = m_data.size();
            return;
        }
        auto it = std::find_if(
            m_data.begin() + start_pos, 
            m_data.end(),
            [](const auto& opt) { return !opt.has_value(); }
        );
        m_first_empty = it - m_data.begin();
    }

public:
    // Iterator class for non-empty elements
    class SparseIterator {
        Storage* m_vector;
        size_t m_current;

        // Find next non-empty position
        void findNextValid() {
            while (m_current < m_vector->size() && !(*m_vector)[m_current].has_value()) {
                ++m_current;
            }
        }

    public:
        // Iterator traits
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        SparseIterator(Storage* vec, const size_t pos)
            : m_vector(vec), m_current(pos) {
            findNextValid();
        }

        SparseIterator& operator++() {
            if (m_current < m_vector->size()) {
                ++m_current;
                findNextValid();
            }
            return *this;
        }

        SparseIterator operator++(int) {
            SparseIterator tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const SparseIterator& other) const {
            return m_current == other.m_current && m_vector == other.m_vector;
        }

        bool operator!=(const SparseIterator& other) const {
            return !(*this == other);
        }

        T& operator*() {
            return (*m_vector)[m_current].value();
        }

        T* operator->() {
            return &(*m_vector)[m_current].value();
        }

        [[nodiscard]] size_t position() const {
            return m_current;
        }
    };

    // Const iterator class for non-empty elements
    class ConstSparseIterator {
        const Storage* m_vector;
        size_t m_current;

        void findNextValid() {
            while (m_current < m_vector->size() && !(*m_vector)[m_current].has_value()) {
                ++m_current;
            }
        }

    public:
        // Iterator traits
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        ConstSparseIterator(const Storage* vec, const size_t pos)
            : m_vector(vec), m_current(pos) {
            findNextValid();
        }

        ConstSparseIterator& operator++() {
            if (m_current < m_vector->size()) {
                ++m_current;
                findNextValid();
            }
            return *this;
        }

        ConstSparseIterator operator++(int) {
            ConstSparseIterator tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const ConstSparseIterator& other) const {
            return m_current == other.m_current && m_vector == other.m_vector;
        }

        bool operator!=(const ConstSparseIterator& other) const {
            return !(*this == other);
        }

        const T& operator*() const {
            return (*m_vector)[m_current].value();
        }

        const T* operator->() const {
            return &(*m_vector)[m_current].value();
        }

        [[nodiscard]] size_t position() const {
            return m_current;
        }
    };

    explicit SparseVector(size_t size) :
        m_data { size } {}

    std::optional<T> operator[](typename Storage::size_type pos) noexcept {
        if (pos >= m_data.size()) {
            return std::nullopt;
        }
        return m_data[pos];
    }

    const std::optional<T>& operator[](typename Storage::size_type pos) const noexcept {
        if (pos >= m_data.size()) {
            return s_nullopt;
        }
        return m_data[pos];
    }

    template<typename U>
    T& add(U&& value) {
        if (m_first_empty < m_data.size()) {
            m_data[m_first_empty].emplace(std::forward<U>(value));
            ++m_size;
            updateFirstEmpty(m_first_empty + 1);
            return m_data[m_first_empty - 1].value();
        }
        m_data.emplace_back(std::forward<U>(value));
        ++m_size;
        m_first_empty = m_data.size();
        return m_data.back().value();
    }

    bool remove(size_t pos) {
        if (pos >= m_data.size() || !m_data[pos].has_value()) {
            return false;
        }
        m_data[pos] = std::nullopt;
        --m_size;
        if (pos < m_first_empty) {
            m_first_empty = pos;
        }
        return true;
    }

    bool remove(const T& value) requires std::equality_comparable<T> {
        for (size_t i = 0; i < m_data.size(); ++i) {
            if (m_data[i].has_value() && m_data[i].value() == value) {
                m_data[i] = std::nullopt;
                --m_size;
                if (i < m_first_empty) {
                    m_first_empty = i;
                }
                return true;
            }
        }
        return false;
    }

    // Remove elements that match the predicate
    template<typename Predicate>
    size_t remove_if(Predicate&& pred) {
        size_t removed_count = 0;
        for (size_t i = 0; i < m_data.size(); ++i) {
            if (m_data[i].has_value() && pred(m_data[i].value())) {
                m_data[i] = std::nullopt;
                --m_size;
                if (i < m_first_empty) {
                    m_first_empty = i;
                }
                ++removed_count;
            }
        }
        return removed_count;
    }

    [[nodiscard]] size_t size() const {
        return m_size;
    }

    // Iterator methods for non-empty elements
    SparseIterator begin() {
        return SparseIterator(&m_data, 0);
    }

    SparseIterator end() {
        return SparseIterator(&m_data, m_data.size());
    }

    // Const iterator methods for non-empty elements
    ConstSparseIterator begin() const {
        return ConstSparseIterator(&m_data, 0);
    }

    ConstSparseIterator end() const {
        return ConstSparseIterator(&m_data, m_data.size());
    }

    // Added cbegin() and cend() methods
    ConstSparseIterator cbegin() const {
        return ConstSparseIterator(&m_data, 0);
    }

    ConstSparseIterator cend() const {
        return ConstSparseIterator(&m_data, m_data.size());
    }

    // Add capacity() method
    [[nodiscard]] size_t capacity() const {
        return m_data.size();
    }

    template<typename Predicate>
    requires std::predicate<Predicate, T&>
    [[nodiscard]] std::optional<T> first_where(Predicate&& pred) {
        for (size_t i = 0; i < m_data.size(); ++i) {
            if (m_data[i].has_value() && pred(m_data[i].value())) {
                return m_data[i].value();
            }
        }
        return std::nullopt;
    }

    template<typename Predicate>
    requires std::predicate<Predicate, const T&>
    [[nodiscard]] std::optional<T> first_where(Predicate&& pred) const {
        for (size_t i = 0; i < m_data.size(); ++i) {
            if (m_data[i].has_value() && pred(m_data[i].value())) {
                return m_data[i].value();
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool empty() const {
        return m_size == 0;
    }

    template<typename Predicate>
    requires std::predicate<Predicate, const T&>
    [[nodiscard]] bool contains(Predicate&& pred) const {
        for (size_t i = 0; i < m_data.size(); ++i) {
            if (m_data[i].has_value() && pred(m_data[i].value())) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool contains(const T& value) const {
        for (size_t i = 0; i < m_data.size(); ++i) {
            if (m_data[i].has_value() && m_data[i].value() == value) {
                return true;
            }
        }
        return false;
    }
};

#endif //SPARSE_VECTOR_HPP
