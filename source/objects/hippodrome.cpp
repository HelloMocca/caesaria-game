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
// Copyright 2012-2014 Dalerank, dalerankn8@gmail.com

#include "hippodrome.hpp"
#include "game/resourcegroup.hpp"
#include "constants.hpp"
#include "gfx/picture.hpp"
#include "city/statistic.hpp"
#include "core/logger.hpp"
#include "gfx/tilemap.hpp"
#include "events/event.hpp"
#include "walker/walker.hpp"
#include "events/clearland.hpp"
#include "walker/circus_charioter.hpp"
#include "objects_factory.hpp"

using namespace gfx;
using namespace events;

REGISTER_CLASS_IN_OVERLAYFACTORY(object::hippodrome, Hippodrome)

const Point hippodromeSectionOffset[] = {
  Point(), Point( 0, 43 ), Point(),
  Point( 0, 16 ), //3
  Point(), Point(), Point(),
  Point( 0, 85 ), //7
  Point( 0, 95 ), //8
  Point( 60, 95 ), //9
  Point(),
  Point(),
  Point( 0, 16 ), //12
  Point(),
  Point( 0, 16 ), //14
  Point(),
  Point( 145, 85 ), //16
  Point( 120, 95 ), //17
  Point( 120, 95 ) //18
};

HippodromeSection::HippodromeSection( Hippodrome& base, Direction direction, Type type )
  : Building( object::fortArea, Size(5) )
{
  setState( pr::inflammability, 0 );
  setState( pr::collapsibility, 0 );

  _fgPictures().resize( 1 );

  _basepos = base.pos();
  _direction = direction;
  _type =  type;

  int pictureIndex = 0;
  int animIndex = 0;
  switch( type )
  {
  case middle:
    switch( _direction )
    {
    case direction::north: pictureIndex = 3; animIndex = 8; break;
    case direction::west:  pictureIndex = 12; animIndex = 17; break;
    default: break;
    }
  break;

  case ended:
    switch( _direction )
    {
    case direction::north: pictureIndex = 1; animIndex = 7; break;
    case direction::west:  pictureIndex = 14; animIndex = 18; break;
    default: break;
    }
  break;
  }

  _picture().load( ResourceGroup::hippodrome, pictureIndex );
  _picture().setOffset( Point( 0, _picture().height() / 2 ) + hippodromeSectionOffset[ pictureIndex ] );

  _animation().addFrame( ResourceGroup::hippodrome, animIndex );
  _animation().setOffset( hippodromeSectionOffset[ animIndex ]);
  _animation().stop();
}

HippodromeSection::~HippodromeSection(){}

void HippodromeSection::destroy()
{
  Building::destroy();

  auto hippodrome = _map().overlay( _basepos ).as<Hippodrome>();
  if( hippodrome.isValid() )
  {
    events::dispatch<ClearTile>( _basepos );
    _basepos = TilePos::invalid();
  }
}


void HippodromeSection::setAnimationVisible(bool visible)
{
  _fgPicture( 0 ) = !visible
                          ? Picture::getInvalid()
                          : animation().frames()[ 0 ];
}

class Hippodrome::Impl
{
public:
  Direction direction;
  HippodromeSectionPtr sectionMiddle, sectionEnd;
  Picture fullyPic;  
  WalkerList charioters;
  Pictures charioterPictures;
};

Direction Hippodrome::direction() const { return _d->direction; }

void Hippodrome::timeStep(const unsigned long time)
{
  EntertainmentBuilding::timeStep( time );

  int index = 0;
  foreach( it, _d->charioters )
  {
    (*it)->timeStep( time );
    _d->charioterPictures.clear();
    (*it)->getPictures( _d->charioterPictures );
    _fgPicture( 2 + index )     = _d->charioterPictures[0];
    _fgPicture( 2 + index + 1 ) = _d->charioterPictures[1];
    const Point& mp = (*it)->mappos();
    const Point& xp = tile().mappos();
    _fgPicture( 2 + index     ).addOffset( Point( mp.x() - xp.x() - 5, -mp.y() + xp.y() + 5 ) );
    _fgPicture( 2 + index + 1 ).addOffset( Point( mp.x() - xp.x() - 5, -mp.y() + xp.y() + 5 ) );
    index+=2;
  }
}

const Pictures& Hippodrome::pictures(Renderer::Pass pass) const
{
  return EntertainmentBuilding::pictures( pass );
}

Hippodrome::Hippodrome() : EntertainmentBuilding(Service::hippodrome, object::hippodrome, Size(15,5) ), _d( new Impl )
{
  _fgPictures().resize(5);
  _d->direction = direction::west;
  _d->charioterPictures.reserve(3);
  _init();  

  _addNecessaryWalker( walker::charioteer );
}

std::string Hippodrome::troubleDesc() const
{
  std::string ret = EntertainmentBuilding::troubleDesc();

  if( ret.empty() )
  {
    ret = isRacesCarry() ? "##trouble_hippodrome_full_work##" : "##trouble_hippodrome_no_charioters##";
  }

  return ret;
}

bool Hippodrome::canBuild( const city::AreaInfo& areaInfo ) const
{
  const_cast<Hippodrome*>( this )->_checkDirection( areaInfo );
  if( _d->direction != direction::none )
  {
    const_cast<Hippodrome*>( this )->_init();
  }

  int hp_n = areaInfo.city->statistic().objects.count<Hippodrome>();
  if( hp_n > 0 )
  {
    const_cast<Hippodrome*>( this )->_setError( "##may_build_only_once_hippodrome##");
    return false;
  }

  return _d->direction != direction::none;
}

void Hippodrome::deliverService()
{
  EntertainmentBuilding::deliverService();

  if( animation().isStopped() )
  {
    _fgPicture( 0 ) = Picture::getInvalid();
    _fgPicture( 1 ) = Picture::getInvalid();
    _fgPicture( 2 ) = Picture::getInvalid();
    _fgPicture( 3 ) = Picture::getInvalid();
    _fgPicture( 4 ) = Picture::getInvalid();
  }
  else
  {
    _fgPicture( 0 ) = _d->fullyPic;
  }

  _d->sectionEnd->setAnimationVisible( animation().isRunning() );
  _d->sectionMiddle->setAnimationVisible( animation().isRunning() );
}

bool Hippodrome::build( const city::AreaInfo& info )
{
  _checkDirection( info );

  setSize( Size( 5 ) );
  EntertainmentBuilding::build( info );

  TilePos offset = _d->direction == direction::north ? TilePos( 0, 5 ) : TilePos( 5, 0 );

  _d->sectionMiddle = _addSection( HippodromeSection::middle, offset );
  _d->sectionEnd = _addSection( HippodromeSection::ended, offset * 2 );

  _init( true );
  info.city->addOverlay( _d->sectionMiddle.object() );
  info.city->addOverlay( _d->sectionEnd.object() );

  _d->sectionEnd->setAnimationVisible( false );
  _d->sectionMiddle->setAnimationVisible( false );
  _animation().start();

  auto charioter = Walker::create<CircusCharioter>( _city(), this );
  _d->charioters.push_back( charioter.object() );

  return true;
}

void Hippodrome::destroy()
{
  EntertainmentBuilding::destroy();

  if( _d->sectionEnd.isValid() )
  {
    events::dispatch<ClearTile>( _d->sectionEnd->pos() );
    _d->sectionEnd = 0;
  }

  if( _d->sectionMiddle.isValid() )
  {
    events::dispatch<ClearTile>( _d->sectionMiddle->pos() );
    _d->sectionMiddle = 0;
  }
}

bool Hippodrome::isRacesCarry() const { return animation().isRunning(); }

Hippodrome::~Hippodrome()
{

}

WalkerList Hippodrome::_specificWorkers() const
{
  WalkerList ret;

  for( auto i : walkers() )
  {
    if( i->type() == walker::charioteer )
      ret << i;
  }

  return ret;
}

void Hippodrome::_init( bool onBuild )
{
  if( onBuild )
  {
    _fgPicture( 0 ) = Picture::getInvalid();
    _fgPicture( 1 ) = Picture::getInvalid();
  }

  _animation().clear();

  switch( _d->direction )
  {
  case direction::north:
  {
    _picture().load( ResourceGroup::hippodrome, 5 );
    _d->fullyPic.load( ResourceGroup::hippodrome, 9 );
    _d->fullyPic.setOffset( hippodromeSectionOffset[ 9 ] );
    if( !onBuild )
    {
      _fgPicture( 0 ).load( ResourceGroup::hippodrome, 3);
      _fgPicture( 0 ).setOffset( 150, 181 );
      _fgPicture( 1 ).load( ResourceGroup::hippodrome, 1);
      _fgPicture( 1 ).setOffset( 300, 310 );
    }
  }
  break;

  case direction::west:
  {
    _d->fullyPic.load( ResourceGroup::hippodrome, 16 );
    _d->fullyPic.setOffset( hippodromeSectionOffset[ 16 ] );
    Picture pic( ResourceGroup::hippodrome, 10 );
    pic.setOffset( 0, pic.height() / 2 + 42 );
    setPicture( pic );

    if( !onBuild )
    {
      _fgPicture( 0 ).load( ResourceGroup::hippodrome, 12);
      _fgPicture( 0 ).setOffset( 150, 31 );
      _fgPicture( 1 ).load( ResourceGroup::hippodrome, 14 );
      _fgPicture( 1 ).setOffset( 300, -43 );
    }
  }
  break;

  default:
    Logger::warning( "Hippodrome: Unknown direction" );
    //_CAESARIA_DEBUG_BREAK_IF( true && "Hippodrome: Unknown direction");
  }
}

HippodromeSectionPtr Hippodrome::_addSection(HippodromeSection::Type type, TilePos offset )
{
  HippodromeSectionPtr ret = new HippodromeSection( *this, _d->direction, type );
  city::AreaInfo info( _city(), pos() + offset );
  ret->build( info );
  ret->drop();

  return ret;
}

void Hippodrome::_checkDirection( const city::AreaInfo& areaInfo )
{
  const_cast<Hippodrome*>( this )->setSize( Size( 15, 5 ) );
  _d->direction = direction::west;
  bool mayBuild = EntertainmentBuilding::canBuild( areaInfo ); //check horizontal direction

  if( !mayBuild )
  {
    _d->direction = direction::north;
    const_cast<Hippodrome*>( this )->setSize( Size( 5, 15 ) );
    mayBuild = EntertainmentBuilding::canBuild( areaInfo ); //check vertical direction
  }

  if( !mayBuild )
  {
    _d->direction = direction::none;
  }
}
