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

#include "goodstore_simple.hpp"
#include "core/math.hpp"
#include "core/foreach.hpp"
#include "core/variant.hpp"
#include "core/stringhelper.hpp"
#include "core/logger.hpp"

class SimpleGoodStore::Impl
{
public:
  typedef std::vector<GoodStock> StockList;
  StockList stocks;
  int maxQty;
};

SimpleGoodStore::SimpleGoodStore() : _gsd( new Impl )
{
  _gsd->maxQty = 0;

  _gsd->stocks.resize(Good::goodCount);
  for (int n = 0; n < (int)Good::goodCount; ++n)
  {
    _gsd->stocks[n].setType( (Good::Type)n );
  }
}


void SimpleGoodStore::setMaxQty(const int maxQty)
{
  _gsd->maxQty = maxQty;
}


int SimpleGoodStore::getMaxQty() const
{
  return _gsd->maxQty;
}


int SimpleGoodStore::getCurrentQty() const
{
  int qty = 0;
  for( Impl::StockList::const_iterator goodIt = _gsd->stocks.begin();
       goodIt != _gsd->stocks.end(); ++goodIt)
  {
    qty += (*goodIt).qty();
  }

  return qty;
}


GoodStock& SimpleGoodStore::getStock(const Good::Type &goodType)
{
  return _gsd->stocks[goodType];
}


int SimpleGoodStore::getCurrentQty(const Good::Type &goodType) const
{
  return _gsd->stocks[goodType].qty();
}


int SimpleGoodStore::getMaxQty(const Good::Type &goodType) const
{
  return _gsd->stocks[goodType].cap();
}


void SimpleGoodStore::setMaxQty(const Good::Type &goodType, const int maxQty)
{
  _gsd->stocks[goodType].setCap( maxQty );
}


void SimpleGoodStore::setCurrentQty(const Good::Type &goodType, const int currentQty)
{
  _gsd->stocks[goodType].setQty( currentQty );
}

int SimpleGoodStore::getMaxStore(const Good::Type goodType)
{
  int freeRoom = 0;
  if( !isDevastation() )
  {
    int globalFreeRoom = getMaxQty() - getCurrentQty();

    // current free capacity
    freeRoom = math::clamp( _gsd->stocks[goodType].cap() - _gsd->stocks[goodType].qty(), 0, globalFreeRoom );

    // remove all storage reservations
    foreach( _Reservations::value_type& item, _getStoreReservations() )
    {
      freeRoom -= item.second.qty();
    }
  }

  return freeRoom;
}

void SimpleGoodStore::applyStorageReservation(GoodStock &stock, const long reservationID)
{
  GoodStock reservedStock = getStorageReservation(reservationID, true);

  if (stock.type() != reservedStock.type())
  {
    Logger::warning( "SimpleGoodStore:GoodType does not match reservation");
    return;
  }

  if (stock.qty() < reservedStock.qty())
  {
    Logger::warning( "SimpleGoodStore:Quantity does not match reservation");
    return;
  }

  int amount = reservedStock.qty();
  GoodStock &currentStock = _gsd->stocks[reservedStock.type()];
  currentStock.push( amount );
  stock.pop( amount );
}


void SimpleGoodStore::applyRetrieveReservation(GoodStock &stock, const long reservationID)
{
  GoodStock reservedStock = getRetrieveReservation(reservationID, true);

  if (stock.type() != reservedStock.type())
  {
    Logger::warning( "SimpleGoodStore:GoodType does not match reservation");
    return;
  }

  if( stock.cap() < stock.qty() + reservedStock.qty())
  {
    Logger::warning( "SimpleGoodStore:Quantity does not match reservation");
    return;
  }

  int amount = reservedStock.qty();
  GoodStock &currentStock = getStock(reservedStock.type());
  currentStock.pop( amount );
  stock.push( amount );
  // std::cout << "SimpleGoodStore, retrieve qty=" << amount << " resID=" << reservationID << std::endl;
}


VariantMap SimpleGoodStore::save() const
{
  VariantMap stream = GoodStore::save();

  stream[ "max" ] = _gsd->maxQty;

  VariantList stockSave;
  for( Impl::StockList::const_iterator itStock = _gsd->stocks.begin();
       itStock != _gsd->stocks.end(); itStock++)
  {
    stockSave.push_back( (*itStock).save() );
  }

  stream[ "stock" ] = stockSave;

  return stream;
}

void SimpleGoodStore::load( const VariantMap& stream )
{
  _gsd->stocks.clear();

  GoodStore::load( stream );
  _gsd->maxQty = (int)stream.get( "max" );

  VariantList stockSave = stream.get( "stock" ).toList();
  for( VariantList::iterator it=stockSave.begin(); it!=stockSave.end(); it++ )
  {
    GoodStock restored;
    restored.load( (*it).toList() );
    _gsd->stocks.push_back( restored );
  }
}

SimpleGoodStore::~SimpleGoodStore()
{

}

void SimpleGoodStore::resize( const GoodStore& other )
{
  setMaxQty( other.getMaxQty() );

  for( int i=Good::wheat; i < Good::goodCount; i++ )
  {
    Good::Type gtype = Good::Type( i );
    setMaxQty( gtype, other.getMaxQty( gtype ) );
  }
}
