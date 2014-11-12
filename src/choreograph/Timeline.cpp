/*
 * Copyright (c) 2014 David Wicks, sansumbrella.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Timeline.h"
#include "detail/VectorManipulation.hpp"

using namespace choreograph;

Timeline::Timeline():
  _updating( false )
{}

void Timeline::removeFinishedAndInvalidMotions()
{
  detail::erase_if( &_items, [] ( const TimelineItemUniqueRef &motion ) { return (motion->getRemoveOnFinish() && motion->isFinished()) || motion->isInvalid(); } );
}

void Timeline::step( Time dt )
{
  { // update all items
    std::lock_guard<std::mutex> lock( _item_mutex );
    _updating = true;

    // Update all animation outputs.
    for( auto &c : _items ) {
      c->step( dt );
    }
    _updating = false;

    removeFinishedAndInvalidMotions();
  }

  processQueue();
}

void Timeline::jumpTo( Time time )
{
  { // update all items
    std::lock_guard<std::mutex> lock( _item_mutex );
    _updating = true;

    // Update all animation outputs.
    for( auto &c : _items ) {
      c->jumpTo( time );
    }
    _updating = false;

    removeFinishedAndInvalidMotions();
  }

  processQueue();
}

Time Timeline::calcDuration() const
{
	Time end = 0;
	for( auto &item : _items ) {
		end = std::max( end, item->getEndTime() );
	}
	return end;
}

void Timeline::processQueue()
{
  using namespace std;
  lock_guard<mutex> queue_lock( _queue_mutex );
  lock_guard<mutex> item_lock( _item_mutex );

  copy( make_move_iterator( _queue.begin() ), make_move_iterator( _queue.end() ), back_inserter( _items ) );
  _queue.clear();
}

void Timeline::remove( void *output )
{
  detail::erase_if( &_items, [=] (const TimelineItemUniqueRef &m) { return m->getTarget() == output; } );
}

void Timeline::add( TimelineItemUniqueRef item )
{
  item->setRemoveOnFinish( _default_remove_on_finish );

  if( _updating ) {
    std::lock_guard<std::mutex> lock( _queue_mutex );
    _queue.emplace_back( std::move( item ) );
  }
  else {
    std::lock_guard<std::mutex> lock( _item_mutex );
    _items.emplace_back( std::move( item ) );
  }
}

CueOptions Timeline::cue( const std::function<void ()> &fn, Time delay )
{
  auto cue = std::make_unique<Cue>( fn, delay );
  CueOptions options( *cue );

  add( std::move( cue ) );

  return options;
}
