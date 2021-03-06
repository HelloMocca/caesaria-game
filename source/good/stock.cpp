// This file is part of CaesarIA.
//
// CaesarIA is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// CaesarIA is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with CaesarIA.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright 2012-2013 Gregoire Athanase, gathanase@gmail.com
// Copyright 2012-2013 Dalerank, dalerankn8@gmail.com

#include "stock.hpp"

#include "core/gettext.hpp"
#include "core/exception.hpp"
#include "core/variant_list.hpp"
#include "core/utils.hpp"
#include "core/logger.hpp"

namespace good
{

Stock::Stock()
{
  setType( none );
  _capacity = 0;
  _qty = 0;
}

Stock::Stock(const Product& which, const int capacity, const int qty)
  : _type( which )
{
  _capacity = capacity;
  _qty = qty;
}

Stock::~Stock() {}

const Product& Stock::type() const { return _type; }

void Stock::takeFrom(good::Stock& stock, const int iAmount)
{
  if (stock.type() == none)
  {
     // nothing to add => nothing to do!
     return;
  }
  if (type() != none && type() != stock.type() )
  {
    std::string errorStr = fmt::format( "GoodTypes do not match: {} vs {}", _type, stock._type );
    Logger::warning( errorStr );
    return;
  }

  int amount = iAmount;  // not const
  if (amount == -1)
  {
     amount = stock._qty;
  }
  if (amount > stock._qty)
  {
    Logger::warning( "GoodStock:Not enough quantity in stock." );
    return;
  }

  amount = math::clamp( amount, 0, _capacity - _qty );
  if (amount+_qty > _capacity)
  {
    Logger::warning( "GoodStock: not enough free room for storage");
    return;
  }

  _type = stock._type;  // in case goodType was Good::none
  _qty += amount;
  stock._qty -= amount;
}

VariantList Stock::save() const
{
  VariantList stream;
  stream << (int)_type
         << _capacity
         << _qty;
  if( _info.sender > 0 )
  {
    stream << _info.sender
           << _info.birth;
  }

  return stream;
}

enum { idxType=0, idxCapacity, idxQty, idxSender, idxBirth };

void Stock::load( const VariantList& stream )
{
  if( stream.empty() )
    return;

  _type = good::Product( stream.get( idxType ).toInt() );
  if( _type >= good::any() )
  {
    Logger::warning( "GoodStock: wrong type of good {0}", _type );
    _type = good::none;
  }

  _capacity = (int)stream.get( idxCapacity );

  _qty = math::clamp( (int)stream.get( idxQty ), 0, _capacity );
  _info.sender = stream.get( idxSender, 0 );
  if( _info.sender > 0 )
    _info.birth = stream.get( idxBirth ).toDateTime();
}

void Stock::setInfo(const DateTime& date, unsigned int tag)
{
  _info.birth = date;
  _info.sender = tag;
}

bool Stock::empty() const {  return _qty == 0; }
void Stock::setInfo(const Stock::Info& info) { _info = info; }
const Stock::Info& Stock::info() const { return _info; }
void Stock::setType( Product goodType ) { _type = goodType; }
void Stock::setCapacity( const int maxQty ){  _capacity = maxQty;}
int Stock::freeQty() const{  return std::max( _capacity - _qty, 0 );}

void Stock::pop(const int qty)
{
  _qty -= math::clamp( qty, 0, _qty );
}

}//end namespace good
