//
// Created by Bruno Kuś on 09/02/2026.
//

#include "CursorView.h"

auto CursorViewIterator::operator++() -> CursorViewIterator&
{
 ++( m_cursorViewDataPtr->m_it);
//  ++(*m_cursor_view->m_it);
  return *this;
}
auto CursorViewIterator::operator++(int) -> void
{
  ++(m_cursorViewDataPtr->m_it);
//  ++(*m_cursor_view->m_it);
}
bool operator==(const CursorViewIterator &iterator_1,const  CursorViewSentinel&)
{
  if (iterator_1.m_cursorViewDataPtr == nullptr){return false; }
  else {
    return iterator_1.m_cursorViewDataPtr->m_it == iterator_1.m_cursorViewDataPtr->m_cursor.end();
  }
//  if (iterator_1.m_cursor_view == nullptr){ return false;}
//  else {
//    return *(iterator_1.m_cursor_view->m_it) == iterator_1.m_cursor_view->m_cursor.end();
//  }
}
auto CursorViewIterator::operator*() const -> const bsoncxx::v_noabi::document::view &
//{ return **m_cursor_view->m_it; }
{
  return *m_cursorViewDataPtr->m_it;
}