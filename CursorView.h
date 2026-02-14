//
// Created by Bruno Kuś on 09/02/2026.
//

#ifndef MONGO_2_CURSORVIEW_H
#define MONGO_2_CURSORVIEW_H

#include <bsoncxx/v_noabi/bsoncxx/document/view-fwd.hpp>
#include <ranges>
#include <mongocxx/cursor.hpp>

struct CursorView;

struct CursorViewData  {
    explicit CursorViewData(mongocxx::cursor&& cursor)
      : m_cursor{std::move(cursor)}
      , m_it{m_cursor.begin()}
    {
    }
    mongocxx::cursor m_cursor;
    mongocxx::cursor::iterator m_it;
//    std::shared_ptr<mongocxx::cursor::iterator> m_it;
};
struct CursorViewSentinel {};

struct CursorViewIterator
{
//    CursorViewIterator() = default;
    
    explicit CursorViewIterator(std::shared_ptr<CursorViewData> cursor_view) : m_cursorViewDataPtr{cursor_view} {}
    
    using difference_type = std::ptrdiff_t;
    using value_type = bsoncxx::v_noabi::document::view;
    
    auto operator*() const -> const bsoncxx::v_noabi::document::view &;
    
    auto operator++() -> CursorViewIterator &;
    
    auto operator++(int) -> void;
    
    friend bool operator==(const CursorViewIterator &iterator_1, const CursorViewSentinel &);
    
    inline friend bool operator==(const CursorViewSentinel& sentinel, const CursorViewIterator & iterator)
    {
      return iterator == sentinel;
    }
    
    private:
    std::shared_ptr<CursorViewData> m_cursorViewDataPtr;
//    const CursorView *m_cursor_view = nullptr;
};

struct CursorView : std::ranges::view_interface<CursorView>
{
    friend CursorViewIterator;
    friend bool operator==(const CursorViewIterator &, const CursorViewSentinel&);
    using iterator = CursorViewIterator;
    using sentinel = CursorViewSentinel;
    
    explicit CursorView(mongocxx::cursor &&cursor)
    : m_dataPtr{
      std::make_shared<CursorViewData>(std::move(cursor))
    }
//      : m_cursor{std::move(cursor)}, m_it{std::make_shared<mongocxx::cursor::iterator>(m_cursor.begin())}
    {
    }
    
    auto begin() const -> iterator { return iterator{m_dataPtr}; }
    
    auto end() const -> sentinel { return sentinel{}; }
    
    private:
    std::shared_ptr<CursorViewData> m_dataPtr;
//    mutable mongocxx::cursor m_cursor;
//    std::shared_ptr<mongocxx::cursor::iterator> m_it;
};


static_assert(std::input_iterator<CursorView::iterator>);
static_assert(std::ranges::range<CursorView>);

static_assert(std::ranges::input_range<CursorView>);

static_assert(std::equality_comparable<mongocxx::cursor::iterator>);


#endif //MONGO_2_CURSORVIEW_H
