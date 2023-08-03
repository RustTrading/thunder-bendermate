#include "BMonDeal.h"
//#include "PyInstrument.h"

namespace MonMate {

    // TODO: add instrument
//boost::python::object BMonDeal::instrument() const {
//    return boost::python::object(PyInstrument(m_deal->instrument()));
//}

std::int64_t BMonDeal::size() const {
    return m_deal->sizeSigned();
}

double BMonDeal::price() const {
    return m_deal->price();
}

BDefs::OrderSide BMonDeal::side() const {
    return m_deal->side();
}

}