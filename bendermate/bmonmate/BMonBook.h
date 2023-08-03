#ifndef BMONBOOK_H
#define BMONBOOK_H

#include "Data/BBook.h"

namespace MonPy
{

class BMonBook
{
public:
    BMonBook(const BBookPtr& book)
    : m_book(book)
        {}

    size_t size() const;

    size_t bidSize(size_t index) const;
    size_t askSize(size_t index) const;

    size_t bidNum(size_t index) const;
    size_t askNum(size_t index) const;

    double bidPrice(size_t index) const;
    double askPrice(size_t index) const;

    double averagePrice(size_t size) const;
    double bidAveragePrice(size_t size) const;
    double askAveragePrice(size_t size) const;

//    boost::python::numpy::ndarray bidPrices() const;
//    boost::python::numpy::ndarray askPrices() const;
//
//    boost::python::numpy::ndarray bidSizes() const;
//    boost::python::numpy::ndarray askSizes() const;

private:
    BBookPtr m_book;
};

}
/****************************************************************************/

/*
 * Local Variables:
 * mode: c++
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */

#endif
