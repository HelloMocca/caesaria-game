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
// Copyright 2012-2015 dalerank, dalerankn8@gmail.com

#include "loader.hpp"

#include "loader_map.hpp"
#include "loader_sav.hpp"
#include "loader_oc3save.hpp"
#include "loader_mission.hpp"
#include "loader_omap.hpp"
#include "core/position.hpp"
#include "gfx/tilemap.hpp"
#include "core/utils.hpp"
#include "gfx/imgid.hpp"
#include "resourcegroup.hpp"
#include "gfx/animation_bank.hpp"
#include "game.hpp"
#include "objects/objects_factory.hpp"
#include "city/city.hpp"
#include "core/logger.hpp"
#include "objects/waymark.hpp"
#include "climatemanager.hpp"

#include <vector>

using namespace gfx;

namespace game
{

namespace loader
{
typedef SmartPtr< loader::Base > BasePtr;
}

class Loader::Impl
{
public:
  typedef std::vector< loader::BasePtr > Loaders;
  typedef Loaders::iterator LoaderIterator;
  Loaders loaders;
  std::string restartFile;

  void initLoaders();
  void initEntryExitTile( const TilePos& tlPos, PlayerCityPtr city );
  void initTilesAnimation( Tilemap& tmap );
  void finalize(Game& game , bool needInitEnterExit);
  bool maySetSign( const Tile& tile );
  void clearTile(Tile& tile);

public signals:
  Signal1<std::string> onUpdateSignal;
};

Loader::Loader() : _d( new Impl )
{
  _d->initLoaders();
}

Loader::~Loader() {}

void Loader::Impl::initEntryExitTile( const TilePos& tlPos, PlayerCityPtr city )
{
  TilePos tlOffset;
  Tilemap& tmap = city->tilemap();
  if( tlPos.i() == 0 || tlPos.i() == tmap.size() - 1 )
  {    
    tlOffset = TilePos( 0, 1 );
  }
  else if( tlPos.j() == 0 || tlPos.j() == tmap.size() - 1 )
  {
    tlOffset = TilePos( 1, 0 );
  }

  Tile& tmpTile = tmap.at( tlPos + tlOffset );
  if( !maySetSign( tmpTile ) )
  {
    tlOffset = -tlOffset;
  }

  Tile& signTile = tmap.at( tlPos + tlOffset );

  Logger::warning( "({}, {})", tlPos.i(),    tlPos.j()    );
  Logger::warning( "({}, {})", tlOffset.i(), tlOffset.j() );

  if( maySetSign( signTile ) )
  {
    clearTile( signTile );
    OverlayPtr waymark = Overlay::create( object::waymark );
    city::AreaInfo info( city, tlPos + tlOffset );
    waymark->build( info );
    city->addOverlay( waymark );
  }
}

void Loader::Impl::initTilesAnimation( Tilemap& tmap )
{
  TilesArray area = tmap.allTiles();

  const Animation& meadow = AnimationBank::simple( AnimationBank::animMeadow );
  for( auto tile : area )
  {
    int rId = tile->imgId() - 364;
    if( rId >= 0 && rId < 8 )
    {
      Animation water = AnimationBank::simple( AnimationBank::animWater );
      water.setIndex( rId );
      tile->setAnimation( water );
      tile->setFlag( Tile::tlDeepWater, true );
    }

    if( tile->getFlag( Tile::tlMeadow ) )
    {
      Animation meadowAnim;
      int index = math::random( meadow.size()-1 );
      meadowAnim.addFrame( meadow.frame( index ) );
      meadowAnim.setDelay( 10 );

      if( !tile->picture().isValid() )
      {
        Picture pic = object::Info::find( object::terrain ).randomPicture( Size(1) );
        tile->setPicture( pic );
      }
      tile->setAnimation( meadowAnim );
    }
  }
}

void Loader::Impl::finalize( Game& game, bool needInitEnterExit )
{
  Tilemap& tileMap = game.city()->tilemap();

  // exit and entry can't point to one tile or .... can!
  if( needInitEnterExit )
  {
    initEntryExitTile( game.city()->getBorderInfo( PlayerCity::roadEntry ).epos(), game.city() );
    initEntryExitTile( game.city()->getBorderInfo( PlayerCity::roadExit ).epos(),  game.city() );
  }

  initTilesAnimation( tileMap );
}

bool Loader::Impl::maySetSign(const Tile &tile)
{
  return (tile.isWalkable( true ) && !tile.getFlag( Tile::tlRoad)) || tile.getFlag( Tile::tlTree );
}

void Loader::Impl::clearTile(Tile& tile)
{
  int startOffset  = ( (math::random( 10 ) > 6) ? 62 : 232 );
  int imgId = math::random( 58 );

  Picture pic( config::rc.land1a, startOffset + imgId );
  tile.setPicture( config::rc.land1a, startOffset + imgId );
  tile.setImgId( imgid::fromResource( pic.name() ) );
}

void Loader::Impl::initLoaders()
{
  loaders.push_back( new loader::C3Map() );
  loaders.push_back( new loader::C3Sav() );
  loaders.push_back( new loader::OC3() );
  loaders.push_back( new loader::Mission() );
  loaders.push_back( new loader::OMap() );
}

Signal1<std::string>& Loader::onUpdate() { return _d->onUpdateSignal; }

bool Loader::load(vfs::Path filename, Game& game)
{
  // try to load file based on file extension
  for( auto loader : _d->loaders )
  {
    if( !loader->isLoadableFileExtension( filename.toString() ) )
      continue;

    ClimateType currentClimate = (ClimateType)loader->climateType( filename.toString() );
    if( currentClimate >= 0  )
    {
      game::climate::initialize( currentClimate );
    }

    bool loadok = loader->load( filename.toString(), game );
    bool needToFinalizeMap = loader->finalizeMap();
    if( loadok )
    {
      _d->restartFile = loader->restartFile();

      _d->finalize( game, needToFinalizeMap );
    }

    return loadok;
  }

  Logger::warning( "WARNING !!! GameLoader not found loader for " + filename.toString() );

  return false; // failed to load
}

std::string Loader::restartFile() const { return _d->restartFile; }

}//end namespace game
