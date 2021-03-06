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
// Copyright 2012-2015 Dalerank, dalerankn8@gmail.com

#include "dock.hpp"
#include "gfx/tile_config.hpp"
#include "core/variant_map.hpp"
#include "game/resourcegroup.hpp"
#include "city/statistic.hpp"
#include "gfx/tilemap.hpp"
#include "good/helper.hpp"
#include "walker/merchant_sea.hpp"
#include "walker/cart_supplier.hpp"
#include "good/storage.hpp"
#include "events/event.hpp"
#include "game/gamedate.hpp"
#include "walker/cart_pusher.hpp"
#include "events/fundissue.hpp"
#include "pathway/pathway_helper.hpp"
#include "objects_factory.hpp"

using namespace direction;
using namespace events;
using namespace gfx;

REGISTER_CLASS_IN_OVERLAYFACTORY(object::dock, Dock)

class Dock::Impl
{
public:
  enum { southPic=29, northPic=5, westPic=41, eastPic=17 };

  struct
  {
    good::Storage exporting;
    good::Storage importing;
    good::Storage requested;
  } goods;

  DateTime dateSendGoods;
  std::vector<int> saved_tile;
  Direction direction;

public:
  bool isFlatCoast( const Tile& tile ) const;
  Direction getDirection(PlayerCityPtr city, TilePos pos, Size size);
  bool isConstructibleArea( const TilesArray& tiles );
  bool isCoastalArea( const TilesArray& tiles );
  SeaMerchantList getMerchantsOnWait( PlayerCityPtr city, TilePos pos );
  void initStores();
};

class DockConfig
{
public:
  struct AnimConfig
  {
    static const int noOffset = -9999;
    AnimConfig( const VariantMap& stream )
    {
      VARIANT_LOAD_ANY( pingpong, stream )
      VARIANT_LOAD_STR( rc, stream )
      VARIANT_LOAD_ANY( start, stream )
      VARIANT_LOAD_ANY( count, stream )
      VARIANT_LOAD_ANY( delay, stream )
      VARIANT_LOAD_ANYDEF( offset, stream, Point( noOffset, noOffset ) )
      customOffset = offset.x() != noOffset;
    }

    bool pingpong;
    std::string rc;
    unsigned int delay;
    int start;
    bool customOffset;
    Point offset;
    int count;
  };

  void load( const object::Info& md, Direction dir )
  {
    std::string configName = "image." + direction::Helper::instance().findName( dir );

    VariantMap stream = md.getOption( configName ).toMap();
    if( !stream.empty() )
    {
      VARIANT_LOAD_PICTURE( image, stream )
      VARIANT_LOAD_PICTURE( stock, stream )

      AnimConfig anim( stream.get( "animation" ).toMap() );
      animation.load( anim.rc, anim.start, anim.count );
      animation.setDelay( anim.delay );

      if( anim.pingpong )
        animation.load( anim.rc, anim.start + anim.count, anim.count, true );

      if( anim.customOffset )
        animation.setOffset( anim.offset );
    }
  }

  Picture image;
  Picture stock;
  Animation animation;
};

Dock::Dock(): WorkingBuilding( object::dock, Size(3) ), _d( new Impl )
{
  // dock pictures
  // transport 5        animation = 6~16
  // transport 17       animation = 18~28
  // transport 29       animation = 30~40
  // transport 41       animation = 42~51
  setPicture( Picture( ResourceGroup::transport, 5 ) );

  _d->initStores();
  _fgPictures().resize(1);
  _animation().setDelay( 5 );
  _setClearAnimationOnStop( false );
}

bool Dock::canBuild( const city::AreaInfo& areaInfo ) const
{
  bool is_constructible = true;//Construction::canBuild( city, pos );

  Direction direction = _d->getDirection( areaInfo.city, areaInfo.pos, size() );

  const_cast< Dock* >( this )->_setDirection( direction );

  return (is_constructible && direction != direction::none );
}

bool Dock::build( const city::AreaInfo& info )
{
  _setDirection( _d->getDirection( info.city, info.pos, size() ) );

  TilesArea area(  info.city->tilemap(), info.pos, size() );

  for( auto tile : area )
     _d->saved_tile.push_back( tile::encode( *tile ) );

  WorkingBuilding::build( info );

  TilePos landingPos = landingTile().pos();
  Pathway way = PathwayHelper::create( landingPos, info.city->getBorderInfo( PlayerCity::boatEntry ).epos(),
                                       PathwayHelper::deepWater );
  if( !way.isValid() )
  {
    _setError( "##inland_lake_has_no_access_to_sea##" );
  }

  return true;
}

void Dock::destroy()
{
  TilesArray tiles = area();

  int index=0;
  for( auto tile : tiles )
    tile::decode( *tile, _d->saved_tile[ index++ ] );

  WorkingBuilding::destroy();
}

void Dock::timeStep(const unsigned long time)
{
  if( time % game::Date::days2ticks( 1 ) == 0 )
  {
    if( _d->dateSendGoods < game::Date::current() )
    {
      _tryReceiveGoods();
      _tryDeliverGoods();

      _d->dateSendGoods = game::Date::current();
      _d->dateSendGoods.appendWeek();
    }
  }

  WorkingBuilding::timeStep( time );
}

void Dock::save(VariantMap& stream) const
{
  WorkingBuilding::save( stream );

  VARIANT_SAVE_ANY_D( stream, _d, direction )
  VARIANT_SAVE_ANY_D( stream, _d, dateSendGoods )
  VARIANT_SAVE_CLASS_D_LIST( stream, _d, saved_tile )
  VARIANT_SAVE_CLASS_D( stream, _d, goods.exporting )
  VARIANT_SAVE_CLASS_D( stream, _d, goods.importing )
  VARIANT_SAVE_CLASS_D( stream, _d, goods.requested )
}

void Dock::load(const VariantMap& stream)
{
  Building::load( stream );

  _d->direction = (Direction)stream.get( TEXT(direction), direction::southWest ).toInt();
  VARIANT_LOAD_CLASS_D_AS_LIST( _d, saved_tile, stream )
  VARIANT_LOAD_CLASS_D( _d, goods.exporting, stream )
  VARIANT_LOAD_CLASS_D( _d, goods.importing, stream )
  VARIANT_LOAD_CLASS_D( _d, goods.requested, stream )
  VARIANT_LOAD_TIME_D( _d, dateSendGoods, stream );

  _updatePicture( _d->direction );
}

void Dock::reinit()
{
  info().reload();
  _updatePicture( _d->direction );
}

std::string Dock::workersProblemDesc() const
{
  return WorkingBuildingHelper::productivity2desc( const_cast<Dock*>( this ), isBusy() ? "busy" : "" );
}

bool Dock::isBusy() const
{
  SeaMerchantList merchants = _city()->statistic().walkers.find<SeaMerchant>( walker::seaMerchant,
                                                                        landingTile().pos() );

  return !merchants.empty();
}

const Tile& Dock::landingTile() const
{
  TilePos offset( -999, -999 );
  switch( _d->direction )
  {
  case direction::south: offset = TilePos( 0, -1 ); break;
  case direction::west: offset = TilePos( -1, 0 ); break;
  case direction::north: offset = TilePos( 0, 3 ); break;
  case direction::east: offset = TilePos( 3, 0 ); break;

  default: break;
  }

  return _map().at( pos() + offset );
}

int Dock::queueSize() const
{
  TilePos offset( 3, 3 );
  SeaMerchantList merchants = _city()->statistic().walkers.find<SeaMerchant>( walker::seaMerchant,
                                                                              pos() - offset, pos() + offset );

  for( SeaMerchantList::iterator it=merchants.begin(); it != merchants.end(); )
  {
    if( !(*it)->isWaitFreeDock() ) { it = merchants.erase( it ); }
    else { ++it; }
  }

  return merchants.size();
}

const good::Store& Dock::exportStore() const { return _d->goods.exporting; }

const Tile& Dock::queueTile() const
{
  TilesArea tiles( _map(), 3, pos() );
  tiles = tiles.select( Tile::tlDeepWater );

  for( auto tile : tiles )
  {
    bool needMove;
    bool busyTile = _city()->statistic().map.isTileBusy<SeaMerchant>( tile->pos(), WalkerPtr(), needMove );
    if( !busyTile )
    {
      return *tile;
    }
  }

  return tile::getInvalid();
}

void Dock::requestGoods(good::Stock& stock)
{
  int maxRequest = std::min( stock.qty(), _d->goods.requested.getMaxStore( stock.type() ) );
  maxRequest -= _d->goods.exporting.qty( stock.type() );

  if( maxRequest > 0 )
  {
    _d->goods.requested.store( stock, maxRequest );
  }
}

int Dock::importingGoods( good::Stock& stock)
{
  const good::Store& cityOrders = _city()->buys();

  //try sell goods
  int traderMaySell = std::min( stock.qty(), cityOrders.capacity( stock.type() ) );
  int dockMayStore = _d->goods.importing.freeQty( stock.type() );

  traderMaySell = std::min( traderMaySell, dockMayStore );
  int cost = 0;
  if( traderMaySell > 0 )
  {
    _d->goods.importing.store( stock, traderMaySell );

    events::dispatch<Payment>( stock.type(), traderMaySell );

    cost = good::Helper::importPrice( _city(), stock.type(), traderMaySell );
  }

  return cost;
}

void Dock::storeGoods( good::Stock &stock, const int)
{
  _d->goods.exporting.store( stock, stock.qty() );
}

int Dock::exportingGoods( good::Stock& stock, int qty )
{
  qty = std::min( qty, _d->goods.exporting.getMaxRetrieve( stock.type() ) );
  _d->goods.exporting.retrieve( stock, qty );

  int cost = 0;
  if( qty > 0 )
  {
    GameEventPtr e = Payment::exportg( stock.type(), qty );
    e->dispatch();

    cost = good::Helper::exportPrice( _city(), stock.type(), qty );
  }

  return cost;
}

Dock::~Dock(){}

void Dock::_updatePicture(Direction direction)
{
  DockConfig config;
  config.load( info(), direction );

  setPicture( config.image );
  setAnimation( config.animation );
}

void Dock::_setDirection(Direction direction)
{
  _d->direction = direction;
  _updatePicture( direction );
}

bool Dock::Impl::isFlatCoast(const Tile& tile) const
{
  int imgId = tile.imgId();
  return (imgId >= 372 && imgId <= 387);
}

Direction Dock::Impl::getDirection(PlayerCityPtr city, TilePos pos, Size size)
{
  Tilemap& tilemap = city->tilemap();

  int s = size.width();
  TilesArray constructibleTiles = tilemap.area( pos + TilePos( 0, 1 ), pos + TilePos( s-1, s-1 ) );
  TilesArray coastalTiles = tilemap.area( pos, pos + TilePos( s, 0 ) );

  if( isConstructibleArea( constructibleTiles ) && isCoastalArea( coastalTiles ) )
  { return south; }

  constructibleTiles = tilemap.area( pos, pos + TilePos( s-1, 1 ) );
  coastalTiles = tilemap.area( pos + TilePos( 0, s-1 ), pos + TilePos( s-1, s-1 ) );

  if( isConstructibleArea( constructibleTiles ) && isCoastalArea( coastalTiles ) )
  { return north; }

  constructibleTiles = tilemap.area( pos + TilePos( 1, 0 ), pos + TilePos( 2, 2 ) );
  coastalTiles = tilemap.area( pos, pos + TilePos( 0, 2 ) );

  if( isConstructibleArea( constructibleTiles ) && isCoastalArea( coastalTiles ) )
  { return west; }

  constructibleTiles = tilemap.area( pos, pos + TilePos( 1, 2 ) );
  coastalTiles = tilemap.area( pos + TilePos( 2, 0), pos + TilePos( 2, 2 ) );

  if( isConstructibleArea( constructibleTiles ) && isCoastalArea( coastalTiles ) )
  { return east; }

  return direction::none;
}

bool Dock::Impl::isConstructibleArea(const TilesArray& tiles)
{
  bool ret = true;
  for( auto tile : tiles )
    ret &= tile->getFlag( Tile::isConstructible );

  return ret;
}

bool Dock::Impl::isCoastalArea(const TilesArray& tiles)
{
  bool ret = true;
  for( auto tile : tiles )
    ret &= tile->getFlag( Tile::tlWater ) && isFlatCoast( *tile );

  return ret;
}

void Dock::Impl::initStores()
{
  goods.importing.setCapacity( good::any(), 1000 );
  goods.exporting.setCapacity( good::any(), 1000 );
  goods.requested.setCapacity( good::any(), 1000 );

  goods.importing.setCapacity( 4000 );
  goods.exporting.setCapacity( 4000 );
  goods.requested.setCapacity( 4000 );
}

void Dock::_tryDeliverGoods()
{
  if( walkers().size() > 2 )
  {
    return;
  }

  for( auto& gtype : good::all() )
  {
    int qty = std::min( _d->goods.importing.getMaxRetrieve( gtype ), 400 );

    if( qty > 0 )
    {
      auto cartPusher = Walker::create<CartPusher>( _city() );
      good::Stock pusherStock( gtype, qty, 0 );
      _d->goods.importing.retrieve( pusherStock, qty );
      cartPusher->send2city( BuildingPtr( this ), pusherStock );

      //success to send cartpusher
      if( !cartPusher->isDeleted() )
      {
        if( cartPusher->pathway().isValid() )
        {
          addWalker( cartPusher.object() );
        }
        else
        {
          _d->goods.importing.store( pusherStock, qty );
          cartPusher->deleteLater();
        }
      }
      else
      {
        _d->goods.importing.store( pusherStock, qty );
      }
    }
  }
}

void Dock::_tryReceiveGoods()
{
  if( walkers().size() >= 2 )
  {
    return;
  }

  for( auto& gtype : good::all() )
  {
    if( _d->goods.requested.qty( gtype ) > 0 )
    {
      auto cartSupplier = Walker::create<CartSupplier>( _city() );
      int qty = std::min( 400, _d->goods.requested.getMaxRetrieve( gtype ) );
      cartSupplier->send2city( this, gtype, qty );

      if( !cartSupplier->isDeleted() )
      {
        addWalker( cartSupplier.object() );
        good::Stock tmpStock( gtype, qty, 0 );
        _d->goods.requested.retrieve( tmpStock, qty );
        return;
      }
      else
      {
        _d->goods.requested.setQty( gtype, 0 );
      }
    }
  }
}
