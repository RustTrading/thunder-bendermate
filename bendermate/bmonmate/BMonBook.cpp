#include "BMonBook.h"

#include <limits>

namespace MonPy {

size_t BMonBook::size() const {
    return m_book->size();
}

size_t BMonBook::bidSize(size_t index) const {
    return m_book->bidSize(index);
}

size_t BMonBook::askSize(size_t index) const {
    return m_book->askSize(index);
}

size_t BMonBook::bidNum(size_t index) const {
    return m_book->bidNum(index);
}

size_t BMonBook::askNum(size_t index) const {
    return m_book->askNum(index);
}

double BMonBook::bidPrice(size_t index) const {
    return m_book->bidPrice(index);
}

double BMonBook::askPrice(size_t index) const {
    return m_book->askPrice(index);
}

double BMonBook::averagePrice(size_t size) const {
    if (m_book)
        return m_book->averagePrice(size);

    return std::numeric_limits<double>::quiet_NaN();
}

double BMonBook::bidAveragePrice(size_t size) const {
    if (m_book)
        return m_book->bidAveragePrice(size);

    return std::numeric_limits<double>::quiet_NaN();
}

double BMonBook::askAveragePrice(size_t size) const {
    if (m_book)
        return m_book->askAveragePrice(size);

    return std::numeric_limits<double>::quiet_NaN();
}

}