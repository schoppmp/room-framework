#pragma once

// like boost::combine, but with std::pairs
template <typename Range1, typename Range2>
boost::iterator_range<boost::iterators::zip_iterator<
    std::pair<typename Range1::iterator, typename Range2::iterator>>>
combine_pair(Range1 r1, Range2 r2) {
  auto begin_pair = std::make_pair(boost::begin(r1), boost::begin(r2));
  auto begin_iterator = boost::make_zip_iterator(begin_pair);
  return boost::make_iterator_range_n(begin_iterator, boost::size(r1));
}
